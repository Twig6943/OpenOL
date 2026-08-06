#include "Multiplayer.h"
#include "HeroChannel.h"
#include "PushableChannel.h"
#include "WorldChannelPackets.h"

// ============================================================================
// Internal helpers
// ============================================================================

static FORCEINLINE UBOOL CanWorldSend()
{
    return GMpConn.bIsConnected && GMpConn.bIsHandshaked;
}

static void SendWorldString(BYTE PktType, const FString& Str)
{
    INT Len = Min(Str.Len(), 255);
    BYTE B[2 + 255];
    B[0] = PktType;
    B[1] = (BYTE)Len;
    for (INT i = 0; i < Len; i++)
        B[2 + i] = (BYTE)((*Str)[i] & 0x7F);
    GMpConn.SendBinary(B, 2 + Len);
}

static void SendWorldIntString(BYTE PktType, INT Value, const FString& Str)
{
    INT Len = Min(Str.Len(), 255);
    BYTE B[6 + 255];
    B[0] = PktType;
    B[1] = (BYTE)(Value & 0xFF);
    B[2] = (BYTE)((Value >> 8) & 0xFF);
    B[3] = (BYTE)((Value >> 16) & 0xFF);
    B[4] = (BYTE)((Value >> 24) & 0xFF);
    B[5] = (BYTE)Len;
    for (INT i = 0; i < Len; i++)
        B[6 + i] = (BYTE)((*Str)[i] & 0x7F);
    GMpConn.SendBinary(B, 6 + Len);
}

// Decode [strLen(1)][ASCII] from Data+Offset into OutStr. Returns false on underflow.
static UBOOL ReadWorldString(BYTE* Data, INT DataLen, INT Offset, FString& OutStr)
{
    if (Offset >= DataLen) return FALSE;
    INT Len = Data[Offset++];
    if (Offset + Len > DataLen) return FALSE;
    TCHAR Buf[256];
    for (INT i = 0; i < Len; i++)
        Buf[i] = (TCHAR)Data[Offset + i];
    Buf[Len] = 0;
    OutStr = FString(Buf);
    return TRUE;
}

// Fires all SeqEvent_RemoteEvent nodes whose EventName matches EventName.
static void TriggerRemoteKismetEvent(AMultiplayerController* Ctrl, FName EventName)
{
    USequence* GameSeq = GWorld->GetWorldInfo()->GetGameSequence();
    if (!GameSeq) return;

    Ctrl->bTriggeringRemoteEvent = TRUE;
    TArray<USequenceObject*> AllEvents;
    GameSeq->FindSeqObjectsByClass(USeqEvent_RemoteEvent::StaticClass(), AllEvents, TRUE);
    for (INT i = 0; i < AllEvents.Num(); i++)
    {
        USeqEvent_RemoteEvent* Evt = Cast<USeqEvent_RemoteEvent>(AllEvents(i));
        if (Evt && Evt->EventName == EventName && Evt->bEnabled)
            Evt->CheckActivate(GWorld->GetWorldInfo(), NULL);
    }
    Ctrl->bTriggeringRemoteEvent = FALSE;
}

// Fires Kismet pickup event for a pickable object found by path.
static AOLPickableObject* FindPickupNear(const FVector& Loc, FLOAT MaxDist = 200.f)
{
    FLOAT BestDist = MaxDist;
    AOLPickableObject* Best = NULL;
    for (FActorIterator It; It; ++It)
    {
        AOLPickableObject* P = Cast<AOLPickableObject>(*It);
        if (!P) continue;
        FLOAT D = FDist(P->Location, Loc);
        if (D < BestDist) { BestDist = D; Best = P; }
    }
    return Best;
}

static FVector ReadPickupLoc(BYTE* Data)
{
    INT IX = (INT)((DWORD)Data[0]|((DWORD)Data[1]<<8)|((DWORD)Data[2]<<16)|((DWORD)Data[3]<<24));
    INT IY = (INT)((DWORD)Data[4]|((DWORD)Data[5]<<8)|((DWORD)Data[6]<<16)|((DWORD)Data[7]<<24));
    INT IZ = (INT)((DWORD)Data[8]|((DWORD)Data[9]<<8)|((DWORD)Data[10]<<16)|((DWORD)Data[11]<<24));
    return FVector((FLOAT)IX, (FLOAT)IY, (FLOAT)IZ);
}

