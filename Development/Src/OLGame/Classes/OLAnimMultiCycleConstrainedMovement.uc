class OLAnimMultiCycleConstrainedMovement extends AnimNodeBlendList
	native(Anim);                                         

var() bool bUpDown;
var() bool bCompleteCyclesOnly;
var() bool bCommitMoves;
var transient float SmoothedDelta;
var transient float CurrentRatio;
var transient int CurrentIdx;
// When true, TickAnim uses NetSmoothedDelta instead of computing from Owner movement.
// Set by SetDummyLadderDelta() on remote dummy players for multiplayer sync.
var transient bool bNetControlled;
var transient float NetSmoothedDelta;

cpptext
{
	virtual void TickAnim(FLOAT deltaTime);
	virtual void OnBecomeRelevant();

	UBOOL CloserToA() const; // returns whether A or B is the closer node
	FLOAT GetCurrentPositionRatio() const; // return the current ration of the root motion when between A and B
}

defaultproperties
{
	Children(0)=(Name="IdleA",Weight=1.0)
	Children(1)=(Name="IdleB")
	Children(2)=(Name="MoveAtoB")
	Children(3)=(Name="MoveBtoA")
	bFixNumChildren=true

	bCommitMoves=true

	CategoryDesc = "Outlast"
}
