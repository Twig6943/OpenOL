class OLPawn extends UDKPawn
	native
	config(Game)
	abstract
	dependson(OLTypes);

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

var OLPawn DefaultPawn;

var OLAnimNodeSlot FullBodyAnimSlot;
var OLAnimCustomBlend CustomBlendNode;

var vector RealVelocity; // actual pawn velocity
var vector PreviousLocation; // Last frame's location

// Some Hero stuff that doesn't really belong here

enum CameraRotationMode
{
	CRM_UserControlled,		// Full user control, default pitch limits
	CRM_Limited,				// Limited rotation in camera-space
	CRM_Spring,					// Like limited, but a spring brings the camera to center view when there's no input
	CRM_FullyAnimated,		// Fully animated (location and rotation from the camera bone)
	CRM_Locked,					// Same as Limited, except player input isn't applied
};

var enum ELocomotionMode
{
	// Important: When changing this enum, change OLAnimBlendByLocomotionMode accordingly

	LM_Walk,				// 0
	LM_Fall,				// 1
	LM_SpecialMove,	// 2
	LM_Ladder,			// 3
	LM_LedgeHang,		// 4
	LM_LedgeWalk,		// 5
	LM_Squeeze,			// 6
	LM_Door,				// 7
	LM_Locker,			// 8
	LM_Cinematic,		// 9
	LM_Bed,				// 10
	LM_LookBack,		// 11
	LM_Struggle,		// 12
	LM_Grabbed,			// 13
	LM_Pushing,			// 14
	LM_ContextualLean,	// 15
} LocomotionMode;

//
// SpecialMove
// 

var enum ESpecialMoveType
{
	SMT_None,

	// OLHero moves
	SMT_Crouch,
	SMT_Uncrouch,
	SMT_JumpOnSpot,
	SMT_BigLanding,
	SMT_JumpOver,
	SMT_JumpOverAndGrabLedge,
	SMT_SlideOver,
	SMT_ClimbUpObstacle,
	SMT_ClimbUpWall,
	SMT_ClimbOverWall,
	SMT_StepUpAndLand,
	SMT_EnterLookBack,
	SMT_ExitLookBack,
	SMT_GrabLedgeFromGround,
	SMT_GrabLedgeFromAir,
	SMT_LedgeHangTransition,
	SMT_ClimbUpLedge,
	SMT_DropFromLedge,
	SMT_GrabAndClimb,
	SMT_EnterLedgeWalk,
	SMT_ExitLedgeWalk,
	SMT_LedgeWalkTransition,
	SMT_JumpFromLedgeWalk,
	SMT_EnterSqueeze,
	SMT_ExitSqueeze,	
	SMT_AutomaticSqueeze,
	SMT_SqueezeReload,
	SMT_EnterDoorInteraction,
	SMT_OpenDoorInstant,
	SMT_OpenDoorPartial,
	SMT_TryOpenLockedDoor,
	SMT_RunThroughDoor,
	SMT_CloseDoor,
	SMT_CloseDoorPositionned,
	SMT_ClearClosingDoor,
	SMT_DoorClosedFromOtherSide,
	SMT_OpenLockerFromOutside,
	SMT_EnterLocker,
	SMT_ExitLocker,
	SMT_EnterBed,
	SMT_ExitBed,
	SMT_BedReload,
	SMT_EnterLadderFromGround,
	SMT_EnterLadderFromAbove,
	SMT_ExitLadderOnGround,
	SMT_ExitLadderOnTop,
	SMT_DropFromLadder,
	SMT_GrabLadderFromAir,
	SMT_PickupObject,
	SMT_CSA,
	SMT_EnterStruggle,
	SMT_ExitStruggle,
	SMT_KilledInStruggle,
	SMT_StartPushingObject,
	SMT_StopPushingObject,
	SMT_PushFromLedgeProcedural,
	SMT_PushFromLedgeAnimated,
	SMT_EnterContextualLean,
	SMT_ExitContextualLean,
	SMT_ExitContextualLeanForward,
	SMT_ContextualLeanInsideTransition,

	SMT_HeroGrabbedNormal,
	SMT_HeroGrabbedFromSqueeze,
	SMT_HeroGrabbedFromLocker,
	SMT_HeroGrabbedFromBed,
	SMT_HeroGrabbedFromUnder,
	SMT_HeroThrown,
	SMT_HeroDecapitate,
	SMT_HeroKilled,

	SMT_Dying,

	// OLEnemyPawn moves
	SMT_AttackNormal,
	SMT_AttackGrab,
	SMT_AttackLocker,
	SMT_AttackBed,
	SMT_AttackGrabUnder,
	SMT_AttackCrouch,
	SMT_AttackQuick,
	SMT_AttackPush,

	SMT_AttackSqueezeStart,
	SMT_AttackSqueezeStartToWait,
	SMT_AttackSqueezeWaitToFail,
	SMT_AttackSqueezeWaitToSuccess,
	SMT_AttackSqueezeSuccess,

	SMT_ThrowHero,
	SMT_KillHero,

	SMT_InvestigateLocker,
	SMT_InvestigateBed,

	SMT_Bash,

	SMT_BashDoorStart,
	SMT_BashDoorLoop,
	SMT_BashDoorFinish,
	SMT_BashDoorFailed,

	SMT_Avoiding,
	SMT_Knockedback,

	SMT_TurnOnSpot,

	SMT_AIVault,

	SMT_NanoThroughDoor,

	SMT_Disturbed,

	SMT_AlignAnim,

	SMT_SlideToNavMesh,

	SMTTT_FixCompileErrorWithSteam_SMT_MAX,

} SpecialMove;

