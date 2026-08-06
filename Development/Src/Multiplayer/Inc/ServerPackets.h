#pragma once

// ============================================================================
// Server → client binary notifications (not relayed between players).
// Codes >= 0xE0; NativeReceivedData routes them as binary (first byte >= 0xE0).
//
// 0xE0  SRV_READY        [player_id LE(4)][name_len(1)][server name ASCII]
// 0xE1  SRV_ONLINE_COUNT [count LE(4)]
// 0xE2  SRV_HELLO_FAIL   [reason_len(1)][reason ASCII]
// 0xE3  SRV_DISCONNECT   [player_id LE(4)]   — remote player disconnected
// 0xE4  SRV_DOOR_DENY    [key_len(1)][key ASCII]
// ============================================================================

#define SRV_READY        0xE0
#define SRV_ONLINE_COUNT 0xE1
#define SRV_HELLO_FAIL   0xE2
#define SRV_DISCONNECT   0xE3
#define SRV_DOOR_DENY    0xE4
