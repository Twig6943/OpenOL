// Handles Kismet events, matinees, pickups, level streaming, and connection messages.
// Implementation in Src/WorldChannel.cpp.
class WorldChannel extends Object
    native;

var native MultiplayerController    ControllerOwner;
var native OLHero          HeroPawn;

// --- Send ---
native function SendPickupState(int CurSMT);
native function SendPickupKismet(OLPickableObject Pickup);
native function SendItemConsume(name ItemName);
native function SendRecordingMarker(OLRecordingMarker Marker);
native function SendMatineeState();
// --- Local events (called from Controller on local world events) ---
native function OnPawnTouchedTrigger(Actor TriggerActor);

// --- Receive ---
native function OnBinaryNick(int SenderID, string Nick);
native function OnBinaryWorldPacket(int SenderID, byte PktType, out byte Data[255], int DataLen);
native function OnDisconnected(array<string> Parts, int SenderID);

DefaultProperties
{
}