static void TriggerRemotePickupKismetEvent(AMultiplayerController* Ctrl, const FString& PickupPath)
{
    AOLPickableObject* Pickup = Cast<AOLPickableObject>(
        UObject::StaticFindObject(AOLPickableObject::StaticClass(), NULL, *PickupPath, FALSE));
    if (!Pickup) return;

    Ctrl->bTriggeringRemoteEvent = TRUE;
    for (INT i = 0; i < Pickup->GeneratedEvents.Num(); i++)
    {
        UOLSeqEvent_Pickup* Evt = Cast<UOLSeqEvent_Pickup>(Pickup->GeneratedEvents(i));
        if (Evt && Evt->bEnabled)
            Evt->CheckActivate(Pickup, NULL);
    }
    Ctrl->bTriggeringRemoteEvent = FALSE;
}

// Applies a remote touch trigger event by path.
static void ApplyRemoteTriggerAct(AMultiplayerController* Ctrl, const FString& EventPath, INT RemoteTriggerCount)
{
    // Check blacklist
    for (INT i = 0; i < Ctrl->TriggerActBlacklist.Num(); i++)
    {
        const FString& Filter = Ctrl->TriggerActBlacklist(i);
        if (Filter.Len() > 0 && EventPath.ToUpper().InStr(Filter.ToUpper()) != -1)
            return;
    }

    USeqEvent_Touch* TouchEvent = Cast<USeqEvent_Touch>(
        UObject::StaticFindObject(USeqEvent_Touch::StaticClass(), NULL, *EventPath, FALSE));
    if (!TouchEvent || !TouchEvent->bEnabled) return;

    if (RemoteTriggerCount > TouchEvent->TriggerCount)
        TouchEvent->TriggerCount = RemoteTriggerCount;

    if (TouchEvent->MaxTriggerCount > 0 && TouchEvent->TriggerCount >= TouchEvent->MaxTriggerCount)
        return;

    // ForceActivateTouchEvent is on AOLPlayerController
    AOLPlayerController* OLPC = Cast<AOLPlayerController>(Ctrl);
    if (OLPC)
        OLPC->ForceActivateTouchEvent(TouchEvent);
}

// ============================================================================
// Send — pickup state
// ============================================================================

void UWorldChannel::SendPickupState(INT CurSMT)
{
    // Implemented in HeroChannel::SendSpecialMoveType where LastSentSpecialMove and HeroPawn are available.
}

// ============================================================================
// Send — pickup Kismet event
// ============================================================================

void UWorldChannel::SendPickupKismet(AOLPickableObject* Pickup)
{
    if (!CanWorldSend()) return;
    if (!GMpConn.SyncPickups || !GMpConn.SyncMatinees) return;
    if (!Pickup) return;

    SendWorldString( MPKT_WORLD_PICKUP_KISMET, Pickup->GetPathName());
}

// ============================================================================
// Send — CSA activated
// ============================================================================

void UWorldChannel::SendCSA(AOLCSA* CSA)
{
    if (!CSA || !CanWorldSend()) return;
    if (!GMpConn.SyncMatinees) return;

    FString CSAPath = CSA->GetPathName();
    if (ControllerOwner->CSAActBlacklist.FindItemIndex(CSAPath) != INDEX_NONE) return;

    SendWorldString( MPKT_WORLD_CSA, CSAPath);
}

// ============================================================================
// Send — inventory item consumed
// ============================================================================

void UWorldChannel::SendItemConsume(FName ItemName)
{
    if (!CanWorldSend()) return;
    if (!GMpConn.SyncInteractable) return;

    SendWorldString( MPKT_WORLD_ITEM_CONSUME, ItemName.ToString());
}

// ============================================================================
// Send — recording marker
// ============================================================================

void UWorldChannel::SendRecordingMarker(AOLRecordingMarker* Marker)
{
    if (!Marker || !CanWorldSend()) return;
    if (!GMpConn.SyncPickups) return;

    SendWorldString( MPKT_WORLD_RECORDING, Marker->GetPathName());
}

// ============================================================================
// OnPawnTouchedTrigger — local player touched a trigger; send to remote.
// ============================================================================

