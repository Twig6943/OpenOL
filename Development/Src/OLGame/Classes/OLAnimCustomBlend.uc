
class OLAnimCustomBlend extends AnimNodeBlendList
	native(Anim);

var transient bool bActive;
var transient bool bBlendingOut;
var transient float BlendOutTime;
var transient float PlaybackTime;
var transient bool bKeepLastPose; // when set, wait until the animations have finished playing before initiating the blend out
var transient bool bInhibitEndNotifies;
var transient float BlendWeights[3];
var transient float bEarlyAnimEndFired;

var transient float TimeRemaining;
var transient int NumActiveAnims; // 1, 2 or 3

cpptext
{
	virtual void TickAnim(FLOAT deltaSeconds);

	FLOAT PlaySingleAnim(FName animName, FLOAT blendInTime=0.0f, FLOAT blendOutTime=0.0f, FLOAT rate=1.0f, FLOAT startRatio=0.0f); 
	FLOAT PlayCustomBlend(FName animNameA, FName animNameB, FLOAT blendWeightA, FLOAT blendInTime=0.0f, FLOAT blendOutTime=0.0f, FLOAT rate=1.0f, FLOAT startRatio=0.0f);
	FLOAT PlayCustomBlend(FName animNameA, FName animNameB, FName animNameC, FLOAT blendWeightA, FLOAT blendWeightB, FLOAT blendInTime=0.0f, FLOAT blendOutTime=0.0f, FLOAT rate=1.0f, FLOAT startRatio=0.0f);
	FLOAT PlayBlendSpace(const FBlendSpaceNode nodes[], INT numNodes, const INT blendSpaces[][3], INT numSpaces, const FVector2D& coord, FLOAT blendInTime=0.0f, FLOAT blendOutTime=0.0f, FLOAT rate=1.0f, FLOAT startRatio=0.0f);
	void StartBlendingOut();
	void FinishPlaying();
	void StopTickingSequences();

	FBoneAtom GetTotalRootMotion() const;	
	FBoneAtom GetCameraFinalRelativePosition() const;	

private:
	FLOAT PlayCustomBlend_Internal(FName animNames[3], FLOAT blendInTime=0.0f, FLOAT blendOutTime=0.0f, FLOAT rate=1.0f, FLOAT startRatio=0.0f);
	UAnimNodeSequence* GetPrimaryNode() const;
};

native function float PlayCustomBlendUC(name AnimA, name AnimB, float WeightA, float BlendIn, float BlendOut);

defaultproperties
{
	NodeName="CustomBlend"
	bFixNumChildren=TRUE
	Children(0)=(Name="Normal")
	Children(1)=(Name="AnimA")
	Children(2)=(Name="AnimB")
	Children(3)=(Name="AnimC")
	TargetWeight(0)=1.0
	TargetWeight(1)=0.0
	TargetWeight(2)=0.0
	TargetWeight(3)=0.0

	CategoryDesc = "Outlast"
}
