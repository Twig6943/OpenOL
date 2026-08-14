/*=============================================================================
    OLImGui.cpp: ImGui integration for Outlast (D3D9 + Win32).
=============================================================================*/

// ImGui headers — resolved via SystemIncludePaths set in UE3BuildExternal.cs (../External/imgui/include).
// Must come before any UE3 headers (UE3 overrides operator new/delete).
#include "imgui_compat_msvc2012.h"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

// Now pull in UE3 (D3D9DrvPrivate.h → Engine.h).
#include "D3D9DrvPrivate.h"
#include "OLImGui.h"

// Relay server panel — _WINSOCK2API_ tells RelayThread.h that winsock2 symbols
// are already available via windows.h pulled by D3D9DrvPrivate.h above.
#ifndef _WINSOCK2API_
#  define _WINSOCK2API_
#endif
#include "..\..\Multiplayer\Inc\RelayThread.h"

// ImGui_ImplWin32_WndProcHandler is intentionally hidden in a #if 0 block in imgui_impl_win32.h
// to avoid dragging in <windows.h> dependencies. We must forward-declare it manually.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void EnsureTickerCreated(); // forward declaration — defined after FOLImGuiTicker

// Copy a TCHAR string into a char[] buffer (ASCII-safe, truncates at Len-1).
static void TCHARToCharBuf(char* Dst, int Len, const TCHAR* Src)
{
    int i = 0;
    while (i < Len - 1 && Src[i]) { Dst[i] = (char)Src[i]; ++i; }
    Dst[i] = '\0';
}

/*-----------------------------------------------------------------------------
    Scene snapshot — built on the game thread, read on the rendering thread.
    Uses TArray for unbounded actor/property lists.
    Double-buffered: game thread writes to one buffer, rendering thread reads
    the other. Swap is done by pointer exchange under a volatile flag.
-----------------------------------------------------------------------------*/

static const int SNAP_NAME_LEN = 128;
static const int SNAP_STR_LEN  = 128;

/*-----------------------------------------------------------------------------
    Scene snapshot — two-level lazy design:
      Level 1 (FSceneSnap): actor names + pointers only — rebuilt every tick.
      Level 2 (FDetailSnap): full property list for ONE actor — rebuilt only
        when that actor's TreeNode is open (rendering thread requests it).

    Both levels are double-buffered (Front = rendering thread reads,
    Back = game thread writes) and swapped via volatile ready flags.
-----------------------------------------------------------------------------*/

// Property type tag.
enum EPropSnapType
{
    PST_Bool,
    PST_Int,
    PST_Float,
    PST_Byte,
    PST_Name,   // read-only text
    PST_Str,    // editable string
    PST_Object, // read-only text (object name)
};

struct FPropSnap
{
    char          Name[SNAP_NAME_LEN];
    EPropSnapType Type;
    INT           Offset;
    BITFIELD      BitMask;          // PST_Bool only
    TArray<FString> EnumValues;     // PST_Byte with enum
    union
    {
        bool   Bool;
        INT    Int;
        FLOAT  Float;
        BYTE   Byte;
    } Value;
    char StrValue[SNAP_STR_LEN];    // PST_Name / PST_Str / PST_Object
};

// Level-1: pointer + pre-built name. Names are filled on the game thread
// (GetFName().ToString() once per tick per object), so the rendering thread
// can filter with plain strstr — zero allocations during search.
struct FObjectEntry
{
    UObject* Ptr;
    char     Name[SNAP_NAME_LEN];
    char     AssetName[SNAP_NAME_LEN]; // mesh/asset name inside the component, empty if N/A
};

struct FSceneSnap
{
    TArray<FObjectEntry> Objects;
};

// One class level in the property hierarchy (e.g. "OLHero", "OLPawn", "Actor").
struct FPropGroup
{
    char              ClassName[SNAP_NAME_LEN];
    TArray<FPropSnap> Props;
};

// Level-2: full property detail for one actor, grouped by class hierarchy.
struct FDetailSnap
{
    UObject*           ActorPtr;  // which actor these props belong to
    TArray<FPropGroup> Groups;    // most-derived first (OLHero → ... → UObject)
};

// Level-1 double-buffer (rebuilt every tick).
static FSceneSnap*  GSnapFront = NULL;
static FSceneSnap*  GSnapBack  = NULL;
static volatile int GSnapReady = 0;

