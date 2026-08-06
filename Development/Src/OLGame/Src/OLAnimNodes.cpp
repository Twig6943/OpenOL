#include "OLGame.h"
#include "EngineAnimClasses.h"
#include "UDKBaseAnimationClasses.h"
#include "OLGameAnimClasses.h"
#include "OLUtilities.h"

IMPLEMENT_CLASS(UOLAnimBlendByLocomotionMode);
IMPLEMENT_CLASS(UOLAnimBlendByEnemyMode);
IMPLEMENT_CLASS(UOLAnimBlendByEnemyEnvironment);
IMPLEMENT_CLASS(UOLAnimBlendBySwarmForm);
IMPLEMENT_CLASS(UOLAnimBlendDirectional);
IMPLEMENT_CLASS(UOLAnimBlendBySpeed);
IMPLEMENT_CLASS(UOLAnimBlendBySpeedExpandable);
IMPLEMENT_CLASS(UOLAnimBlendByPosture);
IMPLEMENT_CLASS(UOLAnimBlendByLeaning);
IMPLEMENT_CLASS(UOLAnimConstrainedMovement);
IMPLEMENT_CLASS(UOLAnimMultiCycleConstrainedMovement);
IMPLEMENT_CLASS(UOLAnimDoorInteraction);
IMPLEMENT_CLASS(UOLAnimCustomBlend);
IMPLEMENT_CLASS(UOLAnimNodeSlot);
IMPLEMENT_CLASS(UOLAnimNodeDelayed);
IMPLEMENT_CLASS(UOLAnimCameraSpace);
IMPLEMENT_CLASS(UOLAnimTurnOnSpot);
IMPLEMENT_CLASS(UOLAnimCrouchedTurnOnSpot);
IMPLEMENT_CLASS(UOLAnimSelectByCamcorder);
IMPLEMENT_CLASS(UOLAnimSelectByShadowProxy);
IMPLEMENT_CLASS(UOLAnimStruggleCycle);
IMPLEMENT_CLASS(UOLAnimPushing);
IMPLEMENT_CLASS(UOLAnimEnemyStruggle);
IMPLEMENT_CLASS(UOLAnimNotify_Blur);
IMPLEMENT_CLASS(UOLAnimNotify_Fade);
IMPLEMENT_CLASS(UOLAnimNotify_Door);
IMPLEMENT_CLASS(UOLAnimNotify_ProceduralAdjust);
IMPLEMENT_CLASS(UOLAnimNotify_OverrideCameraParams);
IMPLEMENT_CLASS(UOLAnimNotify_LeftHandIK);
IMPLEMENT_CLASS(UOLAnimNotify_PropAttachment);
IMPLEMENT_CLASS(UOLAnimBlendByIdle);
IMPLEMENT_CLASS(UOLAnimPeeking);
IMPLEMENT_CLASS(UOLAnimLocomotion);
IMPLEMENT_CLASS(UOLAnimHeatShielding);
IMPLEMENT_CLASS(UOLAnimParrying);
IMPLEMENT_CLASS(UOLAnimSelectByPhysicsVolume);
IMPLEMENT_CLASS(UOLAnimSelectByWalkingStyle);
IMPLEMENT_CLASS(UOLAnimSmartIdle);
IMPLEMENT_CLASS(UOLAnimSmarterIdle);
IMPLEMENT_CLASS(UOLAnimCycleBreaker);
IMPLEMENT_CLASS(UOLAnimNotify_ModifyPlayrate);
IMPLEMENT_CLASS(UOLAnimNotify_NonFatalDamage);
IMPLEMENT_CLASS(UOLAnimNotify_PlayWeaponAnimation);
IMPLEMENT_CLASS(UOLAnimRandomCycle);
IMPLEMENT_CLASS(UOLAnimNotify_AttachWeapon);
IMPLEMENT_CLASS(UOLAnimBlendByHobblingIntensity);

////////////////////////////////////////////////////////////////////////////////////////////
// Utils
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL IsShadowProxy(UAnimNode* animNode)
{
	AOLHero* Owner = animNode->SkelComponent ? Cast<AOLHero>(animNode->SkelComponent->GetOwner()) : NULL;
	return (Owner && Owner->ShadowProxy == animNode->SkelComponent);
}

