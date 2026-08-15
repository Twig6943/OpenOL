#pragma once

// ============================================================================
// Door channel binary packet types
// ============================================================================

#define MPKT_DOOR_LOCK      0x10
#define MPKT_DOOR_UNLOCK    0x11
#define MPKT_DOOR_STATE     0x12
#define MPKT_DOOR_OPEN      0x13
#define MPKT_DOOR_CLOSE     0x14
#define MPKT_DOOR_ANGLE     0x15
#define MPKT_DOOR_PARAMS    0x16
#define MPKT_DOOR_DENY      0x17
// DOOR_INIT — sent once per level load for every door (initial registration).
// Same layout as DOOR_ANGLE: [type(1)][X(4)][Y(4)][Z(4)][AngleX1000(4)]  Total: 17 bytes.
// Server stores in angle_pkt only if no snapshot exists yet (first-write-wins).
#define MPKT_DOOR_INIT      0x29

// ============================================================================
// All door packets use INT location key (X,Y,Z rounded to nearest int).
// Floats sent as INT * 1000 to avoid locale issues with decimal separator.
// ============================================================================

// DOOR_LOCK — sent when local hero starts interacting with a door.
// [type(1)][X(4)][Y(4)][Z(4)][OpeningType(1)][GrabPosX(4)][GrabPosY(4)][GrabPosZ(4)]
// [GrabDirX(4)][GrabDirY(4)][GrabDirZ(4)][PartialOpenType(1)][ClosingType(1)][bQuiet(1)]
// Total: 38 bytes
#pragma pack(push, 1)
struct FDoorLockPacket
{
    INT  X, Y, Z;
    BYTE OpeningType;
    INT  GrabPosX, GrabPosY, GrabPosZ;   // * 10 (cm precision)
    INT  GrabDirX, GrabDirY, GrabDirZ;   // * 10000 (unit vector precision)
    BYTE PartialOpenType;
    BYTE ClosingType;
    BYTE bQuiet;
};
#pragma pack(pop)
#define DOOR_LOCK_SIZE (1 + sizeof(FDoorLockPacket))

// DOOR_UNLOCK — sent when local hero releases the door.
// [type(1)][X(4)][Y(4)][Z(4)]  Total: 13 bytes
#pragma pack(push, 1)
struct FDoorUnlockPacket
{
    INT X, Y, Z;
};
#pragma pack(pop)
#define DOOR_UNLOCK_SIZE (1 + sizeof(FDoorUnlockPacket))

// DOOR_STATE — interactive angle update (while holding door) and locker exit.
// [type(1)][X(4)][Y(4)][Z(4)][AngleX1000(4)][SpeedX1000(4)]  Total: 21 bytes
#pragma pack(push, 1)
struct FDoorStatePacket
{
    INT X, Y, Z;
    INT AngleX1000;
    INT SpeedX1000;
};
#pragma pack(pop)
#define DOOR_STATE_SIZE (1 + sizeof(FDoorStatePacket))

// DOOR_OPEN — door opened by local hero (auto-open).
// [type(1)][X(4)][Y(4)][Z(4)]  Total: 13 bytes
#pragma pack(push, 1)
struct FDoorOpenPacket
{
    INT X, Y, Z;
};
#pragma pack(pop)
#define DOOR_OPEN_SIZE (1 + sizeof(FDoorOpenPacket))

// DOOR_CLOSE — same layout as DOOR_OPEN.
#define FDoorClosePacket FDoorOpenPacket
#define DOOR_CLOSE_SIZE  DOOR_OPEN_SIZE

// DOOR_ANGLE — broadcast initial angle on REQUEST_STATE.
// Same layout as DOOR_STATE but speed is always 0 (not sent, implicit).
// [type(1)][X(4)][Y(4)][Z(4)][AngleX1000(4)]  Total: 17 bytes
#pragma pack(push, 1)
struct FDoorAnglePacket
{
    INT X, Y, Z;
    INT AngleX1000;
};
#pragma pack(pop)
#define DOOR_ANGLE_SIZE (1 + sizeof(FDoorAnglePacket))

// DOOR_PARAMS — sent before SMT_POS for door SMTs 29-36.
// [type(1)][OpeningType(1)][PartialOpenType(1)][ClosingType(1)][bQuiet(1)]  Total: 5 bytes
#pragma pack(push, 1)
struct FDoorParamsPacket
{
    BYTE OpeningType;
    BYTE PartialOpenType;
    BYTE ClosingType;
    BYTE bQuiet;
};
#pragma pack(pop)
#define DOOR_PARAMS_SIZE (1 + sizeof(FDoorParamsPacket))

// DOOR_DENY — server→client, sent when server rejects a DOOR_LOCK.
// [type(1)][X(4)][Y(4)][Z(4)]  Total: 13 bytes
#pragma pack(push, 1)
struct FDoorDenyPacket
{
    INT X, Y, Z;
};
#pragma pack(pop)
#define DOOR_DENY_SIZE (1 + sizeof(FDoorDenyPacket))
