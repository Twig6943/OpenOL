#include "Multiplayer.h"
#include "OLGameClasses.h"
#include "..\..\D3D9Drv\Src\OLImGui.h"
#include "HeroChannel.h"
#include "HeroChannelPackets.h"
#include "DoorChannel.h"
#include "EnemyChannel.h"
#include "EnemyChannelPackets.h"
#include "PushableChannel.h"
#include "WorldChannelPackets.h"
#include "ServerPackets.h"
#include "MultiplayerHUD.h"

extern FMpConnectionTicker* GMpTicker;

// ImGui debug overlay: spawn a Soldier near the local player via OLCheatManager::SpawnEnemy.
static void ImGuiAction_SpawnSoldier()
{
    if (!GEngine || GEngine->GamePlayers.Num() == 0)
        return;
    ULocalPlayer* LP = GEngine->GamePlayers(0);
    if (!LP || !LP->Actor || !LP->Actor->CheatManager)
        return;
    UOLCheatManager* CM = Cast<UOLCheatManager>(LP->Actor->CheatManager);
    if (!CM)
        return;

    struct FSpawnEnemyParms
    {
        FString EnemyType;
        BYTE    WeaponToUse;
        UBOOL   ShouldAttack;
    } Parms;
    Parms.EnemyType   = TEXT("Soldier");
    Parms.WeaponToUse = 0; // EWeaponToUse::Weapon_None
    Parms.ShouldAttack = TRUE;
    CM->ProcessEvent(CM->FindFunctionChecked(FName(TEXT("SpawnEnemy"))), &Parms);
}

void AMultiplayerController::NativeInit()
{
    debugf(TEXT("[MP] NativeInit"));
    GMultiplayerController            = this;
    GMultiplayerHero                  = NULL;
    GHeroChannelTicker.Channel        = HeroChannel;
    GHeroChannelReceiveTicker.Channel = HeroChannel;

    if (!GMpTicker)
    {
        GMpTicker = new FMpConnectionTicker();
        atexit([]() { GMpConn.Disconnect(); });
    }

    // Register ImGui debug overlay actions (idempotent: only adds once per process lifetime).
    static bool bActionsRegistered = false;
    if (!bActionsRegistered)
    {
        OLImGui_RegisterAction("Spawn Soldier", ImGuiAction_SpawnSoldier);
        bActionsRegistered = true;
    }

    GMpConn.Connect();

    if (GMpConn.bIsConnected)
    {
        ServerName  = GMpConn.ServerName;
        OnlineCount = GMpConn.OnlineCount;

        for (INT i = 0; i < GMpConn.KnownPlayers.Num(); i++)
        {
            const FMpConnection::FRemoteNick& KP = GMpConn.KnownPlayers(i);
            if (KP.PlayerID != GMpConn.LocalPlayerID)
                RegisterRemotePlayer(KP.PlayerID, KP.Username);
        }
    }
}

void AMultiplayerController::NativeDestroyed()
{
    GMultiplayerController            = NULL;
    GMultiplayerHero                  = NULL;
    GHeroChannelTicker.Channel        = NULL;
    GHeroChannelReceiveTicker.Channel = NULL;
    HeroChannel     = NULL;
    DoorChannel     = NULL;
    PushableChannel = NULL;
    EnemyChannel    = NULL;
    WorldChannel    = NULL;
}

void AMultiplayerController::NativeSetHero(AOLHero* Hero)
{
    GMultiplayerHero = Hero;
}

UBOOL AMultiplayerController::IsConnected()
{
    return GMpConn.bIsConnected && GMpConn.bIsHandshaked;
}

UBOOL AMultiplayerController::IsReady()
{
    return GMultiplayerHero != NULL && GMpConn.bIsConnected && GMpConn.bIsHandshaked;
}

FString AMultiplayerController::NativeGetUsername()
{
    return GMpConn.Username.Len() > 0 ? FString(GMpConn.Username) : FString(TEXT("Player"));
}


INT AMultiplayerController::FindRemoteIndex(INT PlayerID)
{
    for (INT i = 0; i < RemotePlayers.Num(); i++)
        if (RemotePlayers(i) && RemotePlayers(i)->PlayerID == PlayerID)
            return i;
    return -1;
}

