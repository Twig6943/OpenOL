class OLCSA extends OLGameplayMarker
	native
	placeable
	config(Game);

var() bool bAutomatic;
var() int MaxTriggerCount;

/** Max horizontal distance from eye */
var() float InteractDistHorz;

/** Max vertical distance from eye */
var() float InteractDistVert;

/** Player must aim within this radius to interact */
var() float InteractRadius;

/** Max angle from player to CSA's forward */
var() float MaxPlayerAngle;

/** Whether to check for LineOfSight */
var() bool bCheckLOS;

var() name RequiredItem;
var() bool bConsumeItem;
var() bool bNoPrompt;
var() name AnimName;
var() float AnimStartDistFwd;
var() float AnimStartDistRight;

/** If set, compute the anim start location and rotation relative to the ReferenceAnimActor */
var() actor ReferenceAnimActor;
var() name ActivationPromptTextId;
var() name RequiredItemPromptTextId;

/** Prop that the player interacts with */
var() StaticMeshActor AnimatedProp;

/** If set, unhide the animated prop's StaticMeshComponent after LastValidCheckpoint */ 
var() bool bShowPropAfterLastValidCheckpoint;

/** If set, the CSA is disabled for subsequent checkpoints */
var() name LastValidCheckpoint;

var int TriggerCount;

var DrawSphereComponent PreviewComp;

cpptext
{
	UBOOL TryActivate(class AOLHero* hero, UBOOL playerInteraction);
	void Completed(class AOLHero* hero);

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

	virtual void Reset();

}

// Called on the observer to replicate a remote player's CSA interaction.
// If bConsumeActivation is true, increments TriggerCount (blocks future local use).
// If false, fires Kismet events without consuming the activation.
native function RemoteActivate(bool bConsumeActivation);

defaultproperties
{
	InteractRadius=30.0
	MaxPlayerAngle=60.0
	bEnabled=true
	MaxTriggerCount=1
	InteractDistHorz=100.0
	InteractDistVert=100.0
	bConsumeItem=true
	AnimStartDistFwd=60.0
	ActivationPromptTextId=PromptGenericCSA

	Begin Object Class=DrawSphereComponent Name=SphereComp
		CollideActors=False
		BlockActors=False
		BlockZeroExtent=False
		BlockNonZeroExtent=False
		BlockRigidBody=False
		SphereRadius=30.0
		SphereColor=(R=126,G=137,B=204,A=255)
	End Object
	PreviewComp=SphereComp
	Components.Add(SphereComp)
	
	Begin Object Name=Sprite
		Sprite=Texture2D'Utility.OLCSA-01_D'	
	End Object

	Begin Object Class=ArrowComponent Name=ArrowComponent0
		ArrowColor=(R=173,G=183,B=243)
		bTreatAsASprite=True
		SpriteCategoryName="OutlastGameplay"
	End Object
	Components.Add(ArrowComponent0)

	SupportedEvents.Add(class'OLSeqEvent_CSAActivated')
}