struct native GameplayParams
{
	var ERootMotionMode RMM;
	var CameraRotationMode CameraMode;

	var float	MinYaw;
	var float	MaxYaw;
	var float	MinPitchWS; // world-space
	var float	MaxPitchWS;
	var float	MinPitchCS; // cam-space
	var float	MaxPitchCS;
	var float	ViewLimitsApproachCoeff;
	var float	FOVOverride;

	var bool		DisableCollisions;
	var bool		IgnorePawnCollisions;
	var float	CollisionHeight;
	var float	CollisionRadius;

	var EPhysics	Physics;
	var bool		BothHandsNeeded;

	structdefaultproperties
	{
		RMM=RMM_Ignore,
		CameraMode=CRM_UserControlled,
		CollisionHeight=-1.0,
		CollisionRadius=-1.0,
		FOVOverride=-1.0
		ViewLimitsApproachCoeff=0.99,
		MinYaw=0.0,
		MaxYaw=0.0,
		MinPitchCS=-84.0,
		MaxPitchCS=84.0,
		MinPitchWS=-84.0,
		MaxPitchWS=70.0,
		Physics=PHYS_Custom,
	}
};

var struct native SpecialMoveParameters
{	
	var GameplayParams GP;

	var ELocomotionMode NextLocomotionMode;

	var Name AnimName; // shorthand - if not specified, PlayAnimForSpecialMove() will be called on the pawn
	var bool bNoAnim;
	var float AnimBlendInTime;
	var bool bExitOnBlendOut;
	var bool bPlayAnimWhenPositioned;
	var bool bTargettedYawSmoothing; // when the exit yaw is known ahead of time, for a smoother camera blend (e.g. exit squeeze, bed)
	var bool bAlwaysInterruptible;
	var bool bInterruptibleAfterTrigger;
	var bool bCollisionChangeOnTrigger;
		
	var bool CameraInputEnabled;
	var bool PlayerInputEnabled;	
	var bool KeepLocomotionMode; // do not set locomotion mode to LM_SpecialMove, do not use NextLocomotionMode, do not use Physics

	var bool AdjustPosition;
	var bool AdjustOrientation;
	var bool UseAnimTimeForPositionAdjustment;
	var bool bUsePawnVelocityForPositionning; // Use in conjunction with bPlayAnimWhenPositioned to simulate the pawn keeping its e.g. running velocity
	var float PositionningLinearVelocity; // cm/s. ignored if bUsePawnVelocityForPositionning
	var float PositionningAngularVelocity; // deg/s

	structdefaultproperties
	{
		NextLocomotionMode=LM_Walk,
		PositionningLinearVelocity=200.0,
		PositionningAngularVelocity=150.0,
		AnimBlendInTime=0.25,
	}

} SpecialMoveParams[ESpecialMoveType];

