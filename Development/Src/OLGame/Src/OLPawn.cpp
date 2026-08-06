#include "OLGame.h"
#include "EngineAnimClasses.h"
#include "UDKBaseAnimationClasses.h"
#include "OLGameAnimClasses.h"
#include "OLUtilities.h"

IMPLEMENT_CLASS(AOLPawn);
IMPLEMENT_CLASS(UOLDmgType);
IMPLEMENT_CLASS(UOLDmgType_SoldierThrow);
IMPLEMENT_CLASS(UOLDmgType_SoldierPunch);
IMPLEMENT_CLASS(UOLDmgType_SoldierDecapitate);
IMPLEMENT_CLASS(UOLDmgType_GenericHit);
IMPLEMENT_CLASS(UOLDmgType_Electrified);
IMPLEMENT_CLASS(UOLDmgType_NanoFog);
IMPLEMENT_CLASS(UOLDmgType_Fire);
IMPLEMENT_CLASS(UOLDmgType_Fell);
IMPLEMENT_CLASS(UOLDmgType_Scripted);

////////////////////////////////////////////////////////////////////////////////////////////
// Special Moves
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void AOLPawn::CancelSpecialMove()
{
	// Base implementation is a no-op; AOLHero overrides with real cancellation logic.
}

UBOOL AOLPawn::IsDoingSpecialMove() const
{
	return SpecialMove != SMT_None;
}

UBOOL AOLPawn::CanInterruptSpecialMove() const
{
	if (!IsDoingSpecialMove())
	{
		return TRUE;
	}

	if (ProceduralAnims.Num() > 0)
	{
		return FALSE;
	} 

	if (AdjustPosition.Active && !AdjustPosition.Done)
	{
		return FALSE;
	}

	return bSpecialMoveInterruptible;
}

UBOOL AOLPawn::IsAnimDriven() const
{
	return (Mesh && Mesh->RootMotionMode != RMM_Ignore);	
}

void AOLPawn::StartSpecialMove(BYTE moveType, FVector targetPosition, FVector targetDirection, BYTE targetType)
{
	if (SpecialMove != SMT_None)
	{
		SpecialMoveCompleted();
	}

	check(moveType != SMT_None && moveType < ESpecialMoveType_MAX);

	debugf(TEXT("[%s] Special move started: %s"), *GetName(), *Utils::GetEnumString("ESpecialMoveType", moveType));

	SpecialMove = moveType;	

	if (!SpecialMoveParams[moveType].PlayerInputEnabled)
	{
		Velocity = FVector(0);
		Acceleration = FVector(0);
	}

	bDelayedSpecialMoveAnim = SpecialMoveParams[moveType].bPlayAnimWhenPositioned;

	if (bDelayedSpecialMoveAnim)
	{
		// wait until we're positionned to start the move, *except* if we're already closer than supposed to (targetPosition is our back)
		if (((Rotation.Vector() | (targetPosition - Location).SafeNormal2D()) < 0.0f) && (Rotation.Vector() | targetDirection) > 0.0f)
		{
			bDelayedSpecialMoveAnim = FALSE;
		}
	}

	PlayingSpecialMoveAnims.Empty();

	ApplySpecialMoveParams((ESpecialMoveType)moveType);

	if ((!targetPosition.IsZero() && SpecialMoveParams[moveType].AdjustPosition) || (!targetDirection.IsZero() && SpecialMoveParams[moveType].AdjustOrientation))
	{
		ActivatePositionAdjustment((ESpecialMoveType)moveType, targetPosition, targetDirection, (EAdjustPositionTargetType)targetType);
	}
}

void AOLPawn::ApplySpecialMoveParams(ESpecialMoveType moveType)
{
	const FSpecialMoveParameters& params = SpecialMoveParams[moveType];

	if (params.GP.Physics >= 0 && Physics != params.GP.Physics && !params.KeepLocomotionMode)
	{
		setPhysics(params.GP.Physics);
	}

	if (params.GP.DisableCollisions)
	{
		SetCollision(TRUE, FALSE, TRUE);
		bCollideWorld = FALSE;
	}

	if ((params.GP.CollisionHeight > 0.0f || params.GP.CollisionRadius > 0.0f) && !params.bCollisionChangeOnTrigger)
	{
		FLOAT colHeight = params.GP.CollisionHeight > 0.0f ? params.GP.CollisionHeight : DefaultPawn->CylinderComponent->CollisionHeight;
		FLOAT colRadius = params.GP.CollisionRadius > 0.0f ? params.GP.CollisionRadius : DefaultPawn->CylinderComponent->CollisionRadius;
		TryAdjustCollisionSize(colHeight, colRadius);
	}

	check(FullBodyAnimSlot);
	FullBodyAnimSlot->StopCustomAnim(0.25f); // if already playing a custom anim, stop it now

	if (params.bNoAnim || bDelayedSpecialMoveAnim)
	{
		bPlayingSpecialMoveAnim = FALSE;		
	}
	else
	{
		if (params.AnimName != NAME_None)
		{
			PlayFullBodyAnim(params.AnimName, 1.f, params.AnimBlendInTime, 0.25f);		
		}
		else
		{
			PlayAnimForSpecialMove(moveType);
		} 		
		bPlayingSpecialMoveAnim = TRUE;
	}

	SetRootMotionMode((ERootMotionMode)params.GP.RMM);

	bSpecialMoveInterruptible = params.bAlwaysInterruptible;
	bPendingSpecialMoveAnims = FALSE;
}

void AOLPawn::SetRootMotionMode(ERootMotionMode RMM)
{
	if (RMM == RMM_Accel)
	{
		Mesh->RootMotionMode = RMM_Accel;
		Mesh->RootMotionRotationMode = RMRM_RotateActor;

		FullBodyAnimSlot->SetRootBoneAxisOption(RBA_Translate, RBA_Translate, RBA_Translate);
		FullBodyAnimSlot->SetRootBoneRotationOption(RRO_Extract, RRO_Extract, RRO_Extract);
	}
	else
	{
		Mesh->RootMotionMode = RMM_Ignore;
		Mesh->RootMotionRotationMode = RMRM_Ignore;

		FullBodyAnimSlot->SetRootBoneAxisOption(RBA_Default, RBA_Default, RBA_Default);	
		FullBodyAnimSlot->SetRootBoneRotationOption(RRO_Default, RRO_Default, RRO_Default);
	}
}

