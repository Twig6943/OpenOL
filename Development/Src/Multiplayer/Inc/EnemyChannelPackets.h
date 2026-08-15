#pragma once

// ============================================================================
// Enemy channel binary packet types
// ============================================================================

// MPKT_ENPC_LOC (0x02) defined in EnemyChannel.h — already binary.

#define MPKT_ENPC_SPAWN     0x18
#define MPKT_ENPC_DEL       0x19
#define MPKT_ENPC_SMT       0x1A
#define MPKT_ENPC_DOOR_OPEN  0x1B
#define MPKT_ENPC_DOOR_DONE  0x1C
#define MPKT_ENPC_DOOR_BASH  0x27
#define MPKT_ENPC_DOOR_BREAK 0x28

// ============================================================================
// Variable-length string helper: [len(1)][bytes...]  max 63 chars.
// All packets below start with the enemy name in this format.
// Fixed fields follow immediately after the name.
// ============================================================================

// ENPC_SPAWN fixed body — follows [type(1)][namelen(1)][name...][classlen(1)][class...]
//   [X(4)][Y(4)][Z(4)][Yaw(2)][meshlen(1)][mesh...][Weapon(1)]
//   [bColor(1)] — if 1, [R(2)][G(2)][B(2)][A(2)] follow (each * 1000, stored as I16)
// No fixed-size struct possible due to variable strings; encoded manually.

// ENPC_DEL body — follows [type(1)][namelen(1)][name...]
// No fixed fields.

// ENPC_SMT fixed body — follows [type(1)][namelen(1)][name...]
#pragma pack(push, 1)
struct FEnpcSmtBody
{
    BYTE SMTType;   // SpecialMove (0-127 fits BYTE; cast from BYTE field anyway)
    INT  Param1;
    INT  Param2;
    // Optional door coords follow if SMT uses a door (DoorX != 0 || DoorY != 0 || DoorZ != 0)
    INT  DoorX, DoorY, DoorZ;
};
#pragma pack(pop)
#define ENPC_SMT_BODY_SIZE_NODOOR (1 + 4 + 4)       // SMTType + Param1 + Param2
#define ENPC_SMT_BODY_SIZE        sizeof(FEnpcSmtBody) // with door coords

// ENPC_DOOR_OPEN / ENPC_DOOR_DONE fixed body — follows [type(1)][namelen(1)][name...]
// Speed stored as I16 * 10 (e.g. 95.3 deg/s → 953).
#pragma pack(push, 1)
struct FEnpcDoorBody
{
    INT   DoorX, DoorY, DoorZ;
    short Speed10; // AngleWhenOpen/Duration * 10, or 0 = use door default
    short Angle10; // AngleWhenOpen * 10, or 0 = use door default (MaxOpenAngle)
};
#pragma pack(pop)
#define ENPC_DOOR_BODY_SIZE sizeof(FEnpcDoorBody)

// ENPC_DOOR_BASH / ENPC_DOOR_BREAK fixed body — follows [type(1)][namelen(1)][name...]
#pragma pack(push, 1)
struct FEnpcDoorBashBody
{
    INT   DoorX, DoorY, DoorZ;
    BYTE  bReversed;
};
#pragma pack(pop)
#define ENPC_DOOR_BASH_BODY_SIZE sizeof(FEnpcDoorBashBody)