void UWorldChannel::OnPawnTouchedTrigger(AActor* TriggerActor)
{
    if (!TriggerActor || !CanWorldSend()) return;
    if (!GMpConn.SyncMatinees) return;

    FString Path = TriggerActor->GetPathName();

    // Check blacklist
    for (INT i = 0; i < ControllerOwner->TriggerActBlacklist.Num(); i++)
    {
        const FString& Filter = ControllerOwner->TriggerActBlacklist(i);
        if (Filter.Len() > 0 && Path.ToUpper().InStr(Filter.ToUpper()) != -1)
            return;
    }

    for (INT i = 0; i < TriggerActor->GeneratedEvents.Num(); i++)
    {
        USeqEvent_Touch* TouchEvent = Cast<USeqEvent_Touch>(TriggerActor->GeneratedEvents(i));
        if (TouchEvent && TouchEvent->bEnabled)
        {
            FString EventPath = TouchEvent->GetPathName();
            //debugf(TEXT("[MP] Trigger: %s"), *Path);
            SendWorldIntString(MPKT_WORLD_TRIGGER_ACT, TouchEvent->TriggerCount, EventPath);
        }
    }
}

// ============================================================================
// Receive — nick / disconnect
// ============================================================================

void UWorldChannel::OnNick(const TArray<FString>& Parts, INT SenderID)
{
    // Legacy text NICK — kept for older clients.
    if (Parts.Num() < 3) return;
    OnBinaryNick(SenderID, Parts(2));
}

// Update persistent nick registry in GMpConn.
static void UpdateKnownPlayer(INT PlayerID, const FString& Nick)
{
    for (INT i = 0; i < GMpConn.KnownPlayers.Num(); i++)
        if (GMpConn.KnownPlayers(i).PlayerID == PlayerID)
        {
            GMpConn.KnownPlayers(i).Username = Nick;
            return;
        }
    FMpConnection::FRemoteNick Entry;
    Entry.PlayerID = PlayerID;
    Entry.Username = Nick;
    GMpConn.KnownPlayers.AddItem(Entry);
}

static void RemoveKnownPlayer(INT PlayerID)
{
    for (INT i = GMpConn.KnownPlayers.Num() - 1; i >= 0; i--)
        if (GMpConn.KnownPlayers(i).PlayerID == PlayerID)
            GMpConn.KnownPlayers.Remove(i, 1);
}

static FString GetKnownNick(INT PlayerID)
{
    for (INT i = 0; i < GMpConn.KnownPlayers.Num(); i++)
        if (GMpConn.KnownPlayers(i).PlayerID == PlayerID)
            return GMpConn.KnownPlayers(i).Username;

    // Fallback: check RemotePlayers on the active controller.
    if (GMultiplayerController)
    {
        INT Idx = GMultiplayerController->FindRemoteIndex(PlayerID);
        if (Idx >= 0 && GMultiplayerController->RemotePlayers(Idx))
        {
            const FString& Nick = GMultiplayerController->RemotePlayers(Idx)->PlayerNick;
            if (Nick.Len() > 0)
                return Nick;
        }
    }
    return FString::Printf(TEXT("Player%d"), PlayerID);
}

void UWorldChannel::OnBinaryNick(INT SenderID, const FString& Nick)
{
    if (SenderID == GMpConn.LocalPlayerID) return;

    UpdateKnownPlayer(SenderID, Nick);

    INT Idx = ControllerOwner->FindRemoteIndex(SenderID);
    if (Idx < 0)
    {
        ControllerOwner->RegisterRemotePlayer(SenderID, Nick);

        // Respond with our nick
        if (CanWorldSend())
        {
            FString OurNick = GMpConn.Username.Len() > 0
                ? FString(GMpConn.Username) : FString(TEXT("Player"));
            SendWorldString(MPKT_WORLD_NICK, OurNick);
        }
    }
    else
    {
        ControllerOwner->RemotePlayers(Idx)->PlayerNick = Nick;
    }
}

void UWorldChannel::OnDisconnected(const TArray<FString>& Parts, INT SenderID)
{
    FString Name = GetKnownNick(SenderID);
    RemoveKnownPlayer(SenderID);

    HUD_AddNotification(Cast<AMultiplayerHUD>(ControllerOwner->myHUD), Name + TEXT(" disconnected"));

    ControllerOwner->eventOnPlayerDisconnected(SenderID);
}

// ============================================================================
// Receive — trigger / trigger act
// ============================================================================

void UWorldChannel::OnTrigger(const TArray<FString>& Parts, INT SenderID)
{
    // Legacy TRIGGER packet — no longer sent; kept for older clients.
    if (Parts.Num() < 3 || !GMpConn.SyncMatinees) return;
    TriggerRemoteKismetEvent(ControllerOwner, FName(*Parts(2)));
}

void UWorldChannel::OnTriggerAct(const TArray<FString>& Parts, INT SenderID)
{
    // Legacy text path — kept for older clients.
    if (Parts.Num() < 3 || !GMpConn.SyncMatinees) return;
    INT Count = Parts.Num() >= 4 ? appAtoi(*Parts(3)) : 0;
    ApplyRemoteTriggerAct(ControllerOwner, Parts(2), Count);
}