// ============================================================================
// Connection events
// ============================================================================

void AMultiplayerController::OnConnected()
{
    GMpConn.bIsHandshaked = TRUE;
    if (HeroChannel)
    {
        HeroChannel->LastSentSpecialMove   = -1;
        HeroChannel->LastSentMeshPreset    = -1;
        HeroChannel->LastSentCinematicAnim = TEXT("");
        HeroChannel->bSendingJumpGroundZ   = FALSE;
    }
    GMpConn.PingTimer = 0.5f;

    // Send binary HELLO (0x02): [nick_len(1)][nick ASCII...]
    FString Nick = GMpConn.Username.Len() > 0
        ? FString(GMpConn.Username)
        : FString(TEXT("Player"));
    INT NickLen = Min(Nick.Len(), 32);

    BYTE B[2 + 32];
    INT N = 0;
    N = PutU8(B, N, MPKT_HELLO);
    N = PutU8(B, N, (BYTE)NickLen);
    for (INT i = 0; i < NickLen; i++)
        B[N++] = (BYTE)((*Nick)[i] & 0x7F);
    GMpConn.SendBinary(B, N);
}

void AMultiplayerController::OnDisconnected()
{
    GMpConn.bIsHandshaked = FALSE;
    GMpConn.OnlineCount = 0;
    GMpConn.KnownPlayers.Empty();
    CurrentPingMs = 0.f;
    LastPongTime  = 0.f;
    // Clean up all remote players
    for (INT i = RemotePlayers.Num() - 1; i >= 0; i--)
    {
        if (RemotePlayers(i))
            eventOnPlayerDisconnected(RemotePlayers(i)->PlayerID);
    }

    HUD_AddNotification(Cast<AMultiplayerHUD>(myHUD),
        FString::Printf(TEXT("[%s] Disconnected. Reconnecting..."), *GMpConn.ServerName));
}

void AMultiplayerController::NativeRemoveRemotePlayer(INT PlayerID)
{
    INT Idx = FindRemoteIndex(PlayerID);
    if (Idx == -1)
        return;

    URemotePlayer* P = RemotePlayers(Idx);

    // Release any door this player was holding.
    INT LockedIdx = P->LockedDoorIdx;
    if (LockedIdx != -1 && LockedIdx < CachedDoors.Num())
    {
        AOLDoor* D = Cast<AOLDoor>(CachedDoors(LockedIdx));
        if (D && D->DoorUser == NULL &&
            (D->DoorState == DS_PlayerInteracting ||
             D->DoorState == DS_Opening ||
             D->DoorState == DS_Closing))
            D->DoorState = DS_Idle;
        if (LockedIdx < RemoteDoorLockExpiry.Num())
            RemoteDoorLockExpiry(LockedIdx) = 0.f;
    }

    // Destroy the dummy hero pawn.
    if (P->DummyPlayer)
    {
        GWorld->DestroyActor(P->DummyPlayer);
        P->DummyPlayer = NULL;
    }

    // Destroy all dummy enemies that belonged to this player and remove from the list.
    for (INT j = RemoteEnemies.Num() - 1; j >= 0; j--)
    {
        if (RemoteEnemies(j).OwnerID == PlayerID)
        {
            if (RemoteEnemies(j).DummyEnemy)
                GWorld->DestroyActor(RemoteEnemies(j).DummyEnemy);
            RemoteEnemies.Remove(j, 1);
        }
    }

    RemotePlayers.Remove(Idx, 1);
}

void AMultiplayerController::NativeDestroyRemoteEnemies()
{
    // Destroy all remote enemy dummy actors and clear the array.
    for (INT i = 0; i < RemoteEnemies.Num(); i++)
    {
        if (RemoteEnemies(i).DummyEnemy)
            GWorld->DestroyActor(RemoteEnemies(i).DummyEnemy);
    }
    RemoteEnemies.Empty();
}

// ============================================================================
// Text packet routing
// ============================================================================

void AMultiplayerController::OnReceiveData(const FString& Data)
{
    // All packets are binary now; text path is unused.
    (void)Data;
}

