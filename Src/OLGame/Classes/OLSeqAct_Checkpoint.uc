class OLSeqAct_Checkpoint extends SequenceAction
	native;

var() name CheckpointName;

var transient bool bStatusIsOk;

cpptext
{
public:
	virtual void UpdateStatus();
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

#if WITH_EDITOR	
	virtual void DrawExtraInfo(FCanvas* Canvas, const FVector& BoxCenter);
protected:
	FString GetDisplayTitle() const;
#endif
}

static event int GetObjClassVersion()
{
	return Super.GetObjClassVersion() + 1;
}

native function OLCheckpoint GetCheckpointFromName(name cpName);

event Activated()
{
	local OLEngine Engine;
	local OLCheckpoint CheckpointTarget;
	local OLGame CurrentGame;
	local OLPlayerController OLPC;

	if( InputLinks[0].bHasImpulse )
	{
		if (CheckpointName == '')
		{
			`warn("OLSeqAct_Checkpoint without a CheckpointName.");
			return;
		}

		CheckpointTarget = GetCheckpointFromName(CheckpointName);

		Engine = OLEngine(class'Engine'.static.GetEngine());
		CurrentGame = OLGame(GetWorldInfo().Game);

		foreach GetWorldInfo().AllControllers(class'OLPlayerController', OLPC)
		{
			break;
		}

		OLPC.SavePersistentState();

		if (Engine != None && CurrentGame != None && class'OLCheckpointList'.static.Script_IsUnreached(CheckpointName, CurrentGame.CurrentCheckpointName))
		{
			Engine.SaveCheckpoint(CheckpointName, true);

			if (OLPC.HUD != None)
			{
				OLPC.HUD.NotifyGameSaved();
			}
		}

		CheckpointTarget.SetEnabled(true);

		OLPC.OnCheckpointReached(CheckpointName);
	}
}

defaultproperties
{
	ObjName="Checkpoint"
	ObjCategory="Outlast - Systems"

	InputLinks(0)=(LinkDesc="Save")
	OutputLinks.Empty()
	VariableLinks.Empty()
}