// Level-2 double-buffer (rebuilt only for the open actor).
static FDetailSnap*  GDetailFront = NULL;
static FDetailSnap*  GDetailBack  = NULL;
static volatile int  GDetailReady = 0;

// Request from rendering thread: "please build props for this actor".
// NULL means no request pending.
static UObject* volatile GDetailRequest = NULL;

// Pending property write: rendering thread enqueues, game thread applies.
static const int WRITE_QUEUE_SIZE = 64;

enum EPropWriteType { PWT_Bool, PWT_Int, PWT_Float, PWT_Byte, PWT_Str, PWT_Name };

struct FPropWrite
{
    UObject*       Actor;
    INT            Offset;
    EPropWriteType Type;
    BITFIELD       BitMask;
    union
    {
        bool   Bool;
        INT    Int;
        FLOAT  Float;
        BYTE   Byte;
    } NewValue;
    char StrValue[SNAP_STR_LEN]; // PWT_Str only
};
static FPropWrite   GWriteQueue[WRITE_QUEUE_SIZE];
static volatile int GWriteCount = 0;

static bool GImGuiInitialized = false;
static bool GImGuiShowDemoWindow = false;

// Pending init parameters — set from main thread, consumed on first NewFrame (rendering thread).
static IDirect3DDevice9* GPendingDevice = NULL;
static HWND              GPendingWnd    = NULL;

// Registered action buttons (label + callback), populated by game-side code.
struct OLImGuiAction
{
    const char*      Label;
    OLImGuiActionFn  Fn;
};

static const int         GActionCapacity = 16;
static OLImGuiAction     GActions[GActionCapacity];
static int               GActionCount = 0;

// Debug vectors for runtime tuning (render thread writes, game thread reads).
volatile float GDebugVec0[3] = { 0.f, 0.f,  8.f }; // WorkerHead->Origin
volatile float GDebugVec1[3] = { 0.f, -5.f, 0.f }; // AttachComponent offset

// Pending actions fired from the rendering thread, drained on the game thread.
// Each entry is an index into GActions; -1 means empty slot.
// Simple ring-free array: rendering thread writes GPendingActions[GPendingWrite],
// game thread reads and clears. GActionCapacity slots is more than enough.
static volatile int GPendingActions[GActionCapacity];
static volatile int GPendingCount = 0; // written by render thread, read by game thread

void OLImGui_RegisterAction(const char* Label, OLImGuiActionFn Fn)
{
    if (GActionCount < GActionCapacity)
    {
        GActions[GActionCount].Label = Label;
        GActions[GActionCount].Fn   = Fn;
        ++GActionCount;
    }
}

void OLImGui_DrainPendingActions()
{
    // Called from game thread. Snapshot and clear the pending list, then invoke callbacks.
    int Count = GPendingCount;
    GPendingCount = 0;
    for (int i = 0; i < Count; ++i)
    {
        int Idx = GPendingActions[i];
        if (Idx >= 0 && Idx < GActionCount && GActions[Idx].Fn)
            GActions[Idx].Fn();
    }
}

void OLImGui_Init(IDirect3DDevice9* Device, HWND Wnd)
{
    // Just store parameters; actual init happens on the rendering thread in OLImGui_NewFrame.
    GPendingDevice = Device;
    GPendingWnd    = Wnd;

    // Allocate double-buffer snapshots on first call (game thread).
    if (!GSnapFront)   GSnapFront   = new FSceneSnap();
    if (!GSnapBack)    GSnapBack    = new FSceneSnap();
    if (!GDetailFront) GDetailFront = new FDetailSnap();
    if (!GDetailBack)  GDetailBack  = new FDetailSnap();

    // Register the game-thread ticker that drains pending action callbacks.
    EnsureTickerCreated();
}

static void OLImGui_InitInternal()
{
    if (GImGuiInitialized || !GPendingDevice || !GPendingWnd)
        return;

    debugf(TEXT("OLImGui: CreateContext"));
    ImGui::CreateContext();

    ImGuiIO& IO = ImGui::GetIO();
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    IO.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange; // Off by default; enabled when overlay opens.
    IO.IniFilename = NULL;

    ImGui::StyleColorsDark();

    debugf(TEXT("OLImGui: ImGui_ImplWin32_Init"));
    ImGui_ImplWin32_Init(GPendingWnd);

    debugf(TEXT("OLImGui: ImGui_ImplDX9_Init"));
    ImGui_ImplDX9_Init(GPendingDevice);

    GImGuiInitialized = true;
    debugf(TEXT("OLImGui: initialized OK"));
}