void UWorldChannel::OnCSA(const TArray<FString>& Parts, INT SenderID)
{
    if (Parts.Num() < 3 || !GMpConn.SyncMatinees) return;
    const FString& CSAPath = Parts(2);
    if (ControllerOwner->CSAActBlacklist.FindItemIndex(CSAPath) != INDEX_NONE) return;
    AOLCSA* CSA = Cast<AOLCSA>(UObject::StaticFindObject(AOLCSA::StaticClass(), NULL, *CSAPath, FALSE));
    if (!CSA || (CSA->MaxTriggerCount > 0 && CSA->TriggerCount >= CSA->MaxTriggerCount)) return;
    ControllerOwner->bExcludeFromKismetPlayer = TRUE;
    CSA->RemoteActivate(TRUE);
}

void UWorldChannel::OnItemConsume(const TArray<FString>& Parts, INT SenderID)
{
    if (Parts.Num() < 3 || !GMpConn.SyncInteractable) return;
    AOLHero* Hero = Cast<AOLHero>(ControllerOwner->Pawn);
    if (!Hero || !Hero->OLPC || !Hero->OLPC->InventoryManager) return;
    Hero->OLPC->InventoryManager->ConsumeItem(FName(*Parts(2)));
}

void UWorldChannel::OnLevel(const TArray<FString>& Parts, INT SenderID)
{
    if (!ControllerOwner->bDoorsIndexed || !ControllerOwner->bPushablesIndexed)
    {
        UBOOL bNewDoors = ControllerOwner->IndexDoors();
        UBOOL bNewPush  = ControllerOwner->IndexPushables();
        if (bNewDoors || bNewPush)
            ControllerOwner->ApplyPendingPushStates();
    }
}

void UWorldChannel::OnPickupState(const TArray<FString>& Parts, INT SenderID)
{
    if (Parts.Num() < 5 || !GMpConn.SyncPickups) return;
    FVector PickupLoc(appAtof(*Parts(2)), appAtof(*Parts(3)), appAtof(*Parts(4)));
    FLOAT BestDist = 200.0f;
    AOLPickableObject* Best = NULL;
    for (FActorIterator It; It; ++It)
    {
        AOLPickableObject* P = Cast<AOLPickableObject>(*It);
        if (!P) continue;
        FLOAT D = FDist(P->Location, PickupLoc);
        if (D < BestDist) { BestDist = D; Best = P; }
    }
    if (Best && Best->PickupMesh)
        Best->PickupMesh->SetHiddenGame(TRUE);
}

void UWorldChannel::OnPickupKismet(const TArray<FString>& Parts, INT SenderID)
{
    if (Parts.Num() < 3 || !GMpConn.SyncPickups || !GMpConn.SyncMatinees) return;
    TriggerRemotePickupKismetEvent(ControllerOwner, Parts(2));
}

void UWorldChannel::OnRecordingMarker(const TArray<FString>& Parts, INT SenderID)
{
    if (Parts.Num() < 3) return;
    AOLRecordingMarker* Marker = Cast<AOLRecordingMarker>(
        UObject::StaticFindObject(AOLRecordingMarker::StaticClass(), NULL, *Parts(2), FALSE));
    if (!Marker || Marker->bRecorded) return;
    AOLPlayerController* OLPC = Cast<AOLPlayerController>(ControllerOwner);
    if (OLPC) OLPC->NativeApplyRemoteRecording(Marker);
}

// ============================================================================
// Receive — binary world packets
// ============================================================================