void AOLPawn::ActivatePositionAdjustment(ESpecialMoveType moveType, const FVector& targetPosition, const FVector& targetDirection, EAdjustPositionTargetType targetType)
{	
	const FSpecialMoveParameters& params = SpecialMoveParams[moveType];

	FVector positionError = FVector(0.0f);
	FLOAT headingError = 0.0f;

	UAnimNodeSequence* animNodeSeq = NULL;
	FLOAT animLength = 0.0f;
	
	if (CustomBlendNode && CustomBlendNode->bActive && !params.bNoAnim)
	{
		animLength = CustomBlendNode->PlaybackTime;
	}
	else if (FullBodyAnimSlot && !params.bNoAnim)
	{
		animNodeSeq = Cast<UAnimNodeSequence>(FullBodyAnimSlot->GetCustomAnimNodeSeq());

		if (animNodeSeq)
		{
			animLength = animNodeSeq->GetAnimPlaybackLength();
		}
	}

	if (targetType == APTT_TargetAtStart || params.bNoAnim)
	{
		positionError = targetPosition - Location;
		// For EnterLadderFromAbove on dummy: suppress AdjustPosition — the lateral positionError
		// fights root motion and causes diagonal drift. LOC interpolation handles final positioning.
		headingError = bIsDummyPawn ? 0.0f : UNR_TO_DEG * (FLOAT)FRotator::NormalizeAxis(targetDirection.Rotation().Yaw - Rotation.Yaw);
	}
	else if (CustomBlendNode && CustomBlendNode->bActive)
	{
		check(!params.bPlayAnimWhenPositioned);
		FBoneAtom rootMotion = CustomBlendNode->GetTotalRootMotion();

		FVector predictedLocation = Location + Rotation.Quaternion().RotateVector(rootMotion.GetTranslation());
		positionError = targetPosition - predictedLocation;

		FLOAT predictedYaw = Rotation.Yaw + rootMotion.GetRotation().Rotator().Yaw;
		headingError = UNR_TO_DEG * (FLOAT)FRotator::NormalizeAxis(targetDirection.Rotation().Yaw - predictedYaw);
	}
	else if (animNodeSeq)
	{
		check(!params.bPlayAnimWhenPositioned);
		FBoneAtom rootMotion = animNodeSeq->GetTotalRootMotion();

		FVector predictedLocation = Location + Rotation.Quaternion().RotateVector(rootMotion.GetTranslation());
		positionError = targetPosition - predictedLocation;

		FLOAT predictedYaw = Rotation.Yaw + rootMotion.GetRotation().Rotator().Yaw;
		headingError = UNR_TO_DEG * (FLOAT)FRotator::NormalizeAxis(targetDirection.Rotation().Yaw - predictedYaw);
	}

	FLOAT timeRequired = 0.0f;

	if (params.UseAnimTimeForPositionAdjustment)
	{
		timeRequired = animLength;
	}
	else
	{
		if (params.AdjustPosition)
		{
			FLOAT positionningVel = params.bUsePawnVelocityForPositionning ? Max(params.PositionningLinearVelocity, RealVelocity.Size()) : params.PositionningLinearVelocity;
			timeRequired = positionError.Size() / positionningVel;
		}

		if (params.AdjustOrientation)
		{
			timeRequired = Max(timeRequired, Abs(headingError) / params.PositionningAngularVelocity);
		}
	}

	if (timeRequired > 0.0f)
	{
		AdjustPosition.Active = TRUE;
		AdjustPosition.Done = FALSE;
		AdjustPosition.PositionError = positionError;
		AdjustPosition.HeadingError = headingError;
		AdjustPosition.TargetType = targetType;
		AdjustPosition.OriginalPosition = Location;
		AdjustPosition.OriginalRotation = Rotation;
		AdjustPosition.CorrectionTime = timeRequired;
		AdjustPosition.ElapsedTime = 0.0f;

		debugf(TEXT("[%s] Activating adjustment of [%s] with heading error %.2f degs in %.2f seconds"), *GetName(), *positionError.ToString(), headingError, timeRequired);
	}
}

void AOLPawn::FixUpRootMotion()
{
	// Dummy pawn rotation is pre-set to the target direction before StartSpecialMove,
	// so headingError is always 0 — FixUpRootMotion would corrupt RMDelta if the pawn
	// happened to face the wrong way at the time headingError was calculated.
	if (bIsDummyPawn)
		return;

	const FSpecialMoveParameters& params = SpecialMoveParams[SpecialMove];

	check(AdjustPosition.CorrectionTime > 0.0f);
	FLOAT pctCorrection = AdjustPosition.ElapsedTime / AdjustPosition.CorrectionTime;
	FLOAT angleCorrection = 0.0f;

	if (AdjustPosition.TargetType == APTT_TargetAtEnd && !params.bNoAnim)
	{
		// Use uncorrected rotation (errors were calculated after applying the anim motion)				
		angleCorrection = -(pctCorrection * AdjustPosition.HeadingError);		
	}
	else
	{
		// Use fully corrected rotation (errors were calculated before applying the anim motion)
		angleCorrection = (1.0f - pctCorrection) * AdjustPosition.HeadingError;		
	}

	FRotator fixedRot = Rotation;
	fixedRot.Yaw = FRotator::NormalizeAxis(Rotation.Yaw + (INT)(DEG_TO_UNR * angleCorrection));

	FVector relVel = Rotation.Quaternion().Inverse().RotateVector(Velocity);
	Velocity = fixedRot.Quaternion().RotateVector(relVel);
}

void AOLPawn::ApplyPositionAdjustment(FLOAT deltaTime)
{
	const FSpecialMoveParameters& params = SpecialMoveParams[SpecialMove];
	
	AdjustPosition.ElapsedTime += deltaTime;
	FLOAT deltaCorrection = deltaTime / AdjustPosition.CorrectionTime; // linear for now

	if (AdjustPosition.ElapsedTime > AdjustPosition.CorrectionTime)
	{
		deltaCorrection = (AdjustPosition.CorrectionTime - (AdjustPosition.ElapsedTime - deltaTime)) / AdjustPosition.CorrectionTime;
		AdjustPosition.ElapsedTime = AdjustPosition.CorrectionTime;
	}

	if (params.AdjustOrientation)
	{
		FLOAT angleCorrection = deltaCorrection * AdjustPosition.HeadingError;
		FRotator newRot = Rotation;
		newRot.Yaw = FRotator::NormalizeAxis(Rotation.Yaw + (INT)(DEG_TO_UNR * angleCorrection));
		SetRotation(newRot);
		SetDesiredRotation(newRot); // For AI Controlled Pawns.
	}

	if (params.AdjustPosition)
	{
		FVector correction = deltaCorrection * AdjustPosition.PositionError;
		Velocity += correction / deltaTime;
	}
}