// ============================================================================
// Binary packet routing
// ============================================================================

void AMultiplayerController::OnReceiveBinaryData(BYTE PktType, INT SenderID, BYTE* Data, INT DataLen)
{
    switch (PktType)
    {
        // ---- Server → client notifications ----
        // ---- Server → client notifications (SenderID=0, payload starts at Data[0]) ----
        case SRV_READY:
            // Handled in MultiplayerLink.cpp (parses into GMpConn before this call).
            ServerName  = GMpConn.ServerName;
            OnlineCount = GMpConn.OnlineCount;
            break;
        case SRV_ONLINE_COUNT:
        {
            // payload: [count LE4]
            if (DataLen < 4) break;
            INT Count = (INT)((DWORD)Data[0] | ((DWORD)Data[1]<<8) | ((DWORD)Data[2]<<16) | ((DWORD)Data[3]<<24));
            GMpConn.OnlineCount = Max(Count, 1);
            OnlineCount = GMpConn.OnlineCount;
            break;
        }
        case SRV_HELLO_FAIL:
            // payload: [reason_len(1)][reason ASCII] — no action needed in C++
            break;
        case SRV_DISCONNECT:
        {
            // payload: [player_id LE4]
            if (DataLen < 4) break;
            INT PID = (INT)((DWORD)Data[0] | ((DWORD)Data[1]<<8) | ((DWORD)Data[2]<<16) | ((DWORD)Data[3]<<24));
            if (WorldChannel)
            {
                TArray<FString> Parts;
                Parts.AddItem(FString::Printf(TEXT("%d"), PID));
                Parts.AddItem(TEXT("DISCONNECT"));
                WorldChannel->OnDisconnected(Parts, PID);
            }
            break;
        }
        case SRV_DOOR_DENY:
        {
            // payload: [key_len(1)][key ASCII], key is "X,Y,Z"
            if (DataLen < 2) break;
            BYTE KLen = Data[0];
            INT  KBytes = Min((INT)KLen, DataLen - 1);
            TCHAR TmpKey[256] = {0};
            for (INT i = 0; i < KBytes && i < 255; i++)
                TmpKey[i] = (TCHAR)Data[1 + i];
            if (DoorChannel)
            {
                TArray<FString> KParts;
                FString(TmpKey).ParseIntoArray(&KParts, TEXT(","), TRUE);
                if (KParts.Num() >= 3)
                    DoorChannel->OnDoorDeny(appAtoi(*KParts(0)), appAtoi(*KParts(1)), appAtoi(*KParts(2)));
            }
            break;
        }

        // ---- Hero channel ----
        case MPKT_STATE:          HeroChannel->OnBinaryLoc(SenderID, Data, DataLen);            break;
        case MPKT_HEAD_ROT:       HeroChannel->OnBinaryHeadRot(SenderID, Data, DataLen);         break;
        case MPKT_MESH_PRESET:    HeroChannel->OnBinaryMesh(SenderID, Data, DataLen);            break;
        case MPKT_CINEMATIC_ANIM: HeroChannel->OnBinaryCinematicAnim(SenderID, Data, DataLen);   break;
        case MPKT_SMT_TYPE:       HeroChannel->OnBinarySmtType(SenderID, Data, DataLen);         break;
        case MPKT_PLAYER_EVENT:      HeroChannel->OnBinaryPlayerEvent(SenderID, Data, DataLen);      break;
        case MPKT_PLAYER_LIFECYCLE:  HeroChannel->OnBinaryPlayerLifecycle(SenderID, Data, DataLen);  break;
        case MPKT_ENPC_LOC:
            if (EnemyChannel) EnemyChannel->OnBinaryLoc(SenderID, Data, DataLen);
            break;
        // ---- Enemy channel ----
        case MPKT_ENPC_SPAWN:
        case MPKT_ENPC_DEL:
        case MPKT_ENPC_SMT:
        case MPKT_ENPC_DOOR_OPEN:
        case MPKT_ENPC_DOOR_DONE:
        case MPKT_ENPC_DOOR_BASH:
        case MPKT_ENPC_DOOR_BREAK:
            if (EnemyChannel) EnemyChannel->OnBinaryPacket(SenderID, PktType, Data, DataLen);
            break;
        // ---- Door channel ----
        case MPKT_DOOR_LOCK:
        case MPKT_DOOR_UNLOCK:
        case MPKT_DOOR_STATE:
        case MPKT_DOOR_OPEN:
        case MPKT_DOOR_CLOSE:
        case MPKT_DOOR_ANGLE:
        case MPKT_DOOR_PARAMS:
            if (DoorChannel) DoorChannel->OnBinaryPacket(SenderID, PktType, Data, DataLen);
            break;
        case MPKT_PUSH_STATE:
            PushableChannel_OnBinaryPushState(PushableChannel, SenderID, Data, DataLen);
            break;
        // ---- World channel ----
        case MPKT_WORLD_NICK:
        case MPKT_WORLD_TRIGGER_ACT:
        case MPKT_WORLD_ITEM_CONSUME:
        case MPKT_WORLD_PICKUP_KISMET:
        case MPKT_WORLD_PICKUP_STATE:
        case MPKT_WORLD_PICKUP_START:
        case MPKT_WORLD_PICKUP_ATTACH:
        case MPKT_WORLD_RECORDING:
            if (WorldChannel) WorldChannel->OnBinaryWorldPacket(SenderID, PktType, Data, DataLen);
            break;
        default: break;
    }
}

