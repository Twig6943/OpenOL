#pragma once

// ============================================================================
// WorldChannel binary packets
//
// String payloads: [strLen(1)][ASCII bytes]  (max 255 bytes)
//
// 0x1E  MPKT_WORLD_NICK              [strLen(1)][nick ASCII]
// 0x1F  MPKT_WORLD_TRIGGER_ACT       [countLE(4)][strLen(1)][path ASCII]
// 0x20  MPKT_WORLD_CSA               [strLen(1)][path ASCII]
// 0x21  MPKT_WORLD_ITEM_CONSUME      [strLen(1)][item name ASCII]
// 0x22  MPKT_WORLD_PICKUP_KISMET     [strLen(1)][path ASCII]
// 0x23  MPKT_WORLD_RECORDING         [strLen(1)][path ASCII]
// 0x24  MPKT_WORLD_DISCONNECT        (no payload)
// 0x25  MPKT_WORLD_REQUEST_STATE     [strLen(1)][level filter ASCII]  (may be 0-len)
// 0x26  MPKT_WORLD_REQUEST_ENEMIES   (no payload)
// 0x0C  MPKT_WORLD_PICKUP_STATE      [X I32][Y I32][Z I32]
// 0x0D  MPKT_WORLD_PICKUP_START      [X I32][Y I32][Z I32]
// 0x0E  MPKT_WORLD_PICKUP_ATTACH     [X I32][Y I32][Z I32]
// ============================================================================

#define MPKT_WORLD_NICK              0x1E
#define MPKT_WORLD_TRIGGER_ACT       0x1F
#define MPKT_WORLD_CSA               0x20
#define MPKT_WORLD_ITEM_CONSUME      0x21
#define MPKT_WORLD_PICKUP_KISMET     0x22
#define MPKT_WORLD_RECORDING         0x23
#define MPKT_WORLD_DISCONNECT        0x24
#define MPKT_WORLD_REQUEST_STATE     0x25
#define MPKT_WORLD_REQUEST_ENEMIES   0x26
#define MPKT_WORLD_PICKUP_STATE      0x0C
#define MPKT_WORLD_PICKUP_START      0x0D
#define MPKT_WORLD_PICKUP_ATTACH     0x0E