void AOLPawn::UpdateSpecialMove(FLOAT deltaTime)
{
	if (AdjustPosition.Active && !AdjustPosition.Done && AdjustPosition.ElapsedTime >= AdjustPosition.CorrectionTime)
	{
		AdjustPosition.Done = TRUE;

		if (SpecialMove != SMT_None && bDelayedSpecialMoveAnim)
		{
			if (SpecialMoveParams[SpecialMove].AnimName != NAME_None)
			{
				PlayFullBodyAnim(SpecialMoveParams[SpecialMove].AnimName, 1.f, SpecialMoveParams[SpecialMove].AnimBlendInTime, 0.25f);		
			}
			else
			{
				PlayAnimForSpecialMove((ESpecialMoveType)SpecialMove);
			} 		
			bPlayingSpecialMoveAnim = TRUE;
			SetRootMotionMode((ERootMotionMode)SpecialMoveParams[SpecialMove].GP.RMM);
		}		
	}

	if (IsSpecialMoveCompleted())
	{
		if (bPendingSpecialMoveAnims)
		{
			SpecialMovePhaseCompleted();
		}
		else
		{
			SpecialMoveCompleted();
		}
	}
	else if (bProceduralAnimsDelayedAfterSpecialMove && ((!AdjustPosition.Active || AdjustPosition.Done) && !bPlayingSpecialMoveAnim))
	{
		// we're done with the anim and adjustments, proceed with the delayed procedural stuff
		bProceduralAnimsDelayedAfterSpecialMove = FALSE;
	}
}

UBOOL AOLPawn::TryCompleteSpecialMove()
{
	if (!bPendingSpecialMoveAnims && IsSpecialMoveCompleted())
	{
		SpecialMoveCompleted();
		return TRUE;
	}
	return FALSE;
}

UBOOL AOLPawn::IsSpecialMoveCompleted() 
{
	if (ProceduralAnims.Num() > 0)
	{
		return FALSE;
	} 

	return ((!AdjustPosition.Active || AdjustPosition.Done) && !bPlayingSpecialMoveAnim);
}

UBOOL AOLPawn::TryCommitToSpecialMove(ESpecialMoveType moveType, AActor* refActor)
{
	UBOOL bOk = TRUE;

	for (APawn* pawn = GWorld->GetWorldInfo()->PawnList; pawn != NULL; pawn = pawn->NextPawn)
	{
		AOLPawn* olPawn = Cast<AOLPawn>(pawn);
		if (olPawn && olPawn != this && !olPawn->ShouldAllowOtherPawnSpecialMove(this, moveType, refActor))
		{
			bOk = FALSE;
			break;
		}
	}

	if (!bOk)
	{
		return FALSE;
	}

	// we can commit - notify the other pawns

	for (APawn* pawn = GWorld->GetWorldInfo()->PawnList; pawn != NULL; pawn = pawn->NextPawn)
	{
		AOLPawn* olPawn = Cast<AOLPawn>(pawn);
		if (olPawn && olPawn != this)
		{
			olPawn->OnOtherPawnStartSpecialMove(this, moveType, refActor);
		}
	}

	return TRUE;
}

void AOLPawn::SpecialMoveCompleted() 
{
	debugf(TEXT("[%s] Special move completed: %s"), *GetName(), *Utils::GetEnumString("ESpecialMoveType", SpecialMove));

	const FSpecialMoveParameters& params = SpecialMoveParams[SpecialMove];

	if (params.GP.DisableCollisions)
	{
		SetCollision(TRUE, TRUE, FALSE);
		bCollideWorld = TRUE;
	}

	// Dummy pawns use PlayShadowOnlyAnim/StartDummySpecialMove which set RMM directly.
	// Calling SetRootMotionMode here on a dummy corrupts the AnimTree locomotion nodes.
	if (!Cast<AOLHero>(this) || !((AOLHero*)this)->bIsDummyPawn)
		SetRootMotionMode(RMM_Ignore);

	SpecialMove = SMT_None;
	AdjustPosition.Active = FALSE;
	AdjustPosition.Done = FALSE;	
}

void AOLPawn::ApplyProceduralAnims(FLOAT deltaTime)
{
	check(ProceduralAnims.Num() > 0);

	FProceduralAnimData& animData = ProceduralAnims(0);

	if (animData.bWaitForNotify)
	{
		// Delayed until we receive an animnotify
		return;
	}

	UBOOL done = FALSE;

	FLOAT delta = 0.0f;

	TWEAKABLE UBOOL bSmoothed = TRUE;

	if (bSmoothed)
	{
		FLOAT alpha = animData.ElapsedTime / animData.TotalTime;
		FLOAT beta = Saturate((animData.ElapsedTime + deltaTime) / animData.TotalTime);

		FLOAT pctA = Utils::SmootherStep(alpha);
		FLOAT pctB = Utils::SmootherStep(beta);
		delta = pctB - pctA;

		if ((animData.ElapsedTime + deltaTime) > animData.TotalTime)
		{
			// Near the end
			animData.ElapsedTime = animData.TotalTime;
			done = TRUE;
		}
		else
		{
			animData.ElapsedTime += deltaTime;
		}
	}
	else
	{
		if ((animData.ElapsedTime + deltaTime) > animData.TotalTime)
		{
			// Near the end
			delta = (animData.TotalTime - animData.ElapsedTime) / animData.TotalTime;
			animData.ElapsedTime = animData.TotalTime;
			done = TRUE;
		}
		else
		{
			animData.ElapsedTime += deltaTime;
			delta = deltaTime / animData.TotalTime;
		}
	}

	FVector deltaPos = delta * animData.PositionDelta;
	FVector addedVelocity = deltaPos / deltaTime;

	FLOAT angleCorrection = delta * animData.HeadingDelta;

	INT DesiredYaw = FRotator::NormalizeAxis(Rotation.Yaw + (INT)(DEG_TO_UNR * angleCorrection));
	DesiredRotation.Yaw = DesiredYaw; // for ai
	Rotation.Yaw = DesiredYaw;

	if (done)
	{
		for (INT i = 0; i < ProceduralAnims.Num() - 1; i++)
		{
			ProceduralAnims(i) = ProceduralAnims(i+1);
		}

		ProceduralAnims.Remove(ProceduralAnims.Num() - 1);
	}

	if (Mesh->RootMotionMode != RMM_Ignore)
	{
		Velocity += addedVelocity;
	}
	else
	{
		Velocity = addedVelocity; // we don't currently support non-RMX mixing with procedural anims, as the velocity would keep adding over last frame's correction
	}
}

void AOLPawn::QueueProceduralAnim(FProceduralAnimData& animData)
{
	QueueProceduralAnim(animData, ProceduralAnimLinearVelocity, ProceduralAnimAngularVelocity);
}