void UWorldChannel::OnBinaryWorldPacket(INT SenderID, BYTE PktType, BYTE* Data, INT DataLen)
{
    FString Str;
    switch (PktType)
    {
    case MPKT_WORLD_NICK:
        if (ReadWorldString(Data, DataLen, 0, Str))
            OnBinaryNick(SenderID, Str);
        break;

    case MPKT_WORLD_TRIGGER_ACT:
        if (DataLen >= 5 && GMpConn.SyncMatinees)
        {
            INT Count = (INT)(Data[0] | (Data[1] << 8) | (Data[2] << 16) | (Data[3] << 24));
            if (ReadWorldString(Data, DataLen, 4, Str))
                ApplyRemoteTriggerAct(ControllerOwner, Str, Count);
        }
        break;

    case MPKT_WORLD_CSA:
        if (ReadWorldString(Data, DataLen, 0, Str) && GMpConn.SyncMatinees)
        {
            AOLCSA* CSA = Cast<AOLCSA>(UObject::StaticFindObject(AOLCSA::StaticClass(), NULL, *Str, FALSE));
            if (ControllerOwner->CSAActBlacklist.FindItemIndex(Str) == INDEX_NONE &&
                CSA && !(CSA->MaxTriggerCount > 0 && CSA->TriggerCount >= CSA->MaxTriggerCount))
            {
                AOLPlayerController* OLPC = Cast<AOLPlayerController>(ControllerOwner);
                if (OLPC)
                {
                    ControllerOwner->bExcludeFromKismetPlayer = TRUE;
                    OLPC->ObserverActivateCSA(CSA, TRUE);
                    ControllerOwner->bExcludeFromKismetPlayer = FALSE;
                }
            }
        }
        break;

    case MPKT_WORLD_ITEM_CONSUME:
        if (ReadWorldString(Data, DataLen, 0, Str) && GMpConn.SyncInteractable)
        {
            AOLHero* Hero = Cast<AOLHero>(ControllerOwner->Pawn);
            if (Hero && Hero->OLPC && Hero->OLPC->InventoryManager)
                Hero->OLPC->InventoryManager->ConsumeItem(FName(*Str));
        }
        break;

    case MPKT_WORLD_PICKUP_KISMET:
        if (ReadWorldString(Data, DataLen, 0, Str) && GMpConn.SyncPickups && GMpConn.SyncMatinees)
            TriggerRemotePickupKismetEvent(ControllerOwner, Str);
        break;

    case MPKT_WORLD_RECORDING:
        if (ReadWorldString(Data, DataLen, 0, Str))
        {
            AOLRecordingMarker* Marker = Cast<AOLRecordingMarker>(
                UObject::StaticFindObject(AOLRecordingMarker::StaticClass(), NULL, *Str, FALSE));
            if (Marker && !Marker->bRecorded)
            {
                AOLPlayerController* OLPC = Cast<AOLPlayerController>(ControllerOwner);
                if (OLPC) OLPC->NativeApplyRemoteRecording(Marker);
            }
        }
        break;

    case MPKT_WORLD_PICKUP_START:
        if (DataLen >= 12 && GMpConn.SyncPickups)
        {
            // Store pickup location on the remote player slot for ATTACH lookup
            INT Idx = ControllerOwner->FindRemoteIndex(SenderID);
            if (Idx != -1)
                ControllerOwner->RemotePlayers(Idx)->LastRemotePickupLoc = ReadPickupLoc(Data);
        }
        break;

    case MPKT_WORLD_PICKUP_ATTACH:
        if (DataLen >= 12 && GMpConn.SyncPickups)
        {
            INT Idx = ControllerOwner->FindRemoteIndex(SenderID);
            if (Idx != -1)
            {
                AOLHero* Dummy = Cast<AOLHero>(ControllerOwner->RemotePlayers(Idx)->DummyPlayer);
                AOLPickableObject* Best = FindPickupNear(ReadPickupLoc(Data));
                if (Dummy && Best)
                {
                    Dummy->AttachPickupMeshToDummyHand(Best);
                    TriggerRemotePickupKismetEvent(ControllerOwner, Best->GetPathName());
                }
            }
        }
        break;

    case MPKT_WORLD_PICKUP_STATE:
        if (DataLen >= 12 && GMpConn.SyncPickups)
        {
            AOLPickableObject* Best = FindPickupNear(ReadPickupLoc(Data));
            if (Best && !Best->bUsed)
            {
                Best->bUsed = TRUE;
                if (Best->PickupMesh)
                    Best->PickupMesh->SetHiddenGame(TRUE);

                AOLPlayerController* OLPC = Cast<AOLPlayerController>(ControllerOwner);
                if (OLPC && OLPC->InventoryManager)
                {
                    AOLCollectiblePickup* Doc = Cast<AOLCollectiblePickup>(Best);
                    AOLGameplayItemPickup* Item = Doc ? NULL : Cast<AOLGameplayItemPickup>(Best);
                    if (Doc)
                    {
                        OLPC->InventoryManager->AddCollectible(Doc->CollectibleName);
                        if (OLPC->myHUD)
                            Cast<AOLHUD>(OLPC->myHUD)->SetLatestDocument(Doc->CollectibleName);
                    }
                    else if (Item)
                    {
                        OLPC->InventoryManager->AddUniqueItem(Item->ItemName);
                    }
                }
            }
        }
        break;

    default: break;
    }
}
