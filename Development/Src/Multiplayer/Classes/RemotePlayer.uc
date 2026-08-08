// State and dummy pawn management for a single remote player.
class RemotePlayer extends Object
    native;

var MultiplayerController   ControllerOwner;

var int     PlayerID;
var string  PlayerNick;
var string  Nick;
var Pawn    DummyPlayer;

// --- Transform (interpolation) ---
var vector  LastReceivedLoc;
var vector  LastReceivedVel;
var rotator LastReceivedRot;
var bool    bHasReceivedData;

// --- Locomotion / SMT ---
var bool    bLastRemoteCrouched;
var bool    bDummyCrouched;
var int     LastRemoteSpecialMove;

// Deferred SMT: StartSpecialMove is held until LOC interpolation brings dummy close to GrabPos.
// Used for SMTs where the local player walks up to the target first (bUsePawnVelocityForPositionning).
var int     PendingSMT;
var vector  PendingSMTGrabPos;
var vector  PendingSMTGrabDir;
var float   DummySMTLockUntil;


var int     LastRemoteLocomotionMode;

var int     LastRemoteHealth;
var int     LastRemoteCamPitch;
var int     LastRemoteCamYaw;

var int     LastRemoteCamcorderState;

var bool    bLastRemoteHeatShielding;
var float   LastRemoteHeatDistance;

// --- Jump/slide ---
var bool    bJumpOverActive;
var float   JumpGroundZ;

// --- Door ---
var int     LockedDoorIdx;
var int     LastRemoteDoorOpeningType;
var int     LastRemoteDoorPartialOpenType;
var int     LastRemoteDoorClosingType;
var bool    bLastRemoteDoorQuiet;
var vector  LastRemoteDoorHandlePos;

// --- Push ---
var bool    bLastRemotePushFromBack;

// --- Corner peek ---
var bool    bLastRemotePeekFromLeft;
var bool    bLastRemotePeekRounded;
var vector  LastRemoteCornerLocation;
var vector  LastRemoteCornerFwdDir;
var bool    bPeekTypeApplied;

// --- Pickup ---
var vector  LastRemotePickupLoc;


// Called every PlayerTick — interpolates position, drives dummy.
native function Tick(float DeltaTime);

DefaultProperties
{
    LockedDoorIdx = -1;
}