INT AMultiplayerController::RegisterRemotePlayer(INT PlayerID, const FString& Nick)
{
    URemotePlayer* P = ConstructObject<URemotePlayer>(URemotePlayer::StaticClass(), this);
    P->PlayerID         = PlayerID;
    P->LockedDoorIdx    = -1;
    P->LastRemoteHealth = 100;
    P->DummyPlayer      = SpawnDummy(this);
    AOLHero* Dummy = Cast<AOLHero>(P->DummyPlayer);
    if (Dummy) Dummy->DummyOwnerID = PlayerID;

    FString NickStr = Nick.Len() > 0 ? Nick : FString::Printf(TEXT("Player%d"), PlayerID);
    P->PlayerNick = NickStr;

    RemotePlayers.AddItem(P);


    return RemotePlayers.Num() - 1;
}

UBOOL AMultiplayerController::IndexDoors()
{
    TArray<AActor*> NewDoors;
    for (FActorIterator It; It; ++It)
    {
        AOLDoor* D = Cast<AOLDoor>(*It);
        if (D && !D->bDeleteMe && !D->bPendingDelete)
            NewDoors.AddItem(D);
    }

    TArray<FLOAT> NewAngles, NewExpiry;
    NewAngles.AddZeroed(NewDoors.Num());
    NewExpiry.AddZeroed(NewDoors.Num());

    UBOOL bAnyNew = FALSE;
    for (INT i = 0; i < NewDoors.Num(); i++)
    {
        UBOOL bFound = FALSE;
        for (INT j = 0; j < CachedDoors.Num(); j++)
        {
            if (CachedDoors(j) == NewDoors(i))
            {
                NewAngles(i) = LastSentDoorAngle(j);
                NewExpiry(i) = RemoteDoorLockExpiry(j);
                bFound = TRUE;
                break;
            }
        }
        if (!bFound)
        {
            NewAngles(i) = -9999.0f;
            bAnyNew = TRUE;
        }
    }

    CachedDoors          = NewDoors;
    LastSentDoorAngle    = NewAngles;
    RemoteDoorLockExpiry = NewExpiry;
    bDoorsIndexed        = TRUE;
    return bAnyNew;
}

// ============================================================================
// Remote player management — previously in UC
// ============================================================================