void AOLPawn::QueueProceduralAnim(FProceduralAnimData& animData, FLOAT linearVelocity, FLOAT angularVelocity)
{
	if (appIsNearlyZero(linearVelocity, KINDA_SMALL_NUMBERF))
	{
		linearVelocity = ProceduralAnimLinearVelocity;
	}
	if (appIsNearlyZero(angularVelocity, KINDA_SMALL_NUMBERF))
	{
		angularVelocity = ProceduralAnimAngularVelocity;
	}

	animData.TotalTime = Max(animData.PositionDelta.Size() / linearVelocity, Abs(animData.HeadingDelta) / angularVelocity);

	if (appIsNearlyZero(animData.TotalTime, KINDA_SMALL_NUMBERF))
	{
		return;
	}
	animData.ElapsedTime = 0.0f;
	ProceduralAnims.AddItem(animData);
}

void AOLPawn::ClearProceduralAnims()
{
	ProceduralAnims.Reset();

	bProceduralAnimsDelayedAfterSpecialMove = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Base classes overrides
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////


void AOLPawn::performPhysics(FLOAT deltaSeconds)
{
	Super::performPhysics(deltaSeconds);
	Rotation = FRotator(0, Rotation.Yaw, 0); // make sure there's no noise (e.g. from long rmx-enabled matinees)
}

void AOLPawn::physCustom(FLOAT deltaTime, INT iterations)
{
	AirSpeed = GroundSpeed;
	
	FVector AccelDir = Acceleration;
	CalcVelocity(AccelDir, deltaTime, AirSpeed, 0.5f * PhysicsVolume->FluidFriction, 1, 0, 0);

	iterations++;
	bJustTeleported = 0;

	FVector OldLocation = Location;
	const FVector Adjusted = (Velocity + PhysicsVolume->GetZoneVelocityForActor(this)) * deltaTime;

	FCheckResult Hit(1.f);
	GWorld->MoveActor(this, Adjusted, Rotation, 0, Hit);

	if( Hit.Time < 1.f )
	{
		Floor = Hit.Normal;
		FVector GravDir = FVector(0,0,-1);
		FVector DesiredDir = Adjusted.SafeNormal();
		FVector VelDir = Velocity.SafeNormal();
		FLOAT UpDown = GravDir | VelDir;

		if( (Abs(Hit.Normal.Z) < 0.2f) && (UpDown < 0.5f) && (UpDown > -0.2f) )
		{
			FLOAT stepZ = Location.Z;
			stepUp(GravDir, DesiredDir, Adjusted * (1.f - Hit.Time), Hit);
			OldLocation.Z = Location.Z + (OldLocation.Z - stepZ);
		}
		else
		{
			processHitWall(Hit, deltaTime);
			//adjust and try again
			FVector OldHitNormal = Hit.Normal;
			FVector Delta = (Adjusted - Hit.Normal * (Adjusted | Hit.Normal)) * (1.f - Hit.Time);
			if( (Delta | Adjusted) >= 0 )
			{
				GWorld->MoveActor(this, Delta, Rotation, 0, Hit);

				if( Hit.Time < 1.f ) //hit second wall
				{
					processHitWall(Hit, deltaTime*(1.f-Hit.Time));
					TwoWallAdjust(DesiredDir, Delta, Hit.Normal, OldHitNormal, Hit.Time);
					GWorld->MoveActor(this, Delta, Rotation, 0, Hit);
				}
			}
		}
	}
	else
	{
		Floor = FVector(0.f,0.f,1.f);
	}

	if( !bJustTeleported )
	{
		Velocity = (Location - OldLocation) / deltaTime;
	}
}

void AOLPawn::CalcVelocity(FVector &AccelDir, FLOAT DeltaTime, FLOAT MaxSpeed, FLOAT Friction, INT bFluid, INT bBrake, INT bBuoyant)
{
	if (SpecialMove != SMT_None && !SpecialMoveParams[SpecialMove].PlayerInputEnabled)
	{
		Velocity = FVector(0);
		Acceleration = FVector(0);
	}

	// physCustom does not go through physWalking's root-motion path — APawn::CalcVelocity
	// reads and clears RootMotionDelta but never sets Velocity from it for physCustom.
	// Capture it before Super clears it, then apply after.
	FVector CapturedRMDelta = FVector::ZeroVector;
	const UBOOL bApplyRootMotion = (Physics == PHYS_Custom && Mesh &&
		Mesh->RootMotionMode == RMM_Accel &&
		(Mesh->PreviousRMM != RMM_Ignore || (bIsDummyPawn && SpecialMove == SMT_EnterLadderFromAbove)) &&
		Mesh->bProcessingRootMotion);
	if (bApplyRootMotion)
		CapturedRMDelta = Mesh->RootMotionDelta.GetTranslation();

	Super::CalcVelocity(AccelDir, DeltaTime, MaxSpeed, Friction, bFluid, bBrake, bBuoyant);

	if (bApplyRootMotion && !CapturedRMDelta.IsNearlyZero())
	{
		Velocity = CapturedRMDelta / DeltaTime;
	}
	if (SpecialMove != SMT_None && AdjustPosition.Active)
	{
		FixUpRootMotion();

		if (!AdjustPosition.Done)
		{
			ApplyPositionAdjustment(DeltaTime);
		}
	}

	if (!bProceduralAnimsDelayedAfterSpecialMove && ProceduralAnims.Num() > 0)
	{
		ApplyProceduralAnims(DeltaTime);
	}
}

void AOLPawn::Crouch(INT bClientSimulation)
{
	if (TryAdjustCollisionSize(CrouchHeight, CrouchRadius))
	{
		bIsCrouched = TRUE;
		bForceFloorCheck = TRUE;

		OnCrouch();
	}
}

void AOLPawn::UnCrouch(INT bClientSimulation)
{
	// If we're uncrouching during a special move which requires a tight collision, don't override it
	UBOOL bUncrouchingForSpecialMove = (SpecialMove != SMT_None && SpecialMove != SMT_Uncrouch && SpecialMove != SMT_Crouch && 
		((SpecialMoveParams[SpecialMove].GP.CollisionRadius < DefaultPawn->CylinderComponent->CollisionRadius) || 
		(SpecialMoveParams[SpecialMove].GP.CollisionHeight < DefaultPawn->CylinderComponent->CollisionHeight)));

	if (bUncrouchingForSpecialMove || TryAdjustCollisionSize(DefaultPawn->CylinderComponent->CollisionHeight, DefaultPawn->CylinderComponent->CollisionRadius))
	{
		bIsCrouched = FALSE;
		bForceFloorCheck = TRUE;

		OnUncrouch();
	}
}

UBOOL AOLPawn::CanUncrouch()
{
	return !bIsCrouched || !WouldEncroach(Location);
}

void AOLPawn::NativePostBeginPlay()
{
	DefaultPawn = (AOLPawn*)(GetClass()->GetDefaultActor());

	if (Utils::GetSoundEnvManager())
	{
		Utils::GetSoundEnvManager()->RegisterSoundEmitterForPawn(this);
	}

	PreviousLocation = Location;
}

FLOAT AOLPawn::PlayFullBodyAnim(const FName& animName, FLOAT playRate, FLOAT blendInTime, FLOAT blendOutTime, FLOAT startTime, FLOAT endTime)
{
	FLOAT RetValue = FullBodyAnimSlot->PlayCustomAnim(animName, playRate, blendInTime, blendOutTime, FALSE, FALSE, startTime, endTime);
	FullBodyAnimSlot->SetActorAnimEndNotification(TRUE);

	if (RetValue > 0.f)
	{
		PlayingSpecialMoveAnims.AddItem(animName);
	}

	return RetValue;
}

FLOAT AOLPawn::PlayBlendedAnim(const FName& animNameA, const FName& animNameB, FLOAT weightA, FLOAT blendInTime, FLOAT blendOutTime, FLOAT rate, FLOAT startRatio)
{
	FLOAT RetValue = CustomBlendNode->PlayCustomBlend(animNameA, animNameB, weightA, blendInTime, blendOutTime, rate, startRatio);

	if (RetValue > 0.f)
	{
		PlayingSpecialMoveAnims.AddItem(animNameA);
		PlayingSpecialMoveAnims.AddItem(animNameB);
	}

	return RetValue;
}

FLOAT AOLPawn::PlayBlendedAnim3(const FName& animNameA, const FName& animNameB, const FName& animNameC, FLOAT blendWeightA, FLOAT blendWeightB, FLOAT blendInTime, FLOAT blendOutTime, FLOAT rate, FLOAT startRatio)
{
	FLOAT RetValue = CustomBlendNode->PlayCustomBlend(animNameA, animNameB, animNameC, blendWeightA, blendWeightB, blendInTime, blendOutTime, rate, startRatio);

	if (RetValue > 0.f)
	{
		PlayingSpecialMoveAnims.AddItem(animNameA);
		PlayingSpecialMoveAnims.AddItem(animNameB);
		PlayingSpecialMoveAnims.AddItem(animNameC);
	}

	return RetValue;
}

FLOAT AOLPawn::PlayBlendSpace(const FBlendSpaceNode nodes[], INT numNodes, const INT blendSpaces[][3], INT numSpaces, const FVector2D& coord, FLOAT blendInTime, FLOAT blendOutTime, FLOAT rate, FLOAT startRatio)
{
	FLOAT RetValue = CustomBlendNode->PlayBlendSpace(nodes, numNodes, blendSpaces, numSpaces, coord, blendInTime, blendOutTime, rate, startRatio);
	
	if (RetValue > 0.f)
	{
		// we don't really know which anims actually play so we add them all
		for (INT i = 0; i < numNodes; i++)
		{
			PlayingSpecialMoveAnims.AddItem(nodes[i].AnimName);
		}
	}

	return RetValue;
}

void AOLPawn::NativeOnAnimEnd(UAnimNodeSequence* seqNode, FLOAT playedTime, FLOAT excessTime)
{
	UBOOL bValidTrigger = (bPlayingSpecialMoveAnim && SpecialMove != SMT_None && !SpecialMoveParams[SpecialMove].bExitOnBlendOut);
	bValidTrigger = bValidTrigger && (!bFilterAnimEndNotifies || PlayingSpecialMoveAnims.ContainsItem(seqNode->AnimSeqName));

	if (bValidTrigger)
	{
		bPlayingSpecialMoveAnim = FALSE;
	}
}

void AOLPawn::OnEarlyAnimEnd(UAnimNodeSequence* seqNode)
{
	UBOOL bValidTrigger = (bPlayingSpecialMoveAnim && SpecialMove != SMT_None && SpecialMoveParams[SpecialMove].bExitOnBlendOut);
	bValidTrigger = bValidTrigger && (!bFilterAnimEndNotifies || PlayingSpecialMoveAnims.ContainsItem(seqNode->AnimSeqName));

	if (bValidTrigger)
	{
		bPlayingSpecialMoveAnim = FALSE;
	}
}

void AOLPawn::StartMatinee(FVector startLoc, FRotator startRot, FLOAT blendTime)
{
	FProceduralAnimData animData;

	animData.HeadingDelta = UNR_TO_DEG * (FLOAT)FRotator::NormalizeAxis(startRot.Yaw - Rotation.Yaw);
	animData.PositionDelta = startLoc - Location;

	FLOAT linearVel = 200.0f;
	FLOAT angularVel = 180.0f;

	if (blendTime > 0.0f)
	{
		linearVel = animData.PositionDelta.Size() / blendTime;
		angularVel = animData.HeadingDelta / blendTime;
	}

	QueueProceduralAnim(animData, linearVel, angularVel);
}

void AOLPawn::MoveInterruptibleNotify()
{
	if (SpecialMove != SMT_None && SpecialMoveParams[SpecialMove].bInterruptibleAfterTrigger)
	{
		bSpecialMoveInterruptible = TRUE;
	}
}

void AOLPawn::ChangeCollisionSizeNotify()
{
	if (SpecialMove == SMT_None || bIsDummyPawn)
		return;

	const FSpecialMoveParameters& params = SpecialMoveParams[SpecialMove];

	if (params.bCollisionChangeOnTrigger && (params.GP.CollisionHeight > 0.0f || params.GP.CollisionRadius > 0.0f))
	{
		FLOAT colHeight = params.GP.CollisionHeight > 0.0f ? params.GP.CollisionHeight : DefaultPawn->CylinderComponent->CollisionHeight;
		FLOAT colRadius = params.GP.CollisionRadius > 0.0f ? params.GP.CollisionRadius : DefaultPawn->CylinderComponent->CollisionRadius;
		TryAdjustCollisionSize(colHeight, colRadius);
	}
}

void AOLPawn::RestoreCollisionSizeNotify()
{
	if (SpecialMove == SMT_None || bIsDummyPawn)
		return;

	const FSpecialMoveParameters& params = SpecialMoveParams[SpecialMove];

	if (params.GP.CollisionHeight > 0.0f || params.GP.CollisionRadius > 0.0f)
	{
		FLOAT colHeight = DefaultPawn->CylinderComponent->CollisionHeight;
		FLOAT colRadius = DefaultPawn->CylinderComponent->CollisionRadius;
		bFailedCollisionSet = TryAdjustCollisionSize(colHeight, colRadius);
	}
}

void AOLPawn::ProceduralAdjustNotify(FLOAT duration)
{
	check(duration > 0.0f);

	for (INT i = 0; i < ProceduralAnims.Num(); i++)
	{
		FProceduralAnimData& animData = ProceduralAnims(i);
		if (animData.bWaitForNotify)
		{
			animData.bWaitForNotify = FALSE;
			animData.TotalTime = duration;
			return;
		}
	}
}

UBOOL AOLPawn::Tick(FLOAT deltaTime, enum ELevelTick TickType )
{
	if (Utils::IsInMainMenu())
	{
		return FALSE;
	}

	TickPrePhysics(deltaTime);
	return Super::Tick(deltaTime, TickType);
}

void AOLPawn::TickSpecial(FLOAT deltaTime)
{
	Super::TickSpecial(deltaTime);
	TickPostPhysics(deltaTime);
}

void AOLPawn::TickPrePhysics(FLOAT deltaTime)
{
	if (SpecialMove != SMT_None)
	{
		UpdateSpecialMove(deltaTime);
	}
}

void AOLPawn::TickPostPhysics(FLOAT deltaTime)
{
	if (deltaTime > 0.0f)
	{
		RealVelocity = (Location - PreviousLocation) / deltaTime;
		PreviousLocation = Location;
	}

	if (PendingAnimSetUpdateTime > 0.0f && GWorld->GetTimeSeconds() > PendingAnimSetUpdateTime)
	{
		UpdateAnimSetList();
		PendingAnimSetUpdateTime = -1.0f;
	}

	if (Utils::GetCheatManager() && Utils::GetCheatManager()->bDebugTrajectory)
	{
		FDebugTrajectoryPoint point;
		point.Position = Location;
		point.Speed = RealVelocity.Size();
		point.TimeStamp = GWorld->GetTimeSeconds();
		point.PointType = DTT_Other;

		if (ProceduralAnims.Num() > 0 && !ProceduralAnims(0).bWaitForNotify)
		{
			point.PointType = DTT_ProceduralAnim;
		}
		else if (SpecialMove != NULL)
		{
			if (AdjustPosition.Active && !AdjustPosition.Done)
			{
				point.PointType = DTT_AdjustPosition;
			}
			else
			{
				point.PointType = DTT_SpecialMove;
			}
		}
		else if (Physics == PHYS_Walking)
		{
			point.PointType = DTT_Walking;
		}
		else if (Physics == PHYS_Falling)
		{
			point.PointType = DTT_Falling;
		}

		Utils::GetCheatManager()->AddDebugTrajectoryPoint(point);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// Collisions
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////


UBOOL	AOLPawn::WouldEncroach(const FVector& testLocation)
{
	return WouldEncroach(testLocation, DefaultPawn->CylinderComponent->CollisionRadius, DefaultPawn->CylinderComponent->CollisionHeight);
}

UBOOL	AOLPawn::WouldEncroach(const FVector& testLocation, FLOAT radius, FLOAT height)
{
	const FLOAT Fudge = 5.0f;

	FLOAT oldHeight = CylinderComponent->CollisionHeight;
	FLOAT oldRadius = CylinderComponent->CollisionRadius;

	FVector realLocation = Location;
	Location = testLocation;

	if (oldHeight != height || oldRadius != radius)
	{
		CylinderComponent->Translation.Z = height + Fudge;
		CylinderComponent->SetCylinderSize(radius, height);		
	}

	CylinderComponent->ConditionalUpdateTransform(LocalToWorld());

	AActor* oldBase = Base;
	FVector oldFloor = Floor;
	SetBase(NULL, oldFloor, 0);

	UBOOL oldCollideWorld = bCollideWorld;
	UBOOL oldCollideActors = bCollideActors;
	bCollideWorld = TRUE;
	bCollideActors = TRUE;

	FMemMark Mark(GMainThreadMemStack);
	FCheckResult* FirstHit = GWorld->Hash->ActorEncroachmentCheck(GMainThreadMemStack, this, testLocation, Rotation, TRACE_AllBlocking);

	UBOOL bEncroached	= FALSE;
	for( FCheckResult* Test = FirstHit; Test!=NULL; Test=Test->GetNext() )
	{
		if ( (Test->Actor != this) && IsBlockedBy(Test->Actor, Test->Component) )
		{
			bEncroached = TRUE;
			break;
		}
	}
	Mark.Pop();

	// Attempt to move to the adjusted location
	if ( !bEncroached && !GWorld->FarMoveActor(this, testLocation, TRUE, FALSE, TRUE) )
	{
		bEncroached = TRUE;
	}

	bCollideWorld = oldCollideWorld;
	bCollideActors = oldCollideActors;

	// Reset the correct values
	Location = realLocation;
	SetBase(oldBase, oldFloor, 0);

	if (oldHeight != height || oldRadius != radius)
	{
		CylinderComponent->Translation.Z = oldHeight + Fudge;
		CylinderComponent->SetCylinderSize(oldRadius, oldHeight);		
	}

	CylinderComponent->ConditionalUpdateTransform(LocalToWorld());

	return bEncroached;
}

UBOOL AOLPawn::TryAdjustCollisionSize(FLOAT newHeight, FLOAT newRadius)
{
	FLOAT oldHeight = CylinderComponent->CollisionHeight;
	FLOAT oldRadius = CylinderComponent->CollisionRadius;

	UBOOL bSuccess = TryAdjustCollisionSizeExact(newHeight, newRadius);

	if (bSuccess)
	{
		return TRUE;
	}
	
	// Failed, but try to get at least the height to match
	UBOOL bAtLeastHeightWorked = TryAdjustCollisionSizeExact(newHeight, oldRadius);

	if (!bAtLeastHeightWorked)
	{
		// Failed, try with the radius
		TryAdjustCollisionSizeExact(oldHeight, newRadius);
	}

	return FALSE;
}

UBOOL AOLPawn::TryAdjustCollisionSizeExact(FLOAT newHeight, FLOAT newRadius)
{
	const FLOAT Fudge = 5.0f;

	FLOAT oldHeight = CylinderComponent->CollisionHeight;
	FLOAT oldRadius = CylinderComponent->CollisionRadius;

	// Do not perform if collision is already at desired size.
	if (oldHeight == newHeight && oldRadius == newRadius)
	{
		return TRUE;
	}

	CylinderComponent->Translation.Z = newHeight + Fudge;
	CylinderComponent->SetCylinderSize(newRadius, newHeight);
	CylinderComponent->ConditionalUpdateTransform(LocalToWorld());

	if ( (newRadius > oldRadius) || (newHeight > oldHeight) )
	{
		AActor* oldBase = Base;
		FVector oldFloor = Floor;
		SetBase(NULL, oldFloor, 0);

		UBOOL oldCollideWorld = bCollideWorld;
		UBOOL oldCollideActors = bCollideActors;
		bCollideWorld = TRUE;
		bCollideActors = TRUE;
		
		FMemMark Mark(GMainThreadMemStack);
		FCheckResult* FirstHit = GWorld->Hash->ActorEncroachmentCheck(GMainThreadMemStack, this, Location, Rotation, TRACE_AllBlocking);

		UBOOL bEncroached	= FALSE;
		for( FCheckResult* Test = FirstHit; Test!=NULL; Test=Test->GetNext() )
		{
			if ( (Test->Actor != this) && IsBlockedBy(Test->Actor,Test->Component) )
			{
				bEncroached = TRUE;
				break;
			}
		}
		Mark.Pop();

		// Attempt to move to the adjusted location
		if ( !bEncroached && !GWorld->FarMoveActor(this, Location, 0, FALSE, TRUE) )
		{
			bEncroached = TRUE;
		}
		
		bCollideWorld = oldCollideWorld;
		bCollideActors = oldCollideActors;
		
		if( bEncroached )
		{
			// Cancel - we'd be encroaching
			CylinderComponent->Translation.Z = oldHeight + Fudge;
			CylinderComponent->SetCylinderSize(oldRadius, oldHeight);
			CylinderComponent->ConditionalUpdateTransform(LocalToWorld());
			SetBase(oldBase, oldFloor, 0);
			return FALSE;
		}
	}

	SetCollisionSize(newRadius, newHeight);

	bForceFloorCheck = TRUE;
	return TRUE;
}

UBOOL AOLPawn::IgnoreBlockingBy(const AActor* Other) const
{
	if (Super::IgnoreBlockingBy(Other))
	{
		return TRUE;
	}

	const AStaticMeshActor* staticMesh = ConstCast<AStaticMeshActor>(Other);
	
	if (staticMesh && staticMesh->StaticMeshComponent->StaticMesh && staticMesh->StaticMeshComponent->StaticMesh->BodySetup == NULL)
	{
		return TRUE;
	}

	const AOLPawn* otherPawn = ConstCast<AOLPawn>(Other);
	if (otherPawn && SpecialMove != SMT_None && SpecialMoveParams[SpecialMove].GP.IgnorePawnCollisions)
	{
		return TRUE;
	}

	return FALSE;
}

FName AOLPawn::GetMaterialBelowFeetAt(const FVector* TraceOrigin)
{
	UPhysicalMaterial* physMat = NULL;

	// First, check if we're touching a OLPhysicalMaterialVolume

	INT bestFloorMatPrio = -1;

	for (INT i = 0; i < Touching.Num(); i++)
	{
		AOLFloorMaterialVolume* floorMatVol = Cast<AOLFloorMaterialVolume>(Touching(i));
		if (floorMatVol && floorMatVol->bEnabled && floorMatVol->PhysMaterial && (bestFloorMatPrio < 0 || floorMatVol->Priority > bestFloorMatPrio))
		{
			physMat = floorMatVol->PhysMaterial;
			bestFloorMatPrio = floorMatVol->Priority;
		}
	}

	// Otherwise trace down from the active foot bone position (if provided) or pawn center.
	// Use MultiLineCheck to skip hidden actors (invisible collision meshes over real geometry).

	if (!physMat)
	{
		FVector TraceBase = TraceOrigin ? FVector(TraceOrigin->X, TraceOrigin->Y, Location.Z) : Location;
		FMemMark Mark(GMainThreadMemStack);
		FCheckResult* Hits = GWorld->MultiLineCheck(GMainThreadMemStack, TraceBase - VecZ(50.0f), TraceBase + VecZ(50.0f), FVector(0.0f), TRACE_AllBlocking | TRACE_ComplexCollision | TRACE_Material, this);
		for (FCheckResult* Hit = Hits; Hit; Hit = Hit->GetNext())
		{
			if (Hit->Actor && !Hit->Actor->bHidden && (!Hit->Component || !Hit->Component->HiddenGame))
			{
				physMat = DetermineCorrectPhysicalMaterial(*Hit);
				break;
			}
		}
		Mark.Pop();
	}

	if (physMat)
	{
		UOLPhysicalMaterialProperty* physMatProp = Cast<UOLPhysicalMaterialProperty>(physMat->PhysicalMaterialProperty);

		if (!physMatProp && physMat->Parent)
		{
			physMatProp = Cast<UOLPhysicalMaterialProperty>(physMat->Parent->PhysicalMaterialProperty); // technically, we should continue trying deeper than one parent level, although in practice we shouldn't need this
		}

		if (physMatProp)
		{
			return physMatProp->MaterialType;
		}
	}

	return NAME_None;
}

FName AOLPawn::GetMaterialBelowFeet()
{
	return GetMaterialBelowFeetAt(NULL);
}


////////////////////////////////////////////////////////////////////////////////////////////
// Misc
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void AOLPawn::PlayVOLine(UAkEvent* LineToPlay, FName BoneName)
{
	AOLGame* CurrentGame = Cast<AOLGame>(GWorld->GetGameInfo());
	if (CurrentGame != NULL)
	{
		AkPlayingID PlayingID;
		CurrentGame->VoiceManager->PlayVOLine(this, LineToPlay, BoneName, PlayingID);
	}
}

UBOOL AOLPawn::UseFootPlacementThisTick()
{
	if (!bEnableFootPlacement)
	{
		return FALSE;
	}

	// Pawn must be walking
	if (Physics != PHYS_Walking)
	{
		return FALSE;
	}

	// Pawn can't be crouched
	if (bIsCrouched)
	{
		return FALSE;
	}

	// is the pawn stopped?
	if (Velocity.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		return FALSE;
	}

	return TRUE;
}

void AOLPawn::EnableFootPlacement(UBOOL bEnabled)
{
	// TODO
}

void AOLPawn::DoFootPlacement(FLOAT deltaTime)
{
	// TODO
}

FVector AOLPawn::GetAIRepulsion(const AOLEnemyPawn* Agent)
{
	if (!bRepelsAI)
	{
		return FVector(0.f);
	}

	FVector RepulsionDir = Agent->Location - Location;
	RepulsionDir.Z = 0.f;
	FLOAT Distance = RepulsionDir.Size();

	RepulsionDir /= Distance;

	FVector Sidestep = FVector(0.f);

	// Check if Agent is heading towards us.
	FVector AgentDestination = Agent->Controller->GetDestinationPosition();
	if (Agent->CurrentMovePath.Num() > 0)
	{
		AgentDestination = Agent->CurrentMovePath(0);
	}

	FVector AgentVelDir = (AgentDestination - Agent->Location).SafeNormal2D();
	FVector AgentCross = AgentVelDir ^ -RepulsionDir;
	FLOAT AgentAngle = appAsin(AgentCross.Size()); 
	if (AgentAngle <= RepulsionCollisionAngle*DEG_TO_RAD)
	{
		Distance *= RepulsionCollisionFactor;
	}

	// Calculate SideStep Direction
	FVector Proj = (-RepulsionDir).ProjectOnTo(AgentVelDir);
	FVector ProjToVel = Proj + RepulsionDir;

	if (ProjToVel.SizeSquared() > Square(KINDA_SMALL_NUMBER))
	{
		Sidestep = ProjToVel.SafeNormal();
	}
	else
	{
		Sidestep = AgentVelDir.RotateAngleAxis(90.0f * DEG_TO_UNR, FVector(0.f, 0.f, 1.f));
	}

	// Check if we're heading towards the Agent.
	FVector EntityCross = Velocity.SafeNormal2D() ^ RepulsionDir;
	FLOAT EntityAngle = appAsin(EntityCross.Size());
	if (EntityAngle <= RepulsionCollisionAngle*DEG_TO_RAD)
	{
		Distance *= RepulsionCollisionFactor;
	}

	FLOAT Magnitude = 0.f;
	if (Distance <= RepulsionMaxDistance)
	{
		Magnitude = 1.f / Distance;
		//Sidestep = Sidestep * (Magnitude * RepulsionSidestepFactor);
		Magnitude *= RepulsionMagnitudeFactor;
		Magnitude = Min(Magnitude, RepulsionMaxMagnitude);
	}

	return (Sidestep * Magnitude);
}

FVector AOLPawn::GetFutureDestination(AOLPawn* Agent)
{
	FLOAT DistanceToAgent = Agent->Location.Distance(Location);
	FLOAT TimeToTarget = DistanceToAgent / Agent->GroundSpeed;

	FLOAT PredictionDistance = Clamp(Velocity.Size() * TimeToTarget * DestinationPredictionFactor, 0.0f, DestinationPredictionMax);

	FVector NewDestination = Location + Velocity.SafeNormal() * PredictionDistance;

	FCheckResult Hit(1.0f);
	if (!GWorld->SingleLineCheck(Hit, this, NewDestination + CollisionComponent->Translation, Location + CollisionComponent->Translation, TRACE_AllBlocking, GetCylinderExtent()))
	{
		NewDestination = Hit.Location - CollisionComponent->Translation;
	}

	return NewDestination;
}

void AOLPawn::DrawDebugAnimNode(FName NodeName, UAnimNode* aNode, TArray<UAnimNode*>& visitedAnimNodes, UCanvas* aCanvas, FLOAT& out_YL, FLOAT& out_YPos, FLOAT XL, FLOAT XBasePos, FLOAT XOffset)
{
	FLOAT XPos = XBasePos + XOffset;

	aCanvas->SetDrawColor(255, 232, 139);

	FLOAT spacingXPos = XBasePos;
	while (spacingXPos < XPos - 5.0f)
	{
		aCanvas->SetPos(spacingXPos, out_YPos);
		aCanvas->DrawText(TEXT("."));
		spacingXPos += XL;
	}

	aCanvas->SetPos(XPos, out_YPos);	

	UAnimNodeSequence* animSeqNode = Cast<UAnimNodeSequence>(aNode);
	UAnimNodeBlendBase* animBlendNode = Cast<UAnimNodeBlendBase>(aNode);

	FString outputText = (NodeName == NAME_None) ? "" : FString::Printf(TEXT("%s: "), *NodeName.ToString());

	if (animSeqNode)
	{
		FLinearColor minColor(1.0f, 1.0f, 1.0f); 
		FLinearColor maxColor(1.0f, 0.55f, 0.0f);
		FColor drawColor = (minColor + aNode->NodeTotalWeight*(maxColor - minColor)).ToFColor(FALSE);
		aCanvas->SetDrawColor(drawColor.R, drawColor.G, drawColor.B);

		outputText += *animSeqNode->AnimSeqName.ToString();

		if (aNode->NodeTotalWeight < 0.99f)
		{
			outputText += FString::Printf(TEXT(" [w: %.0f]"), 100.0f*aNode->NodeTotalWeight);
		}

		if (animSeqNode->AnimSeq)
		{
			FLOAT pctComp = 100.0f * (animSeqNode->AnimSeq->SequenceLength > 0.0f ? (animSeqNode->CurrentTime / animSeqNode->AnimSeq->SequenceLength) : 1.0f);
			outputText += FString::Printf(TEXT(" - %.2f/%.2f (%.0f%%)"), animSeqNode->CurrentTime, animSeqNode->AnimSeq->SequenceLength, pctComp);
		}

		if (!appIsNearlyEqual(animSeqNode->Rate, 1.0f, 0.01f) && animSeqNode->Rate > 0.01f)
		{
			outputText += FString::Printf(TEXT(" (@ %.2fx)"), animSeqNode->Rate);
		}
	}
	else 
	{
		outputText += (aNode->NodeName != NAME_None) ?  FString::Printf(TEXT("%s (%s)"), *aNode->NodeName.ToString(), *aNode->GetName()) : *aNode->GetName();

		if (aNode->NodeTotalWeight < 1.0f)
		{
			outputText += FString::Printf(TEXT(" [w: %.0f]"), 100.0f*aNode->NodeTotalWeight);
		}		
	}
	aCanvas->DrawText(outputText);
	out_YPos += out_YL;	

	if (animBlendNode)
	{		
		if (visitedAnimNodes.ContainsItem(aNode))
		{
			// Don't recurse twice down the same node

			FLOAT spacingOffset = 0.0f;
			FLOAT endOffset = XOffset + XL;
			while (spacingOffset < endOffset - 5.0f)
			{
				aCanvas->SetPos(XBasePos + spacingOffset, out_YPos);
				aCanvas->DrawText(TEXT("."));
				spacingOffset += XL;
			}

			aCanvas->SetPos(XBasePos + endOffset, out_YPos);
			aCanvas->DrawText(TEXT("[shown above]"));
			out_YPos += out_YL;			
		}
		else
		{
			visitedAnimNodes.AddItem(aNode);

			for (INT i = 0; i < animBlendNode->Children.Num(); i++)
			{
				if (animBlendNode->Children(i).Anim && animBlendNode->Children(i).Weight > ZERO_ANIMWEIGHT_THRESH)
				{
					DrawDebugAnimNode(animBlendNode->Children(i).Name, animBlendNode->Children(i).Anim, visitedAnimNodes, aCanvas, out_YL, out_YPos, XL, XBasePos, XOffset + XL);
				}
			}
		}
	}
}


void AOLPawn::StartSpecialMoveByInt(INT moveType)
{
	StartSpecialMove((ESpecialMoveType)moveType, FVector(EC_EventParm), FVector(EC_EventParm), 0);
}
