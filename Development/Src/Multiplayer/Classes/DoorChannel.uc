// Door state channel — send/receive door angles, lock, open/close events.
// Implementation in Src/DoorChannel.cpp.
class DoorChannel extends Object
    native;

var native MultiplayerController    ControllerOwner;
var native OLHero          HeroPawn;

// Delta-tracking stored as C++ statics (see DoorChannel.cpp).

// --- Send (tick-driven) ---
native function TickSend(float DeltaTime);

// --- Send (event-driven) ---
native function OnLocalDoorOpen(OLDoor D);
native function OnLocalDoorClose(OLDoor D);

// --- Receive (called from MultiplayerController.OnReceiveData) ---
native function OnDoorLock(array<string> Parts, int SenderID);
native function OnDoorUnlock(array<string> Parts, int SenderID);
native function OnDoorState(array<string> Parts, int SenderID);
native function OnDoorOpen(array<string> Parts, int SenderID);
native function OnDoorClose(array<string> Parts, int SenderID);
native function OnDoorAngle(array<string> Parts, int SenderID);
native function OnDoorParams(array<string> Parts, int SenderID);
native function OnDoorDeny(int X, int Y, int Z);

// --- Broadcast (called on REQUEST_STATE) ---
native function BroadcastDoorStates(string LevelFilter);

// --- Binary receive dispatch ---
native function OnBinaryPacket(int SenderID, byte PktType, byte Data[255], int DataLen);

DefaultProperties
{
}