void AMultiplayerController::RemoveRemotePlayer(INT PlayerID)
{
    INT Idx = FindRemoteIndex(PlayerID);
    if (Idx < 0)
        return;

    URemotePlayer* P = RemotePlayers(Idx);
    INT LockedIdx = P->LockedDoorIdx;
    if (LockedIdx >= 0 && LockedIdx < CachedDoors.Num())
    {
        AOLDoor* D = Cast<AOLDoor>(CachedDoors(LockedIdx));
        if (D && !D->DoorUser &&
            (D->DoorState == DS_PlayerInteracting || D->DoorState == DS_Opening || D->DoorState == DS_Closing))
            D->DoorState = DS_Idle;
    }

    if (P->DummyPlayer && GWorld)
        GWorld->DestroyActor(P->DummyPlayer);

    for (INT j = RemoteEnemies.Num() - 1; j >= 0; j--)
    {
        if (RemoteEnemies(j).OwnerID == P->PlayerID)
        {
            if (RemoteEnemies(j).DummyEnemy && GWorld)
                GWorld->DestroyActor(RemoteEnemies(j).DummyEnemy);
            RemoteEnemies.Remove(j, 1);
        }
    }

    RemotePlayers.Remove(Idx, 1);
}

void AMultiplayerController::DestroyRemoteEnemies()
{
    if (GWorld)
        for (INT i = 0; i < RemoteEnemies.Num(); i++)
            if (RemoteEnemies(i).DummyEnemy)
                GWorld->DestroyActor(RemoteEnemies(i).DummyEnemy);
    RemoteEnemies.Empty();
}

// ============================================================================
// Game code callbacks
// ============================================================================

void AMultiplayerController::NotifyDummyPlayerHit(AOLHero* DummyTarget, FLOAT Damage, FLOAT KnockbackPower, FVector HitDir)
{
    if (DummyTarget && HeroChannel)
        HeroChannel->SendPlayerHit(DummyTarget->DummyOwnerID, Damage, KnockbackPower, HitDir);
}

void AMultiplayerController::NotifyDummyPlayerGrab(INT TargetPlayerID, FVector GrabTargetLoc, FVector CharDir, UBOOL bCrouched, INT EnemyTypeInt, FLOAT BlendAlpha, UBOOL bLeftAnim, INT GrabType)
{
    if (HeroChannel)
        HeroChannel->SendPlayerGrab(TargetPlayerID, GrabTargetLoc, CharDir, bCrouched, EnemyTypeInt, BlendAlpha, bLeftAnim, GrabType);
}

void AMultiplayerController::NotifyDummyPlayerThrow(INT TargetPlayerID, FLOAT ThrowRotation)
{
    if (HeroChannel)
        HeroChannel->SendPlayerThrow(TargetPlayerID, ThrowRotation);
}

void AMultiplayerController::NotifyDummyPlayerKill(INT TargetPlayerID, INT EnemyTypeInt, INT WeaponType, UBOOL bBackAnim, UBOOL bLeftAnim, FLOAT BlendAlpha, FVector AnimStart, FVector CharDir, INT KillType, INT VictimYaw)
{
    if (HeroChannel)
        HeroChannel->SendPlayerKill(TargetPlayerID, EnemyTypeInt, WeaponType, bBackAnim, bLeftAnim, BlendAlpha, AnimStart, CharDir, KillType, VictimYaw);
}

void AMultiplayerController::NotifyDummyEnemySMT(AOLEnemyPawn* Enemy, INT SMTType, INT Param1, INT Param2)
{
    if (Enemy && EnemyChannel)
        EnemyChannel->SendSMTDirect(Enemy, SMTType, Param1, Param2);
}

void AMultiplayerController::NativeNotifyEnemyDoorOpen(AOLEnemyPawn* Enemy, AOLDoor* D, FLOAT Speed, FLOAT Angle)
{
    debugf(TEXT("### NativeNotifyEnemyDoorOpen: Enemy=%s Door=%s Speed=%.1f Angle=%.1f Chan=%s"),
        Enemy ? *Enemy->GetName() : TEXT("NULL"),
        D ? *D->GetName() : TEXT("NULL"),
        Speed, Angle,
        EnemyChannel ? TEXT("OK") : TEXT("NULL"));
    if (Enemy && D && EnemyChannel)
        EnemyChannel->SendEnemyDoorOpen(Enemy, D, Speed, Angle);
}