var bool bPlayingSpecialMoveAnim;
var bool bDelayedSpecialMoveAnim; // same as params.bPendingSpecialMoveAnims, except may be dynamically modified
var bool bSpecialMoveInterruptible;
var bool bPendingSpecialMoveAnims; // set for multi-part special moves

var const bool bFilterAnimEndNotifies;
var array<name> PlayingSpecialMoveAnims; // used to filter anim end triggers, making sure that we're not misinterpreting a trigger from a move already blending out

//
// AdjustPosition
// 

enum EAdjustPositionTargetType
{
	APTT_TargetAtStart,
	APTT_TargetAtEnd
};

var struct native AdjustPositionData
{
	var bool Active;
	var bool Done;

	var vector PositionError; // world space
	var float HeadingError; // degs
	var EAdjustPositionTargetType TargetType;

	var vector OriginalPosition;
	var rotator OriginalRotation;

	var float CorrectionTime;
	var float ElapsedTime;
} AdjustPosition;


struct native ProceduralAnimData
{
	var vector PositionDelta;
	var float HeadingDelta; // degs
	var bool bWaitForNotify; // Wait to a ProceduralAdjust notify, that will specify the TotalTime

	var float TotalTime;
	var float ElapsedTime;

	structcpptext
	{
		FProceduralAnimData()
		{
			appMemzero(this, sizeof(FProceduralAnimData));
		}

		FProceduralAnimData(EEventParm)
		{
			appMemzero(this, sizeof(FProceduralAnimData));
		}
	}
};
var Array<ProceduralAnimData> ProceduralAnims;
var bool bProceduralAnimsDelayedAfterSpecialMove; // note: could be removed and replaced by using the bWaitForNotify flag

var const float ProceduralAnimLinearVelocity;
var const float ProceduralAnimAngularVelocity; // degs/s

struct native TurningData
{
	var bool bActive;
	var float StartTime;
	var float StartYaw; // degrees
	var float StartSpeed;
	var float InitialDeltaYaw; // degrees
	var float Duration;
	var float SmoothedTargetYaw; // degrees
	var vector FocalPoint;
};
var TurningData Turning;

//
// Sound
//

/** Max distance from listener to play footstep sounds */
var float MaxFootstepDistSq;

/** Max distance from listener to play jump/land sounds */
var float MaxJumpSoundDistSq;

//
// AI Repulsion

var bool bRepelsAI;
var const float RepulsionMaxDistance;
var const float RepulsionCollisionAngle;
var const float RepulsionCollisionFactor;
var const float RepulsionMagnitudeFactor;
var const float RepulsionMaxMagnitude;
var const float RepulsionSidestepFactor;

//
// Misc
//

var DynamicLightEnvironmentComponent LightEnvironment;
var name HeadBoneName;

var float DefaultAirControl;

var float DestinationPredictionFactor;
var float DestinationPredictionMax;

var bool bFailedCollisionSet;
var bool bIsDummyPawn;    // set on remote dummy pawns spawned by MultiplayerController
var bool bNetInDarkness;  // replicated from remote player's darkness state; read by OLBot when bIsDummyPawn
var int  DummyOwnerID;    // PlayerID of the remote player this dummy represents; set by MultiplayerController on spawn

var name FloorMaterialGroup;
var AkEvent FootStepSound_Walk;

var const string SubtitleName;

var float PendingAnimSetUpdateTime; // needed to delay updating the anim sets until some external animation has finished playing

////////////////////////////////////////////////////////////////////////////////////////////
// Native interface
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

