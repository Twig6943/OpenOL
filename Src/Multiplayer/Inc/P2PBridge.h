/*=============================================================================
    P2PBridge.h — FP2PBridge: tunnels Steam P2P packets to/from the embedded
    relay server running on 127.0.0.1:RelayPort.

    SERVER mode (bClientMode=FALSE, default):
      Each P2P client gets a dedicated loopback socket bound to a unique port
      (RelayPort+2, RelayPort+3, ...).  server.c identifies clients by
      127.0.0.1:<vport> — no modifications to server.c needed.

      P2P client → SendP2PPacket → bridge reads → sendto(relay via vport sock)
      relay → sendto(127.0.0.1:vport) → bridge reads → SendP2PPacket(client)

    CLIENT mode (bClientMode=TRUE):
      A single listening socket on 127.0.0.1:(RelayPort+1) bridges FMpConnection
      to the remote host over Steam P2P.

      FMpConnection → UDP to 127.0.0.1:(RelayPort+1) → bridge → SendP2PPacket(host)
      ReadP2PPacket(host) → bridge → UDP to FMpConnection's bound port

    Thread: FP2PBridge runs in its own FRunnableThread.
    Callbacks (P2PSessionRequest_t) fire on the game thread via SteamAPI_RunCallbacks();
    bridge queues the accept and processes it in Run().
=============================================================================*/
#pragma once

#include "Core.h"
// Note: Steam headers (ISteamNetworking, GSteamNetworking) are included only in P2PBridge.cpp,
// not here, to avoid pulling OnlineSubsystemSteamworks into every translation unit.

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

#define BRIDGE_P2P_CHANNEL   7          // Steam P2P channel; must not conflict
#define MAX_P2P_SLOTS        32         // max simultaneous P2P clients
#define BRIDGE_BUF_SIZE      4096
// Server mode: virtual ports start at RelayPort+2.
// Client mode: listening port is RelayPort+1.
#define BRIDGE_VPORT_OFFSET  2
#define BRIDGE_CLIENT_PORT_OFFSET 1

// ---------------------------------------------------------------------------
// FP2PBridge
// ---------------------------------------------------------------------------

class FP2PBridge : public FRunnable
{
public:
    FP2PBridge()
        : Thread(NULL)
        , bRunning(FALSE)
        , bStopRequested(FALSE)
        , bClientMode(FALSE)
        , RelayPort(7777)
        , HostSteamID(0)
        , ClientSock((UINT_PTR)(~0))
        , bClientReturnKnown(FALSE)
        , PendingAcceptCount(0)
    {
        appMemzero(Slots, sizeof(Slots));
        appMemzero(PendingAccept, sizeof(PendingAccept));
        appMemzero(&RelayAddr, sizeof(RelayAddr));
        appMemzero(&ClientReturnAddr, sizeof(ClientReturnAddr));
    }

    // Start in SERVER mode (host side).  Call after relay is running.
    UBOOL Start(WORD InRelayPort)
    {
        if (bRunning) return TRUE;
        bClientMode    = FALSE;
        bStopRequested = FALSE;
        RelayPort      = InRelayPort;
        debugf(NAME_Log, TEXT("FP2PBridge::Start server mode port=%d"), (int)InRelayPort);
        Thread = GThreadFactory->CreateThread(this, TEXT("OLP2PBridge"),
            FALSE, FALSE, 0, TPri_Normal);
        return Thread != NULL;
    }

    // Start in CLIENT mode.  FMpConnection sends UDP to 127.0.0.1:(RelayPort+1);
    // bridge forwards to HostSteamID via Steam P2P, and vice versa.
    UBOOL StartClient(WORD InRelayPort, QWORD InHostSteamID)
    {
        if (bRunning) return TRUE;
        bClientMode    = TRUE;
        bStopRequested = FALSE;
        RelayPort      = InRelayPort;
        HostSteamID    = InHostSteamID;
        debugf(NAME_Log, TEXT("FP2PBridge::StartClient port=%d SteamID=%llu"), (int)InRelayPort, (unsigned long long)InHostSteamID);
        Thread = GThreadFactory->CreateThread(this, TEXT("OLP2PBridgeClient"),
            FALSE, FALSE, 0, TPri_Normal);
        return Thread != NULL;
    }

    void StopBridge()
    {
        if (!bRunning && !Thread) return;
        bStopRequested = TRUE;
        if (Thread) { GThreadFactory->Destroy(Thread); Thread = NULL; }
        bRunning = FALSE;
    }

    UBOOL IsRunning()    const { return bRunning; }
    UBOOL IsClientMode() const { return bClientMode; }

    // Returns the local UDP port FMpConnection should target in client mode.
    WORD GetClientListenPort() const
    {
        return (WORD)(RelayPort + BRIDGE_CLIENT_PORT_OFFSET);
    }

    // Called from game thread when P2PSessionRequest_t fires.
    void OnSessionRequest(QWORD SteamID)
    {
        FScopeLock Lock(&PendingLock);
        if (PendingAcceptCount < MAX_P2P_SLOTS)
            PendingAccept[PendingAcceptCount++] = SteamID;
    }

    // ---------------------------------------------------------------------------
    // FRunnable
    // ---------------------------------------------------------------------------
    virtual UBOOL Init() OVERRIDE;
    virtual DWORD Run() OVERRIDE;
    virtual void  Stop() OVERRIDE { bStopRequested = TRUE; }
    virtual void  Exit() OVERRIDE { CleanupSlots(); bRunning = FALSE; }

private:
    // One slot per P2P client (server mode).
    struct SlotEntry
    {
        UBOOL    bUsed;
        QWORD    SteamID;
        UINT_PTR VSock;  // loopback socket bound to 127.0.0.1:VPort (SOCKET == UINT_PTR on Win64)
        WORD     VPort;  // virtual port seen by relay
    };

    FRunnableThread* Thread;
    UBOOL            bRunning;
    volatile UBOOL   bStopRequested;
    UBOOL            bClientMode;
    WORD             RelayPort;

    // Client mode fields.
    QWORD            HostSteamID;
    UINT_PTR         ClientSock;          // listen socket on 127.0.0.1:(RelayPort+1)
    struct sockaddr_in ClientReturnAddr;  // FMpConnection's source address (filled on first recv)
    UBOOL            bClientReturnKnown;  // set on first recv from FMpConnection

    // Server mode fields.
    SlotEntry Slots[MAX_P2P_SLOTS];

    FCriticalSection PendingLock;
    QWORD  PendingAccept[MAX_P2P_SLOTS];
    INT    PendingAcceptCount;

    // Relay destination address (127.0.0.1:RelayPort), server mode.
    struct sockaddr_in RelayAddr;

    // Server mode helpers.
    INT  AllocSlot(QWORD SteamID);
    INT  FindSlotBySteamID(QWORD SteamID) const;
    INT  FindSlotByVSock(UINT_PTR s) const;
    void FreeSlot(INT idx);
    void CleanupSlots();
    void DrainPendingAccepts();

    // Per-mode run loops.
    void RunServer();
    void RunClient();
};

extern FP2PBridge GP2PBridge;        // server-side bridge (started with relay)
extern FP2PBridge GP2PBridgeClient;  // client-side bridge (started on ConnectP2P)

// C-callable wrapper — allows OnlineSubsystemSteamworks to forward P2PSessionRequest_t
// to the bridge without pulling in winsock2.h or the full P2PBridge class.
extern void GP2PBridge_OnSessionRequest(QWORD SteamID);
