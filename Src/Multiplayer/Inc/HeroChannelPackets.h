#pragma once

// ============================================================================
// HELLO binary packet — sent after READY to register nick with the server.
//
// Layout:
//   [0] BYTE NickLen     — length of nick string (1-32)
//   [1..N] ASCII nick bytes — no null terminator
// ============================================================================

#define MPKT_HELLO 0x02

// ============================================================================
// HeadRot binary packet
// ============================================================================

#pragma pack(push, 1)
struct FHeadRotPacket
{
    INT     CamPitch;   // raw rotator units
    INT     CamYaw;     // raw rotator units
};
#pragma pack(pop)

#define MPKT_HEAD_ROT 0x04

// ============================================================================
// MeshPreset binary packet — 1 byte preset index
// ============================================================================

#pragma pack(push, 1)
struct FMeshPresetPacket
{
    BYTE    PresetIndex;
};
#pragma pack(pop)

#define MPKT_MESH_PRESET 0x05

// ============================================================================
// CinematicAnim binary packet  (MPKT_CINEMATIC_ANIM 0x06)
//
// Layout:
//   [0] BYTE  bStop        — 1 = stop, 0 = play
//   [1] BYTE  AnimPathLen  — only present when bStop == 0
//   [2..N] ASCII path bytes — "Package.Name|AnimSeqName", no null terminator
// ============================================================================

#define MPKT_CINEMATIC_ANIM 0x06

// ============================================================================
// SmtType binary packet — unified SMT transition packet.
// Contains all pre-params (formerly in FSmtTypePacket) and all position/anim
// params (formerly in the removed FSmtPosPacket). One packet per transition.
// ============================================================================

#pragma pack(push, 1)
struct FSmtTypePacket
{
    INT     SMT;

    // Grab position (AdjustPosition target / expectedAnimStart)
    FLOAT   GrabPosX;
    FLOAT   GrabPosY;
    FLOAT   GrabPosZ;

    // Grab direction (AdjustPosition dir / expectedAnimFwd)
    FLOAT   GrabDirX;
    FLOAT   GrabDirY;
    FLOAT   GrabDirZ;

    // Anim length hint — used to set DummySMTLockUntil on receiver
    FLOAT   GrabLength;

    // Blend alpha (SMT_HeroKilled / Decapitate)
    INT     BlendAlphaX1000;

    // Yaw target (SMT_HeroThrown)
    INT     SpecialMoveTargetYawX1000;

    // Enemy params (SMT_HeroKilled / Decapitate)
    BYTE    EnemyType;
    BYTE    EnemyWeapon;

    // Ledge / climb enums
    BYTE    LedgeTransitionType;    // ELedgeTransitionType (SMT 16,20,21,22)
    BYTE    LedgeClimbType;         // ELedgeClimbType      (SMT 17)

    // Door params (SMT 29-36)
    BYTE    DoorOpeningType;
    BYTE    DoorPartialOpenType;
    BYTE    DoorClosingType;
    BYTE    bQuietDoorInteraction;

    // Pickup params (SMT 49)
    INT     PickupDist2DX10;
    INT     PickupDeltaZX10;
    BYTE    bPickupCrouched;
    BYTE    bPickupIsCollectible;

    // Bool flags
    BYTE    bLeftAnim;              // SMT 24,25,26,40,41,43,62,68,69
    BYTE    bPushingFromBackEdge;   // SMT 54,55
    BYTE    bExitLadderLeftHand;    // SMT 46
    BYTE    bIsCrouched;            // SMT 62
    BYTE    bBackAnim;              // SMT 68,69
    BYTE    bRunningTraversalMove;  // SMT 5,8
    BYTE    bMustCrouchAfterSMT;    // SMT 8
    BYTE    bJumpRun;               // SMT 5

    // ContextualLean params (SMT_EnterContextualLean)
    BYTE    bPeekFromLeft;
    BYTE    bPeekRounded;
    FLOAT   CornerLocX;
    FLOAT   CornerLocY;
    FLOAT   CornerLocZ;
    FLOAT   CornerFwdX;
    FLOAT   CornerFwdY;
    FLOAT   CornerFwdZ;

    // CSA params (SMT_CSA): anim name + object path, both length-prefixed ASCII
    BYTE    CSAAnimLen;
    BYTE    CSAAnimName[31];
    BYTE    CSAPathLen;
    BYTE    CSAPath[127];

    // Struggle params (SMT_EnterStruggle): entry + cycle anim names for player,
    // plus AnimSet path to load (mirrors Cinematic "Package.Name|AnimSeqName" approach).
    BYTE    StruggleEntryAnimPlayerLen;
    BYTE    StruggleEntryAnimPlayer[63];
    BYTE    StruggleCycleAnimPlayerLen;
    BYTE    StruggleCycleAnimPlayer[63];
    BYTE    StruggleCycleAnimEnemyLen;
    BYTE    StruggleCycleAnimEnemy[63];
    BYTE    StruggleAnimSetPathLen;
    BYTE    StruggleAnimSetPath[127];

    BYTE    _pad[2];
};
#pragma pack(pop)

#define MPKT_SMT_TYPE 0x07

// PING/PONG: [type(1)][player_id(4)][sent_ms LE u32(4)] — not relayed, server echoes back
#define MPKT_PING 0x08

// ============================================================================
// PlayerEvent binary packet (MPKT_PLAYER_EVENT 0x0A)
//
// One unified packet for HIT / GRAB / THROW / KILL.
// Layout: [0x0A][FPlayerEventPacket]
// ============================================================================

enum EPlayerEventType
{
    PEVT_Hit   = 0,
    PEVT_Grab  = 1,
    PEVT_Throw = 2,
    PEVT_Kill  = 3,
};

#pragma pack(push, 1)
struct FPlayerEventPacket
{
    INT   TargetPlayerID;
    BYTE  EventType;        // EPlayerEventType

    // HIT
    INT   DamageX1;         // int(Damage)
    INT   KnockbackX1;      // int(KnockbackPower)
    INT   HitDirX1000[3];   // HitDir * 1000

    // GRAB / KILL
    INT   LocX10[3];        // GrabTargetLoc/AnimStart * 10
    INT   DirX10000[3];     // CharDir * 10000
    INT   BlendAlphaX10000; // BlendAlpha * 10000
    INT   EnemyTypeInt;
    INT   WeaponType;       // KILL only
    INT   KillType;         // KILL only
    INT   VictimYaw;        // KILL only
    INT   ThrowRotX100000;  // THROW only: ThrowRotation * 100000
    INT   GrabType;         // GRAB only
    BYTE  bCrouched;        // GRAB only
    BYTE  bBackAnim;        // KILL only
    BYTE  bLeftAnim;        // GRAB / KILL
    BYTE  _pad;
};
#pragma pack(pop)

#define MPKT_PLAYER_EVENT 0x0A

// ============================================================================
// PlayerLifecycle binary packet (MPKT_PLAYER_LIFECYCLE 0x0B)
//
// One byte: 0 = Died, 1 = Respawned.
// ============================================================================

#define MPKT_PLAYER_LIFECYCLE     0x0B
#define MPKT_LIFECYCLE_DIED       0
#define MPKT_LIFECYCLE_RESPAWNED  1