UAnimNode* FindShadowNode(UAnimNode* masterNode)
{
	AOLHero* Owner = masterNode->SkelComponent ? Cast<AOLHero>(masterNode->SkelComponent->GetOwner()) : NULL;
	if (Owner && Owner->ShadowProxy != masterNode->SkelComponent)
	{
		return Owner->ShadowProxy->FindAnimNode(masterNode->NodeName);
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimBlendByLocomotionMode
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimBlendByLocomotionMode::TickAnim(FLOAT deltaTime)
{
	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (Owner)
	{
		UBOOL bForceContextualLean = Owner->LocomotionMode == LM_SpecialMove && Owner->GetNextLocomotionMode((ESpecialMoveType)Owner->SpecialMove) == LM_ContextualLean; // yeah... that's not exactly pretty
		// Dummy grab-ledge SMTs end in LedgeHang — pre-switch to child[4] while the SMT anim
		// plays so LedgeHang idle is already active when FullBodyAnimSlot blends out.
		UBOOL bDummyPreLedgeHang = Owner->bIsDummyPawn
			&& (Owner->SpecialMove == SMT_GrabLedgeFromGround || Owner->SpecialMove == SMT_GrabLedgeFromAir
				|| Owner->SpecialMove == SMT_JumpOverAndGrabLedge);
		if (Owner->LocomotionMode != LM_SpecialMove || bForceContextualLean || bDummyPreLedgeHang)
		{
			INT desiredIdx = bForceContextualLean ? LM_ContextualLean : (bDummyPreLedgeHang ? LM_LedgeHang : Owner->LocomotionMode);

			if (ActiveChildIndex != desiredIdx)
			{
				check(ChildBlendTimes.Num() == Children.Num());
				SetActiveChild(desiredIdx, ChildBlendTimes(desiredIdx));
			}
		}
	}

	Super::TickAnim(deltaTime);
}

void UOLAnimBlendByLocomotionMode::OnBecomeRelevant()
{
	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	// Force setting the right child when becoming relevant
	if (Owner)
	{
		ELocomotionMode actualLocoMode = (ELocomotionMode)(Owner->LocomotionMode == LM_SpecialMove ? Owner->GetNextLocomotionMode((ESpecialMoveType)Owner->SpecialMove) : Owner->LocomotionMode);

		if (ActiveChildIndex != actualLocoMode)
		{
			SetActiveChild( actualLocoMode, 0.0f );
		}
	}

	Super::OnBecomeRelevant();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimBlendByPosture
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimBlendByPosture::TickAnim(FLOAT deltaTime)
{
	Super::TickAnim(deltaTime);
}

void UOLAnimBlendByPosture::OnBecomeRelevant()
{
	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	// Force setting the right child when becoming relevant
	if (Owner)
	{
		UBOOL shouldCrouch = (Owner->bIsCrouched || Owner->bForcedCrouch || (Owner->IsDoingSpecialMove() && Owner->bMustCrouchAfterSpecialMove));
		if (shouldCrouch  && ActiveChildIndex !=1)
		{
			SetActiveChild(1, 0.0f);
		}
		else if (!shouldCrouch && ActiveChildIndex != 0)
		{
			SetActiveChild(0, 0.0f);
		}
	}

	Super::OnBecomeRelevant();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimBlendByLeaning
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimBlendByLeaning::TickAnim(FLOAT deltaTime)
{
	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (Owner)
	{
		TWEAKABLE FLOAT ApproachCoeff = 0.99999f;
		CurrentRatio = Utils::Approach(CurrentRatio, Owner->CurrentLean, ApproachCoeff, deltaTime);

		Children(IdxIdle).Weight = 1.0f - Abs(CurrentRatio);
		Children(IdxLeft).Weight = Max(0.0f, -CurrentRatio);
		Children(IdxRight).Weight = Max(0.0f, CurrentRatio);
	}

	Super::TickAnim(deltaTime);
}

void UOLAnimBlendByLeaning::OnBecomeRelevant()
{
	Children(IdxIdle).Weight = 1.0f;
	Children(IdxLeft).Weight = 0.0f;
	Children(IdxRight).Weight = 0.0f;

	Super::OnBecomeRelevant();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimBlendBySpeed
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimBlendBySpeed::TickAnim(FLOAT DeltaSeconds)
{
	if (SkelComponent != NULL && SkelComponent->GetOwner() != NULL)
	{
		FLOAT targetWeight = Saturate((SkelComponent->GetOwner()->Velocity.Size() - MinSpeed) / (MaxSpeed - MinSpeed));
		FLOAT deltaWeight = targetWeight - CurrentWeight;

		if (DeltaSeconds <= 0.0f || Abs(deltaWeight)/DeltaSeconds < MaxWeightRate)
		{
			CurrentWeight = targetWeight;
		}
		else
		{
			// Limit variation speed at MaxWeightVel, to prevent sudden jerks
			FLOAT allowedDelta = MaxWeightRate*DeltaSeconds*Sgn(deltaWeight);
			CurrentWeight += allowedDelta;
		}
		Child2Weight = Saturate(CurrentWeight);
	}

	Super::TickAnim(DeltaSeconds);
}

void UOLAnimBlendBySpeed::OnBecomeRelevant()
{
	if (SkelComponent != NULL && SkelComponent->GetOwner() != NULL)
	{
		CurrentWeight = Saturate((SkelComponent->GetOwner()->Velocity.Size() - MinSpeed) / (MaxSpeed - MinSpeed));
	}
	else
	{
		CurrentWeight = 0.0f;
	}
	Super::OnBecomeRelevant();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimBlendBySpeedExpandable
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimBlendBySpeedExpandable::InitAnim( USkeletalMeshComponent* meshComp, UAnimNodeBlendBase* Parent )
{
	START_INITANIM_TIMER
	{
		EXCLUDE_PARENT_TIME
			Super::InitAnim(meshComp, Parent);
	}

	if (Children.Num() > 0)
	{
		Children(0).Weight = 1.0f;

		for (INT Idx = 1; Idx < Children.Num(); ++Idx)
		{
			Children(Idx).Weight = 0.f;
		}
	}
}

void UOLAnimBlendBySpeedExpandable::TickAnim(FLOAT DeltaSeconds)
{
	if (SkelComponent != NULL && SkelComponent->GetOwner() != NULL && Children.Num() != 0)
	{
		FLOAT targetSpeed = SkelComponent->GetOwner()->Velocity.Size2D();

		CurrentSpeed = Utils::Approach(CurrentSpeed, targetSpeed, SmoothingCoefficient, DeltaSeconds);

		SetWeightsFromSpeed(CurrentSpeed);
	}

	Super::TickAnim(DeltaSeconds);
}

void UOLAnimBlendBySpeedExpandable::SetWeightsFromSpeed(FLOAT Speed)
{
	if (Speed <= AnimSpeeds(0) || Speed >= AnimSpeeds(AnimSpeeds.Num() - 1))
	{
		// Outside the limits

		UBOOL bSlowSide = Speed <= AnimSpeeds(0);

		INT fullWeightIdx = bSlowSide ? 0 : (AnimSpeeds.Num() - 1);

		Children(fullWeightIdx).Weight = 1.0f;
		for (INT Idx = 0; Idx < Children.Num(); ++Idx)
		{
			Children(Idx).Weight = (Idx == fullWeightIdx) ? 1.0f : 0.0f;
		}

		if ((bSlowSide && bTimeScaleDown) || (!bSlowSide && bTimeScaleUp))
		{
			// Try to timescale
			UAnimNodeSequence* animNodeSeq = Cast<UAnimNodeSequence>(Children(fullWeightIdx).Anim);

			if (animNodeSeq)
			{
				FLOAT playRate = Speed / AnimSpeeds(fullWeightIdx);
				animNodeSeq->Rate = playRate;
			}
			else
			{
				warnf(TEXT("UOLAnimBlendBySpeedExpandable - can't timescale child %d"), fullWeightIdx);
			}
		}
	}
	else
	{
		// Within the interval
		
		INT firstIndex = 0;
		while (firstIndex < AnimSpeeds.Num() - 1)
		{
			if (Speed < AnimSpeeds(firstIndex+1))
			{
				break;
			}

			++firstIndex;
		}

		if (firstIndex >= AnimSpeeds.Num() - 1)
		{
			firstIndex = AnimSpeeds.Num() - 2;
		}

		FLOAT targetWeight = Max(0.0f, (Speed - AnimSpeeds(firstIndex)) / (AnimSpeeds(firstIndex+1) - AnimSpeeds(firstIndex)));

		targetWeight = Saturate(targetWeight);

		FLOAT childFirstWeight = 1.0f - targetWeight;
		FLOAT childSecondWeight = targetWeight;

		// Update Weights
		for (INT Idx = 0; Idx < Children.Num(); ++Idx)
		{
			if (Idx == firstIndex)
			{
				Children(Idx).Weight = childFirstWeight;
			}
			else if (Idx == firstIndex + 1)
			{
				Children(Idx).Weight = childSecondWeight;
			}
			else
			{
				Children(Idx).Weight = 0.f;
			}
		}
	}
}

void UOLAnimBlendBySpeedExpandable::OnBecomeRelevant()
{
	if (SkelComponent != NULL && SkelComponent->GetOwner() != NULL)
	{
		CurrentSpeed = SkelComponent->GetOwner()->Velocity.Size();
	}

	Super::OnBecomeRelevant();
}

FLOAT UOLAnimBlendBySpeedExpandable::GetSliderPosition(INT SliderIndex, INT ValueIndex)
{
	check(0 == SliderIndex && 0 == ValueIndex);
	return SliderPosition;
}

void UOLAnimBlendBySpeedExpandable::HandleSliderMove(INT SliderIndex, INT ValueIndex, FLOAT NewSliderValue)
{
	check(0 == SliderIndex && 0 == ValueIndex);
	SliderPosition = NewSliderValue;

	if( Children.Num() > 0 && AnimSpeeds.Num() > 0 )
	{
		FLOAT currentSpeed = AnimSpeeds(0) + (SliderPosition * (AnimSpeeds(AnimSpeeds.Num()-1) - AnimSpeeds(0)));

		SetWeightsFromSpeed(currentSpeed);
	}
}

FString UOLAnimBlendBySpeedExpandable::GetSliderDrawValue(INT SliderIndex)
{
	check(0 == SliderIndex);
	//INT TargetChild = appRound( SliderPosition*(Children.Num() - 1) );
	if( Children.Num() > 0 && AnimSpeeds.Num() > 0)
	{
		FLOAT currentSpeed = AnimSpeeds(0) + (SliderPosition * (AnimSpeeds(AnimSpeeds.Num()-1) - AnimSpeeds(0)));

		return FString::Printf( TEXT("%3.2f"), currentSpeed );
	}

	return FString::Printf( TEXT("%3.2f"), SliderPosition );
}

void UOLAnimBlendBySpeedExpandable::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyThatChanged != NULL && PropertyThatChanged->GetName()==TEXT("AnimSpeeds"))
	{
		for (INT Idx = 1; Idx < AnimSpeeds.Num(); ++Idx)
		{
			if (AnimSpeeds(Idx) < AnimSpeeds(Idx-1))
			{
				AnimSpeeds(Idx) = AnimSpeeds(Idx-1);
			}
		}
	}
}

void UOLAnimBlendBySpeedExpandable::OnAddChild(INT ChildNum)
{
	Super::OnAddChild(ChildNum);

	AnimSpeeds.InsertZeroed(ChildNum);

	if (ChildNum > 0)
	{
		AnimSpeeds(ChildNum) = AnimSpeeds(ChildNum - 1);
	}
}

void UOLAnimBlendBySpeedExpandable::OnRemoveChild(INT ChildNum)
{
	Super::OnRemoveChild(ChildNum);

	if (AnimSpeeds.Num() > ChildNum)
	{
		AnimSpeeds.Remove(ChildNum);
	}
}

void UOLAnimBlendBySpeedExpandable::ResetAnimNodeToSource(UAnimNode *SourceNode)
{
	Super::ResetAnimNodeToSource(SourceNode);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimConstrainedMovement
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimConstrainedMovement::OnBecomeRelevant()
{
	SetActiveChild(0, 0.0f);
	CurrentRatio = 0.0f;

	UAnimNodeSequence* nodeSeq1 = Cast<UAnimNodeSequence>(Children(1).Anim);
	UAnimNodeSequence* nodeSeq2 = Cast<UAnimNodeSequence>(Children(2).Anim);

	if (nodeSeq1 && nodeSeq2)
	{
		if (nodeSeq1->AnimSeq == NULL)
		{
			nodeSeq1->SetAnim(nodeSeq1->AnimSeqName);
		}

		if (nodeSeq2->AnimSeq == NULL)
		{
			nodeSeq2->SetAnim(nodeSeq2->AnimSeqName);
		}
	}

	SmoothedDelta = 0.0f;
	LastTargetIdx = 0;
}

void UOLAnimConstrainedMovement::TickAnim(FLOAT deltaTime)
{
	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (!Owner)
	{
		return;
	}

	UAnimNodeSequence* nodeSeq1 = Cast<UAnimNodeSequence>(Children(1).Anim);
	UAnimNodeSequence* nodeSeq2 = Cast<UAnimNodeSequence>(Children(2).Anim);

	if (nodeSeq1 && nodeSeq2)
	{
		if (nodeSeq1->AnimSeq == NULL)
		{
			nodeSeq1->SetAnim(nodeSeq1->AnimSeqName);
		}

		if (nodeSeq2->AnimSeq == NULL)
		{
			nodeSeq2->SetAnim(nodeSeq2->AnimSeqName);
		}
	}

	if (Owner->bKillConstrainedMovement)
	{
		SmoothedDelta = 0.0f;
		CurrentRatio = 0.0f;
		SetActiveChild(0, 0.25f);
	}
	else
	{
		const FVector& effectiveDirection = Owner->DesiredMoveDirection;
		UBOOL noInput = bNetControlled ? appIsNearlyZero(NetSmoothedDelta) : effectiveDirection.IsNearlyZero();
		FLOAT inputScaling = Owner->MovementSpeedModifier;

		INT targetIdx = 0; // which clip we're going towards (0: idle, 1: negative, 2: positive)
		bAutomaticMotion = FALSE;

		if (bNetControlled)
		{
			if (!appIsNearlyZero(NetSmoothedDelta))
				targetIdx = (NetSmoothedDelta > 0.0f) ? 2 : 1;
		}
		else if (Owner->bBlockConstrainedMovement && appIsNearlyZero(CurrentRatio))
		{
			targetIdx = 0;
		}
		else if (noInput)
		{
			if (bCompleteCyclesOnly)
			{
				targetIdx = (CurrentRatio > 0.5f) ? 2 : (CurrentRatio < -0.5f ? 1 : 0);
				bAutomaticMotion = TRUE;
			}
			else
			{
				targetIdx = LastTargetIdx;
			}
		}
		else
		{
			if (bUpDown)
			{
				targetIdx = (effectiveDirection.Z > 0.0f) ? 2 : 1;
			}
			else
			{
				targetIdx = ((effectiveDirection | Owner->Rotation.Right()) > 0.0f) ? 2 : 1;
			}

			if (Owner->bBlockConstrainedMovement)
			{
				if ((targetIdx == 2 && CurrentRatio < 0.0f) || (targetIdx == 1 && CurrentRatio > 0.0f))
				{
					targetIdx = 0;
				}
			}
		}

		LastTargetIdx = targetIdx;

		INT currentIdx = appIsNearlyZero(CurrentRatio) ? 0 : (CurrentRatio > 0.0f ? 2 : 1); // which clip we're currently playing

		if (currentIdx != 0 || targetIdx != 0) // nothing to do if staying in idle
		{
			INT baseAnimIdx = (currentIdx != 0) ? currentIdx : targetIdx; // used to calculate the ratio per frame
			UAnimNodeSequence* baseNodeSeq = Cast<UAnimNodeSequence>(Children(baseAnimIdx).Anim);
			if (baseNodeSeq && baseNodeSeq->AnimSeq)
			{
				FLOAT maxDeltaRatio = inputScaling * (deltaTime / baseNodeSeq->AnimSeq->SequenceLength);
				FLOAT deltaRatio = 0.0f; 

				// Calculate the desired delta for this frame

				if (bNetControlled)
				{
					// targetIdx already encodes direction via sign of NetSmoothedDelta.
					// SmoothedDelta must be positive — deltaRatio applies the sign below.
					SmoothedDelta = Abs(NetSmoothedDelta);
				}
				else
				{
					FLOAT targetDelta = (noInput && !bCompleteCyclesOnly) ? 0.0f : maxDeltaRatio;
					TWEAKABLE FLOAT SmoothingCoeff = 0.999f;
					SmoothedDelta = Utils::Approach(SmoothedDelta, targetDelta, SmoothingCoeff, deltaTime);
				}

				if (appIsNearlyZero(SmoothedDelta, KINDA_SMALL_NUMBERF))
				{	
					UAnimNodeSequence* animNode = Cast<UAnimNodeSequence>(Children(ActiveChildIndex).Anim);
					if (animNode && animNode->AnimSeq)
					{
						animNode->PreviousTime = animNode->CurrentTime; // make sure to kill the root motion
						animNode->ConditionalClearCachedData();
					}
				}
				else
				{
					if (targetIdx == 2)
					{
						deltaRatio = SmoothedDelta;
					}
					else if (targetIdx == 1)
					{
						deltaRatio = -SmoothedDelta;
					}
					else
					{
						deltaRatio = (CurrentRatio > 0.0f ? -SmoothedDelta : SmoothedDelta);

						if (Abs(deltaRatio) > Abs(CurrentRatio)) // don't overshoot
						{
							deltaRatio = -CurrentRatio;
						}
					}

					// Apply the delta to find the new ratio

					UBOOL loopedAround = FALSE;
					FLOAT newRatio = CurrentRatio + deltaRatio;

					if (newRatio > 1.0f)
					{
						if (CurrentRatio == 1.0f)
						{
							if (Owner->bBlockConstrainedMovement || noInput) // don't start another move - force going to idle
							{
								newRatio = 0.0f;
							}
							else
							{
								newRatio = deltaRatio;
								loopedAround = TRUE;
							}
						}
						else
						{
							newRatio = 1.0f;
						}
					}
					else if (newRatio < -1.0f)
					{
						if (CurrentRatio == -1.0f)
						{
							if (Owner->bBlockConstrainedMovement || noInput) // don't start another move - force going to idle
							{
								newRatio = 0.0f;
							}
							else
							{
								newRatio = deltaRatio;
								loopedAround = TRUE;
							}
						}
						else
						{
							newRatio = -1.0f;
						}
					}

					CurrentRatio = newRatio;

					// Apply the ratio to the target animation

					INT animIdx = appIsNearlyZero(CurrentRatio) ? 0 : (CurrentRatio > 0.0f ? 2 : 1);

					if (animIdx != 0)
					{
						FLOAT animPct = Abs(CurrentRatio);

						if (animPct == 1.0f)
						{
							animPct -= KINDA_SMALL_NUMBERF;
						}

						UAnimNodeSequence* animNode = Cast<UAnimNodeSequence>(Children(animIdx).Anim);
						if (animNode && animNode->AnimSeq)
						{
							FLOAT newTime = animPct * animNode->AnimSeq->SequenceLength;

							if (animIdx != ActiveChildIndex)
							{
								SetActiveChild(animIdx, 0.1f);
								animNode->CurrentTime = newTime;
								animNode->PreviousTime = 0.0f;
							}
							else
							{
								animNode->PreviousTime = (loopedAround ? 0.0f : animNode->CurrentTime);
								animNode->CurrentTime = newTime;
								animNode->IssueNotifies(deltaTime);
							}
							animNode->ConditionalClearCachedData();
						}
					}
					else if (ActiveChildIndex != 0)
					{
						SetActiveChild(0, 0.1f);
					}
				}
			}
		}		
	}

	Super::TickAnim(deltaTime);
}

void UOLAnimConstrainedMovement::GetBoneAtoms(FBoneAtomArray& Atoms, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion, FCurveKeyArray& CurveKeys)
{
	Super::GetBoneAtoms(Atoms, DesiredBones, RootMotionDelta, bHasRootMotion, CurveKeys);

	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if ((bAutomaticMotion && bNoAutomaticRootMotion) || (Owner && Owner->bKillConstrainedMovement))
	{
		RootMotionDelta.SetIdentity();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimMultiCycleConstrainedMovement
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimMultiCycleConstrainedMovement::OnBecomeRelevant()
{
	SetActiveChild(0, 0.0f);
	CurrentRatio = 0.0f;
	CurrentIdx = 0;

	for (INT i = 1; i < Children.Num(); i++)
	{
		UAnimNodeSequence* nodeSeq = Cast<UAnimNodeSequence>(Children(i).Anim);
		if (nodeSeq && nodeSeq->AnimSeq == NULL)
		{
			nodeSeq->SetAnim(nodeSeq->AnimSeqName);
		}
	}

	SmoothedDelta = 0.0f;
}

void UOLAnimMultiCycleConstrainedMovement::TickAnim(FLOAT deltaTime)
{
	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (Owner)
	{
		const FVector& effectiveDirection = Owner->DesiredMoveDirection;
		UBOOL noInput = effectiveDirection.IsNearlyZero();
		FLOAT inputScaling = Owner->MovementSpeedModifier;

		if (bNetControlled)
		{
			// Remote dummy: use NetSmoothedDelta directly, skip local input/smoothing logic.
			SmoothedDelta = NetSmoothedDelta;
		}
		else
		{
			UBOOL inMovingState = CurrentIdx > 1;
			INT targetDirection = 0; // 0: no move, 1: up, -1: down

			if (noInput)
			{
				if (bCompleteCyclesOnly && inMovingState)
				{
					if (bCommitMoves)
					{
						targetDirection = (SmoothedDelta > 0.0f) ? 1 : -1;
					}
					else
					{
						if (CurrentRatio > 0.5f)
						{
							targetDirection = 1;
						}
						else if (CurrentRatio > 0.0f)
						{
							targetDirection = -1;
						}
						else if (CurrentRatio > -0.5f)
						{
							targetDirection = 1;
						}
						else
						{
							targetDirection = -1;
						}
					}
				}
			}
			else
			{
				if (Owner->bBlockConstrainedMovement && !inMovingState)
				{
					targetDirection = 0;
				}
				else if (bUpDown)
				{
					targetDirection = (effectiveDirection.Z > 0.0f) ? 1 : -1;
				}
				else
				{
					targetDirection = ((effectiveDirection | Owner->Rotation.Right()) > 0.0f) ? 1 : -1;
				}
			}

			UAnimNodeSequence* baseNodeSeq = Cast<UAnimNodeSequence>(Children(2).Anim); // we take A to B as reference (and assume B to A is same length)
			if (!baseNodeSeq || !baseNodeSeq->AnimSeq)
			{
				return;
			}

			FLOAT maxDeltaRatio = inputScaling * (deltaTime / baseNodeSeq->AnimSeq->SequenceLength);

			if (!inMovingState && (targetDirection == 0 || noInput))
			{
				SmoothedDelta = 0.0f;
			}
			else
			{
				FLOAT targetDelta = targetDirection*maxDeltaRatio;
				TWEAKABLE FLOAT SmoothingCoeff = 0.999f;
				SmoothedDelta = Utils::Approach(SmoothedDelta, targetDelta, SmoothingCoeff, deltaTime);
			}
		}

		FLOAT newRatio = CurrentRatio + SmoothedDelta;

		// normalize
		{
			if (newRatio < -1.0f) newRatio += 2.0f;
			else if (newRatio > 1.0f) newRatio -= 2.0f;
		}

		// Force passing through the intersections (for root motion to work correctly)
		if (!appIsNearlyZero(CurrentRatio, KINDA_SMALL_NUMBERF) && !appIsNearlyZero(CurrentRatio - 1.0f, KINDA_SMALL_NUMBERF) && Sgn(newRatio) != Sgn(CurrentRatio))
		{
			if (Abs(CurrentRatio) < 0.5f)
			{
				newRatio = 0.0f;
			}
			else
			{
				newRatio = 1.0f;
			}
		}

		CurrentRatio = newRatio;

		INT animIdx = 0;

		if (appIsNearlyZero(CurrentRatio, KINDA_SMALL_NUMBERF))
		{
			animIdx = 0; // idle a
		}
		else if (appIsNearlyZero(Abs(CurrentRatio) - 1.0f, KINDA_SMALL_NUMBERF))
		{
			animIdx = 1; // idle b
		}
		else if (CurrentRatio > 0.0f)
		{ 
			animIdx = 2; // a to b
		}
		else
		{
			animIdx = 3; // b to a
		}

		CurrentIdx = animIdx;

		if (animIdx > 1) // moving, we need to set the pct
		{
			FLOAT animPct = 0.0f;
			
			if (CurrentRatio < 0.0f)
			{
				animPct = CurrentRatio + 1.0f;
			}
			else
			{
				animPct = CurrentRatio;
			}

			if (animPct == 1.0f)
			{
				animPct -= KINDA_SMALL_NUMBERF; // 100% won't set the time properly
			}

			UAnimNodeSequence* animNode = Cast<UAnimNodeSequence>(Children(animIdx).Anim);
			if (animNode && animNode->AnimSeq)
			{
				FLOAT newTime = animPct * animNode->AnimSeq->SequenceLength;

				if (animIdx != ActiveChildIndex)
				{
					SetActiveChild(animIdx, 0.0f);
					animNode->CurrentTime = newTime;
					animNode->PreviousTime = newTime;
				}
				else
				{
					animNode->PreviousTime = animNode->CurrentTime;
					animNode->CurrentTime = newTime;
				}

				animNode->ConditionalClearCachedData();
				animNode->IssueNotifies(deltaTime);
			}
		}
		else if (ActiveChildIndex != animIdx)
		{
			SetActiveChild(animIdx, 0.0f);
		}
	}

	Super::TickAnim(deltaTime);
}

UBOOL UOLAnimMultiCycleConstrainedMovement::CloserToA() const
{
	if (CurrentIdx == 0)
	{
		return TRUE; // idle a
	}
	else if (CurrentIdx == 1)
	{
		return FALSE; // idle b
	}
	else if (CurrentIdx == 2)
	{
		return CurrentRatio < 0.5f; // a to b
	}
	else
	{
		return CurrentRatio > 0.5f; // b to a
	}
}

FLOAT UOLAnimMultiCycleConstrainedMovement::GetCurrentPositionRatio() const
{
	if (CurrentIdx >= 2)
	{
		UAnimNodeSequence* animNode = Cast<UAnimNodeSequence>(Children(CurrentIdx).Anim);
		if (animNode && animNode->AnimSeq)
		{
			FAnimSetMeshLinkup* AnimLinkup = &animNode->AnimSeq->GetAnimSet()->LinkupCache(animNode->AnimLinkupIndex);
			const INT TrackIndex = AnimLinkup->BoneToTrackTable(0);

			FBoneAtom currentFrameAtom;
			FBoneAtom lastFrameAtom;
			animNode->AnimSeq->GetBoneAtom(currentFrameAtom, TrackIndex, animNode->CurrentTime, FALSE, SkelComponent->bUseRawData);
			animNode->AnimSeq->GetBoneAtom(lastFrameAtom, TrackIndex, animNode->AnimSeq->SequenceLength, FALSE, SkelComponent->bUseRawData);

			return currentFrameAtom.GetTranslation().Size() / lastFrameAtom.GetTranslation().Size();
		}
	}

	return 0.0f;
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimDoorInteraction
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimDoorInteraction::OnBecomeRelevant()
{
	CurrentRatio = Clamp(InitialRatio, 0.0f, 0.99f);

	for (INT i = 0; i < Children.Num(); i++)
	{
		UAnimNodeSequence* nodeSeq = Cast<UAnimNodeSequence>(Children(i).Anim);
		if (nodeSeq)
		{
			if (nodeSeq->AnimSeq == NULL)
			{
				nodeSeq->SetAnim(nodeSeq->AnimSeqName);			
			}

			check (nodeSeq->AnimSeq);
			FLOAT newTime = CurrentRatio * nodeSeq->AnimSeq->SequenceLength;

			nodeSeq->PreviousTime = newTime;
			nodeSeq->CurrentTime = newTime;
		}
	}
	
	SmoothedDelta = 0.0f;	
}

void UOLAnimDoorInteraction::TickAnim(FLOAT deltaTime)
{
	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (Owner && Owner->ActiveDoor)
	{
		const FVector& effectiveDirection = Owner->DesiredMoveDirection;
		UBOOL noInput = effectiveDirection.IsNearlyZero();
		FLOAT inputScaling = Owner->MovementSpeedModifier;

		INT targetDirection = 0; // 0: no move, 1: forward, -1: backward

		if (!noInput)
		{
			targetDirection = ((effectiveDirection | Owner->ActiveDoor->GetStaticDirection()) > 0.0f) ? 1 : -1;
		}

		UAnimNodeSequence* animNodeSeq = Cast<UAnimNodeSequence>(Children(ActiveChildIndex).Anim); // we take A to B as reference (and assume B to A is same length)
		if (!animNodeSeq || !animNodeSeq->AnimSeq)
		{
			return;
		}

		FLOAT maxDeltaRatio = PlayRate * inputScaling * deltaTime;
		FLOAT deltaRatio = 0.0f; 

		// Calculate the desired delta for this frame
		
		FLOAT targetDelta = targetDirection*maxDeltaRatio;
		TWEAKABLE FLOAT SmoothingCoeff = 0.999999f;
		SmoothedDelta = Utils::Approach(SmoothedDelta, targetDelta, SmoothingCoeff, PlayRate * deltaTime);
		
		FLOAT newRatio = Clamp(CurrentRatio + SmoothedDelta, 0.0f, MaxRatio > 0.0f ? MaxRatio : 1.0f);
		CurrentRatio = newRatio;

		FLOAT animPct = CurrentRatio;

		if (animPct == 1.0f)
		{
			animPct -= KINDA_SMALL_NUMBERF; // 100% won't set the time properly
		}

		FLOAT newTime = animPct * animNodeSeq->AnimSeq->SequenceLength;

		animNodeSeq->PreviousTime = animNodeSeq->CurrentTime;
		animNodeSeq->CurrentTime = newTime;

		animNodeSeq->ConditionalClearCachedData();
		animNodeSeq->IssueNotifies(deltaTime);
	}

	Super::TickAnim(deltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimPeeking
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimPeeking::SetPeekingType(UBOOL bFromLeft, UBOOL bRounded)
{
	INT childIdx = 0;
	if (bFromLeft)
	{
		childIdx = (bRounded ? IdxLeftSoft : IdxLeftHard);
	}
	else
	{
		childIdx = (bRounded ? IdxRightSoft : IdxRightHard);
	}

	ActiveChildIdx = childIdx;
}

void UOLAnimPeeking::StartPeeking(FLOAT initialRatio)
{
	for (INT i = 0; i < Children.Num(); i++)
	{
		UAnimNodeSequence* nodeSeq = Cast<UAnimNodeSequence>(Children(i).Anim);
		if (nodeSeq)
		{
			if (nodeSeq->AnimSeq == NULL)
			{
				nodeSeq->SetAnim(nodeSeq->AnimSeqName);			
			}

			check (nodeSeq->AnimSeq);			
		}

		if (i == ActiveChildIdx)
		{
			Children(i).Weight = 1.0f;
			
			if (nodeSeq)
			{
				FLOAT animPct = Clamp(initialRatio, 0.0f, 0.999f);	
				FLOAT newTime = animPct * nodeSeq->AnimSeq->SequenceLength;

				nodeSeq->PreviousTime = newTime;
				nodeSeq->CurrentTime = newTime;
				nodeSeq->ConditionalClearCachedData();
			}
		}
		else
		{
			Children(i).Weight = 0.0f;
		}
	}

	CurrentRatio = initialRatio;
	TargetRatio = initialRatio;
}

void UOLAnimPeeking::TickAnim(FLOAT deltaTime)
{
	UAnimNodeSequence* nodeSeq = Cast<UAnimNodeSequence>(Children(ActiveChildIdx).Anim);

	if (nodeSeq && nodeSeq->AnimSeq)
	{
		TWEAKABLE FLOAT ApproachCoeff = 0.999f;
		CurrentRatio = Utils::Approach(CurrentRatio, TargetRatio, ApproachCoeff, deltaTime);

		FLOAT animPct = Clamp(CurrentRatio, 0.0f, 0.999f);	
		FLOAT newTime = animPct * nodeSeq->AnimSeq->SequenceLength;

		nodeSeq->PreviousTime = newTime;
		nodeSeq->CurrentTime = newTime;
		nodeSeq->ConditionalClearCachedData();
	}

	Super::TickAnim(deltaTime);
}

void UOLAnimPeeking::GetHandEffectorMtx(FMatrix& out_HandMtx, const FMatrix& cornerMtx, UBOOL bFromLeft, UBOOL bRounded)
{
	INT childIdx = 0;
	if (bFromLeft)
	{
		childIdx = (bRounded ? IdxLeftSoft : IdxLeftHard);
	}
	else
	{
		childIdx = (bRounded ? IdxRightSoft : IdxRightHard);
	}

	FQuatRotationTranslationMatrix handMtx(CornerToHandQuats(childIdx), CornerToHandVecs(childIdx));
	out_HandMtx = handMtx * cornerMtx;
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimCustomBlend
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UAnimNodeSequence* UOLAnimCustomBlend::GetPrimaryNode() const
{
	UAnimNodeSequence* seqNodeA = Cast<UAnimNodeSequence>(Children(1).Anim);
	UAnimNodeSequence* seqNodeB = Cast<UAnimNodeSequence>(Children(2).Anim);
	UAnimNodeSequence* seqNodeC = Cast<UAnimNodeSequence>(Children(3).Anim);

	if (NumActiveAnims == 1)
	{
		return seqNodeA;
	}
	else if (NumActiveAnims == 2)
	{
		return (BlendWeights[1] > BlendWeights[0]) ? seqNodeB : seqNodeA;
	}
	else
	{
		check (NumActiveAnims == 3);

		if (BlendWeights[0] >= BlendWeights[1] && BlendWeights[0] >= BlendWeights[2])
		{
			return seqNodeA;
		}
		else if (BlendWeights[1] >= BlendWeights[0] && BlendWeights[1] >= BlendWeights[2])
		{
			return seqNodeB;
		}
		else
		{
			return seqNodeC;
		}
	}
}

void UOLAnimCustomBlend::TickAnim(FLOAT deltaSeconds)
{
	if (bActive)
	{
		UAnimNodeSequence* primaryNode = GetPrimaryNode();

		UBOOL bDone = !primaryNode || !primaryNode->AnimSeq || (bBlendingOut && appIsNearlyZero(BlendTimeToGo));
		UBOOL bFinishedPlaying =  appIsNearlyZero(primaryNode->GetTimeLeft());

		UBOOL bLeave = bDone || (bFinishedPlaying && !bKeepLastPose);
		UBOOL bTriggerEarlyOut = !bFinishedPlaying && !bBlendingOut && !bKeepLastPose && BlendOutTime >= 0.f && primaryNode->GetTimeLeft() <= BlendOutTime;
		UBOOL bTriggerLateOut = bFinishedPlaying && bKeepLastPose && !bBlendingOut;
		UBOOL bTriggerEarlyOutOnLeaving = bLeave && !bEarlyAnimEndFired && !bKeepLastPose && BlendOutTime >= 0.f;

		TimeRemaining = primaryNode->GetTimeLeft();

		if (bLeave)
		{
			if (bTriggerEarlyOutOnLeaving)
			{
				bEarlyAnimEndFired = TRUE;

				if (!SkelComponent->bNoAnimNotifies && !bInhibitEndNotifies)
				{
					APawn* ownerPawn = SkelComponent->GetOwner()->GetAPawn();
					if (ownerPawn)
					{
						ownerPawn->OnEarlyAnimEnd(primaryNode);
					}
				}
			}

			// Stop now
			FinishPlaying();
		}
		else if (bTriggerEarlyOut)
		{
			// Send an early anim end notify, and blend out
			StartBlendingOut();
			bBlendingOut = TRUE;
			bEarlyAnimEndFired = TRUE;
						
			if (!SkelComponent->bNoAnimNotifies && !bInhibitEndNotifies)
			{
				APawn* ownerPawn = SkelComponent->GetOwner()->GetAPawn();
				if (ownerPawn)
				{
					ownerPawn->OnEarlyAnimEnd(primaryNode);
				}
			}
		}
		else if (bTriggerLateOut)
		{
			// Keep the last pose for BlendOutTime, but don't actually blend out
			BlendTimeToGo = BlendOutTime;
			bBlendingOut = TRUE;

			if (!SkelComponent->bNoAnimNotifies && !bInhibitEndNotifies)
			{
				SkelComponent->GetOwner()->eventOnAnimEnd(primaryNode, deltaSeconds, 0.f);
			}
		}
	}
	else
	{
		TimeRemaining = 0.f;
	}

	Super::TickAnim(deltaSeconds);
}

void UOLAnimCustomBlend::FinishPlaying()
{
	BlendOutTime = 0.0f;
	StartBlendingOut();
	bActive = FALSE;
	bBlendingOut = FALSE;
	BlendTimeToGo = 0.0f;

	for (INT i = 0; i < 3; i++)
	{
		UAnimNodeSequence* animSeq = Cast<UAnimNodeSequence>(Children(i + 1).Anim);

		if (animSeq && animSeq->AnimSeq)
		{
			SkelComponent->AnimAlwaysTickArray.RemoveItem(animSeq);
		}
	}

	if (!bKeepLastPose && !SkelComponent->bNoAnimNotifies && !bInhibitEndNotifies)
	{
		UAnimNodeSequence* primaryNode = GetPrimaryNode();
		SkelComponent->GetOwner()->eventOnAnimEnd(primaryNode, 0.0f, 0.0f);
	}
}

void UOLAnimCustomBlend::StopTickingSequences()
{
	for (INT i = 0; i < 3; i++)
	{
		UAnimNodeSequence* animSeq = Cast<UAnimNodeSequence>(Children(i + 1).Anim);

		if (animSeq && animSeq->AnimSeq)
		{
			SkelComponent->AnimAlwaysTickArray.RemoveItem(animSeq);
		}
	}
}

FLOAT UOLAnimCustomBlend::PlaySingleAnim(FName animName, FLOAT blendInTime, FLOAT blendOutTime, FLOAT rate, FLOAT startRatio)
{
	NumActiveAnims = 1;

	BlendWeights[0] = 1.0f;
	BlendWeights[1] = 0.0f;
	BlendWeights[2] = 0.0f;

	FName animNames[3];
	animNames[0] = animName;
	animNames[1] = NAME_None;
	animNames[2] = NAME_None;

	return PlayCustomBlend_Internal(animNames, blendInTime, blendOutTime, rate, startRatio);
}

FLOAT UOLAnimCustomBlend::PlayCustomBlend(FName animNameA, FName animNameB, FLOAT blendWeightA, FLOAT blendInTime, FLOAT blendOutTime, FLOAT rate, FLOAT startRatio)
{
	NumActiveAnims = 2;

	BlendWeights[0] = Saturate(blendWeightA);
	BlendWeights[1] = Saturate(1.0f - BlendWeights[0]);
	BlendWeights[2] = 0.0f;

	FName animNames[3];
	animNames[0] = animNameA;
	animNames[1] = animNameB;
	animNames[2] = NAME_None;

	return PlayCustomBlend_Internal(animNames, blendInTime, blendOutTime, rate, startRatio);
}

FLOAT UOLAnimCustomBlend::PlayCustomBlend(FName animNameA, FName animNameB, FName animNameC, FLOAT blendWeightA, FLOAT blendWeightB, FLOAT blendInTime, FLOAT blendOutTime, FLOAT rate, FLOAT startRatio)
{
	NumActiveAnims = 3;

	BlendWeights[0] = Saturate(blendWeightA);
	BlendWeights[1] = Saturate(blendWeightB);
	BlendWeights[2] = Saturate(1.0f - (BlendWeights[0] + BlendWeights[1]));

	FName animNames[3];
	animNames[0] = animNameA;
	animNames[1] = animNameB;
	animNames[2] = animNameC;

	return PlayCustomBlend_Internal(animNames, blendInTime, blendOutTime, rate, startRatio);
}

FLOAT UOLAnimCustomBlend::PlayBlendSpace(const FBlendSpaceNode nodes[], INT numNodes, const INT blendSpaces[][3], INT numSpaces, const FVector2D& coord, FLOAT blendInTime, FLOAT blendOutTime, FLOAT rate, FLOAT startRatio)
{
	FVector closestBarycentric(0.0f);
	FLOAT closestNegativeSum = 0.0f;
	INT closestSpace = -1;

	// Find the space (triangle) where the coord resides
	for (INT i = 0; i < numSpaces; i++)
	{
		const FBlendSpaceNode& nodeA = nodes[blendSpaces[i][0]];
		const FBlendSpaceNode& nodeB = nodes[blendSpaces[i][1]];
		const FBlendSpaceNode& nodeC = nodes[blendSpaces[i][2]];

		FVector barycentric = ComputeBaryCentric2D(FVector(coord, 0.0f), FVector(nodeA.Coords, 0.0f), FVector(nodeB.Coords, 0.0f), FVector(nodeC.Coords, 0.0f));

		if (barycentric.X >= 0.0f && barycentric.Y >= 0.0f && barycentric.Z >= 0.0f)
		{
			// Found the right space - play the anims with the corresponding weights

			// Normalize
			FLOAT weightSum = 0.0f;
			for (INT i = 0; i < 3; i++)
			{
				weightSum += barycentric[i];
			}

			barycentric.X /= weightSum;
			barycentric.Y /= weightSum;

			return PlayCustomBlend(nodeA.AnimName, nodeB.AnimName, nodeC.AnimName, barycentric.X, barycentric.Y, blendInTime, blendOutTime, rate, startRatio);
		}

		FLOAT negativeSum = 0.0f;
		for (INT j = 0; j < 3; j++)
		{
			if (barycentric[j] < 0.0f)
			{
				negativeSum += barycentric[j];
			}
		}

		if (closestSpace < 0 || negativeSum > closestNegativeSum)
		{
			closestBarycentric = barycentric;
			closestNegativeSum = negativeSum;
			closestSpace = i;			
		}
	}

	// Outside the blend space proper - Remove the negatives, renormalize and hope for the best
	{
		FLOAT currentSum = 0.0f;
		for (INT i = 0; i < 3; i++)
		{
			if (closestBarycentric[i] < 0.0f)
			{
				closestBarycentric[i] = 0.0f;
			}
			else
			{
				currentSum += closestBarycentric[i];
			}
		}

		closestBarycentric.X /= currentSum;
		closestBarycentric.Y /= currentSum;

		check(closestSpace >= 0);
		const FBlendSpaceNode& nodeA = nodes[blendSpaces[closestSpace][0]];
		const FBlendSpaceNode& nodeB = nodes[blendSpaces[closestSpace][1]];
		const FBlendSpaceNode& nodeC = nodes[blendSpaces[closestSpace][2]];
	
		return PlayCustomBlend(nodeA.AnimName, nodeB.AnimName, nodeC.AnimName, closestBarycentric.X, closestBarycentric.Y, blendInTime, blendOutTime, rate, startRatio);
	}
}

FLOAT UOLAnimCustomBlend::PlayCustomBlend_Internal(FName animNames[3], FLOAT blendInTime, FLOAT blendOutTime, FLOAT rate, FLOAT startRatio)
{
	// BlendWeights and NumActiveAnims are expected to be set before calling this function

	UAnimNodeSequence* seqNodes[3] = { NULL };
	FLOAT weightedTime = 0.0f;
	
	for (INT i = 0; i < NumActiveAnims; i++)
	{
		seqNodes[i] = Cast<UAnimNodeSequence>(Children(i + 1).Anim);
		check(seqNodes[i]);
		check(animNames[i] != NAME_None);

		seqNodes[i]->SetAnim(animNames[i]);

		if (seqNodes[i]->AnimSeq == NULL)
		{
			warnf(TEXT("PlayCustomBlend %s, CustomChildIndex: %i => Animation Not Found"), *animNames[i].ToString(), i + 1);
			return 0.f;
		}

		check(BlendWeights[i] >= 0.0f && BlendWeights[i] <= 1.0f);
		weightedTime += BlendWeights[i] * seqNodes[i]->AnimSeq->SequenceLength;
	}
	
	PlaybackTime = weightedTime / rate;

	SkelComponent->AnimAlwaysTickArray.AddUniqueItem(this);

	for (INT i = 0; i < NumActiveAnims; i++)
	{
		seqNodes[i]->PlayAnim(FALSE, seqNodes[i]->AnimSeq->SequenceLength / PlaybackTime, seqNodes[i]->AnimSeq->SequenceLength * startRatio);
		seqNodes[i]->bCauseActorAnimEnd = FALSE; // we manually send the anim end event
		SkelComponent->AnimAlwaysTickArray.AddUniqueItem(seqNodes[i]);
	}
	
	PlaybackTime *= (1.0f - startRatio);
	
	TargetWeight(0) = 0.0f;
	TargetWeight(1) = BlendWeights[0];
	TargetWeight(2) = BlendWeights[1];
	TargetWeight(3) = BlendWeights[2];
	BlendTimeToGo = blendInTime;

	// if we have 0.f blend time then just set the values 
	if( BlendTimeToGo <= 0.f )
	{
		BlendTimeToGo = 0.f; 

		const INT ChildNum = Children.Num();
		for(INT i=0; i<ChildNum; i++)
		{
			Children(i).Weight = TargetWeight(i);
		}
	}

	bActive = TRUE;
	bBlendingOut = FALSE;	
	BlendOutTime = blendOutTime;
	bKeepLastPose = FALSE;
	bInhibitEndNotifies = FALSE;
	bEarlyAnimEndFired = FALSE;

	TimeRemaining = PlaybackTime;

	return PlaybackTime;
}

void UOLAnimCustomBlend::StartBlendingOut()
{
	BlendTimeToGo = BlendOutTime;
	TargetWeight(0) = 1.0f;
	TargetWeight(1) = 0.0f;
	TargetWeight(2) = 0.0f;
	TargetWeight(3) = 0.0f;

	// if no blend out time, set target weight immediately
	if (appIsNearlyZero(BlendOutTime, KINDA_SMALL_NUMBERF))
	{
		BlendTimeToGo = 0.0f;
		const INT ChildNum = Children.Num();
		for(INT i=0; i<ChildNum; i++)
		{
			Children(i).Weight = TargetWeight(i);
		}
	}
}

FBoneAtom UOLAnimCustomBlend::GetTotalRootMotion() const
{
	FBoneAtom totalMotion = FBoneAtom::Identity;

	if (bActive)
	{
		UAnimNodeSequence* seqNodeA = Cast<UAnimNodeSequence>(Children(1).Anim);
		UAnimNodeSequence* seqNodeB = Cast<UAnimNodeSequence>(Children(2).Anim);
		UAnimNodeSequence* seqNodeC = Cast<UAnimNodeSequence>(Children(3).Anim);

		if (NumActiveAnims == 1 || appIsNearlyEqual(BlendWeights[0], 1.0f, KINDA_SMALL_NUMBERF))
		{
			totalMotion = seqNodeA->GetTotalRootMotion();
		}
		else if (NumActiveAnims == 2 || appIsNearlyZero(BlendWeights[2], KINDA_SMALL_NUMBERF))
		{
			totalMotion.Blend(seqNodeB->GetTotalRootMotion(), seqNodeA->GetTotalRootMotion(), BlendWeights[0]);
		}
		else if (NumActiveAnims == 3)
		{			
			FLOAT blendAlphaBC = BlendWeights[1] / (BlendWeights[1] + BlendWeights[2]);
			FBoneAtom motionBC = FBoneAtom::Identity;
			motionBC.Blend(seqNodeC->GetTotalRootMotion(), seqNodeB->GetTotalRootMotion(), blendAlphaBC);
			totalMotion.Blend(motionBC, seqNodeA->GetTotalRootMotion(), BlendWeights[0]);
		}
	}

	return totalMotion;
}

FBoneAtom UOLAnimCustomBlend::GetCameraFinalRelativePosition() const
{
	FBoneAtom relPosition = FBoneAtom::Identity;

	const FName camBoneName = FName(TEXT("Hero-Camera"));

	if (bActive)
	{
		UAnimNodeSequence* seqNodeA = Cast<UAnimNodeSequence>(Children(1).Anim);
		UAnimNodeSequence* seqNodeB = Cast<UAnimNodeSequence>(Children(2).Anim);
		UAnimNodeSequence* seqNodeC = Cast<UAnimNodeSequence>(Children(3).Anim);

		if (NumActiveAnims == 1 || appIsNearlyEqual(BlendWeights[0], 1.0f, KINDA_SMALL_NUMBERF))
		{
			relPosition = seqNodeA->GetFinalBoneRelativePosition(camBoneName);
		}
		else if (NumActiveAnims == 2 || appIsNearlyZero(BlendWeights[2], KINDA_SMALL_NUMBERF))
		{
			relPosition.Blend(seqNodeB->GetFinalBoneRelativePosition(camBoneName), seqNodeA->GetFinalBoneRelativePosition(camBoneName), BlendWeights[0]);
		}
		else if (NumActiveAnims == 3)
		{			
			FLOAT blendAlphaBC = BlendWeights[1] / (BlendWeights[1] + BlendWeights[2]);
			FBoneAtom motionBC = FBoneAtom::Identity;
			motionBC.Blend(seqNodeC->GetFinalBoneRelativePosition(camBoneName), seqNodeB->GetFinalBoneRelativePosition(camBoneName), blendAlphaBC);
			relPosition.Blend(motionBC, seqNodeA->GetFinalBoneRelativePosition(camBoneName), BlendWeights[0]);
		}
	}

	FBoneAtom rootMotion = GetTotalRootMotion();
	FBoneAtom finalRelPos = relPosition * rootMotion;
	return finalRelPos;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimCameraSpace
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimCameraSpace::InitAnim(USkeletalMeshComponent* MeshComp, UAnimNodeBlendBase* Parent)
{
	Super::InitAnim(MeshComp, Parent);
	ChildSlot = Cast<UAnimNodeSlot>(Children(1).Anim);
	ChildBlend = Cast<UOLAnimCustomBlend>(Children(1).Anim);
	CameraBoneIdx = MeshComp->MatchRefBone(Utils::GetCameraBoneName());

	StartBoneIndices.Empty();
	for (INT i = 0; i < BranchStartBoneName.Num(); i++)
	{
		StartBoneIndices.AddItem(MeshComp->MatchRefBone(BranchStartBoneName(i)));
	}

	// Build the parent chain
	ParentChain.Empty();
	ParentChain.AddItem(CameraBoneIdx);

	TArray<FMeshBone>& RefSkel = MeshComp->SkeletalMesh->RefSkeleton;
	INT parentIdx = RefSkel(CameraBoneIdx).ParentIndex;
	while (parentIdx > 0)
	{
		ParentChain.AddUniqueItem(parentIdx);
		parentIdx = RefSkel(parentIdx).ParentIndex;
	}

	for (INT i = 0; i < StartBoneIndices.Num(); i++)
	{
		INT childBoneIdx = StartBoneIndices(i);
		ParentChain.InsertItem(childBoneIdx, 0);

		INT insertIdx = 1;
		parentIdx = RefSkel(childBoneIdx).ParentIndex;
		while (parentIdx > 0)
		{
			ParentChain.InsertItem(parentIdx, insertIdx);
			insertIdx++;
			parentIdx = RefSkel(parentIdx).ParentIndex;
		}		
	}

	ParentChain.AddItem(0);
}

void UOLAnimCameraSpace::BuildWeightList()
{
	Super::BuildWeightList();
}

void UOLAnimCameraSpace::TickAnim(FLOAT deltaTime)
{
	if (ChildSlot)
	{
		UAnimNodeSequence* seqNode = ChildSlot->GetCustomAnimNodeSeq();
		UBOOL bBlendingOut = seqNode && seqNode->bBlendingOut;

		if (!bBlendingOut && (ChildSlot->bIsPlayingCustomAnim || ChildSlot->BlendTimeToGo > KINDA_SMALL_NUMBERF))
		{
			if (Child2WeightTarget < 0.5f)
			{
				SetBlendTarget(1.f, ChildSlot->BlendTimeToGo);
			}
		}
		else
		{
			if (Child2WeightTarget > 0.5f)
			{
				SetBlendTarget(0.f, ChildSlot->BlendTimeToGo);
			}
		}
	}
	else if (ChildBlend)
	{
		if (!ChildBlend->bBlendingOut && (ChildBlend->bActive || ChildBlend->BlendTimeToGo > KINDA_SMALL_NUMBERF))
		{
			if (Child2WeightTarget < 0.5f)
			{
				SetBlendTarget(1.f, ChildBlend->BlendTimeToGo);
			}
		}
		else
		{
			if (Child2WeightTarget > 0.5f)
			{
				SetBlendTarget(0.f, ChildBlend->BlendTimeToGo);
			}
		}
	}

	Super::TickAnim(deltaTime);
}

void UOLAnimCameraSpace::GetBoneAtoms(FBoneAtomArray& Atoms, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion, FCurveKeyArray& CurveKeys)
{	
	// See if results are cached.
	if( GetCachedResults(Atoms, RootMotionDelta, bHasRootMotion, CurveKeys, DesiredBones.Num()) )
	{
		return;
	}

	// If drawing all from Children(0), no need to look at Children(1). Pass Atoms array directly to Children(0).
	if( Children(1).Weight <= ZERO_ANIMWEIGHT_THRESH )
	{
		if( Children(0).Anim )
		{
			Children(0).Anim->GetBoneAtoms(Atoms, DesiredBones, RootMotionDelta, bHasRootMotion, CurveKeys);
		}
		else
		{
			RootMotionDelta.SetIdentity();
			bHasRootMotion	= 0;
			FillWithRefPose(Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
		}

		// Pass-through, no caching needed.
		return;
	}

	TArray<FMeshBone>& RefSkel = SkelComponent->SkeletalMesh->RefSkeleton;
	const INT NumAtoms = RefSkel.Num();

	FBoneAtomArray Child1Atoms, Child2Atoms;

	FMemMark Mark(GMainThreadMemStack);

	// Get bone atoms from each child (if no child - use ref pose).
	Child1Atoms.Add(NumAtoms);
	FBoneAtom	Child1RMD				= FBoneAtom::Identity;
	INT  		bChild1HasRootMotion	= FALSE;
	if( Children(0).Anim )
	{
		Children(0).Anim->GetBoneAtoms(Child1Atoms, DesiredBones, Child1RMD, bChild1HasRootMotion, CurveKeys);
	}
	else
	{
		FillWithRefPose(Child1Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
	}

	// Get only the necessary bones from child2. The ones that have a Child2PerBoneWeight(BoneIndex) > 0
	Child2Atoms.Add(NumAtoms);
	FBoneAtom Child2RMD = FBoneAtom::Identity;
	INT bChild2HasRootMotion = FALSE;

	if( Children(1).Anim )
	{
		Children(1).Anim->GetBoneAtoms(Child2Atoms, DesiredBones, Child2RMD, bChild2HasRootMotion, CurveKeys);
	}
	else
	{
		FillWithRefPose(Child2Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
	}

	// Not bothering with root motion on child 2
	bHasRootMotion = bChild1HasRootMotion;
	if( bChild1HasRootMotion )
	{
		RootMotionDelta = Child1RMD;
	}

	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (!Owner || Owner->ShadowProxy == SkelComponent || !Owner->OLPC)
	{
		// Shadow proxy and remote dummies (no OLPC) play it straight - not in camera space
		for (INT i=0; i < DesiredBones.Num(); i++)
		{
			INT BoneIndex = DesiredBones(i);
			const FLOAT Child2BoneWeight	= Children(1).Weight * Child2PerBoneWeight(BoneIndex);

			Atoms(BoneIndex).Blend(Child1Atoms(BoneIndex), Child2Atoms(BoneIndex), Child2BoneWeight);
		}
	}
	else
	{		
		// Compute component space for the start and camera bones
		FBoneAtomArray compSpace1; 
		FBoneAtomArray compSpace2; 
		{
			compSpace1.AddZeroed(SkelComponent->SkeletalMesh->RefSkeleton.Num());
			compSpace2.AddZeroed(SkelComponent->SkeletalMesh->RefSkeleton.Num());

			// Walk the chain to compute comp space
			for (INT i = ParentChain.Num() - 1; i >= 0; i--)
			{
				INT BoneIndex = ParentChain(i);
				INT ParentIndex = RefSkel(BoneIndex).ParentIndex;

				compSpace1(BoneIndex) = Child1Atoms(BoneIndex);
				compSpace2(BoneIndex) = Child2Atoms(BoneIndex);

				if (BoneIndex > 0)
				{
					compSpace1(BoneIndex) *= compSpace1(ParentIndex);
					compSpace2(BoneIndex) *= compSpace2(ParentIndex);
				}
			}
		}

		for (INT i=0; i < DesiredBones.Num(); i++)
		{
			INT BoneIndex = DesiredBones(i);
			const FLOAT Child2BoneWeight	= Children(1).Weight * Child2PerBoneWeight(BoneIndex);

			UBOOL bIsStartBone = FALSE;
			for (INT i = 0; i < StartBoneIndices.Num(); i++)
			{
				if (BoneIndex == StartBoneIndices(i))
				{
					bIsStartBone = TRUE;
					break;
				}
			}

			if (bIsStartBone)
			{
				// The idea here is to reapply the component-space offset from the reference pose between camera and start bones,
				// so that the chain after the start bone effectively plays in local camera space

				FBoneAtom newAtomCS = compSpace2(BoneIndex);
				FBoneAtom currentCameraCS = compSpace2(CameraBoneIdx);
				FBoneAtom correctionOffset = newAtomCS * currentCameraCS.Inverse();
				FBoneAtom desiredAtomCS = correctionOffset * Owner->CameraCompSpace;	// desired component-space transform of the start bone
				INT parentIdx = RefSkel(BoneIndex).ParentIndex;
				FBoneAtom& parentAtom1 = compSpace1(parentIdx);
				FBoneAtom& parentAtom2 = compSpace2(parentIdx);			
				desiredAtomCS.NormalizeRotation();
				parentAtom1.NormalizeRotation();
				parentAtom2.NormalizeRotation();

				FLOAT parentWeight2 = Child2PerBoneWeight(parentIdx);
				FVector effectiveParentPosCS = Lerp(parentAtom1.GetTranslation(), parentAtom2.GetTranslation(), parentWeight2);
				FQuat effectiveParentRotCS = LerpQuat(parentAtom1.GetRotation(), parentAtom2.GetRotation(), parentWeight2);

				FBoneAtom effectiveParentCS(effectiveParentRotCS, effectiveParentPosCS);
				FBoneAtom desiredAtomLS = desiredAtomCS * effectiveParentCS.Inverse(); // transform this offset back into local space
			
				Atoms(BoneIndex).Blend(Child1Atoms(BoneIndex), desiredAtomLS, Child2BoneWeight);
			}
			else
			{
				Atoms(BoneIndex).Blend(Child1Atoms(BoneIndex), Child2Atoms(BoneIndex), Child2BoneWeight);
			}
		}
	}

	Mark.Pop();

	SaveCachedResults(Atoms, RootMotionDelta, bHasRootMotion, CurveKeys, DesiredBones.Num());
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimBlendByEnemyMode
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimBlendByEnemyMode::TickAnim(FLOAT deltaTime)
{
	AOLEnemyPawn* Owner = SkelComponent ? Cast<AOLEnemyPawn>(SkelComponent->GetOwner()) : NULL;

	if (Owner)
	{
		INT desiredIdx = Owner->EnemyMode;

		if (ActiveChildIndex != desiredIdx)
		{
			SetActiveChild(desiredIdx, 0.25f);
		}
	}

	Super::TickAnim(deltaTime);
}

void UOLAnimBlendByEnemyMode::OnBecomeRelevant()
{
	AOLEnemyPawn* Owner = SkelComponent ? Cast<AOLEnemyPawn>(SkelComponent->GetOwner()) : NULL;

	// Force setting the right child when becoming relevant
	if (Owner)
	{
		INT desiredIdx = Owner->EnemyMode;

		if (ActiveChildIndex != desiredIdx)
		{
			SetActiveChild( desiredIdx, 0.0f );			
		}
	}

	Super::OnBecomeRelevant();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimNodeDelayed
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimNodeDelayed::OnBecomeRelevant()
{
	ElapsedTime = 0.0f;
	SetActiveChild(0, 0.0f);
}

void UOLAnimNodeDelayed::TickAnim(FLOAT deltaTime)
{
	FLOAT prevTime = ElapsedTime;
	ElapsedTime += deltaTime;

	if (prevTime < TimeDelay && ElapsedTime >= TimeDelay)
	{
		SetActiveChild(1, BlendTime);
	}

	Super::TickAnim(deltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimBlendByEnemyEnvironment
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimBlendByEnemyEnvironment::TickAnim(FLOAT deltaTime)
{
	AOLEnemyPawn* Owner = SkelComponent ? Cast<AOLEnemyPawn>(SkelComponent->GetOwner()) : NULL;

	if (Owner)
	{
		INT desiredIdx = Owner->Bot ? Owner->Bot->CurrentEnvironment : Owner->NetEnvironment;

		if (ActiveChildIndex != desiredIdx)
		{
			SetActiveChild(desiredIdx, 0.25f);
		}
	}

	Super::TickAnim(deltaTime);
}

void UOLAnimBlendByEnemyEnvironment::OnBecomeRelevant()
{
	AOLEnemyPawn* Owner = SkelComponent ? Cast<AOLEnemyPawn>(SkelComponent->GetOwner()) : NULL;

	// Force setting the right child when becoming relevant
	if (Owner)
	{
		INT desiredIdx = Owner->Bot ? Owner->Bot->CurrentEnvironment : Owner->NetEnvironment;

		if (ActiveChildIndex != desiredIdx)
		{
			SetActiveChild( desiredIdx, 0.0f );
		}
	}

	Super::OnBecomeRelevant();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimBlendBySwarmForm
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimBlendBySwarmForm::OnBecomeRelevant()
{
	AOLEnemyNanoCloud* Owner = SkelComponent ? Cast<AOLEnemyNanoCloud>(SkelComponent->GetOwner()) : NULL;

	if (Owner)
	{
		SetActiveChild( IdxSwarm, 0.0f ); // Swarm by default if a nano cloud
	}
	else
	{
		SetActiveChild( IdxHuman, 0.0f ); // Human by default otherwise
	}

	Super::OnBecomeRelevant();
}

void UOLAnimBlendBySwarmForm::TickAnim(FLOAT deltaTime)
{
	AOLEnemyNanoCloud* Owner = SkelComponent ? Cast<AOLEnemyNanoCloud>(SkelComponent->GetOwner()) : NULL;

	if (Owner)
	{
		INT desiredIdx = 0;
		
		switch ((NanoCloudForm)Owner->CurrentForm)
		{
		case NCF_Swarm:
			desiredIdx = IdxSwarm;
			break;
		case NCF_Human:
			desiredIdx = IdxHuman;
			break;
		case NCF_MorphingToSwarm:
			desiredIdx = IdxMorphingToSwarm;
			break;
		case NCF_MorphingToHuman:
			desiredIdx = IdxMorphingToHuman;
			break;
		}

		if (ActiveChildIndex != desiredIdx)
		{
			SetActiveChild(desiredIdx, 0.25f);
			if (desiredIdx == IdxMorphingToSwarm || desiredIdx == IdxMorphingToHuman)
			{
				Children(desiredIdx).Anim->PlayAnim(FALSE, 1.0f / Owner->MorphMultiplier);
			}
		}		
	}

	Super::TickAnim(deltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimBlendDirectional
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimBlendDirectional::TickAnim(FLOAT deltaTime)
{
	FLOAT dirAngleDegs = 0.0f;
	FLOAT ForwardPct = 1.0f;
	FLOAT LeftPct = 0.0f;

	FVector Velocity = FVector(0.f);
	FRotator Rotation = FRotator(EC_EventParm);
	AActor* actor = SkelComponent->GetOwner(); 
	if (actor)
	{
		Velocity = actor->Velocity;
		Rotation = actor->Rotation;
	}
	else
	{
		Velocity = ExpectedPawnSpeed * FVector(1.0f, 0.f, 0.f);
		Rotation = FRotator(0, SliderDirAngle * RAD_TO_UNR, 0);
	}

	AOLEnemyPawn* enemyPawn = Cast<AOLEnemyPawn>(actor);
	if (enemyPawn && enemyPawn->Turning.bActive)
	{
		dirAngleDegs = 0.0f;
		ForwardPct = 1.0f;
		LeftPct = 0.0f;
	}
	else if (!Velocity.IsNearlyZero())
	{
		FVector VelDir = Velocity;
		VelDir.Z = 0.0f;
		VelDir = VelDir.SafeNormal();

		FVector LookDir = Rotation.Vector();
		LookDir.Z = 0.f;
		LookDir = LookDir.SafeNormal();
	
		FVector LeftDir = LookDir ^ FVector(0.f,0.f,1.f);
		LeftDir = LeftDir.SafeNormal();
	
		ForwardPct = (LookDir | VelDir);
		LeftPct = (LeftDir | VelDir);
	
		dirAngleDegs = RAD_TO_DEG * appAcos(ForwardPct);
		if( LeftPct > 0.f )
		{
			dirAngleDegs *= -1.f;
		}
	}

	FLOAT targetWeights[7] = {0.0f};
	TWEAKABLE FLOAT Hysteresis = 15.0f;
	UBOOL bFullSideWeight = FALSE;
	
	UAnimNodeSequence* reverseLeftAnimSeq = (Children.Num() > IdxReverseLeft) ? Cast<UAnimNodeSequence>(Children(IdxReverseLeft).Anim) : NULL;
	UAnimNodeSequence* reverseRightAnimSeq = (Children.Num() > IdxReverseRight) ? Cast<UAnimNodeSequence>(Children(IdxReverseRight).Anim) : NULL;
	UBOOL bUseFullAnimForReverseSide = reverseLeftAnimSeq && reverseRightAnimSeq;

	if (bUseReversedSideForBackwards)
	{
		if (IsBetween(dirAngleDegs, -95.0f, 95.0f))
		{
			bReversingBackward = FALSE;
		}
		else if ((dirAngleDegs < (-95.0f - Hysteresis)) || (dirAngleDegs > (95.0f + Hysteresis)))
		{
			bReversingBackward = TRUE;
		}
		else
		{
			// hysterysis zone - keep whatever current value
			bFullSideWeight = !bReversingBackward;
		}
	}

	targetWeights[IdxReverseLeft] = 0.f;
	targetWeights[IdxReverseRight] = 0.0f;
	
	// Work out target weights based on DirAngle.
	if (dirAngleDegs < -90.0f) // Back and left
	{
		if (bReversingBackward && bUseFullAnimForReverseSide)
		{
			targetWeights[IdxReverseRight] = (dirAngleDegs/90.0f) + 2.f;

			targetWeights[IdxRight] = 0.0f;
			targetWeights[IdxLeft] = 0.0f;

			targetWeights[IdxForward] = 0.f;
			targetWeights[IdxBackward] = 1.f - targetWeights[IdxReverseRight];
		}
		else if (bReversingBackward)
		{
			targetWeights[IdxRight] = (dirAngleDegs/90.0f) + 2.f;
			targetWeights[IdxLeft] = 0.f;

			targetWeights[IdxForward] = 0.f;
			targetWeights[IdxBackward] = 1.f - targetWeights[IdxRight];
		}
		else if (bFullSideWeight)
		{
			targetWeights[IdxLeft] = 1.0f;
			targetWeights[IdxRight] = 0.f;

			targetWeights[IdxForward] = 0.f;
			targetWeights[IdxBackward] = 0.0f;
		}
		else
		{
			targetWeights[IdxLeft] = (dirAngleDegs/90.0f) + 2.f;
			targetWeights[IdxRight] = 0.f;
	
			targetWeights[IdxForward] = 0.f;
			targetWeights[IdxBackward] = 1.f - targetWeights[IdxLeft];
		}
	}
	else if (dirAngleDegs < 0.0f) // Forward and left
	{
		targetWeights[IdxLeft] = -dirAngleDegs/90.0f;
		targetWeights[IdxRight] = 0.f;
	
		targetWeights[IdxForward] = 1.f - targetWeights[IdxLeft];
		targetWeights[IdxBackward] = 0.f;
	}
	else if (dirAngleDegs < 90.0f) // Forward and right
	{
		targetWeights[IdxLeft] = 0.f;
		targetWeights[IdxRight] = dirAngleDegs/90.0f;
	
		targetWeights[IdxForward] = 1.f - targetWeights[IdxRight];
		targetWeights[IdxBackward] = 0.f;
	}
	else // Back and right
	{
		if (bReversingBackward && bUseFullAnimForReverseSide)
		{
			targetWeights[IdxReverseLeft] = (-dirAngleDegs/90.0f) + 2.f;

			targetWeights[IdxRight] = 0.0f;
			targetWeights[IdxLeft] = 0.0f;

			targetWeights[IdxForward] = 0.f;
			targetWeights[IdxBackward] = 1.f - targetWeights[IdxReverseLeft];
		}
		else if (bReversingBackward)
		{
			targetWeights[IdxRight] = 0.f;
			targetWeights[IdxLeft] = (-dirAngleDegs/90.0f) + 2.f;
	
			targetWeights[IdxForward] = 0.f;
			targetWeights[IdxBackward] = 1.f - targetWeights[IdxLeft];
		}
		else if (bFullSideWeight)
		{
			targetWeights[IdxRight] = 1.0f;
			targetWeights[IdxLeft] = 0.f;

			targetWeights[IdxForward] = 0.f;
			targetWeights[IdxBackward] = 0.0f;
		}
		else
		{
			targetWeights[IdxLeft] = 0.f;
			targetWeights[IdxRight] = (-dirAngleDegs/90.0f) + 2.f;

			targetWeights[IdxForward] = 0.f;
			targetWeights[IdxBackward] = 1.f - targetWeights[IdxRight];
		}
	}

	FLOAT speedPct = 1.0f;

	if ((bTimescaleBelowExpectedSpeed || bTimescaleAboveExpectedSpeed) && ExpectedPawnSpeed > 0.0f)
	{
		FLOAT actualSpeedPct = (Velocity.Size2D() / ExpectedPawnSpeed);
		if (bTimescaleBelowExpectedSpeed && !bTimescaleAboveExpectedSpeed)
		{
			speedPct = Min(actualSpeedPct, 1.0f);
		}
		else if (bTimescaleAboveExpectedSpeed && !bTimescaleBelowExpectedSpeed)
		{
			speedPct = Max(actualSpeedPct, 1.0f);
		}

		if (bTimescaleBelowExpectedSpeed && BlendInIdleMaxSpeedPct > 0.0f && speedPct < BlendInIdleMaxSpeedPct && Children.Num() >= 5 && Children(IdxIdle).Anim)
		{
			targetWeights[IdxIdle] = Saturate(1.0f - speedPct / BlendInIdleMaxSpeedPct);
			FLOAT remainingWeigth = 1.0f - targetWeights[IdxIdle];
			
			for (INT i = 0; i < 4; i++) // normalize the rest
			{
				targetWeights[i] = Saturate(targetWeights[i] * remainingWeigth);
			}
		}
	}

	FLOAT diagonalAdjustRate[4] = {0.0f};

	if (bTimescaleDiagonals)
	{
		if (targetWeights[IdxForward] > 0.0f)
		{
			diagonalAdjustRate[IdxForward] = Max(0.0f, ForwardPct) / targetWeights[0];
		}
		if (targetWeights[IdxBackward] > 0.0f)
		{
			diagonalAdjustRate[IdxBackward] = Max(0.0f, -ForwardPct) / targetWeights[1];
		}
		if (targetWeights[IdxLeft] > 0.0f)
		{
			diagonalAdjustRate[IdxLeft] = Max(0.0f, LeftPct) / targetWeights[2];
		}
		if (targetWeights[IdxRight] > 0.0f)
		{
			diagonalAdjustRate[IdxRight] = Max(0.0f, -LeftPct) / targetWeights[3];
		}
	}

	if (bTimescaleBelowExpectedSpeed || bTimescaleAboveExpectedSpeed || bTimescaleDiagonals)
	{
		for (INT i = 0; i < 4; i++)
		{
			FLOAT rate = (bTimescaleDiagonals ? (speedPct * diagonalAdjustRate[i]) : speedPct);

			UAnimNodeSequence* animSeq = Cast<UAnimNodeSequence>(Children(i).Anim);
			if (animSeq)
			{
				animSeq->Rate = rate;
			}
			else
			{
				UAnimNodeBlendBase* blendNode = Cast<UAnimNodeBlendBase>(Children(i).Anim);

				if (blendNode)
				{
					for (INT j = 0; j < blendNode->Children.Num(); j++)
					{
						animSeq = Cast<UAnimNodeSequence>(blendNode->Children(j).Anim);

						if (animSeq)
						{
							animSeq->Rate = rate;
						}
					}
				}
			}
		}
	}

	TWEAKABLE FLOAT WeightApproachCoeff = 0.99f;

	FLOAT totalWeights = 0.0f;

	for (INT i = 0; i < Children.Num(); i++)
	{
		Children(i).Weight = Utils::Approach(Children(i).Weight, targetWeights[i], WeightApproachCoeff, deltaTime);
		totalWeights += Children(i).Weight;
	}

	// Renormalize, as the approach can induce very slight normalization errors, and the blend code is picky
	for (INT i = 0; i < Children.Num(); i++)
	{
		Children(i).Weight /= totalWeights;
	}

	if (bUseReversedSideForBackwards && !bUseFullAnimForReverseSide)
	{
		UAnimNodeSequence* animSeqLeft = Cast<UAnimNodeSequence>(Children(IdxLeft).Anim);
		UAnimNodeSequence* animSeqRight = Cast<UAnimNodeSequence>(Children(IdxRight).Anim);
		if (animSeqLeft && animSeqRight)
		{
			if (bReversingBackward && Children(IdxBackward).Weight > 0.01f)
			{
				if (dirAngleDegs > 0.0f)
				{
					animSeqLeft->bReverseSync = TRUE;
				}
				else
				{
					animSeqRight->bReverseSync = TRUE;
				}
			}
			else
			{
				if (dirAngleDegs < 0.0f)
				{
					animSeqLeft->bReverseSync = FALSE;
				}
				else
				{
					animSeqRight->bReverseSync = FALSE;
				}
			}
			// Note: purposefully not touching the unaffected side, to allow blending out smoothly (keep its reversesync as it was)
		}
		else
		{
			UAnimNodeBlendBase* blendLeft = Cast<UAnimNodeBlendBase>(Children(IdxLeft).Anim);

			if (blendLeft)
			{
				for (INT i = 0; i < blendLeft->Children.Num(); i++)
				{
					animSeqLeft = Cast<UAnimNodeSequence>(blendLeft->Children(i).Anim);

					if (animSeqLeft)
					{
						animSeqLeft->bReverseSync = bReversingBackward && (dirAngleDegs > 0.0f);
					}
				}
			}

			UAnimNodeBlendBase* blendRight = Cast<UAnimNodeBlendBase>(Children(IdxRight).Anim);

			if (blendRight)
			{
				for (INT i = 0; i < blendRight->Children.Num(); i++)
				{
					animSeqRight = Cast<UAnimNodeSequence>(blendRight->Children(i).Anim);

					if (animSeqRight)
					{
						animSeqRight->bReverseSync = bReversingBackward && (dirAngleDegs < 0.0f);
					}
				}
			}
		}
	}
	
	Super::TickAnim(deltaTime);
}

void UOLAnimBlendDirectional::SetFullForwardWeight()
{
	for (INT i = 0; i < Children.Num(); i++)
	{
		Children(i).Weight = (i == IdxForward) ? 1.0f : 0.0f;
		bReversingBackward = FALSE;

		UAnimNodeSequence* animSeq = Cast<UAnimNodeSequence>(Children(i).Anim);
		if (animSeq)
		{
			animSeq->ConditionalClearCachedData();
		}
	}
}

void UOLAnimBlendDirectional::OnBecomeRelevant()
{
	Super::OnBecomeRelevant();
	bReversingBackward = FALSE;
}

FLOAT UOLAnimBlendDirectional::GetSliderPosition(INT SliderIndex, INT ValueIndex)
{
	check(0 == SliderIndex && 0 == ValueIndex);
	// DirAngle is between -PI and PI. Return value between 0.0 and 1.0 - so 0.5 is straight ahead.
	return 0.5f + (0.5f * (SliderDirAngle / (FLOAT)PI));
}

void UOLAnimBlendDirectional::HandleSliderMove(INT SliderIndex, INT ValueIndex, FLOAT NewSliderValue)
{
	check(0 == SliderIndex && 0 == ValueIndex);
	// Convert from 0.0 -> 1.0 to -PI to PI.
	SliderDirAngle = (FLOAT)PI * 2.f * (NewSliderValue - 0.5f);
}

FString UOLAnimBlendDirectional::GetSliderDrawValue(INT SliderIndex)
{
	check(0 == SliderIndex);
	return FString::Printf( TEXT("%3.2f%c"), SliderDirAngle * (180.f/(FLOAT)PI), 176 );
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimTurnOnSpot
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimTurnOnSpot::TickAnim(FLOAT deltaTime)
{
	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (!Owner)
	{
		return;
	}

	FLOAT playerHeading = Utils::NormalizeRotAngle(Owner->EyeRotation.Yaw * UNR_TO_DEG);
	FLOAT feetOffset = Utils::NormalizeRotAngle(CurrentFeetHeading - playerHeading);
	feetOffset = Clamp(feetOffset, -90.0f, 90.0f);

	UAnimNodeSequence* lookAnimSeq = Cast<UAnimNodeSequence>(Children(0).Anim);
	UAnimNodeSequence* correctLeftSeq = Cast<UAnimNodeSequence>(Children(1).Anim);
	UAnimNodeSequence* correctRightSeq = Cast<UAnimNodeSequence>(Children(2).Anim);
	if (!lookAnimSeq || !lookAnimSeq->AnimSeq || !correctLeftSeq || !correctRightSeq)
	{
		return;
	}

	TWEAKABLE FLOAT blendTime = 0.1f;
	TWEAKABLE FLOAT angleThreshForAnim = 65.0f;
	TWEAKABLE FLOAT correctionAmount = 95.0f;

	UBOOL bDebug = Utils::GetCheatManager() && Utils::GetCheatManager()->bDebugGameplay;

	FLOAT pctThreshold = ( (90.0f - angleThreshForAnim) / 180.0f);
	UBOOL bPlayingCorrectionAnim = Children(0).Weight < 0.99f;
	FLOAT normalizedOffset = 1.0f - (feetOffset + 90.0f) / 180.0f; // [0, 1]

	if (bPlayingCorrectionAnim)
	{
		UBOOL correctingRight = (Children(2).Weight > Children(1).Weight);
		UAnimNodeSequence* correctionSeq = correctingRight ? correctRightSeq : correctLeftSeq;

		if (correctionSeq->AnimSeq)
		{
			FLOAT animRatio = correctionSeq->CurrentTime / correctionSeq->AnimSeq->SequenceLength;
			FLOAT effectiveStartHeading = Utils::NormalizeRotAngle(playerHeading - (correctingRight ? angleThreshForAnim : -angleThreshForAnim));
			FLOAT correctedFeetHeading = Utils::NormalizeRotAngle(effectiveStartHeading + (correctingRight ? correctionAmount : -correctionAmount));			
			FLOAT correctionHeading = Utils::NormalizeRotAngle(correctedFeetHeading - effectiveStartHeading);
			CurrentFeetHeading = Utils::NormalizeRotAngle(effectiveStartHeading + animRatio * correctionHeading);

			FLOAT timeLeft = correctionSeq->GetTimeLeft();

			// if it's time to blend back to previous animation, do so.
			if (ActiveChildIndex > 0 && timeLeft <= 0.01f)
			{
				SetActiveChild(0, blendTime);
			}

			if (bDebug)
			{
				GWorld->GetWorldInfo()->DrawDebugLine(Owner->Location + VecZ(5.0f), Owner->Location + VecZ(5.0f) + 50.0f*FRotator(0, DEG_TO_UNR*effectiveStartHeading, 0).Vector(), 255,32,0,FALSE);
				GWorld->GetWorldInfo()->DrawDebugLine(Owner->Location + VecZ(5.0f), Owner->Location + VecZ(5.0f) + 50.0f*FRotator(0, DEG_TO_UNR*correctedFeetHeading, 0).Vector(), 32,255,0,FALSE);
			}
		}
	}
	else if (normalizedOffset >= (1.0f - pctThreshold) || normalizedOffset <= pctThreshold)
	{
		// start correction
		UBOOL correctingRight = normalizedOffset > 0.5f;		
		SetActiveChild(correctingRight ? 2 : 1, blendTime);

		UAnimNodeSequence* correctionSeq = (correctingRight ? correctRightSeq : correctLeftSeq);
		correctionSeq->PlayAnim();
	}

	FLOAT animPct = Clamp(normalizedOffset, 0.0f, 0.999f);	
	FLOAT newTime = animPct * lookAnimSeq->AnimSeq->SequenceLength;

	lookAnimSeq->PreviousTime = newTime;
	lookAnimSeq->CurrentTime = newTime;
	lookAnimSeq->ConditionalClearCachedData();

	if (bDebug)
	{
		GWorld->GetWorldInfo()->DrawDebugLine(Owner->Location + VecZ(5.0f), Owner->Location + VecZ(5.0f) + 50.0f*FRotator(0, DEG_TO_UNR*playerHeading, 0).Vector(), 255,128,0,FALSE);
		GWorld->GetWorldInfo()->DrawDebugLine(Owner->Location + VecZ(5.0f), Owner->Location + VecZ(5.0f) + 50.0f*FRotator(0, DEG_TO_UNR*CurrentFeetHeading, 0).Vector(), 0,128,255,FALSE);
	}

	Super::TickAnim(deltaTime);
}

void UOLAnimTurnOnSpot::OnBecomeRelevant()
{
	CurrentFeetHeading = 0.0f;

	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (Owner)
	{
		CurrentFeetHeading = Owner->EyeRotation.Yaw * UNR_TO_DEG;
	}

	SetActiveChild(0, 0.0f);

	Super::OnBecomeRelevant();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimCrouchedTurnOnSpot
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimCrouchedTurnOnSpot::TickAnim(FLOAT deltaTime)
{
	TWEAKABLE FLOAT angleThreshForAnim = 30.0f;
	TWEAKABLE FLOAT velocityThreshold = 1.0f;

	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (!Owner || appIsNearlyZero(deltaTime, KINDA_SMALL_NUMBERF))
	{
		return;
	}

	FLOAT playerHeading = Utils::NormalizeRotAngle(Owner->EyeRotation.Yaw * UNR_TO_DEG);	
	FLOAT deltaHeading = Utils::NormalizeRotAngle(playerHeading - LastHeading);
	LastHeading = playerHeading;

	FLOAT instantVelocity = Abs(deltaHeading) / deltaTime;
	TWEAKABLE FLOAT VelocityApproachCoeff = 0.999f;
	TurningVelocity = Utils::Approach(TurningVelocity, instantVelocity, VelocityApproachCoeff, deltaTime);

	UBOOL bMoving = Owner->RealVelocity.SizeSquared2D() > Square(5.0f);
		
	if (bHandUp)
	{
		// See if we stopped turning
		if (!bMoving && TurningVelocity < velocityThreshold)
		{
			FVector handPos = FRotationTranslationMatrix(Owner->Rotation, Owner->Location).TransformFVector(ActorSpaceIKOffset);
			FRotator handRot = FRotator(SkelComponent->LocalToWorldBoneAtom.GetRotation() * ActorSpaceIKRotation);

			TWEAKABLE FLOAT MaxHandTravelUp = 20.0f;
			TWEAKABLE FLOAT MaxHandTravelDown = -10.0f;
			TWEAKABLE FLOAT TestOffsetFwd = 15.0f;

			// test that IK would be appropriate
			FCheckResult Hit(1.f);
			FVector startTrace = handPos + VecZ(MaxHandTravelUp) + TestOffsetFwd * handRot.Vector();
			FVector endTrace = startTrace + VecZ(MaxHandTravelDown - MaxHandTravelUp);

			if (Utils::GetCheatManager() && Utils::GetCheatManager()->bDebugGameplay)
			{
				GWorld->GetWorldInfo()->DrawDebugLine(startTrace, endTrace, 255,165,200,FALSE);
			}

			if (!GWorld->SingleLineCheck( Hit, Owner, endTrace, startTrace, TRACE_AllBlocking, FVector(0.0f)))
			{
				FLOAT heightOffset = MaxHandTravelUp + Hit.Time*(MaxHandTravelDown - MaxHandTravelUp);
				bHandUp = FALSE;
				BaseHeading = playerHeading;

				IKPosition = handPos + VecZ(heightOffset);
				IKRotationWS = handRot;

				Owner->TriggerSoundEvent(HandDownSound);
			}
		}
	}
	else 
	{
		FLOAT baseOffset = Utils::NormalizeRotAngle(BaseHeading - playerHeading);
		baseOffset = Clamp(baseOffset, -90.0f, 90.0f);

		// See if we reached our angle threshold
		if (bMoving || Abs(baseOffset) > angleThreshForAnim)
		{
			bHandUp = TRUE;
			TurningVelocity = 360.0f; // force a minimum velocity so that we have a cool down time, before we stop again

			Owner->TriggerSoundEvent(HandUpSound);
		}
	}
	
	FLOAT targetHandUpWeight = bHandUp ? 1.0f : 0.0f;
	FLOAT targetIKStrength = 1.0f - targetHandUpWeight;

	TWEAKABLE FLOAT AnimApproachCoeff = 0.99f;
	TWEAKABLE FLOAT IKApproachCoeffIn = 0.95f;
	TWEAKABLE FLOAT IKApproachCoeffOut = 0.99f;

	HandUpWeight = Utils::Approach(HandUpWeight, targetHandUpWeight, AnimApproachCoeff, deltaTime);
	IKStrength = Utils::Approach(IKStrength, targetIKStrength, targetIKStrength > IKStrength ? IKApproachCoeffIn : IKApproachCoeffOut, deltaTime);

	Child2Weight = HandUpWeight;

	if (Utils::GetCheatManager() && Utils::GetCheatManager()->bDebugGameplay)
	{
		GWorld->GetWorldInfo()->DrawDebugLine(Owner->Location + VecZ(5.0f), Owner->Location + VecZ(5.0f) + 50.0f*FRotator(0, DEG_TO_UNR*playerHeading, 0).Vector(), 255,128,0,FALSE);
		GWorld->GetWorldInfo()->DrawDebugLine(Owner->Location + VecZ(5.0f), Owner->Location + VecZ(5.0f) + 50.0f*FRotator(0, DEG_TO_UNR*BaseHeading, 0).Vector(), 0,128,255,FALSE);
	}

	Super::TickAnim(deltaTime);
}

void UOLAnimCrouchedTurnOnSpot::OnBecomeRelevant()
{
	// start with the hand up
	BaseHeading = 0.0f;
	IKStrength = 0.0f;
	HandUpWeight = 1.0f;
	Child2Weight = 1.0f;
	LastHeading = 0.0f;
	TurningVelocity = 360.0f;
	bHandUp = TRUE;

	Super::OnBecomeRelevant();
}

void UOLAnimCrouchedTurnOnSpot::OnCeaseRelevant()
{
	IKStrength = 0.0f;	
	Super::OnCeaseRelevant();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimSelectByCamcorder
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimSelectByCamcorder::TickAnim(FLOAT deltaTime)
{
	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (Owner)
	{
		INT desiredIdx = (Owner->IsCamcorderActive() && Owner->LocomotionMode != LM_Cinematic) ? 1 : 0;

		if (ActiveChildIndex != desiredIdx)
		{
			SetActiveChild(desiredIdx, 0.0f);
		}
	}

	Super::TickAnim(deltaTime);
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimSelectByPhysicsVolume
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimSelectByPhysicsVolume::TickAnim(FLOAT deltaTime)
{
	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (Owner)
	{
		INT desiredIdx = Owner->IsInWaterVolume() ? 1 : 0;

		if (ActiveChildIndex != desiredIdx)
		{
			SetActiveChild(desiredIdx, 0.25f);
		}
	}

	Super::TickAnim(deltaTime);
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimSelectByWalkingStyle
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimSelectByWalkingStyle::TickAnim(FLOAT deltaTime)
{
	AOLHero* hero = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (hero)
	{
		INT desiredIdx = 0;

		if (hero->ForcedWalkingStyle != HWS_Default)
		{
			desiredIdx = hero->ForcedWalkingStyle;
		}
		else if (hero->IsInWaterVolume())
		{
			desiredIdx = 1;
		}
		else if (hero->bLimping)
		{
			desiredIdx = 2;
		}
		else if (hero->bHobbling)
		{
			desiredIdx = 3;
		}

		if (ActiveChildIndex != desiredIdx)
		{
			SetActiveChild(desiredIdx, 0.25f);
		}
	}

	Super::TickAnim(deltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimSelectByShadowProxy
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimSelectByShadowProxy::OnBecomeRelevant()
{
	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (Owner)
	{
		INT desiredIdx = (SkelComponent == Owner->ShadowProxy) ? 1 : 0;

		if (ActiveChildIndex != desiredIdx)
		{
			SetActiveChild(desiredIdx, 0.0f);
		}
	}

	Super::OnBecomeRelevant();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimStruggleCycle
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimStruggleCycle::OnBecomeRelevant()
{
	AOLPlayerController* OLPC = Utils::GetOLPC();

	if (OLPC && SkelComponent)
	{
		UBOOL bHero = Cast<AOLHero>(SkelComponent->GetOwner()) != NULL;
		FName animName = bHero ? OLPC->Struggle.Config.CycleAnimPlayer : OLPC->Struggle.Config.CycleAnimEnemy;
		SetAnim(animName);
		PlayAnim(TRUE, OLPC->Struggle.SmoothedAnimPlayRate, 0.0f);
	}

	Super::OnBecomeRelevant();
}

void UOLAnimStruggleCycle::TickAnim(FLOAT deltaTime)
{
	AOLPlayerController* OLPC = Utils::GetOLPC();

	if (OLPC)
	{
		Rate = OLPC->Struggle.SmoothedAnimPlayRate;		
	}

	Super::TickAnim(deltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimEnemyStruggle
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimEnemyStruggle::OnBecomeRelevant()
{
	if (ActiveChildIndex != 1)
	{
		SetActiveChild(0, 0.0f);
	}
	Super::OnBecomeRelevant();
}

void UOLAnimEnemyStruggle::SetPhase(EStruggleAnimPhase phase, const FStruggleConfig& cfg)
{
	INT childIdx = (INT)phase;
	switch(phase)
	{
	case SAP_StartIdle:
		Children(childIdx).Anim->SetAnim(cfg.InitIdleAnimEnemy);
		Children(childIdx).Anim->PlayAnim(TRUE);
		break;
	case SAP_Entry:
		Children(childIdx).Anim->SetAnim(cfg.EntryAnimEnemy);
		Children(childIdx).Anim->PlayAnim();
		break;
	case SAP_Cycle:
		break;
	case SAP_Success:
		Children(childIdx).Anim->SetAnim(cfg.ExitAnimEnemy);
		Children(childIdx).Anim->PlayAnim();
		break;
	case SAP_Fail:
		if (cfg.KillAnimEnemy == NAME_None)
		{
			return; // ignore - let the cycle continue
		}
		Children(childIdx).Anim->SetAnim(cfg.KillAnimEnemy);
		Children(childIdx).Anim->PlayAnim();
		break;
	case SAP_SuccessIdle:
		// This cases are currently ignored as we have no specific anims
		return;
	}

	if (phase == SAP_Entry && cfg.InitIdleAnimEnemy == NAME_None)
	{
		SetActiveChild(childIdx, 0.0f);
	}
	else
	{
		SetActiveChild(childIdx, 0.1f);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimPushing
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimPushing::OnBecomeRelevant()
{
	PushRatio = 0.0f;
	Super::OnBecomeRelevant();

	Children(0).Weight = 1.0f;
	for (INT i = 1; i < Children.Num(); i++)
	{
		Children(i).Weight = 0.0f;
	}
}

void UOLAnimPushing::TickAnim(FLOAT deltaTime)
{
	AOLHero* owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;
	AOLHero* hero = (owner && owner->bIsDummyPawn) ? owner : Utils::GetHero();

	if (hero && hero->ActivePushable)
	{
		UBOOL bPushingLeft = hero->bPushingFromBackEdge;

		FLOAT targetPushRatio = hero->ActivePushable->MaxSpeed == 0.0f ? 0.0f : Abs(hero->ActivePushable->CurrentVelocity) / hero->ActivePushable->MaxSpeed;
		targetPushRatio = Saturate(targetPushRatio);

		if (targetPushRatio >= PushRatio)
		{
			PushRatio = targetPushRatio;
		}
		else
		{
			// damp the slowdown, to prevent jerks when suddenly stopping
			TWEAKABLE FLOAT ApproachCoeff = 0.999f;
			PushRatio = Utils::Approach(PushRatio, targetPushRatio, ApproachCoeff, deltaTime);
		}

		UAnimNodeSequence* loopSeq = Cast<UAnimNodeSequence>(Children(bPushingLeft ? IdxLoopLeft : IdxLoopRight).Anim);

		if (loopSeq && loopSeq->AnimSeq)
		{
			loopSeq->Rate = PushRatio;
			if (!hero->bIsDummyPawn)
				hero->ActivePushable->CurrentPhase = loopSeq->CurrentTime / loopSeq->AnimSeq->SequenceLength;
		}
		
		Children(IdxIdleLeft).Weight = (bPushingLeft ? 1.0f - PushRatio : 0.0f);
		Children(IdxIdleRight).Weight = (bPushingLeft ? 0.0f : 1.0f - PushRatio);
		Children(IdxLoopLeft).Weight = (bPushingLeft ? PushRatio : 0.0f);
		Children(IdxLoopRight).Weight = (bPushingLeft ? 0.0f : PushRatio);
	}
	
	Super::TickAnim(deltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimNotify_Blur
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimNotify_Blur::Notify( UAnimNodeSequence* NodeSeq )
{
	if( !GWorld->HasBegunPlay() )
	{
		debugf( NAME_Log, TEXT("Editor: skipping AnimNotify_CameraEffect %s"), *GetName() );
		return;
	}
	AOLHero* Owner = Cast<AOLHero>(NodeSeq->SkelComponent ? NodeSeq->SkelComponent->GetOwner() : NULL);
	if (Owner != NULL && Owner->bIsDummyPawn)
		return;
	if (Utils::GetFXManager())
	{
		Utils::GetFXManager()->TriggerBlur(Amount, Duration, Desaturation, BlendInTime, BlendOutTime);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimNotify_Fade
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimNotify_Fade::Notify( UAnimNodeSequence* NodeSeq )
{
	if( !GWorld->HasBegunPlay() )
	{
		debugf( NAME_Log, TEXT("Editor: skipping UOLAnimNotify_Fade %s"), *GetName() );
		return;
	}

	AOLPlayerController* OLPC = Utils::GetOLPC();
	if (OLPC)
	{
		FVector2D fadeAlpha;		
		fadeAlpha.X = (bForceStartValue ? (bFadeIn ? 0.0f : Opacity) : OLPC->FadeAmount);
		fadeAlpha.Y = bFadeIn ? Opacity : 0.0f;

		OLPC->eventClientSetCameraFade(TRUE, FadeColor, fadeAlpha, Duration, FALSE);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimNotify_Door
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimNotify_Door::Notify( UAnimNodeSequence* NodeSeq )
{
	USkeletalMeshComponent* SkelComp = NodeSeq->SkelComponent;
	check( SkelComp );
	check (AngleWhenOpen > 0.0f);

	AOLEnemyPawn* Owner = Cast<AOLEnemyPawn>(SkelComp->GetOwner());

	if (Owner != NULL && Owner->Bot != NULL && Owner->Bot->ActiveDoor != NULL)
	{
		Owner->Bot->ActiveDoor->MaxOpenAngle = AngleWhenOpen; // reset automatically upon close

		FLOAT Speed = AngleWhenOpen / Duration;
		AOLPlayerController* OLPC = Utils::GetOLPC();
		debugf(TEXT("### AnimNotify_Door: Owner=%s Interaction=%d Speed=%.1f OLPC=%s"),
			*Owner->GetName(), (INT)Interaction, Speed,
			OLPC ? *OLPC->GetName() : TEXT("NULL"));
		switch (Interaction)
		{
		case DI_Open:
			Owner->Bot->ActiveDoor->Open(Owner, Speed);
			if (OLPC)
				OLPC->eventNotifyEnemyDoorOpen(Owner, Owner->Bot->ActiveDoor, Speed, AngleWhenOpen);
			break;
		case DI_Close:
			Owner->Bot->ActiveDoor->Close(Owner, Speed);
			if (OLPC)
				OLPC->eventNotifyEnemyDoorDone(Owner, Owner->Bot->ActiveDoor, Speed);
			break;
		}
	}
}

void UOLAnimNotify_Door::AnimNotifyEventChanged(class UAnimNodeSequence* NodeSeq, FAnimNotifyEvent* OwnerEvent)
{
	Super::AnimNotifyEventChanged(NodeSeq, OwnerEvent);
	Duration = OwnerEvent->Duration;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimNotify_ProceduralAdjust
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimNotify_ProceduralAdjust::Notify( UAnimNodeSequence* NodeSeq )
{
	USkeletalMeshComponent* SkelComp = NodeSeq->SkelComponent;
	check( SkelComp );

	AOLPawn* Owner = Cast<AOLPawn>(SkelComp->GetOwner());

	if (Owner != NULL)
	{
		Owner->ProceduralAdjustNotify(Duration);
	}
}

void UOLAnimNotify_ProceduralAdjust::AnimNotifyEventChanged(class UAnimNodeSequence* NodeSeq, FAnimNotifyEvent* OwnerEvent)
{
	Super::AnimNotifyEventChanged(NodeSeq, OwnerEvent);
	Duration = OwnerEvent->Duration;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimNotify_LeftHandIK
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimNotify_LeftHandIK::Notify( UAnimNodeSequence* NodeSeq )
{
	USkeletalMeshComponent* SkelComp = NodeSeq->SkelComponent;
	check( SkelComp );

	AOLHero* Owner = Cast<AOLHero>(SkelComp->GetOwner());

	if (Owner != NULL)
	{
		Owner->ActivateLeftHandIK((IKTargetType)TargetType, Duration, FadeInTime, FadeOutTime, EffectorOffset);
	}
}

void UOLAnimNotify_LeftHandIK::AnimNotifyEventChanged(class UAnimNodeSequence* NodeSeq, FAnimNotifyEvent* OwnerEvent)
{
	Super::AnimNotifyEventChanged(NodeSeq, OwnerEvent);
	Duration = OwnerEvent->Duration;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimNotify_PropAttachment
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimNotify_PropAttachment::Notify( UAnimNodeSequence* NodeSeq )
{
	USkeletalMeshComponent* SkelComp = NodeSeq->SkelComponent;
	check( SkelComp );

	AOLHero* Owner = Cast<AOLHero>(SkelComp->GetOwner());

	if (Owner != NULL)
	{
		Owner->AttachCSAProp(Duration, BlendInTime, BlendOutTime, PositionOffset, OrientationOffset, bDiscardOffsets, bHideWhenDone);
	}
}

void UOLAnimNotify_PropAttachment::AnimNotifyEventChanged(class UAnimNodeSequence* NodeSeq, FAnimNotifyEvent* OwnerEvent)
{
	Super::AnimNotifyEventChanged(NodeSeq, OwnerEvent);
	Duration = OwnerEvent->Duration;
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimNotify_AttachWeapon
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimNotify_AttachWeapon::Notify( UAnimNodeSequence* NodeSeq )
{
	USkeletalMeshComponent* SkelComp = NodeSeq->SkelComponent;
	check( SkelComp );

	AOLPawn* Owner = Cast<AOLPawn>(SkelComp->GetOwner());
	if (!Owner)
	{
		return;
	}
		
	USkeletalMeshComponent* skelMeshComp = ConstructObject<USkeletalMeshComponent>(USkeletalMeshComponent::StaticClass(), Owner);
	skelMeshComp->SetSkeletalMesh(WeaponSkelMesh);		
	Owner->Mesh->AttachComponent(skelMeshComp, FName(*AttachBoneName));
}

void UOLAnimNotify_AttachWeapon::NotifyEnd( class UAnimNodeSequence* NodeSeq, FLOAT AnimCurrentTime )
{
	USkeletalMeshComponent* SkelComp = NodeSeq->SkelComponent;
	check( SkelComp );

	AOLPawn* Owner = Cast<AOLPawn>(SkelComp->GetOwner());
	if (!Owner)
	{
		return;
	}

	for ( INT i = 0; i < Owner->AllComponents.Num(); ++i )
	{
		USkeletalMeshComponent * skelMeshComp = Cast<USkeletalMeshComponent>( Owner->AllComponents( i ) );
		if (skelMeshComp && skelMeshComp->SkeletalMesh == WeaponSkelMesh) 
		{
			skelMeshComp->DetachFromAny();
		}
	}
}

void UOLAnimNotify_AttachWeapon::AnimNotifyEventChanged(class UAnimNodeSequence* NodeSeq, FAnimNotifyEvent* OwnerEvent)
{
	Super::AnimNotifyEventChanged(NodeSeq, OwnerEvent);
	Duration = OwnerEvent->Duration;
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimNotify_OverrideCameraParams
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimNotify_OverrideCameraParams::Notify( UAnimNodeSequence* NodeSeq )
{
	USkeletalMeshComponent* SkelComp = NodeSeq->SkelComponent;
	check( SkelComp );

	AOLHero* Owner = Cast<AOLHero>(SkelComp->GetOwner());

	if (Owner != NULL)
	{
		Owner->OverrideCameraSettingsNotify(this);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimBlendByIdle
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimBlendByIdle::OnBecomeRelevant()
{
	if (SkelComponent != NULL && SkelComponent->GetOwner() != NULL)
	{
		AActor* Owner = SkelComponent->GetOwner();
		if ( Owner )
		{
			//debugf(TEXT("Velocity %i"), Owner->Velocity.Size() );

			if ( Owner->Velocity.SizeSquared() < KINDA_SMALL_NUMBER )
			{
				SetActiveChild(IdxIdle,GetBlendTime(IdxIdle));
			}
			else
			{
				SetActiveChild(IdxMoving,GetBlendTime(IdxMoving));
			}
		}
	}

	Super::OnBecomeRelevant();
}


void UOLAnimBlendByIdle::TickAnim(FLOAT DeltaSeconds)
{
	// Get the Pawn Owner
	if (SkelComponent != NULL && SkelComponent->GetOwner() != NULL)
	{
		AActor* Owner = SkelComponent->GetOwner();
		if ( Owner )
		{
			if ( Owner->Velocity.SizeSquared() < KINDA_SMALL_NUMBER )
			{
				// Not moving

				UAnimNodeSequence* stopAnimSeq = Cast<UAnimNodeSequence>(Children(IdxStop).Anim);
				if (ActiveChildIndex != IdxIdle && ActiveChildIndex != IdxStop)
				{
					if (stopAnimSeq != NULL && stopAnimSeq->AnimSeq != NULL)
					{
						SetActiveChild(IdxStop, GetBlendTime(IdxStop));
					}
					else
					{
						SetActiveChild(IdxIdle, GetBlendTime(IdxIdle));
					}
				}
				else if (ActiveChildIndex == IdxStop)
				{
					if (stopAnimSeq && stopAnimSeq->GetNormalizedPosition() >= 1.0f)
					{
						ResetSynchGroup();
						SetActiveChild(IdxIdle, GetBlendTime(IdxIdle));
					}
				}
			}
			else
			{
				// Moving

				UAnimNodeSequence* startAnimSeq = Cast<UAnimNodeSequence>(Children(IdxStart).Anim);
				if (ActiveChildIndex != IdxMoving && ActiveChildIndex != IdxStart)
				{
					if (startAnimSeq != NULL && startAnimSeq->AnimSeq != NULL)
					{
						SetActiveChild(IdxStart, GetBlendTime(IdxStart));
					}
					else
					{
						SetActiveChild(IdxMoving, GetBlendTime(IdxMoving));
					}
				}
				else if (ActiveChildIndex == IdxStart)
				{
					if (startAnimSeq && startAnimSeq->GetNormalizedPosition() >= 1.0f)
					{
						ResetSynchGroup();
						SetActiveChild(IdxMoving, GetBlendTime(IdxMoving));
					}
				}
			}
		}
	}
	Super::TickAnim(DeltaSeconds);
}

void UOLAnimBlendByIdle::OnChildAnimEnd(UAnimNodeSequence* Child, FLOAT PlayedTime, FLOAT ExcessTime)
{
	Super::OnChildAnimEnd(Child, PlayedTime, ExcessTime);

	if (ActiveChildIndex == IdxStart)
	{
		ResetSynchGroup();
		SetActiveChild(IdxMoving, GetBlendTime(IdxMoving));
	}
	else if (ActiveChildIndex == IdxStop)
	{
		ResetSynchGroup();
		SetActiveChild(IdxIdle, GetBlendTime(IdxIdle));
	}
}

void UOLAnimBlendByIdle::ResetSynchGroup()
{
	if (ChildSynchGroup != NAME_None)
	{
		UAnimTree* RootNode = Cast<UAnimTree>(SkelComponent->Animations);
		if( RootNode )
		{
			RootNode->ForceGroupRelativePosition(ChildSynchGroup, 0.0f);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimLocomotion
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimLocomotion::OnBecomeRelevant()
{
	Super::OnBecomeRelevant();
	ResetAllSynchGroups();

	INT desiredIdx = 0;

	AOLPawn* pawn = (SkelComponent ? Cast<AOLPawn>(SkelComponent->GetOwner()) : NULL);
	if (pawn)
	{
		ActiveChildIdx = (pawn->Velocity.SizeSquared2D() > 0.1f) ? IdxMoving : IdxIdle;
	}

	for (INT i = 0; i < Children.Num(); i++)
	{
		Children(i).Weight = (i == ActiveChildIdx) ? 1.0f : 0.0f;
	}

	bIsTurning = FALSE;

	// Try to set the anims - may fail depending on the anim set
	UAnimNodeSequence* startAnimSeq = Cast<UAnimNodeSequence>(Children(IdxStart).Anim);
	if (startAnimSeq && !startAnimSeq->AnimSeq && startAnimSeq->AnimSeqName != NAME_None)
	{
		startAnimSeq->SetAnim(startAnimSeq->AnimSeqName);		
	}

	UAnimNodeSequence* stopAnimSeq = Cast<UAnimNodeSequence>(Children(IdxStop).Anim);
	if (stopAnimSeq && !stopAnimSeq->AnimSeq && stopAnimSeq->AnimSeqName != NAME_None)
	{
		stopAnimSeq->SetAnim(stopAnimSeq->AnimSeqName);
	}
}

void UOLAnimLocomotion::TickAnim(FLOAT deltaSeconds)
{
	Super::TickAnim(deltaSeconds);

	AOLPawn* pawn = (SkelComponent ? Cast<AOLPawn>(SkelComponent->GetOwner()) : NULL);

	if (!pawn)
	{
		return;
	}

	//TWEAKABLE FLOAT MinMovingContrib = 0.35f;
	//TWEAKABLE FLOAT BlendTimePct = 0.35f;
	TWEAKABLE UBOOL bSmoothBlend = TRUE;
	//TWEAKABLE FLOAT AngleThreshFor180 = 150.0f;
	TWEAKABLE UBOOL bEnableStartAnim = TRUE;
	TWEAKABLE UBOOL bEnableStopAnim = FALSE; // Disabled cause it looks real bad
	TWEAKABLE FLOAT EndAnimThreshold = 0.90f;
	TWEAKABLE FLOAT StartTransitionBlendTime = 0.2f;

	FLOAT currentVelocity = pawn->Velocity.Size2D();
	UBOOL bTurning = pawn->Turning.bActive;
	UBOOL bWasTurning = bIsTurning;
	bIsTurning = bTurning;
	UBOOL bMoving = bTurning || currentVelocity > 0.1f;

	FLOAT effectiveBlendTimePct = BlendTimePct;
	FLOAT effectiveMinMovingContribOver90 = MinMovingContribOver90;

	if (Utils::GetHero() && Utils::GetHero()->bIsCrouched && (Utils::GetHero()->Location.DistanceSquared(pawn->Location) < Square(300.0f)))
	{
		// hack to change values when the player is close and crouched
		effectiveBlendTimePct = 0.15f;
		effectiveMinMovingContribOver90 = 0.05f;
	}

	// Update the anim state

	if (bMoving)
	{
		// Moving

		UAnimNodeSequence* startAnimSeq = Cast<UAnimNodeSequence>(Children(IdxStart).Anim);
		if (ActiveChildIdx != IdxMoving && ActiveChildIdx != IdxStart)
		{
			if (bEnableStartAnim && startAnimSeq && startAnimSeq->AnimSeq && pawn->CanStartAndStop())
			{
				// Starting
				ActiveChildIdx = IdxStart;
				TransitionBlendTime = 0.1f;
				startAnimSeq->PlayAnim(FALSE);
			}
			else
			{
				// Starting, but no start anim
				ActiveChildIdx = IdxMoving;
				TransitionBlendTime = 0.2f;
			}
		}
		else if (ActiveChildIdx == IdxStart)
		{
			if (bMoveOnNextFrame)
			{
				ActiveChildIdx = IdxMoving;
				TransitionBlendTime = StartTransitionBlendTime;
				bMoveOnNextFrame = FALSE;
			}
			else if ((startAnimSeq && startAnimSeq->GetNormalizedPosition() >= EndAnimThreshold) || !pawn->CanStartAndStop())
			{
				// Starting DONE
				
				// Force the directional nodes forward (instead of blending from idle)
				UAnimTree* animTree = Cast<UAnimTree>(SkelComponent->Animations);
				if (animTree)
				{
					TArray<UAnimNode*> animNodes;
					animTree->GetNodesByClass(animNodes, UOLAnimBlendDirectional::StaticClass());

					for (INT i = 0; i < animNodes.Num(); i++)
					{
						UOLAnimBlendDirectional* directionalNode = Cast<UOLAnimBlendDirectional>(animNodes(i));
						if (directionalNode)
						{
							directionalNode->SetFullForwardWeight();
						}
					}
				}

				ResetAllSynchGroups();

				bMoveOnNextFrame = TRUE;
			}
		}
	}
	else
	{
		// Not moving

		UAnimNodeSequence* stopAnimSeq = Cast<UAnimNodeSequence>(Children(IdxStop).Anim);
		if (ActiveChildIdx != IdxIdle && ActiveChildIdx != IdxStop)
		{
			if (bEnableStopAnim && stopAnimSeq && stopAnimSeq->AnimSeq && pawn->CanStartAndStop())
			{
				// Stopping
				ActiveChildIdx = IdxStop;
				TransitionBlendTime = 0.25f;
				stopAnimSeq->PlayAnim(FALSE);
			}
			else
			{
				ActiveChildIdx = IdxIdle;
				TransitionBlendTime = 0.25f;
			}
		}
		else if (ActiveChildIdx == IdxStop)
		{
			if ((stopAnimSeq && stopAnimSeq->GetNormalizedPosition() >= EndAnimThreshold) || !pawn->CanStartAndStop())
			{
				// Stopping DONE
				ActiveChildIdx = IdxIdle;
				TransitionBlendTime = 0.25f;
			}
		}
	}

	FLOAT turningWeight = 0.0f;
	FLOAT turningAlpha = 1.0f;

	INT turningIdxOne = -1;
	INT turningIdxTwo = -1;

	if (bTurning)
	{
		if (!bWasTurning )
		{	
			bReachedHalfTurn = FALSE;
		}

		turningIdxOne = (pawn->Turning.InitialDeltaYaw > 0.0f) ? IdxRight90 : IdxLeft90;
		turningIdxTwo = (pawn->Turning.InitialDeltaYaw > 0.0f) ? IdxRight180 : IdxLeft180;

		UAnimNodeSequence* turningAnimNodeSeqOne = Children(turningIdxOne).Anim ? Children(turningIdxOne).Anim->GetAnimNodeSequence() : NULL;
		if (turningAnimNodeSeqOne && !turningAnimNodeSeqOne->AnimSeq && turningAnimNodeSeqOne->AnimSeqName != NAME_None)
		{
			turningAnimNodeSeqOne->SetAnim(turningAnimNodeSeqOne->AnimSeqName);
			check(turningAnimNodeSeqOne->AnimSeq);
		}
	
		UAnimNodeSequence* turningAnimNodeSeqTwo = Children(turningIdxTwo).Anim ? Children(turningIdxTwo).Anim->GetAnimNodeSequence() : NULL;
		if (turningAnimNodeSeqTwo && !turningAnimNodeSeqTwo->AnimSeq && turningAnimNodeSeqTwo->AnimSeqName != NAME_None)
		{
			turningAnimNodeSeqTwo->SetAnim(turningAnimNodeSeqTwo->AnimSeqName);
			check(turningAnimNodeSeqTwo->AnimSeq);
		}

		if (turningAnimNodeSeqOne && turningAnimNodeSeqOne->AnimSeq &&
			turningAnimNodeSeqTwo && turningAnimNodeSeqTwo->AnimSeq)
		{
			FLOAT elapsedTime = GWorld->GetTimeSeconds() - pawn->Turning.StartTime;			
			FLOAT blendTime = effectiveBlendTimePct * pawn->Turning.Duration;
			FLOAT timeToGo = pawn->Turning.Duration - elapsedTime;

			if (timeToGo <= 0.0f)
			{
				turningWeight = 0.0f;
			}
			else if (elapsedTime < blendTime)
			{
				if (bSmoothBlend)
				{
					turningWeight = Utils::SmootherStep(elapsedTime / blendTime);
				}
				else
				{
					turningWeight = elapsedTime / blendTime;
				}
			}
			else if (timeToGo < blendTime)
			{
				if (bSmoothBlend)
				{
					turningWeight = Utils::SmootherStep(timeToGo / blendTime);
				}
				else
				{
					turningWeight = timeToGo / blendTime;
				}
			}
			else
			{
				turningWeight = 1.0f;
			}

			FLOAT halfDuration = 0.5f * pawn->Turning.Duration;
			if (elapsedTime > halfDuration && !bReachedHalfTurn)
			{
				bReachedHalfTurn = TRUE;
			}

			if (!bWasTurning )
			{				
				FLOAT playRate = turningAnimNodeSeqOne->AnimSeq->SequenceLength / pawn->Turning.Duration;				
				turningAnimNodeSeqOne->PlayAnim(FALSE, playRate);

				playRate = turningAnimNodeSeqTwo->AnimSeq->SequenceLength / pawn->Turning.Duration;				
				turningAnimNodeSeqTwo->PlayAnim(FALSE, playRate);
			}

			if (Abs(pawn->Turning.InitialDeltaYaw) > 90.f)
			{
				turningAlpha = 1.0f - Clamp((Abs(pawn->Turning.InitialDeltaYaw) - 90.0f) / 90.0f, 0.f, 1.f);
				turningWeight *= (1.0f - effectiveMinMovingContribOver90);
			}
			else
			{
				turningWeight *= Clamp(Abs(pawn->Turning.InitialDeltaYaw) / 90.0f, 0.f, 1.f);
				turningWeight *= (1.0f - MinMovingContribUnder90);
			}
		}
	}

	if (bMoveOnNextFrame && ActiveChildIdx != IdxStart)
	{
		bMoveOnNextFrame = FALSE; // invalid - might have switched to idle
	}

	// Update anim weights
	FLOAT activeWeight = (bTurning) ? (1.0f - turningWeight) : 1.0f;
	FLOAT totalWeights = 0.0f;

	for (INT i = 0; i < 4; i++)
	{
		FLOAT targetWeight = (i == ActiveChildIdx) ? activeWeight : 0.0f;

		if (bMoveOnNextFrame)
		{
			// exception - give a very small weight on this frame to the moving animation and switch on the next one, 
			// in order to work around a bug where the activation frame will have idle fully weighted, causing a pop
			TWEAKABLE FLOAT MovingAnimInitWeight = 0.05f;
			if (i == IdxMoving)
			{
				targetWeight = MovingAnimInitWeight;
			}
			else if (i == IdxStart)
			{
				targetWeight -= MovingAnimInitWeight;
			}
		}
		
		if (!appIsNearlyEqual(Children(i).Weight, targetWeight, KINDA_SMALL_NUMBERF))
		{
			if (!appIsNearlyZero(TransitionBlendTime, KINDA_SMALL_NUMBERF))
			{
				FLOAT curWeight = Children(i).Weight;
				FLOAT deltaWeight = deltaSeconds / TransitionBlendTime;
				deltaWeight = Min(deltaWeight, Abs(targetWeight - curWeight));
				Children(i).Weight = Saturate(curWeight + deltaWeight*Sgn(targetWeight - curWeight));
			}
			else
			{
				Children(i).Weight = targetWeight;
			}
		}
		check(Children(i).Weight >= 0.0f && Children(i).Weight <= 1.0f);

		totalWeights += Children(i).Weight;
	}

	// Set the turning weights
	for (INT i = 4; i < 8; i++)
	{
		if (i == turningIdxOne)
		{
			Children(i).Weight = turningWeight * turningAlpha;
		}
		else if (i == turningIdxTwo)
		{
			Children(i).Weight = turningWeight * (1.0f - turningAlpha);
		}
		else
		{
			Children(i).Weight = 0.f;
		}
	}	

	totalWeights += turningWeight;

	// renormalize, for small mathematical errors
	for (INT i = 0; i < 8; i++)
	{
		Children(i).Weight /= totalWeights;
	}	
}

void UOLAnimLocomotion::ResetAllSynchGroups()
{
	UAnimTree* animTree = Cast<UAnimTree>(SkelComponent->Animations);
	if (animTree)
	{
		for (INT i = 0; i < animTree->AnimGroups.Num(); i++)
		{
			animTree->ForceGroupRelativePosition(animTree->AnimGroups(i).GroupName, 0.0f);
		}
	}
}

FLOAT UOLAnimLocomotion::GetSliderPosition(INT SliderIndex, INT ValueIndex)
{
	check(0 == SliderIndex && 0 == ValueIndex);
	return SliderPosition;
}

void UOLAnimLocomotion::HandleSliderMove(INT SliderIndex, INT ValueIndex, FLOAT NewSliderValue)
{
	check(0 == SliderIndex && 0 == ValueIndex);
	SliderPosition = NewSliderValue;

	if( Children.Num() > 0 )
	{
		const INT TargetChannel = appRound(SliderPosition*(Children.Num() - 1));
		if( ActiveChildIdx != TargetChannel )
		{
			ActiveChildIdx = TargetChannel;

			for (INT i = 0; i < Children.Num(); i++)
			{
				Children(i).Weight = (i == ActiveChildIdx) ? 1.0f : 0.0f;
			}
		}
	}
}

FString UOLAnimLocomotion::GetSliderDrawValue(INT SliderIndex)
{
	check(0 == SliderIndex);
	INT TargetChild = appRound( SliderPosition*(Children.Num() - 1) );
	if( Children.Num() > 0 && TargetChild < Children.Num() )
	{
		return FString::Printf( TEXT("%3.2f %s"), SliderPosition, *Children(TargetChild).Name.ToString() );
	}

	return FString::Printf( TEXT("%3.2f"), SliderPosition );
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimHeatShielding
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimHeatShielding::OnBecomeRelevant()
{
	Super::OnBecomeRelevant();
	HeatShieldingWeight = 0.0f;
	HeatStrength = 0.0f;
}

void UOLAnimHeatShielding::SetActive(UBOOL isActive)
{
	if (!bActive && isActive)
	{
		HeatShieldingWeight = 0.0f;
		HeatStrength = 0.0f;
	}
	bActive = isActive;
}

void UOLAnimHeatShielding::TickAnim(FLOAT deltaTime)
{
	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (!Owner)
	{
		return;
	}

	SetActive(Owner->bHeatShielding);

	TWEAKABLE FLOAT ShieldingApproachCoeffIn = 0.9f;
	TWEAKABLE FLOAT ShieldingApproachCoeffOut = 0.6f;
	TWEAKABLE FLOAT StrengthApproachCoeffIn = 0.9f;
	TWEAKABLE FLOAT StrengthApproachCoeffOut = 0.6f;

	// update the ratio of heat vs. normal
	FLOAT targetShielding = bActive ? 1.0f : 0.0f;
	HeatShieldingWeight = Utils::Approach(HeatShieldingWeight, targetShielding, targetShielding > HeatShieldingWeight ? ShieldingApproachCoeffIn : ShieldingApproachCoeffOut, deltaTime);

	// update the heat strength
	FLOAT targetStrength = 0.0f;
	
	if (bActive)
	{
		if (Owner->HeatDistance <= HighDistance)
		{
			targetStrength = 1.0f;
		}
		else if (Owner->HeatDistance <= LowDistance)
		{
			targetStrength = MapClamped(Owner->HeatDistance, HighDistance, LowDistance, 1.0f, 0.5f);
		}
		else if (Owner->HeatDistance <= MinimumDistance)
		{
			targetStrength = MapClamped(Owner->HeatDistance, LowDistance, MinimumDistance, 0.5f, 0.0f);
		}
	}
	HeatStrength = Utils::Approach(HeatStrength, targetStrength, targetStrength > HeatStrength ? StrengthApproachCoeffIn : StrengthApproachCoeffOut, deltaTime);

	FLOAT lowWeight = 0.0f;
	FLOAT highWeight = 0.0f;

	if (HeatStrength >= 0.5f)
	{
		highWeight = Saturate(2.0f * (HeatStrength - 0.5f));
		lowWeight = 1.0f - highWeight;
	}
	else
	{
		lowWeight = Saturate(2.0f * HeatStrength);
	}

	highWeight *= HeatShieldingWeight;
	lowWeight *= HeatShieldingWeight;

	Children(0).Weight = 1.0f - (highWeight + lowWeight);
	Children(1).Weight = lowWeight;
	Children(2).Weight = highWeight;

	Super::TickAnim(deltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimParrying
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimParrying::OnBecomeRelevant()
{
	Super::OnBecomeRelevant();
	ParryWeight = 0.0f;
	ParryStrength = 0.0f;
	ParryAngleRatio = 0.5f;
}

void UOLAnimParrying::SetActive(UBOOL isActive)
{
	bActive = isActive;
}

void UOLAnimParrying::TickAnim(FLOAT deltaTime)
{
	TWEAKABLE FLOAT WeightApproachCoeffIn = 0.9999f;
	TWEAKABLE FLOAT WeightApproachCoeffOut = 0.8f;
	TWEAKABLE FLOAT StrengthApproachCoeffIn = 0.9999f;
	TWEAKABLE FLOAT StrengthApproachCoeffOut = 0.8f;
	TWEAKABLE FLOAT AngleRatioApproachCoeff = 0.99999f;

	//
	// Update global weight (blend in or out)
	// 

	FLOAT targetWeight = bActive ? 1.0f : 0.0f;
	ParryWeight = Utils::Approach(ParryWeight, targetWeight, targetWeight > ParryWeight ? WeightApproachCoeffIn : WeightApproachCoeffOut, deltaTime);

	if (ParryWeight < 0.01f)
	{
		ParryWeight = 0.0f;
		ParryStrength = 0.0f;
		ParryAngleRatio = 0.5f;
		Children(IdxNormal).Weight = 1.0f;
		Children(IdxLow).Weight = 0.0f;
		Children(IdxHigh).Weight = 0.0f;
	}
	else
	{

		//
		// Update strength (distance relative)
		// 

		FLOAT targetStrength = 0.0f;

		if (bActive)
		{
			if (EnemyDistance <= HighDistance)
			{
				targetStrength = 1.0f;
			}
			else if (EnemyDistance <= LowDistance)
			{
				targetStrength = MapClamped(EnemyDistance, HighDistance, LowDistance, 1.0f, 0.5f);
			}
			else if (EnemyDistance <= MinimumDistance)
			{
				targetStrength = MapClamped(EnemyDistance, LowDistance, MinimumDistance, 0.5f, 0.0f);
			}
		}
		ParryStrength = Utils::Approach(ParryStrength, targetStrength, targetStrength > ParryStrength ? StrengthApproachCoeffIn : StrengthApproachCoeffOut, deltaTime);

		//
		// Update angle ratio 
		// 

		FLOAT instantAngleRatio = MapClamped(EnemyRelYaw, -AngleRange, AngleRange, 0.0f, 1.0f);
		ParryAngleRatio = Utils::Approach(ParryAngleRatio, instantAngleRatio, AngleRatioApproachCoeff, deltaTime);

		//
		// Apply results
		// 

		FLOAT lowWeight = 0.0f;
		FLOAT highWeight = 0.0f;

		if (ParryStrength >= 0.5f)
		{
			highWeight = Saturate(2.0f * (ParryStrength - 0.5f));
			lowWeight = 1.0f - highWeight;
		}
		else
		{
			lowWeight = Saturate(2.0f * ParryStrength);
		}

		highWeight *= ParryWeight;
		lowWeight *= ParryWeight;

		if (highWeight > 0.01f)
		{
			UAnimNodeSequence* highNode = Cast<UAnimNodeSequence>(Children(IdxHigh).Anim);

			if (!highNode)
			{
				UAnimNodeBlend* blendNode = Cast<UAnimNodeBlend>(Children(IdxHigh).Anim); // likely a blendperbone node
				if (blendNode && blendNode->Children.Num() >= 2)
				{
					highNode = Cast<UAnimNodeSequence>(blendNode->Children(1).Anim);
				}
			}

			if (highNode && highNode->AnimSeq)
			{
				FLOAT newTime = Clamp(ParryAngleRatio, 0.0f, 0.99f) * highNode->AnimSeq->SequenceLength;
				highNode->PreviousTime = highNode->CurrentTime;
				highNode->CurrentTime = newTime;
				highNode->ConditionalClearCachedData();
			}
		}

		Children(IdxNormal).Weight = 1.0f - (highWeight + lowWeight);
		Children(IdxLow).Weight = lowWeight;
		Children(IdxHigh).Weight = highWeight;
	}

	Super::TickAnim(deltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimSmartIdle
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimSmartIdle::OnBecomeRelevant()
{
	Super::OnBecomeRelevant();

	CurrentIdleRate = 1.0f;
	TargetIdleRate = 1.0f;
	NextIdleRateTransition = 0.0f;
	CurrentBreakerWeight = 0.0f;
	TargetBreakerWeight = 0.0f;
	BreakerWeightTransitionSpeed = 1.0f;
	NextBreakerWeightTransition = 0.0f;
}

void UOLAnimSmartIdle::TickAnim(FLOAT deltaTime)
{
	FLOAT curTime = GWorld->GetTimeSeconds();

	// Check for new transitions

	if (bModifyPlayrateIdle && curTime >= NextIdleRateTransition)
	{
		TargetIdleRate = RandRange(IdleMinRate, IdleMaxRate);
		FLOAT thisRateDuration = RandRange(IdleRateMinDuration, IdleRateMaxDuration);
		NextIdleRateTransition = curTime + thisRateDuration;
	}

	if (curTime >= NextBreakerWeightTransition)
	{
		FLOAT diceRoll = appFrand();
		if (diceRoll <= IdleBias)
		{
			TargetBreakerWeight = 0.0f;
		}
		else
		{
			TargetBreakerWeight = Saturate( ((diceRoll - IdleBias) / (1.0f - IdleBias)) * BreakerMaxWeight );
		}

		BreakerWeightTransitionSpeed = RandRange(MinBreakerWeightTransitionSpeed, MaxBreakerWeightTransitionSpeed);

		FLOAT thisBreakerDuration = RandRange(BreakerMinDuration, BreakerMaxDuration);
		NextBreakerWeightTransition = curTime + thisBreakerDuration;		
	}

	// Update curves

	if (bModifyPlayrateIdle)
	{
		FLOAT rateError = TargetIdleRate - CurrentIdleRate;
		if (!appIsNearlyZero(rateError, KINDA_SMALL_NUMBERF))
		{
			FLOAT thisDelta = Min(deltaTime * IdleRateTransitionSpeed, Abs(rateError));
			CurrentIdleRate += thisDelta * Sgn(rateError);
		}
	}

	FLOAT weightError = TargetBreakerWeight - CurrentBreakerWeight;
	if (!appIsNearlyZero(weightError, KINDA_SMALL_NUMBERF))
	{
		FLOAT thisDelta = Min(deltaTime * BreakerWeightTransitionSpeed, Abs(weightError));
		CurrentBreakerWeight += thisDelta * Sgn(weightError);
	}

	// Apply results
	
	if (bModifyPlayrateIdle)
	{
		UAnimNodeSequence* animNodeSeq = Cast<UAnimNodeSequence>(Children(0).Anim);
		if (animNodeSeq)
		{
			animNodeSeq->Rate = CurrentIdleRate;
		}
		else
		{
			warnf(TEXT("UOLAnimSmartIdle - can't timescale idle clip"));
		}
	}

	Children(0).Weight = 1.0f - CurrentBreakerWeight;
	Children(1).Weight = CurrentBreakerWeight;

	Super::TickAnim(deltaTime);
}

FLOAT UOLAnimSmartIdle::GetSliderPosition(INT SliderIndex, INT ValueIndex)
{
	return CurrentBreakerWeight;
}

FString UOLAnimSmartIdle::GetSliderDrawValue(INT SliderIndex)
{
	return FString::Printf( TEXT("W: %.0f%%, R: %.2fx"), 100.0f*CurrentBreakerWeight, CurrentIdleRate);
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimCycleBreaker
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimCycleBreaker::OnBecomeRelevant()
{
	Super::OnBecomeRelevant();
	NextBreakerWeightTransition = 0.0f;
}

void UOLAnimCycleBreaker::InitAnim(USkeletalMeshComponent* MeshComp, UAnimNodeBlendBase* Parent)
{
	Super::InitAnim(MeshComp, Parent);

	ValidBreakers.Empty();

	AActor* outer = (MeshComp && MeshComp->GetOuter()) ? Cast<AActor>(MeshComp->GetOuter()) : NULL;
	
	for (INT i = 0; i < Breakers.Num(); i++)
	{
		if (Children.Num() <= i+1 || !Children(i+1).Anim)
		{
			continue;
		}

		if (Breakers(i).SpecificClasses.Num() == 0)
		{
			ValidBreakers.AddItem(i);
		}
		else if (outer)
		{
			for (INT j = 0; j < Breakers(i).SpecificClasses.Num(); j++)
			{
				if (outer->IsA(Breakers(i).SpecificClasses(j)))
				{
					ValidBreakers.AddItem(i);
					break;
				}
			}
		}
	}
}

void UOLAnimCycleBreaker::TickAnim(FLOAT deltaTime)
{
	if (Children.Num()-1 != Breakers.Num())
	{
		return;
	}

	FLOAT curTime = GWorld->GetTimeSeconds();

	if (curTime >= NextBreakerWeightTransition)
	{
		ActivateNewBreaker();
	}

	UpdateBreakers(deltaTime);
	
	// Apply results

	FLOAT totalBreakerWeight = 0.0f;
	for (INT i = 0; i < Breakers.Num(); i++)
	{
		FCycleBreaker& breaker = Breakers(i);
		totalBreakerWeight += breaker.CurrentWeight;
	}

	FLOAT defaultWeight = Max(0.0f, 1.0f - totalBreakerWeight);
	FLOAT totalWeight = totalBreakerWeight + defaultWeight;

	Children(0).Weight = defaultWeight;
	for (INT i = 1; i < Children.Num(); i++)
	{
		if (i <= Breakers.Num())
		{
			FCycleBreaker& breaker = Breakers(i-1);
			Children(i).Weight = breaker.CurrentWeight / totalWeight;
		}
		else
		{
			Children(i).Weight = 0.0f;
		}
	}

	Super::TickAnim(deltaTime);
}

void UOLAnimCycleBreaker::ActivateNewBreaker()
{
	FLOAT defaultDiceRoll = appFrand();
	FLOAT newBreakerWeight = 0.0f;

	UBOOL bDefaultCycle = (defaultDiceRoll <= DefaultCycleBias);

	INT breakerIdx = -1;

	if (!bDefaultCycle)
	{
		FLOAT totalProbability = 0.0f;
		for (INT i = 0; i < ValidBreakers.Num(); i++)
		{
			const INT bIdx = ValidBreakers(i);
			totalProbability += Breakers(bIdx).Probability;
		}

		FLOAT selectCursor = RandRange(0.0f, totalProbability);
		FLOAT accCursor = 0.0f;
		for (INT i = 0; i < ValidBreakers.Num(); i++)
		{
			const INT bIdx = ValidBreakers(i);
			accCursor += Breakers(bIdx).Probability;

			if (accCursor >= selectCursor)
			{
				breakerIdx = bIdx;
				break;
			}
		}
	}

	if (bDefaultCycle || breakerIdx < 0)
	{
		// No breaker
		NextBreakerWeightTransition = GWorld->GetTimeSeconds() + DefaultStretchDuration;

		for (INT i = 0; i < Breakers.Num(); i++)
		{
			Breakers(i).TargetWeight = 0.0f;
		}
	}
	else
	{
		FCycleBreaker& breaker = Breakers(breakerIdx);

		FLOAT breakerWeight = RandRange(breaker.MinWeight, breaker.MaxWeight);

		for (INT i = 0; i < Breakers.Num(); i++)
		{
			Breakers(i).TargetWeight = (i == breakerIdx) ? breakerWeight : 0.0f;
		}

		FLOAT animRate = RandRange(breaker.MinRate, breaker.MaxRate);

		UAnimNodeSequence* animNodeSeq = GetAnimNodeForBreaker(breakerIdx);

		FLOAT breakerDuration = 0.0f;

		if (breaker.bPlayStartToEnd)
		{
			if (animNodeSeq)
			{
				animNodeSeq->PlayAnim(FALSE, animRate);

				if (animNodeSeq->AnimSeq)
				{
					breakerDuration = animNodeSeq->AnimSeq->SequenceLength / animRate;
				}
				else
				{
					warnf(TEXT("UOLAnimCycleBreaker - Breaker %i has no valid anim seq"), breakerIdx);
				}
			}
			else
			{
				warnf(TEXT("UOLAnimCycleBreaker - Breaker %i has no valid anim node"), breakerIdx);
			}
		}
		else
		{
			if (animNodeSeq && animNodeSeq->AnimSeq)
			{
				check(breaker.MinDuration >= 0.0f && breaker.MaxDuration >= breaker.MinDuration);
				breakerDuration = RandRange(breaker.MinDuration, breaker.MaxDuration);

				animNodeSeq->Rate = animRate;
			}
			else
			{
				warnf(TEXT("UOLAnimCycleBreaker - Breaker %i has no valid anim node"), breakerIdx);
			}
		}

		NextBreakerWeightTransition = GWorld->GetTimeSeconds() + breakerDuration;

		if (breaker.LayerProbability > 0.01f && appFrand() < breaker.LayerProbability)
		{
			// Allow a layer

			FLOAT totalLayeredProbability = 0.0f;
			for (INT i = 0; i < ValidBreakers.Num(); i++)
			{
				const INT bIdx = ValidBreakers(i);
				if (bIdx != breakerIdx && Breakers(bIdx).LayerProbability > 0.01f)
				{
					totalLayeredProbability += Breakers(bIdx).Probability;
				}
			}

			FLOAT selectCursor = RandRange(0.0f, totalLayeredProbability);
			FLOAT accCursor = 0.0f;
			INT layerIdx = -1;
			for (INT i = 0; i < ValidBreakers.Num(); i++)
			{
				const INT curIdx = ValidBreakers(i);
				if (curIdx != breakerIdx && Breakers(curIdx).LayerProbability > 0.01f)
				{
					accCursor += Breakers(curIdx).Probability;

					if (accCursor >= selectCursor)
					{
						layerIdx = curIdx;
						break;
					}
				}
			}

			if (layerIdx >= 0)
			{
				FCycleBreaker& layerBreaker = Breakers(layerIdx);
				layerBreaker.TargetWeight = RandRange(layerBreaker.MinWeight, layerBreaker.MaxWeight);

				UAnimNodeSequence* layerAnimNodeSeq = GetAnimNodeForBreaker(layerIdx);
				layerAnimNodeSeq->Rate = RandRange(layerBreaker.MinRate, layerBreaker.MaxRate);
			}
		}
	}	
}

UAnimNodeSequence* UOLAnimCycleBreaker::GetAnimNodeForBreaker(INT breakerIdx)
{
	UAnimNodeSequence* animNodeSeq = Cast<UAnimNodeSequence>(Children(breakerIdx+1).Anim);

	if (!animNodeSeq)
	{
		UAnimNodeBlendPerBone* bpbNode = Cast<UAnimNodeBlendPerBone>(Children(breakerIdx+1).Anim);
		if (bpbNode && bpbNode->Children.Num() >= 2)
		{
			animNodeSeq = Cast<UAnimNodeSequence>(bpbNode->Children(1).Anim);			
		}
	}

	if (animNodeSeq && !animNodeSeq->AnimSeq && animNodeSeq->AnimSeqName != NAME_None)
	{
		animNodeSeq->SetAnim(animNodeSeq->AnimSeqName);
	}

	return animNodeSeq;
}

void UOLAnimCycleBreaker::UpdateBreakers(FLOAT deltaTime)
{
	for (INT i = 0; i < Breakers.Num(); i++)
	{
		FCycleBreaker& breaker = Breakers(i);
		FLOAT weightError = breaker.TargetWeight - breaker.CurrentWeight;
		if (!appIsNearlyZero(weightError, KINDA_SMALL_NUMBERF))
		{
			FLOAT transSpeed = (weightError > 0.0f) ? breaker.BlendInSpeed : breaker.BlendOutSpeed;
			FLOAT thisDelta = Min(deltaTime * transSpeed, Abs(weightError));
			breaker.CurrentWeight += thisDelta * Sgn(weightError);
		}
	}
}

void UOLAnimCycleBreaker::OnAddChild(INT newIdx)
{
	Super::OnAddChild(newIdx);

	if (newIdx >= 1)
	{
		INT bIdx = newIdx - 1;
		if (bIdx < Breakers.Num())
		{
			Breakers.InsertZeroed(bIdx, 1);
		}
		else
		{
			Breakers.AddZeroed(1);
		}

		UOLAnimCycleBreaker* defaultObj = Cast<UOLAnimCycleBreaker>(StaticClass()->GetDefaultObject());
		Breakers(bIdx) = defaultObj->BreakerTemplate;
	}
}

void UOLAnimCycleBreaker::OnRemoveChild(INT remIdx)
{
	Super::OnRemoveChild(remIdx);

	if (remIdx > 0 && remIdx <= Breakers.Num())
	{
		INT bIdx = remIdx - 1;

		Breakers.Remove(bIdx);
	}
}

FLOAT UOLAnimCycleBreaker::GetSliderPosition(INT SliderIndex, INT ValueIndex)
{
	FLOAT maxBreakerWeight = 0.0f;
	for (INT i = 0; i < Breakers.Num(); i++)
	{
		FCycleBreaker& breaker = Breakers(i);
		maxBreakerWeight = Max(maxBreakerWeight, breaker.CurrentWeight);
	}
	return maxBreakerWeight;
}

FString UOLAnimCycleBreaker::GetSliderDrawValue(INT SliderIndex)
{
	FString breakerTxt(TEXT(""));

	UBOOL bFirst = TRUE;

	for (INT i = 0; i < Breakers.Num(); i++)
	{
		FCycleBreaker& breaker = Breakers(i);

		if (breaker.CurrentWeight > 0.01f)
		{
			if (bFirst)
			{
				breakerTxt = FString::Printf( TEXT("#%i (%.0f%%)"), i, 100.0f*breaker.CurrentWeight);
				bFirst = FALSE;
			}
			else
			{
				breakerTxt += FString::Printf( TEXT(", #%i (%.0f%%)"), i, 100.0f*breaker.CurrentWeight);
			}
		}
	}
	
	return breakerTxt;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimSmarterIdle
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimSmarterIdle::OnBecomeRelevant()
{
	Super::OnBecomeRelevant();

	CurrentIdleRate = 1.0f;
	TargetIdleRate = 1.0f;
	NextIdleRateTransition = 0.0f;	
}

void UOLAnimSmarterIdle::TickAnim(FLOAT deltaTime)
{
	FLOAT curTime = GWorld->GetTimeSeconds();

	if (bModifyPlayrateIdle && curTime >= NextIdleRateTransition)
	{
		TargetIdleRate = RandRange(IdleMinRate, IdleMaxRate);
		FLOAT thisRateDuration = RandRange(IdleRateMinDuration, IdleRateMaxDuration);
		NextIdleRateTransition = curTime + thisRateDuration;
	}

	if (bModifyPlayrateIdle)
	{
		FLOAT rateError = TargetIdleRate - CurrentIdleRate;
		if (!appIsNearlyZero(rateError, KINDA_SMALL_NUMBERF))
		{
			FLOAT thisDelta = Min(deltaTime * IdleRateTransitionSpeed, Abs(rateError));
			CurrentIdleRate += thisDelta * Sgn(rateError);
		}
	}

	if (bModifyPlayrateIdle)
	{
		UAnimNodeSequence* animNodeSeq = Cast<UAnimNodeSequence>(Children(0).Anim);
		if (animNodeSeq)
		{
			animNodeSeq->Rate = CurrentIdleRate;
		}
		else
		{
			warnf(TEXT("UOLAnimSmartIdle - can't timescale idle clip"));
		}
	}

	Super::TickAnim(deltaTime);
}

FString UOLAnimSmarterIdle::GetSliderDrawValue(INT SliderIndex)
{
	FString breakerTxt = FString::Printf( TEXT("Idle (@%.2fx)"), CurrentIdleRate);

	for (INT i = 0; i < Breakers.Num(); i++)
	{
		FCycleBreaker& breaker = Breakers(i);

		if (breaker.CurrentWeight > 0.01f)
		{
			breakerTxt += FString::Printf( TEXT(", #%i (%.0f%%)"), i, 100.0f*breaker.CurrentWeight);
		}
	}

	return breakerTxt;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimNotify_ModifyPlayrate
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimNotify_ModifyPlayrate::Notify( UAnimNodeSequence* NodeSeq )
{
	if( !GWorld->HasBegunPlay() )
	{
		debugf( NAME_Log, TEXT("Editor: skipping UOLAnimNotify_ModifyPlayrate %s"), *GetName() );
		return;
	}

	NodeSeq->Rate = NewRate;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimNotify_NonFatalDamage
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimNotify_NonFatalDamage::Notify( UAnimNodeSequence* NodeSeq )
{
	USkeletalMeshComponent* SkelComp = NodeSeq->SkelComponent;
	check( SkelComp );

	AOLHero* Owner = Cast<AOLHero>(SkelComp->GetOwner());

	if (Owner != NULL && !Owner->bIsDummyPawn && GWorld->GetTimeSeconds() > Owner->LastScriptedDamageEffectTime + 0.5f)
	{
		INT dmg = Min(DamageAmount, (INT)Owner->PreciseHealth - 5); // we leave a buffer of 5 health
		Owner->eventTakeDamage(dmg, NULL, FVector(0.0f), FVector(0.0f), UOLDmgType_Scripted::StaticClass());
		Owner->LastScriptedDamageEffectTime = GWorld->GetTimeSeconds();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimNotify_PlayWeaponAnimation
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimNotify_PlayWeaponAnimation::Notify( UAnimNodeSequence* NodeSeq )
{
	USkeletalMeshComponent* SkelComp = NodeSeq->SkelComponent;
	check( SkelComp );

	AOLEnemyPawn* enemyPawn = Cast<AOLEnemyPawn>(SkelComp->GetOwner());

	if (enemyPawn)
	{
		enemyPawn->OnPlayWeaponAnimNotify(NodeSeq);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimRandomCycle
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimRandomCycle::OnBecomeRelevant()
{
	Super::OnBecomeRelevant();

	bIsMaster = !IsShadowProxy(this);

	if (bIsMaster)
	{
		ShadowNode = Cast<UOLAnimRandomCycle>(FindShadowNode(this));
		StartNewCycle();
	}
}

void UOLAnimRandomCycle::TickAnim(FLOAT deltaTime)
{
	FLOAT curTime = GWorld->GetTimeSeconds();

	if (bIsMaster && curTime >= NextCycleCheckTime)
	{
		if (ActiveChildIndex >= 0)
		{
			UAnimNodeSequence* nodeSeq = GetAnimNodeForChild(ActiveChildIndex);

			if (nodeSeq && nodeSeq->AnimSeq)
			{
				if (nodeSeq->CurrentTime < 0.1f || (nodeSeq->CurrentTime + deltaTime) >= nodeSeq->AnimSeq->SequenceLength)
				{
					StartNewCycle();
				}
			}
		}
	}
	
	Super::TickAnim(deltaTime);
}

void UOLAnimRandomCycle::StartNewCycle()
{
	INT cycleIdx = -1;

	FLOAT defaultDiceRoll = appFrand();

	FLOAT defaultCycleBias = DefaultCycleBiasWalking;

	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;
	
	if (Owner)
	{
		if (Owner->IsRunning())
		{
			defaultCycleBias = DefaultCycleBiasRunning;
		}

		defaultCycleBias = LerpClamped(Owner->HobblingIntensity, 1.0f, defaultCycleBias); // diminish stumbles as hobble intensity decreases
	}

	if (defaultDiceRoll <= defaultCycleBias)
	{
		cycleIdx = 0; // biasing to default
	}
	else
	{
		FLOAT randomRange = (1.0f - defaultCycleBias);
		FLOAT randomPart = defaultDiceRoll - defaultCycleBias;
		FLOAT randomStep = randomRange / (FLOAT)Children.Num();

		cycleIdx = (INT)(randomPart / randomStep);
		cycleIdx = Clamp(cycleIdx, 0, Children.Num() - 1); // I think the 1.00 random needs this
	}

	StartChild(cycleIdx);

	if (ShadowNode)
	{
		ShadowNode->StartChild(cycleIdx);
	}

	NextCycleCheckTime = GWorld->GetTimeSeconds() + 0.25f;
}

void UOLAnimRandomCycle::StartChild(INT childIdx)
{
	if (childIdx != ActiveChildIndex)
	{
		UAnimNodeSequence* nodeSeq = GetAnimNodeForChild(childIdx);

		if (nodeSeq && nodeSeq->AnimSeq)
		{
			nodeSeq->SetPosition(0.0f, FALSE);
			nodeSeq->PlayAnim(TRUE);

			SetActiveChild(childIdx, BlendTime);
		}
	}
}

UAnimNodeSequence* UOLAnimRandomCycle::GetAnimNodeForChild(INT childIdx)
{
	UAnimNodeSequence* animNodeSeq = Cast<UAnimNodeSequence>(Children(childIdx).Anim);

	if (!animNodeSeq)
	{
		UAnimNodeBlendPerBone* bpbNode = Cast<UAnimNodeBlendPerBone>(Children(childIdx).Anim);
		if (bpbNode && bpbNode->Children.Num() >= 2)
		{
			animNodeSeq = Cast<UAnimNodeSequence>(bpbNode->Children(1).Anim);
		}
	}

	if (animNodeSeq && !animNodeSeq->AnimSeq && animNodeSeq->AnimSeqName != NAME_None)
	{
		animNodeSeq->SetAnim(animNodeSeq->AnimSeqName);
	}

	return animNodeSeq;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLAnimBlendByHobblingIntensity
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLAnimBlendByHobblingIntensity::TickAnim(FLOAT DeltaSeconds)
{
	AOLHero* Owner = SkelComponent ? Cast<AOLHero>(SkelComponent->GetOwner()) : NULL;

	if (Owner)
	{
		Child2Weight = Min(Owner->HobblingIntensity, 1.0f - Owner->CornerPeek.IKStrength);
	}

	Super::TickAnim(DeltaSeconds);
}

FLOAT UOLAnimCustomBlend::PlayCustomBlendUC(FName AnimA, FName AnimB, FLOAT WeightA, FLOAT BlendIn, FLOAT BlendOut)
{
	return PlayCustomBlend(AnimA, AnimB, WeightA, BlendIn, BlendOut);
}

