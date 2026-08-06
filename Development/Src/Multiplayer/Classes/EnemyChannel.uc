// Handles sending and receiving NPC enemy state: spawn, location, SMT, death.
class EnemyChannel extends Object
    native;

var native MultiplayerController    ControllerOwner;
var native OLHero          HeroPawn;

// --- Send (called from tick) ---
native function TickSend(float DeltaTime);
native function SendSpawn(OLEnemyPawn E);
native function SendSMT(OLEnemyPawn E, int i);
native function SendSMTDirect(OLEnemyPawn E, int SMTType, int Param1, int Param2);
native function SendEnemyDoorOpen(OLEnemyPawn Enemy, OLDoor D, float Speed, float Angle);
native function SendEnemyDoorDone(OLEnemyPawn Enemy, OLDoor D, float CloseSpeed);
native function SendEnemyDoorBash(OLEnemyPawn Enemy, OLDoor D, bool bReversed);
native function SendEnemyDoorBreak(OLEnemyPawn Enemy, OLDoor D, bool bReversed);
native function SendAllDeletes();
native function BroadcastSpawns();

// --- Receive ---
native function OnSpawn(array<string> Parts, int SenderID);
native function OnDel(array<string> Parts, int SenderID);
native function OnSmt(array<string> Parts, int SenderID);
native function OnRequestEnemies(int SenderID);
native function OnBinaryLoc(int SenderID, byte Data[255], int DataLen);
native function OnBinaryPacket(int SenderID, byte PktType, byte Data[255], int DataLen);

// --- Helpers ---
native function OLEnemyPawn FindEnemyByName(string EnemyName);

DefaultProperties
{
}
