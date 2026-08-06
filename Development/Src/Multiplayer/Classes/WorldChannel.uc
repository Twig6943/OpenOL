// Handles Kismet events, matinees, pickups, level streaming, and connection messages.
// Implementation in Src/WorldChannel.cpp.
class WorldChannel extends Object
    native;

var native MultiplayerController    ControllerOwner;
var native OLHero          HeroPawn;

var array<name> SentRemoteEvents;

// --- Send ---
native function SendPickupState(int CurSMT);
native function SendPickupKismet(OLPickableObject Pickup);
native function SendCSA(OLCSA CSA);
native function SendItemConsume(name ItemName);
native function SendRecordingMarker(OLRecordingMarker Marker);
// --- Local events (called from Controller on local world events) ---
native function OnPawnTouchedTrigger(Actor TriggerActor);

// --- Receive ---
native function OnNick(array<string> Parts, int SenderID);
native function OnBinaryNick(int SenderID, string Nick);
native function OnBinaryWorldPacket(int SenderID, byte PktType, out byte Data[255], int DataLen);
native function OnDisconnected(array<string> Parts, int SenderID);
native function OnTrigger(array<string> Parts, int SenderID);
native function OnTriggerAct(array<string> Parts, int SenderID);
native function OnCSA(array<string> Parts, int SenderID);
native function OnItemConsume(array<string> Parts, int SenderID);
native function OnLevel(array<string> Parts, int SenderID);
native function OnPickupState(array<string> Parts, int SenderID);
native function OnPickupKismet(array<string> Parts, int SenderID);
native function OnRecordingMarker(array<string> Parts, int SenderID);

DefaultProperties
{
}