cpptext
{
	virtual UBOOL Tick( FLOAT DeltaTime, enum ELevelTick TickType );
	virtual void TickSpecial(FLOAT deltaTime);
	virtual void performPhysics(FLOAT deltaSeconds);

	virtual void Crouch(INT bClientSimulation=0);
	virtual void UnCrouch(INT bClientSimulation=0);
	virtual UBOOL CanUncrouch();

	UBOOL TryCompleteSpecialMove(); // for AI polling

	UBOOL IsDoingSpecialMove() const;
	UBOOL IsAnimDriven() const;
	UBOOL CanInterruptSpecialMove() const;

	void QueueProceduralAnim(FProceduralAnimData& animData);
	void QueueProceduralAnim(FProceduralAnimData& animData, FLOAT linearVelocity, FLOAT angularVelocity);

	virtual void UpdateEyeHeight(FLOAT deltaTime) {}	
	virtual void OnEarlyAnimEnd(UAnimNodeSequence* seqNode);

	virtual FVector GetAIRepulsion(const class AOLEnemyPawn* Agent);

	virtual UBOOL CanStartAndStop() { return TRUE; }

	UBOOL	WouldEncroach(const FVector& testLocation);
	UBOOL	WouldEncroach(const FVector& testLocation, FLOAT radius, FLOAT height);

protected:
	virtual void TickPrePhysics(FLOAT deltaTime);
	virtual void TickPostPhysics(FLOAT deltaTime);
	virtual void physCustom(FLOAT deltaTime, INT iterations);
	virtual void CalcVelocity(FVector &AccelDir, FLOAT DeltaTime, FLOAT MaxSpeed, FLOAT Friction, INT bFluid, INT bBrake, INT bBuoyant);
	virtual UBOOL IgnoreBlockingBy(const AActor* Other) const;

	virtual FLOAT PlayFullBodyAnim(const FName& animName, FLOAT playRate=1.0f, FLOAT blendInTime=0.25f, FLOAT blendOutTime=0.25f, FLOAT startTime=0.0f, FLOAT endTime=0.0f);
	virtual FLOAT PlayBlendedAnim(const FName& animNameA, const FName& animNameB, FLOAT weightA, FLOAT blendInTime=0.0f, FLOAT blendOutTime=0, FLOAT rate=1.0f, FLOAT startRatio=0.0f);
	virtual FLOAT PlayBlendedAnim3(const FName& animNameA, const FName& animNameB, const FName& animNameC, FLOAT blendWeightA, FLOAT blendWeightB, FLOAT blendInTime=0.0f, FLOAT blendOutTime=0, FLOAT rate=1.0f, FLOAT startRatio=0.0f);
	virtual FLOAT PlayBlendSpace(const FBlendSpaceNode nodes[], INT numNodes, const INT blendSpaces[][3], INT numSpaces, const FVector2D& coord, FLOAT blendInTime=0.0f, FLOAT blendOutTime=0.0f, FLOAT rate=1.0f, FLOAT startRatio=0.0f);

	void UpdateReverb(FLOAT deltaTime);

	virtual UBOOL UseFootPlacementThisTick();
	virtual void EnableFootPlacement(UBOOL bEnabled);
	virtual void DoFootPlacement(FLOAT deltaTime);
	
	virtual void OnCrouch() {}
	virtual void OnUncrouch() {}

	// Called when a special move is completed
	virtual void SpecialMoveCompleted();
	virtual void SpecialMovePhaseCompleted() {}; // called when a phase is done in a multi-phase special move

	// Verify if the pawn state allows the specified move
	virtual UBOOL ShouldAllowOtherPawnSpecialMove(AOLPawn* otherPawn, ESpecialMoveType moveType, AActor* refActor) { return TRUE; }
	virtual void OnOtherPawnStartSpecialMove(AOLPawn* otherPawn, ESpecialMoveType moveType, AActor* refActor) {}	
	virtual UBOOL CouldStartSpecialMove(ESpecialMoveType moveType) { return TRUE; }
	virtual UBOOL TryCommitToSpecialMove(ESpecialMoveType moveType, AActor* refActor = NULL);
	
	virtual void ApplySpecialMoveParams(ESpecialMoveType moveType);
	void ActivatePositionAdjustment(ESpecialMoveType moveType, const FVector& targetPosition, const FVector& targetDirection, EAdjustPositionTargetType targetType);
	virtual void PlayAnimForSpecialMove(ESpecialMoveType moveType) { check(FALSE); }

	void ApplyProceduralAnims(FLOAT deltaTime);
	void ClearProceduralAnims();

	virtual void UpdateSpecialMove(FLOAT deltaTime);
	void FixUpRootMotion();
	void ApplyPositionAdjustment(FLOAT deltaTime);
	void SetRootMotionMode(ERootMotionMode RMM);

	UBOOL TryAdjustCollisionSize(FLOAT newHeight, FLOAT newRadius); // tries to adjust both height and radius, but it it can't it will try to adjust either one independently (still returns false)
	UBOOL TryAdjustCollisionSizeExact(FLOAT newHeight, FLOAT newRadius); // must match exactly the specified sizes

	virtual FName GetMaterialBelowFeetAt(const FVector* TraceOrigin);
}

