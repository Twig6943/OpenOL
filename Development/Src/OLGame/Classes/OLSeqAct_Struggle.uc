
class OLSeqAct_Struggle extends SeqAct_Latent
	native(Sequence);

var() StruggleConfig Config;
var SkeletalMeshActor Enemy;
var vector RefLocation;
var vector RefDirection;
var bool bSucceeded; // Upon completion, either succeeded or failed will be set (if they're both false the struggle isn't complete)
var bool bFailed;
var transient bool bObserverOnly;

cpptext
{
	virtual UBOOL UpdateOp(FLOAT deltaTime);
	virtual void DeActivated() {}

	void Init();
	void Start();
}

static event int GetObjClassVersion()
{
	return Super.GetObjClassVersion() + 1;
}

defaultproperties
{
	ObjName="Struggle"
	ObjCategory="Outlast - Gameplay"

	bAutoActivateOutputLinks=false
	bCallHandler=false

	InputLinks(0)=(LinkDesc="Init")
	InputLinks(1)=(LinkDesc="Start")
	OutputLinks(0)=(LinkDesc="Out")
	OutputLinks(1)=(LinkDesc="Success")
	OutputLinks(2)=(LinkDesc="Failure")	
	VariableLinks(0)=(ExpectedType=class'SeqVar_Object', LinkDesc="Enemy",PropertyName=None)
	VariableLinks(1)=(ExpectedType=class'SeqVar_Object', LinkDesc="RefLocation")
}
