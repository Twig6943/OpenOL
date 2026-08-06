#pragma once
#include "Multiplayer.h"
#include "DoorChannelPackets.h"

// Free function called from MultiplayerController.OnReceiveData for DOOR_DENY
// (server message, no SenderID prefix).
void DoorChannel_OnDoorDeny(UDoorChannel* DC, INT X, INT Y, INT Z);

// Helper used by DoorChannel send side to release a door and send DOOR_UNLOCK.
void DoorChannel_SendUnlock(UDoorChannel* DC, AOLDoor* D);