void OLImGui_Shutdown()
{
    GPendingDevice = NULL;
    GPendingWnd    = NULL;

    if (!GImGuiInitialized)
        return;

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    GImGuiInitialized = false;
}

LRESULT OLImGui_WndProcHandler(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (!GImGuiInitialized)
        return 0;
    return ImGui_ImplWin32_WndProcHandler(Wnd, Msg, wParam, lParam);
}

void OLImGui_NewFrame()
{
    // Lazy init: perform actual ImGui setup on the rendering thread (same thread as Render).
    if (!GImGuiInitialized)
        OLImGui_InitInternal();

    if (!GImGuiInitialized)
        return;

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void OLImGui_Render()
{
    if (!GImGuiInitialized)
        return;

    // Build UI
    if (GImGuiShowDemoWindow)
    {
        ImGui::Begin("OLGame Debug", &GImGuiShowDemoWindow);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        if (ImGui::BeginTabBar("##tabs"))
        {
            // ---- Tab: Actions ----
            if (ImGui::BeginTabItem("Actions"))
            {
                // Registered action buttons — enqueued for execution on the game thread.
                for (int i = 0; i < GActionCount; ++i)
                {
                    if (ImGui::Button(GActions[i].Label))
                    {
                        int Slot = GPendingCount;
                        if (Slot < GActionCapacity)
                        {
                            GPendingActions[Slot] = i;
                            GPendingCount = Slot + 1;
                        }
                    }
                }

                ImGui::Separator();
                ImGui::TextUnformatted("DummyHead tuning");
                // Cast away volatile for ImGui (single render thread, safe to read/write directly).
                ImGui::DragFloat3("Origin (Vec0)",  (float*)GDebugVec0, 0.1f);
                ImGui::DragFloat3("Attach (Vec1)",  (float*)GDebugVec1, 0.1f);

                ImGui::EndTabItem();
            }

            // ---- Tab: Scene ----
            if (ImGui::BeginTabItem("Scene"))
            {
                GSnapReady = 0;
                // GDetailReady is NOT reset here — props stay visible while game thread rebuilds.

                if (!GSnapFront)
                {
                    ImGui::TextDisabled("Waiting for snapshot...");
                    ImGui::EndTabItem();
                    goto end_tabbar;
                }
                const FSceneSnap& Snap = *GSnapFront;

                // Case-insensitive substring check.
#define NAME_MATCHES(Hay, Needle) ([&]() -> bool { \
    if ((Needle)[0] == '\0') return true; \
    for (int _h = 0; (Hay)[_h]; ++_h) { \
        int _hi = _h, _ni = 0; \
        while ((Hay)[_hi] && (Needle)[_ni] && ((Hay)[_hi]|0x20)==((Needle)[_ni]|0x20)) { ++_hi; ++_ni; } \
        if ((Needle)[_ni] == '\0') return true; } \
    return false; }())

                // Search bar.
                static char GSceneFilter[128] = "";
                ImGui::SetNextItemWidth(-1.0f);
                bool FilterChanged = ImGui::InputTextWithHint("##filter", "Search actors...", GSceneFilter, sizeof(GSceneFilter));

                // Build filtered index — pure strstr on pre-built char[] names, zero allocs.
                // Rebuilt when filter changes or a new snapshot arrives.
                static TArray<int> GFilteredIdx;
                if (FilterChanged || GSnapReady == 0)
                {
                    GFilteredIdx.Reset();
                    for (int a = 0; a < Snap.Objects.Num(); ++a)
                    {
                        if (NAME_MATCHES(Snap.Objects(a).Name, GSceneFilter))
                            GFilteredIdx.AddItem(a);
                    }
                }

                ImGui::Text("Objects: %d / %d", GFilteredIdx.Num(), Snap.Objects.Num());
                ImGui::Separator();

                // Prop-widget helper macro — enqueues write to game thread.
#define ENQUEUE_WRITE(ActorPtr_, Offset_, Type_, Field_, Val_) \
    { int _s = GWriteCount; if (_s < WRITE_QUEUE_SIZE) { \
        GWriteQueue[_s].Actor = ActorPtr_; GWriteQueue[_s].Offset = (Offset_); \
        GWriteQueue[_s].Type  = Type_;     GWriteQueue[_s].NewValue.Field_ = Val_; \
        GWriteCount = _s + 1; } }

                ImGui::BeginChild("##actors", ImVec2(0, 0), false);

                // ImGuiListClipper: only visible actor rows are rendered.
                ImGuiListClipper Clipper;
                Clipper.Begin(GFilteredIdx.Num());
                while (Clipper.Step())
                {
                    for (int ci = Clipper.DisplayStart; ci < Clipper.DisplayEnd; ++ci)
                    {
                        const FObjectEntry& OE = Snap.Objects(GFilteredIdx(ci));
                        UObject* Obj = OE.Ptr;
                        // Guard against object being destroyed between snapshot and render.
                        if (!Obj || Obj->IsPendingKill() || !Obj->IsValid()) continue;

                        ImGui::PushID(Obj);
                        bool Open = OE.AssetName[0]
                            ? ImGui::TreeNode("##node", "%s  [%s]", OE.Name, OE.AssetName)
                            : ImGui::TreeNode("##node", "%s", OE.Name);
                        ImGui::PopID();

                        if (!Open) continue;

                        // Always request fresh props every frame so values stay live.
                        GDetailRequest = Obj;

                        bool DetailReady = GDetailFront &&
                                          GDetailFront->ActorPtr == Obj &&
                                          GDetailFront->Groups.Num() > 0;
                        if (!DetailReady)
                        {
                            ImGui::TextDisabled("  Loading properties...");
                            ImGui::TreePop();
                            continue;
                        }

                        const FDetailSnap& D = *GDetailFront;
                        if (D.Groups.Num() == 0)
                        {
                            ImGui::TextDisabled("  (no properties)");
                        }
                        else
                        {
                            // Render each class level as a collapsible section with a property table.
                            for (int g = 0; g < D.Groups.Num(); ++g)
                            {
                                const FPropGroup& G = D.Groups(g);
                                ImGui::PushID(g);
                                // Open by default for most-derived class (g==0), collapsed for parents.
                                ImGuiTreeNodeFlags HdrFlags = ImGuiTreeNodeFlags_DefaultOpen |
                                                             ImGuiTreeNodeFlags_SpanAvailWidth;
                                bool GroupOpen = ImGui::CollapsingHeader(G.ClassName, HdrFlags);
                                if (GroupOpen)
                                {
                                    if (ImGui::BeginTable("##props", 2,
                                        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit |
                                        ImGuiTableFlags_Resizable))
                                    {
                                        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 160.0f);
                                        ImGui::TableSetupColumn("Value",    ImGuiTableColumnFlags_WidthStretch);
                                        ImGui::TableHeadersRow();

                                        for (int p = 0; p < G.Props.Num(); ++p)
                                        {
                                            const FPropSnap& PS = G.Props(p);
                                            ImGui::PushID(p);

                                            ImGui::TableNextRow();
                                            ImGui::TableSetColumnIndex(0);
                                            ImGui::TextUnformatted(PS.Name);
                                            ImGui::TableSetColumnIndex(1);
                                            ImGui::SetNextItemWidth(-FLT_MIN); // fill column

                                            switch (PS.Type)
                                            {
                                            case PST_Bool:
                                            {
                                                bool Val = PS.Value.Bool;
                                                if (ImGui::Checkbox("##v", &Val))
                                                {
                                                    int Slot = GWriteCount;
                                                    if (Slot < WRITE_QUEUE_SIZE)
                                                    {
                                                        GWriteQueue[Slot].Actor         = Obj;
                                                        GWriteQueue[Slot].Offset        = PS.Offset;
                                                        GWriteQueue[Slot].Type          = PWT_Bool;
                                                        GWriteQueue[Slot].BitMask       = PS.BitMask;
                                                        GWriteQueue[Slot].NewValue.Bool = Val;
                                                        GWriteCount = Slot + 1;
                                                    }
                                                    GDetailRequest = Obj;
                                                }
                                                break;
                                            }
                                            case PST_Int:
                                            {
                                                int Val = (int)PS.Value.Int;
                                                if (ImGui::DragInt("##v", &Val))
                                                {
                                                    ENQUEUE_WRITE(Obj, PS.Offset, PWT_Int, Int, (INT)Val);
                                                    GDetailRequest = Obj;
                                                }
                                                break;
                                            }
                                            case PST_Float:
                                            {
                                                float Val = PS.Value.Float;
                                                if (ImGui::DragFloat("##v", &Val, 0.01f))
                                                {
                                                    ENQUEUE_WRITE(Obj, PS.Offset, PWT_Float, Float, Val);
                                                    GDetailRequest = Obj;
                                                }
                                                break;
                                            }
                                            case PST_Byte:
                                            {
                                                if (PS.EnumValues.Num() > 0)
                                                {
                                                    int Val = (int)PS.Value.Byte;
                                                    int EC  = PS.EnumValues.Num();
                                                    struct FCtx { const TArray<FString>* V; };
                                                    FCtx Ctx; Ctx.V = &PS.EnumValues;
                                                    auto Getter = [](void* D, int I, const char** O) -> bool {
                                                        FCtx* C = (FCtx*)D;
                                                        static char B[SNAP_NAME_LEN];
                                                        TCHARToCharBuf(B, SNAP_NAME_LEN, *(*C->V)(I));
                                                        *O = B; return true;
                                                    };
                                                    if (Val >= EC) Val = 0;
                                                    if (ImGui::Combo("##v", &Val, Getter, &Ctx, EC))
                                                    {
                                                        ENQUEUE_WRITE(Obj, PS.Offset, PWT_Byte, Byte, (BYTE)Val);
                                                        GDetailRequest = Obj;
                                                    }
                                                }
                                                else
                                                {
                                                    int Val = (int)PS.Value.Byte;
                                                    if (ImGui::DragInt("##v", &Val, 1.0f, 0, 255))
                                                    {
                                                        ENQUEUE_WRITE(Obj, PS.Offset, PWT_Byte, Byte, (BYTE)Val);
                                                        GDetailRequest = Obj;
                                                    }
                                                }
                                                break;
                                            }
                                            case PST_Name:
                                            {
                                                char NameBuf[SNAP_STR_LEN];
                                                appStrncpyANSI(NameBuf, PS.StrValue, SNAP_STR_LEN);
                                                if (ImGui::InputText("##v", NameBuf, SNAP_STR_LEN,
                                                    ImGuiInputTextFlags_EnterReturnsTrue))
                                                {
                                                    int Slot = GWriteCount;
                                                    if (Slot < WRITE_QUEUE_SIZE)
                                                    {
                                                        GWriteQueue[Slot].Actor  = Obj;
                                                        GWriteQueue[Slot].Offset = PS.Offset;
                                                        GWriteQueue[Slot].Type   = PWT_Name;
                                                        appStrncpyANSI(GWriteQueue[Slot].StrValue, NameBuf, SNAP_STR_LEN);
                                                        GWriteCount = Slot + 1;
                                                    }
                                                    GDetailRequest = Obj;
                                                }
                                                break;
                                            }
                                            case PST_Str:
                                            {
                                                char StrBuf[SNAP_STR_LEN];
                                                appStrncpyANSI(StrBuf, PS.StrValue, SNAP_STR_LEN);
                                                if (ImGui::InputText("##v", StrBuf, SNAP_STR_LEN,
                                                    ImGuiInputTextFlags_EnterReturnsTrue))
                                                {
                                                    int Slot = GWriteCount;
                                                    if (Slot < WRITE_QUEUE_SIZE)
                                                    {
                                                        GWriteQueue[Slot].Actor  = Obj;
                                                        GWriteQueue[Slot].Offset = PS.Offset;
                                                        GWriteQueue[Slot].Type   = PWT_Str;
                                                        appStrncpyANSI(GWriteQueue[Slot].StrValue, StrBuf, SNAP_STR_LEN);
                                                        GWriteCount = Slot + 1;
                                                    }
                                                    GDetailRequest = Obj;
                                                }
                                                break;
                                            }
                                            case PST_Object:
                                                ImGui::TextDisabled("[%s]", PS.StrValue);
                                                break;
                                            }

                                            ImGui::PopID();
                                        }
                                        ImGui::EndTable();
                                    } // BeginTable
                                } // GroupOpen
                                ImGui::PopID();
                            } // groups loop
                        }
                        ImGui::TreePop();
                    }
                }
                Clipper.End();

                ImGui::EndChild();
#undef NAME_MATCHES
#undef ENQUEUE_WRITE
                ImGui::EndTabItem();
            }

            // ---- Tab: Relay ----
            if (ImGui::BeginTabItem("Relay"))
            {
                static char SRelayPort[8]       = "7777";
                static char SRelayName[64]      = "My Server";
                static char SRelayDbPath[256]   = "relay_db.json";
                static char SRelayLog[4096]     = {};
                static int  SRelayLogLen        = 0;

                // Drain new log lines from the history ring.
                {
                    char Line[HISTORY_MSG_LEN];
                    while (GRelayThread.PopLogLine(Line))
                    {
                        int LineLen = 0;
                        while (Line[LineLen] && LineLen < HISTORY_MSG_LEN - 1) ++LineLen;
                        if (SRelayLogLen + LineLen + 2 > (int)sizeof(SRelayLog))
                        {
                            // Scroll: discard oldest half.
                            int Half = (int)sizeof(SRelayLog) / 2;
                            memmove(SRelayLog, SRelayLog + Half, SRelayLogLen - Half);
                            SRelayLogLen -= Half;
                        }
                        memcpy(SRelayLog + SRelayLogLen, Line, LineLen);
                        SRelayLogLen += LineLen;
                        SRelayLog[SRelayLogLen++] = '\n';
                        SRelayLog[SRelayLogLen]   = '\0';
                    }
                }

                bool bRunning = GRelayThread.IsRunning() != 0;

                if (bRunning)
                {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Status: RUNNING  port %s", SRelayPort);
                    if (ImGui::Button("Stop Relay"))
                    {
                        GRelayThread.StopRelay();
                        SRelayLogLen = 0; SRelayLog[0] = '\0';
                    }
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "Status: stopped");
                    ImGui::SetNextItemWidth(80.0f);
                    ImGui::InputText("Port",    SRelayPort,   sizeof(SRelayPort));
                    ImGui::SetNextItemWidth(200.0f);
                    ImGui::InputText("Name",    SRelayName,   sizeof(SRelayName));
                    ImGui::SetNextItemWidth(300.0f);
                    ImGui::InputText("DB path", SRelayDbPath, sizeof(SRelayDbPath));
                    if (ImGui::Button("Start Relay"))
                    {
                        int Port = 0;
                        for (int ci = 0; SRelayPort[ci]; ++ci)
                            Port = Port * 10 + (SRelayPort[ci] - '0');
                        GRelayThread.StartRelay((WORD)Port, SRelayName, SRelayDbPath);
                    }
                }

                ImGui::Separator();
                ImGui::BeginChild("##relaylog", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextUnformatted(SRelayLog, SRelayLog + SRelayLogLen);
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                    ImGui::SetScrollHereY(1.0f);
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            end_tabbar:
            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    // Render
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void OLImGui_ToggleOverlay()
{
    GImGuiShowDemoWindow = !GImGuiShowDemoWindow;

    if (!GImGuiInitialized)
        return;

    ImGuiIO& IO = ImGui::GetIO();
    if ( GImGuiShowDemoWindow )
    {
        // Overlay opened: let ImGui manage the cursor.
        IO.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    }
    else
    {
        // Overlay closed: stop ImGui touching the cursor so the game can hide it again.
        IO.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        ::SetCursor(NULL); // Hide immediately — don't wait for next WM_SETCURSOR.

        // Clear detail snapshot so stale object pointers are not dereferenced on next open.
        GDetailRequest = NULL;
        GDetailReady   = 0;
        if (GDetailFront) { GDetailFront->ActorPtr = NULL; GDetailFront->Groups.Empty(); }
        if (GDetailBack)  { GDetailBack->ActorPtr  = NULL; GDetailBack->Groups.Empty();  }
    }
}

bool OLImGui_IsOverlayVisible()
{
    return GImGuiShowDemoWindow;
}


void OLImGui_InvalidateDeviceObjects()
{
    if (!GImGuiInitialized)
        return;
    ImGui_ImplDX9_InvalidateDeviceObjects();
}

void OLImGui_CreateDeviceObjects()
{
    if (!GImGuiInitialized)
        return;
    ImGui_ImplDX9_CreateDeviceObjects();
}

/*-----------------------------------------------------------------------------
    FOLImGuiTicker — FTickableObject that drains pending ImGui actions on the
    game thread each frame. Instantiated as a global so it self-registers.
-----------------------------------------------------------------------------*/
class FOLImGuiTicker : public FTickableObject
{
public:
    virtual void Tick(FLOAT /*DeltaTime*/)
    {
        // 1. Apply pending property writes from the rendering thread.
        int WriteCount = GWriteCount;
        GWriteCount = 0;
        for (int i = 0; i < WriteCount; ++i)
        {
            const FPropWrite& W = GWriteQueue[i];
            BYTE* Base = (BYTE*)W.Actor + W.Offset;
            switch (W.Type)
            {
            case PWT_Bool:
                if (W.NewValue.Bool)
                    *(BITFIELD*)Base |=  W.BitMask;
                else
                    *(BITFIELD*)Base &= ~W.BitMask;
                break;
            case PWT_Int:   *(INT*)Base   = W.NewValue.Int;   break;
            case PWT_Float: *(FLOAT*)Base = W.NewValue.Float; break;
            case PWT_Byte:  *(BYTE*)Base  = W.NewValue.Byte;  break;
            case PWT_Str:
            {
                FString& S = *(FString*)Base;
                S = FString(ANSI_TO_TCHAR(W.StrValue));
                break;
            }
            case PWT_Name:
                *(FName*)Base = FName(ANSI_TO_TCHAR(W.StrValue));
                break;
            }
        }

        // 2. Dispatch pending action button callbacks.
        OLImGui_DrainPendingActions();

        if (!GImGuiShowDemoWindow)
            return;

        // 3a. Level-1: collect pointers + names (game thread, one GetFName per object).
        // Rendering thread filters with plain strstr — no allocations during search.
        if (GSnapBack)
        {
            FSceneSnap& Snap = *GSnapBack;
            Snap.Objects.Reset();
            for (TObjectIterator<UObject> It; It; ++It)
            {
                UObject* Obj = *It;
                if (!Obj || Obj->IsTemplate() || Obj->IsPendingKill())
                    continue;

                if (Obj->HasAnyFlags(RF_ClassDefaultObject |
                                     RF_ArchetypeObject    |
                                     RF_DisregardForGC     |
                                     RF_ZombieComponent))
                    continue;
                if (Obj->IsA(UField::StaticClass())   ||
                    Obj->IsA(UPackage::StaticClass())  ||
                    Obj->IsA(UTextBuffer::StaticClass()))
                    continue;

                FObjectEntry E;
                E.Ptr = Obj;
                TCHARToCharBuf(E.Name, SNAP_NAME_LEN, *Obj->GetFName().ToString());
                // For mesh components, record the asset name so the list is more descriptive.
                E.AssetName[0] = '\0';
                if (USkeletalMeshComponent* SKC = Cast<USkeletalMeshComponent>(Obj))
                {
                    if (SKC->SkeletalMesh)
                        TCHARToCharBuf(E.AssetName, SNAP_NAME_LEN, *SKC->SkeletalMesh->GetFName().ToString());
                }
                else if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Obj))
                {
                    if (SMC->StaticMesh)
                        TCHARToCharBuf(E.AssetName, SNAP_NAME_LEN, *SMC->StaticMesh->GetFName().ToString());
                }
                Snap.Objects.AddItem(E);
            }
            Exchange(GSnapFront, GSnapBack);
            GSnapReady = 1;
        }

        // 3b. Level-2: rebuild props for the actor the rendering thread has open.
        UObject* Requested = GDetailRequest;
        GDetailRequest = NULL; // consume the request
        if (Requested && GDetailBack)
        {
            FDetailSnap& D = *GDetailBack;
            D.ActorPtr = Requested;
            D.Groups.Reset();

            // Walk the class hierarchy from most-derived to UObject.
            // Collect properties declared at each level into a separate group.
            for (UClass* Class = Requested->GetClass(); Class; Class = Class->GetSuperClass())
            {
                FPropGroup Group;
                TCHARToCharBuf(Group.ClassName, SNAP_NAME_LEN, *Class->GetName());

                for (UProperty* Prop = Class->PropertyLink;
                     Prop; Prop = Prop->PropertyLinkNext)
                {
                    // Only include properties declared in this exact class.
                    if (Prop->GetOwnerClass() != Class)
                        continue;

                    FPropSnap PS;
                    TCHARToCharBuf(PS.Name, SNAP_NAME_LEN, *Prop->GetName());
                    PS.Offset      = Prop->Offset;
                    PS.BitMask     = 0;
                    PS.StrValue[0] = '\0';

                    UBoolProperty* BP = Cast<UBoolProperty>(Prop);
                    if (BP) {
                        PS.Type       = PST_Bool;
                        PS.BitMask    = BP->BitMask;
                        PS.Value.Bool = !!(*(BITFIELD*)((BYTE*)Requested + BP->Offset) & BP->BitMask);
                        Group.Props.AddItem(PS); continue;
                    }
                    UIntProperty* IP = Cast<UIntProperty>(Prop);
                    if (IP) {
                        PS.Type      = PST_Int;
                        PS.Value.Int = *(INT*)((BYTE*)Requested + IP->Offset);
                        Group.Props.AddItem(PS); continue;
                    }
                    UFloatProperty* FP = Cast<UFloatProperty>(Prop);
                    if (FP) {
                        PS.Type        = PST_Float;
                        PS.Value.Float = *(FLOAT*)((BYTE*)Requested + FP->Offset);
                        Group.Props.AddItem(PS); continue;
                    }
                    UByteProperty* YP = Cast<UByteProperty>(Prop);
                    if (YP) {
                        PS.Type       = PST_Byte;
                        PS.Value.Byte = *(BYTE*)((BYTE*)Requested + YP->Offset);
                        if (YP->Enum) {
                            int NV = YP->Enum->NumEnums();
                            for (int ei = 0; ei < NV; ++ei)
                                PS.EnumValues.AddItem(YP->Enum->GetEnum(ei).ToString());
                        }
                        Group.Props.AddItem(PS); continue;
                    }
                    UNameProperty* NP = Cast<UNameProperty>(Prop);
                    if (NP) {
                        PS.Type = PST_Name;
                        FName Val = *(FName*)((BYTE*)Requested + NP->Offset);
                        TCHARToCharBuf(PS.StrValue, SNAP_STR_LEN, *Val.ToString());
                        Group.Props.AddItem(PS); continue;
                    }
                    UStrProperty* SP = Cast<UStrProperty>(Prop);
                    if (SP) {
                        PS.Type = PST_Str;
                        FString Val = *(FString*)((BYTE*)Requested + SP->Offset);
                        TCHARToCharBuf(PS.StrValue, SNAP_STR_LEN, *Val);
                        Group.Props.AddItem(PS); continue;
                    }
                    UObjectProperty* OP = Cast<UObjectProperty>(Prop);
                    if (OP) {
                        PS.Type = PST_Object;
                        UObject* Val = *(UObject**)((BYTE*)Requested + OP->Offset);
                        TCHARToCharBuf(PS.StrValue, SNAP_STR_LEN, Val ? *Val->GetName() : TEXT("None"));
                        Group.Props.AddItem(PS); continue;
                    }
                }

                // Skip empty groups (classes with no renderable properties).
                if (Group.Props.Num() > 0)
                    D.Groups.AddItem(Group);
            }

            Exchange(GDetailFront, GDetailBack);
            GDetailReady = 1;
        }
    }

    virtual UBOOL IsTickable() const
    {
        return TRUE;
    }

    virtual UBOOL IsTickableWhenPaused() const
    {
        return TRUE;
    }
};

// Global instance — registered automatically when the module loads (game thread).
// Must be a pointer: FTickableObject ctor asserts IsInGameThread(), and global
// constructors run before the engine initialises the thread ID. We construct it
// lazily on first OLImGui_Init() call which always happens on the game thread.
static FOLImGuiTicker* GOLImGuiTicker = NULL;

// Called from OLImGui_Init (game thread) — safe to register here.
static void EnsureTickerCreated()
{
    if (!GOLImGuiTicker)
        GOLImGuiTicker = new FOLImGuiTicker();
}
