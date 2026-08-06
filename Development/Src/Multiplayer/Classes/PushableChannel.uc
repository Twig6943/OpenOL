// Handles sending and receiving pushable object state.
// Implementation in Src/PushableChannel.cpp.
class PushableChannel extends Object
    native;

var native MultiplayerController    ControllerOwner;
var native OLHero          HeroPawn;

// --- Send (tick-driven) ---
native function TickSend(float DeltaTime);

// --- Receive ---
native function OnState(array<string> Parts, int SenderID);

// --- Broadcast (called on REQUEST_STATE) ---
native function BroadcastPushableStates();

// --- Helpers ---
native function OLPushableObject FindPushableByKey(int KeyX, int KeyY, int KeyZ);

DefaultProperties
{
}
