class OLCheckpointList extends Actor
	placeable
	native;

var() array<name> CheckpointList;

enum OutlastGameType
{
	OGT_Outlast,
	OGT_Whistleblower
};
var() OutlastGameType GameType;

cpptext
{
	virtual void PostBeginPlay();
	virtual void BeginDestroy();
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

	static UBOOL IsCheckpointInList(const FName& checkpointName); 

	// TRUE if current is before test, FALSE otherwise or if either checkpoint isn't found
	static UBOOL IsUnreached(const FName& testCheckpoint, const FName& currentCheckpoint); 

	// TRUE if current is at or after test, FALSE otherwise or if either checkpoint isn't found
	static UBOOL IsReached(const FName& testCheckpoint, const FName& currentCheckpoint); 

	// TRUE if current is after test
	static UBOOL IsCompleted(const FName& testCheckpoint, const FName& currentCheckpoint); 

	static void TriggerApplyCheckpointStateEvent(class UOLSeqEvent_ApplyCheckpointState* applyCPStateEvent);

	static TArray<FName>* GetCheckpointList(); // returns the default checkpoint list (assumes only one)

	static AOLCheckpointList* GetListForCheckpoint(const FName& testCheckpoint);
	static void SetEffectiveList(UBOOL bPlayingDLC);

	static INT GetCheckpointIdx(const FName& testCheckpoint);
}

// TRUE if current is before test, FALSE otherwise or if either checkpoint isn't found
native static function bool Script_IsUnreached(const out name TestCheckpoint, const out name CurrentCheckpoint);

// Fire all OLSeqEvent_ApplyCheckpointState events in the world — called after remotely applying a checkpoint.
native static function Script_TriggerAllApplyCheckpointStateEvents(); 

