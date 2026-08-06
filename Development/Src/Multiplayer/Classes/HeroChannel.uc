// Hero state channel — send/receive position, locomotion, SMT, anim, combat.
// Implementation in Src/HeroChannel.cpp.
class HeroChannel extends Object
    native;

var native MultiplayerController    ControllerOwner;
var native OLHero          HeroPawn;

var bool    bSendingJumpGroundZ;
var float   JumpGroundZ;
var int     LastSentSpecialMove;
var string  LastSentCinematicAnim;
var int     LastSentMeshPreset;
var vector  LastPickupLoc;

// --- Send: state (tick-driven via FTickableObject::Tick, not UC) ---
native function SendGlobalState();
native function SendHeadRotation();
native function SendMesh();
native function SendCinematicAnimation();
native function SendSpecialMoveType();
native function SendSpecialMovePosition(int SMT);
native function SendPickupStart(int CurSMT);
native function SendPickupState(int CurSMT);
native function SendCornerPeekState(int CurSMT);
native function SendCornerPeekData();

// --- Send: combat (event-driven) ---
native function SendPlayerHit(int TargetPlayerID, float Damage, float KnockbackPower, vector HitDir);
native function SendPlayerGrab(int TargetPlayerID, vector GrabTargetLoc, vector CharDir, bool bCrouched, int EnemyTypeInt, float BlendAlpha, bool bLeftAnim, int GrabType);
native function SendPlayerThrow(int TargetPlayerID, float ThrowRotation);
native function SendPlayerKill(int TargetPlayerID, int EnemyTypeInt, int WeaponType, bool bBackAnim, bool bLeftAnim, float BlendAlpha, vector AnimStart, vector CharDir, int KillType, int VictimYaw);

// --- Receive: state ---
native function OnLoc(array<string> Parts, int SenderID);
native function OnHeadRot(array<string> Parts, int SenderID);
native function OnMesh(array<string> Parts, int SenderID);
native function OnAnim(array<string> Parts, int SenderID);
native function OnSmt(array<string> Parts, int SenderID);
native function OnSmtPos(array<string> Parts, int SenderID);
native function OnCornerPeek(array<string> Parts, int SenderID);
native function OnFootstep(array<string> Parts, int SenderID);
native function OnBinaryLoc(int SenderID, out byte Data[255], int DataLen);
native function OnSmtPosBinary(int SenderID, out byte Data[255], int DataLen);
native function OnBinaryHeadRot(int SenderID, out byte Data[255], int DataLen);
native function OnBinaryMesh(int SenderID, out byte Data[255], int DataLen);
native function OnBinaryCinematicAnim(int SenderID, out byte Data[255], int DataLen);
native function OnBinarySmtType(int SenderID, out byte Data[255], int DataLen);
native function OnBinaryPlayerEvent(int SenderID, out byte Data[255], int DataLen);

// --- Internal apply helpers (called from C++ ApplyHeroState) ---
native function ApplyCamcorderState(int Idx, OLHero Dummy, int NewCamcorderState);
native function ApplySpecialMoveTransition(int Idx, OLHero Dummy, int NewSpecialMove);
native function ApplyLocomotionMode(int Idx, OLHero Dummy, int NewLocomotionMode);

// --- Receive: combat ---
native function OnHit(array<string> Parts, int SenderID);
native function OnGrab(array<string> Parts, int SenderID);
native function OnThrow(array<string> Parts, int SenderID);
native function OnKill(array<string> Parts, int SenderID);
native function SendPlayerDied();
native function SendPlayerRespawned();
native function SendDisconnect();
native function SendRequestEnemies();
native function OnBinaryPlayerLifecycle(int SenderID, out byte Data[255], int DataLen);

DefaultProperties
{
    LastSentMeshPreset = -1;
    LastSentSpecialMove = -1;
}