native function bool IsSpecialMoveCompleted();
native function CancelSpecialMove();

// Starts a special move, with position adjustment parameters
native function StartSpecialMove(ESpecialMoveType moveType, optional vector targetPosition, optional vector targetDirection, optional EAdjustPositionTargetType targetType);
// Same as StartSpecialMove but takes a plain int so cross-package callers can avoid enum type errors
native function StartSpecialMoveByInt(int moveType);

// Callback when the specified sequence has finished playing
native function NativeOnAnimEnd(AnimNodeSequence seqNode, float playedTime, float excessTime);
native function NativePostBeginPlay();
native function MoveInterruptibleNotify();
native function ChangeCollisionSizeNotify(); // activate the delayed change to collsion settings defined for the active special move
native function RestoreCollisionSizeNotify(); // restore collision settings to default
native function ProceduralAdjustNotify(float duration);

////////////////////////////////////////////////////////////////////////////////////////////
// UTPawn functions
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////


simulated function PostBeginPlay()
{
	local rotator R;
	local PlayerController PC;

	StartedFallingTime = WorldInfo.TimeSeconds;

	Super.PostBeginPlay();

	if (!bDeleteMe)
	{
		if (Mesh != None)
		{
			BaseTranslationOffset = Mesh.Translation.Z;
			CrouchTranslationOffset = Mesh.Translation.Z + CylinderComponent.CollisionHeight - CrouchHeight;
		}

		// Zero out Pitch and Roll
		R.Yaw = Rotation.Yaw;
		SetRotation(R);

		// add to local HUD's post-rendered list
		ForEach LocalPlayerControllers(class'PlayerController', PC)
		{
			if ( PC.MyHUD != None )
			{
				PC.MyHUD.AddPostRenderedActor(self);
			}
		}

		if ( WorldInfo.NetMode != NM_DedicatedServer )
		{
			UpdateShadowSettings(/*class'UTPlayerController'.default.PawnShadowMode == SHADOW_All*/true);
		}
	}

	NativePostBeginPlay();
}

simulated function UpdateShadowSettings(bool bWantShadow)
{
	local bool bNewCastShadow, bNewCastDynamicShadow;

	if (Mesh != None)
	{
		bNewCastShadow = default.Mesh.CastShadow && bWantShadow;
		bNewCastDynamicShadow = default.Mesh.bCastDynamicShadow && bWantShadow;
		if (bNewCastShadow != Mesh.CastShadow || bNewCastDynamicShadow != Mesh.bCastDynamicShadow)
		{
			// if there is a pending Attach then this will set the shadow immediately as the flags have changed an a reattached has occurred
			Mesh.CastShadow = bNewCastShadow;
			Mesh.bCastDynamicShadow = bNewCastDynamicShadow;

			// defer if we can do so without it being noticeable
			if (LastRenderTime < WorldInfo.TimeSeconds - 1.0)
			{
				SetTimer(0.1 + FRand() * 0.5, false, 'ReattachMesh');
			}
			else
			{
				ReattachMesh();
			}
		}
	}
}

