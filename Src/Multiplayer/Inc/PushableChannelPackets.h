#pragma once

// ============================================================================
// Pushable channel binary packet types
// ============================================================================

#define MPKT_PUSH_STATE 0x1D

// Layout: [type(1)][KeyX(4)][KeyY(4)][KeyZ(4)][DispX1000(4)]
// KeyX/KeyY/KeyZ = int(Location.X/Y/Z) — used as stable actor key.
// DispX1000  = CurrentDisplacement * 1000, stored as I32.
#pragma pack(push, 1)
struct FPushStatePacket
{
    INT KeyX;
    INT KeyY;
    INT KeyZ;
    INT DispX1000;
};
#pragma pack(pop)
