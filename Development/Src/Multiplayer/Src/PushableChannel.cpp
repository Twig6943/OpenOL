#include "Multiplayer.h"
#include "PushableChannel.h"
#include "HeroChannel.h"    // PutI32 / ReadI32 helpers

// ============================================================================
// IndexPushables — fills CachedPushables, preserves LastSentPushDisplacement.
// Returns TRUE if any new pushable was discovered.
// ============================================================================

UBOOL AMultiplayerController::IndexPushables()
{
    TArray<AOLPushableObject*> NewPushables;
    for (FActorIterator It; It; ++It)
    {
        AOLPushableObject* P = Cast<AOLPushableObject>(*It);
        if (P && !P->bDeleteMe && !P->bPendingDelete)
            NewPushables.AddItem(P);
    }

    TArray<FLOAT> NewDisp;
    NewDisp.AddZeroed(NewPushables.Num());

    UBOOL bAnyNew = FALSE;
    for (INT i = 0; i < NewPushables.Num(); i++)
    {
        UBOOL bFound = FALSE;
        for (INT j = 0; j < CachedPushables.Num(); j++)
        {
            if (CachedPushables(j) == NewPushables(i))
            {
                NewDisp(i) = LastSentPushDisplacement(j);
                bFound = TRUE;
                break;
            }
        }
        if (!bFound)
            bAnyNew = TRUE;
    }

    CachedPushables          = NewPushables;
    LastSentPushDisplacement = NewDisp;
    bPushablesIndexed        = TRUE;
    return bAnyNew;
}

// ============================================================================
// ApplyPendingPushStates — flush states received before actors were loaded.
// ============================================================================

void AMultiplayerController::ApplyPendingPushStates()
{
    for (INT i = PendingPushStates.Num() - 1; i >= 0; i--)
    {
        AOLPushableObject* P = PushableChannel
            ? PushableChannel->FindPushableByKey(PendingPushStates(i).KeyX, PendingPushStates(i).KeyY, PendingPushStates(i).KeyZ)
            : NULL;
        if (P)
        {
            if (!P->bPlayerLocked)
            {
                P->SetNetDisplacement(PendingPushStates(i).Displacement);
                P->UpdateLinkedDoorState();
            }
            PendingPushStates.Remove(i, 1);
        }
    }
}

// ============================================================================
// FindPushableByKey
// ============================================================================

AOLPushableObject* UPushableChannel::FindPushableByKey(INT KeyX, INT KeyY, INT KeyZ)
{
    if (!ControllerOwner) return NULL;

    auto Search = [&]() -> AOLPushableObject*
    {
        for (INT i = 0; i < ControllerOwner->CachedPushables.Num(); i++)
        {
            AOLPushableObject* P = ControllerOwner->CachedPushables(i);
            if (P && (INT)P->Location.X == KeyX && (INT)P->Location.Y == KeyY && (INT)P->Location.Z == KeyZ)
                return P;
        }
        return NULL;
    };

    AOLPushableObject* P = Search();
    if (P) return P;
    ControllerOwner->IndexPushables();
    return Search();
}

// ============================================================================
// TickSend — send displacement of the locally active pushable every tick.
// ============================================================================

void UPushableChannel::TickSend(FLOAT DeltaTime)
{
    if (!GMpConn.bIsConnected || !HeroPawn || !GMpConn.SyncInteractable)
        return;

    AOLHero* Hero = Cast<AOLHero>(HeroPawn);
    if (!Hero || !Hero->ActivePushable)
        return;

    AOLPushableObject* AP = Hero->ActivePushable;

    if (!ControllerOwner->bPushablesIndexed)
        ControllerOwner->IndexPushables();

    FLOAT Disp = AP->CurrentDisplacement;
    for (INT i = 0; i < ControllerOwner->CachedPushables.Num(); i++)
    {
        if (ControllerOwner->CachedPushables(i) == AP)
        {
            if (Abs(Disp - ControllerOwner->LastSentPushDisplacement(i)) > 0.1f)
            {
                ControllerOwner->LastSentPushDisplacement(i) = Disp;
                BYTE B[1 + sizeof(FPushStatePacket)];
                INT  N = 0;
                N = PutU8(B, N, MPKT_PUSH_STATE);
                FPushStatePacket Pkt;
                Pkt.KeyX      = (INT)AP->Location.X;
                Pkt.KeyY      = (INT)AP->Location.Y;
                Pkt.KeyZ      = (INT)AP->Location.Z;
                Pkt.DispX1000 = appRound(Disp * 1000.0f);
                appMemcpy(B + N, &Pkt, sizeof(Pkt));
                N += sizeof(Pkt);
                GMpConn.SendBinary(B, N);
            }
            break;
        }
    }
}