/** reattaches the mesh component, because settings were updated */
simulated function ReattachMesh()
{
	DetachComponent(Mesh);
	AttachComponent(Mesh);
	EnsureOverlayComponentLast();
}

simulated event OnAnimEnd(AnimNodeSequence SeqNode, float PlayedTime, float ExcessTime)
{
	NativeOnAnimEnd(SeqNode, PlayedTime, ExcessTime);
}

simulated function SetPawnRBChannels(bool bRagdollMode)
{
	if(bRagdollMode)
	{
		Mesh.SetRBChannel(RBCC_Pawn);
		Mesh.SetRBCollidesWithChannel(RBCC_Default,true);
		Mesh.SetRBCollidesWithChannel(RBCC_Pawn,true);
		Mesh.SetRBCollidesWithChannel(RBCC_Vehicle,true);
		Mesh.SetRBCollidesWithChannel(RBCC_Untitled3,false);
		Mesh.SetRBCollidesWithChannel(RBCC_BlockingVolume,true);
	}
	else
	{
		Mesh.SetRBChannel(RBCC_Untitled3);
		Mesh.SetRBCollidesWithChannel(RBCC_Default,false);
		Mesh.SetRBCollidesWithChannel(RBCC_Pawn,false);
		Mesh.SetRBCollidesWithChannel(RBCC_Vehicle,false);
		Mesh.SetRBCollidesWithChannel(RBCC_Untitled3,true);
		Mesh.SetRBCollidesWithChannel(RBCC_BlockingVolume,false);
	}
}

event EncroachedBy( actor Other )
{
	// Do Nothing
}

function bool Died(Controller Killer, class<DamageType> damageType, vector HitLocation)
{
	if (Super.Died(Killer, DamageType, HitLocation))
	{
		StartFallImpactTime = WorldInfo.TimeSeconds;
		bCanPlayFallingImpacts=true;
		SetPawnAmbientSound(None);
		return true;
	}
	return false;
}

/** starts playing the given sound via the PawnAmbientSound AudioComponent and sets PawnAmbientSoundCue for replicating to clients
 *  @param NewAmbientSound the new sound to play, or None to stop any ambient that was playing
 */
simulated function SetPawnAmbientSound(SoundCue NewAmbientSound)
{
	// if the component is already playing this sound, don't restart it
	if (NewAmbientSound != PawnAmbientSound.SoundCue)
	{
		PawnAmbientSoundCue = NewAmbientSound;
		PawnAmbientSound.Stop();
		PawnAmbientSound.SoundCue = NewAmbientSound;
		if (NewAmbientSound != None)
		{
			PawnAmbientSound.Play();
		}
	}
}

simulated function SoundCue GetPawnAmbientSound()
{
	return PawnAmbientSoundCue;
}

simulated function float GetEyeHeight()
{
	return EyeHeight;
}

simulated function FaceRotation(rotator NewRotation, float DeltaTime)
{
	if ( Physics == PHYS_Ladder )
	{
		NewRotation = OnLadder.Walldir;
	}
	else if ( (Physics == PHYS_Walking) || (Physics == PHYS_Falling) )
	{
		NewRotation.Pitch = 0;
	}
	NewRotation.Roll = 0;
	SetRotation(NewRotation);
}

simulated event PlayFootStepSound(int FootDown, AnimNotify_Footstep footstepNotify)
{
	SetSwitch(FloorMaterialGroup, GetMaterialBelowFeet());
	PlayAkEvent(FootStepSound_Walk);
}


simulated function ClientReStart()
{
	local rotator AdjustedRotation;

	Super.ClientRestart();

	if (Controller != None)
	{
		AdjustedRotation = Controller.Rotation;
		AdjustedRotation.Roll = 0;
		Controller.SetRotation(AdjustedRotation);
	}
}