void AMultiplayerController::NativeNotifyEnemyDoorDone(AOLEnemyPawn* Enemy, AOLDoor* D, FLOAT CloseSpeed)
{
    debugf(TEXT("### NativeNotifyEnemyDoorDone: Enemy=%s Door=%s Speed=%.1f Chan=%s"),
        Enemy ? *Enemy->GetName() : TEXT("NULL"),
        D ? *D->GetName() : TEXT("NULL"),
        CloseSpeed,
        EnemyChannel ? TEXT("OK") : TEXT("NULL"));
    if (Enemy && D && EnemyChannel)
        EnemyChannel->SendEnemyDoorDone(Enemy, D, CloseSpeed);
}

void AMultiplayerController::NotifyEnemyDoorBash(AOLEnemyPawn* Enemy, AOLDoor* D, UBOOL bReversed)
{
    debugf(TEXT("### NotifyEnemyDoorBash: Enemy=%s Door=%s EnemyChannel=%s"),
        Enemy ? *Enemy->GetName() : TEXT("NULL"),
        D ? *D->GetName() : TEXT("NULL"),
        EnemyChannel ? TEXT("OK") : TEXT("NULL"));
    if (Enemy && D && EnemyChannel)
        EnemyChannel->SendEnemyDoorBash(Enemy, D, bReversed);
}

void AMultiplayerController::NotifyEnemyDoorBreak(AOLEnemyPawn* Enemy, AOLDoor* D, UBOOL bReversed)
{
    debugf(TEXT("### NotifyEnemyDoorBreak: Enemy=%s Door=%s EnemyChannel=%s"),
        Enemy ? *Enemy->GetName() : TEXT("NULL"),
        D ? *D->GetName() : TEXT("NULL"),
        EnemyChannel ? TEXT("OK") : TEXT("NULL"));
    if (Enemy && D && EnemyChannel)
        EnemyChannel->SendEnemyDoorBreak(Enemy, D, bReversed);
}

void AMultiplayerController::NotifyPawnTouchedTrigger(AActor* TriggerActor)
{
    if (WorldChannel)
        WorldChannel->OnPawnTouchedTrigger(TriggerActor);
}

void AMultiplayerController::OnInventoryItemConsumed(FName ItemName)
{
    if (WorldChannel)
        WorldChannel->SendItemConsume(ItemName);
}

void AMultiplayerController::OnPickupKismetEvent(AOLPickableObject* Pickup)
{
    if (WorldChannel)
        WorldChannel->SendPickupKismet(Pickup);
}

void AMultiplayerController::OnLocalDoorOpen(AOLDoor* D)
{
    if (DoorChannel)
        DoorChannel->OnLocalDoorOpen(D);
}

void AMultiplayerController::OnLocalDoorClose(AOLDoor* D)
{
    if (DoorChannel)
        DoorChannel->OnLocalDoorClose(D);
}

void AMultiplayerController::OnRecordingMarkerCompleted(AOLRecordingMarker* Marker)
{
    if (WorldChannel)
        WorldChannel->SendRecordingMarker(Marker);
}

void AMultiplayerController::OnToggleCinematicMode(USeqAct_ToggleCinematicMode* Action)
{
    if (Role < ROLE_Authority) return;
    UBOOL bNewCinematicMode;
    if      (Action->InputLinks(0).bHasImpulse) bNewCinematicMode = TRUE;
    else if (Action->InputLinks(1).bHasImpulse) bNewCinematicMode = FALSE;
    else                                         bNewCinematicMode = !bCinematicMode;
    // bObserverOnly means the matinee only affects observers — skip without mutating the flag,
    // so the real OLPlayerController still sees the original value when it processes the action.
    if (Action->bObserverOnly) return;
    eventSetCinematicMode(Action, bNewCinematicMode, Action->bHidePlayer, Action->bHideHUD,
        Action->bDisableMovement, Action->bDisableTurning, Action->bDisableInput);
}

void AMultiplayerController::InterpolationStarted(USeqAct_Interp* InterpAction, UInterpGroupInst* GroupInst)
{
    if (WorldChannel && GMpConn.bIsConnected)
        WorldChannel->SendMatineeState();
}

void AMultiplayerController::InterpolationFinished(USeqAct_Interp* InterpAction)
{
    if (WorldChannel && GMpConn.bIsConnected)
        WorldChannel->SendMatineeState();
}
