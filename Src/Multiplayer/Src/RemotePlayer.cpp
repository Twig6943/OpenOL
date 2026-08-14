/*=============================================================================
    RemotePlayer.cpp — per-remote-player tick: position interpolation
=============================================================================*/
#include "Multiplayer.h"
#include "HeroChannel.h"
#include "OLGameClasses.h"

IMPLEMENT_CLASS(URemotePlayer);

void URemotePlayer::Tick(FLOAT DeltaTime)
{
    if (!DummyPlayer || !bHasReceivedData)
        return;

    AOLHero* Dummy = Cast<AOLHero>(DummyPlayer);
    if (!Dummy)
        return;

    // Position: snap for SMT-locked states, interpolate otherwise.
    // Dead reckoning (velocity extrapolation) can be added later.
    FVector TargetLoc = LastReceivedLoc;

    if (bJumpOverActive)
        TargetLoc.Z = JumpGroundZ;

    const INT CurSMT = Dummy->SpecialMove;
    const INT CurLM  = Dummy->LocomotionMode;

    // During active SMTs or cinematic/door/push modes the engine drives
    // position via animation — don't stomp it.
    UBOOL bSMTLocked = (DummySMTLockUntil > 0.f);
    UBOOL bAnimDriven = (CurLM == LM_SpecialMove)
                     || (CurLM == LM_Cinematic)
                     || (CurLM == LM_Door)
                     || (CurLM == LM_Pushing)
                     || (CurSMT != SMT_None
                         && CurSMT != SMT_Crouch
                         && CurSMT != SMT_Uncrouch);

    if (!bSMTLocked && !bAnimDriven)
    {
        DummyPlayer->SetLocation(TargetLoc);
        DummyPlayer->SetRotation(LastReceivedRot);
    }

    DummyPlayer->Velocity     = LastReceivedVel;
    DummyPlayer->Acceleration = LastReceivedVel;

    // Update heat effects every tick
    Dummy->bHeatShielding  = bLastRemoteHeatShielding;
    Dummy->HeatDistance    = LastRemoteHeatDistance;
}