native function name GetMaterialBelowFeet();

function PlayLandingSound()
{
}

function PlayJumpingSound()
{
}

native function PlayVOLine(AkEvent LineToPlay, optional name BoneName);

simulated event PostInitAnimTree(SkeletalMeshComponent SkelComp)
{
	if (SkelComp == Mesh)
	{
		FullBodyAnimSlot = OLAnimNodeSlot(Mesh.FindAnimNode('FullBodySlot'));
		CustomBlendNode = OLAnimCustomBlend(Mesh.FindAnimNode('CustomBlend'));
	}

	Super.PostInitAnimTree(SkelComp);
}

native function StartMatinee(vector startLoc, rotator startRot, float blendTime);

event MAT_BeginAIGroup(vector StartLoc, rotator StartRot, float BlendTime)
{
	BeginAIGroup();
	bUsedByMatinee = true;

	StartMatinee(StartLoc, StartRot, BlendTime);
}

simulated event Destroyed()
{
	local PlayerController PC;
	local Actor A;

	Super.Destroyed();

	foreach BasedActors(class'Actor', A)
	{
		A.PawnBaseDied();
	}

	// remove from local HUD's post-rendered list
	ForEach LocalPlayerControllers(class'PlayerController', PC)
	{
		if ( PC.MyHUD != None )
		{
			PC.MyHUD.RemovePostRenderedActor(self);
		}
	}
}

event Landed(vector HitNormal, actor FloorActor)
{
	local vector Impulse;

	Super.Landed(HitNormal, FloorActor);

	// adds impulses to vehicles and dynamicSMActors (e.g. KActors)
	Impulse.Z = Velocity.Z * 4.0f; // 4.0f works well for landing on a Scorpion
	if (DynamicSMActor(FloorActor) != None)
	{
		DynamicSMActor(FloorActor).StaticMeshComponent.AddImpulse(Impulse, Location);
	}

	if ( Velocity.Z < -200 )
	{
		OldZ = Location.Z;
	}

	AirControl = DefaultAirControl;	

	SetBaseEyeheight();
}

function PlayLanded(float impactVel)
{
	SetSwitch(FloorMaterialGroup, GetMaterialBelowFeet());
}

simulated function bool IsFirstPerson()
{
	local PlayerController PC;

	ForEach LocalPlayerControllers(class'PlayerController', PC)
	{
		if ( (PC.ViewTarget == self) && PC.UsingFirstPersonCamera() )
			return true;
	}
	return false;
}

native function vector GetFutureDestination(OLPawn Agent);

simulated function DisplayDebug(HUD HUD, out float out_YL, out float out_YPos)
{
	Controller.DisplayDebug(HUD, out_YL, out_YPos);

	Hud.Canvas.SetDrawColor(134, 189, 255);

	Hud.Canvas.SetPos(4, out_YPos);
	Hud.Canvas.DrawText("" $ Name $ " - " $ Controller.Name $ ", Health " $ Health);
	out_YPos += out_YL;

	Hud.Canvas.SetPos(4, out_YPos);
	Hud.Canvas.DrawText("Location:" @ Location @ "Rotation:" @ Rotation, FALSE);
	out_YPos += out_YL;
	

	Hud.Canvas.SetDrawColor(32,255,196);
	Hud.Canvas.SetPos(4,out_YPos);
	Hud.Canvas.DrawText("Velocity: " $ Velocity $ ", Real Velocity: " $ RealVelocity $ ", RMM: " $ Mesh.RootMotionMode $ ", RMRM: " $ Mesh.RootMotionRotationMode);
	out_YPos += out_YL;	

	Hud.Canvas.SetPos(4,out_YPos);
	Hud.Canvas.DrawText("SpecialMove: " $ SpecialMove);
	out_YPos += out_YL;	
}

static native function DrawDebugAnimNode(name NodeName, AnimNode aNode, out array<AnimNode> visitedNodes, Canvas aCanvas, out float out_YL, out float out_YPos, float XL, float XBasePos, float XOffset);

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

