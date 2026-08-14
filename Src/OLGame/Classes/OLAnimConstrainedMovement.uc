class OLAnimConstrainedMovement extends AnimNodeBlendList
	native(Anim);                                         

var() bool bUpDown;
var() bool bCompleteCyclesOnly;
var() bool bNoAutomaticRootMotion; // when bCompleteCyclesOnly is set, disable root-motion during automatic movement completion or retraction

var transient float CurrentRatio;
var transient float SmoothedDelta;
var transient int LastTargetIdx;
var transient bool bAutomaticMotion;
var transient bool bNetControlled;
var transient float NetSmoothedDelta;

cpptext
{
	virtual void TickAnim(FLOAT deltaTime);
	virtual void OnBecomeRelevant();
	virtual void GetBoneAtoms(FBoneAtomArray& Atoms, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion, FCurveKeyArray& CurveKeys);
}

defaultproperties
{
	Children(0)=(Name="Idle",Weight=1.0)
	Children(1)=(Name="MoveNegative")
	Children(2)=(Name="MovePositive")
	bFixNumChildren=true

	CategoryDesc = "Outlast"
}