// ============================================================================
// OnState — legacy text receive (kept for PacketRouter.uc compatibility, no-op).
// All pushable state now arrives via OnBinaryPacket in MultiplayerController.
// ============================================================================

void UPushableChannel::OnState(const TArray<FString>& Parts, INT SenderID)
{
    // Superseded by binary MPKT_PUSH_STATE; no-op.
}

// ============================================================================
// BroadcastPushableStates — send current state of all pushed pushables.
// Called on REQUEST_STATE from a joining player.
// ============================================================================

void UPushableChannel::BroadcastPushableStates()
{
    if (!GMpConn.bIsConnected) return;
    if (!ControllerOwner->bPushablesIndexed)
        ControllerOwner->IndexPushables();

    for (INT i = 0; i < ControllerOwner->CachedPushables.Num(); i++)
    {
        AOLPushableObject* P = ControllerOwner->CachedPushables(i);
        if (!P || P->CurrentDisplacement == 0.0f) continue;

        BYTE B[1 + sizeof(FPushStatePacket)];
        INT  N = 0;
        N = PutU8(B, N, MPKT_PUSH_STATE);
        FPushStatePacket Pkt;
        Pkt.KeyX      = (INT)P->Location.X;
        Pkt.KeyY      = (INT)P->Location.Y;
        Pkt.KeyZ      = (INT)P->Location.Z;
        Pkt.DispX1000 = appRound(P->CurrentDisplacement * 1000.0f);
        appMemcpy(B + N, &Pkt, sizeof(Pkt));
        N += sizeof(Pkt);
        GMpConn.SendBinary(B, N);
    }
}

// ============================================================================
// OnBinaryPushState — called from AMultiplayerController::OnReceiveBinaryData
// ============================================================================

void PushableChannel_OnBinaryPushState(UPushableChannel* Ch, INT SenderID, BYTE* Data, INT DataLen)
{
    if (!Ch || !Ch->ControllerOwner) return;
    if (DataLen < (INT)sizeof(FPushStatePacket)) return;
    if (!GMpConn.SyncInteractable) return;

    const FPushStatePacket* Pkt = (const FPushStatePacket*)Data;
    INT   KeyX = Pkt->KeyX;
    INT   KeyY = Pkt->KeyY;
    INT   KeyZ = Pkt->KeyZ;
    FLOAT Disp = Pkt->DispX1000 / 1000.0f;

    AMultiplayerController* Ctrl = Ch->ControllerOwner;

    if (!Ctrl->bPushablesIndexed)
        Ctrl->IndexPushables();

    AOLPushableObject* P = Ch->FindPushableByKey(KeyX, KeyY, KeyZ);
    if (!P)
    {
        for (INT i = 0; i < Ctrl->PendingPushStates.Num(); i++)
        {
            if (Ctrl->PendingPushStates(i).KeyX == KeyX
                && Ctrl->PendingPushStates(i).KeyY == KeyY
                && Ctrl->PendingPushStates(i).KeyZ == KeyZ)
            {
                Ctrl->PendingPushStates(i).Displacement = Disp;
                return;
            }
        }
        FPendingPushState S; S.KeyX = KeyX; S.KeyY = KeyY; S.KeyZ = KeyZ; S.Displacement = Disp;
        Ctrl->PendingPushStates.AddItem(S);
        return;
    }

    if (P->bPlayerLocked) return;

    if (!P->bNetLocked)
    {
        P->bNetLocked = TRUE;
        P->NetStartPushing();
    }
    P->SetNetDisplacement(Disp);
    P->UpdateLinkedDoorState();
}