simulated event Touch(Actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal)
{
    local OLPlayerController OLPC;
    // Dummy pawns (remote-player visuals) must never interact with triggers.
    if (bIsDummyPawn)
        return;
    super.Touch(Other, OtherComp, HitLocation, HitNormal);
    OLPC = OLPlayerController(Controller);
    if (Other != None && Other.GeneratedEvents.Length > 0)
    {
        if (OLPC != None)
            OLPC.NotifyPawnTouchedTrigger(Other);
    }
}


defaultproperties
{
	ProceduralAnimLinearVelocity=550.0
	ProceduralAnimAngularVelocity=180.0

	////////////////////////////////////////////////////////////////////////////////////////////
	// UTPawn variables
	////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////

	Begin Object Class=DynamicLightEnvironmentComponent Name=MyLightEnvironment
		bSynthesizeSHLight=true
		bIsCharacterLightEnvironment=true
		bUseBooleanEnvironmentShadowing=false
		InvisibleUpdateTime=1
		MinTimeBetweenFullUpdates=.1
		bForceNonCompositeDynamicLights=true
	End Object
	Components.Add(MyLightEnvironment)
	LightEnvironment=MyLightEnvironment

	Begin Object Name=CollisionCylinder
		CollisionRadius=30.0
		CollisionHeight=82.5
		Translation=(Z=87.5)
	End Object
	CylinderComponent=CollisionCylinder

	Begin Object Class=AudioComponent Name=AmbientSoundComponent
	End Object
	PawnAmbientSound=AmbientSoundComponent
	Components.Add(AmbientSoundComponent)
	
	BaseEyeHeight=165.0
	EyeHeight=165.0
	GroundSpeed=150.0
	AirSpeed=440.0
	AccelRate=2048.0
	JumpZ=400
	CrouchHeight=29.0
	CrouchRadius=30.0
	WalkableFloorZ=0.78

	AlwaysRelevantDistanceSquared=+1960000.0
	InventoryManagerClass=None // screw unreal's inventory

	MeleeRange=+20.0
	bMuffledHearing=true

	bCanStrafe=true
	bCanWalkOffLedges=true
	RotationRate=(Pitch=20000,Yaw=20000,Roll=20000)
	AirControl=+0.35
	DefaultAirControl=+0.35
	bCanCrouch=true
	bCanClimbLadders=true
	bCanPickupInventory=true
	SightRadius=+12000.0

	bPushesRigidBodies=true
	
	MaxStepHeight=35.0
	MaxJumpHeight=35.0
	
	bPhysRigidBodyOutOfWorldCheck=true
	bRunPhysicsWithNoController=true

	ControllerClass=class'OLGame.OLBot'

	LeftFootControlName=LeftFootControl
	RightFootControlName=RightFootControl
	bEnableFootPlacement=false
	MaxFootPlacementDistSquared=56250000.0 // 7500 squared

	SlopeBoostFriction=0.2
	
	MaxFallSpeed=+1250.0
	AIMaxFallSpeedFactor=1.1 // so bots will accept a little falling damage for shorter routes
	
	bReplicateRigidBodyLocation=true

	TakeHitPhysicsBlendOutSpeed=0.5

	FallImpactSound=None
	FallSpeedThreshold=125.0
	
	MaxFootstepDistSq=9000000.0
	MaxJumpSoundDistSq=16000000.0

	FloorMaterialGroup="Surface_Floor"
	FootStepSound_Walk=AkEvent'Player_Sound.Footsteps_Walk_Miles'

	bFilterAnimEndNotifies=true

	RepulsionMaxDistance=70.0f
	RepulsionCollisionAngle=30.0f
	RepulsionCollisionFactor=0.5f
	RepulsionMagnitudeFactor=22.0f
	RepulsionMaxMagnitude=0.1f
	RepulsionSidestepFactor=25.0f

	DestinationPredictionFactor=0.8f
	DestinationPredictionMax=500.0f

	SubtitleName="Pawn"
}
