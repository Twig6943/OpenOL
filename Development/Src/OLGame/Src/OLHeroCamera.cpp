#include "OLGame.h"
#include "OLUtilities.h"

IMPLEMENT_CLASS(UOLHeroCamera);

UBOOL GSmoothCamera = FALSE;

//////////////////////////////////////////////////////////////////////////
// UOLHeroCamera
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void UOLHeroCamera::Init(AOLHero* hero)
{
	Hero = hero;
	appMemZero(ViewCS);
	appMemZero(ViewWS);
	InputYaw = 0.0f;
	InputPitch = 0.0f;
	NeckOffsetFwd = 0.0f;
	NeckOffsetSide = 0.0f;
	LookBackRatio = 0.0f;
	LeanPushingRatio = 0.0f;
	PendingYawCorrection = 0.0f;
	bUserControlsPawnRotation = FALSE;
	bActiveLookAt = FALSE;
}

void UOLHeroCamera::LatchInput(const FRotator& deltaRot)
{
	if (GIsGame && !GIsPlayInEditorWorld && GWorld->GetTimeSeconds() < (Hero->SpawnTime + 1.0f))
	{
		// For one second after spawning, no camera control
		return;
	}

	FRotator deltaRotNrm = deltaRot.GetNormalized();
	InputYaw = UNR_TO_DEG * deltaRotNrm.Yaw;
	InputPitch = UNR_TO_DEG * deltaRotNrm.Pitch;
}

void UOLHeroCamera::SetView(const FVector& location, const FRotator& rotation)
{
	ViewCS.Loc = location - BaseLocation;
	ViewCS.SetRot((rotation - BaseRotation).GetNormalized());
	NeckOffsetFwd = 0.0f;
	NeckOffsetSide = 0.0f;
	LookBackRatio = 0.0f;
	LeanPushingRatio = 0.0f;
	bActiveLookAt = FALSE;
	PendingYawCorrection = 0.0f;
	bUserControlsPawnRotation = FALSE;
}

FLOAT UOLHeroCamera::ConsumeYawCorrection(FLOAT deltaTime)
{
	if (appIsNearlyZero(PendingYawCorrection, KINDA_SMALL_NUMBERF))
	{
		PendingYawCorrection = 0.0f;
		return 0.0f;
	}

	TWEAKABLE FLOAT approachFactor = 0.9f;
	FLOAT newCorrection = Utils::Approach(PendingYawCorrection, 0.0f, approachFactor, deltaTime);
	FLOAT delta = PendingYawCorrection - newCorrection;
	PendingYawCorrection = newCorrection;

	return delta;	
}

void UOLHeroCamera::ActivateCameraShake(FCameraShakeData& shakeData, const FVector& sourceLocation)
{
	ShakeData = shakeData;
	ShakeData.bActive = TRUE;
	ShakeData.StartedTime = GWorld->GetTimeSeconds();
	ShakeData.SourceLocation = sourceLocation;
	ShakeData.YawOffset = 0.0f;
	ShakeData.PitchOffset = 0.0f;
	ShakeData.RollOffset = 0.0f;

	AOLPlayerController* OLPC = Utils::GetOLPC();
	if (OLPC)
	{
		TWEAKABLE FLOAT MaxShakeDistance = 1500.0f;
		FLOAT distanceFactor = ShakeData.bPositionless ? 1.0f : Saturate(1.0f - ShakeData.SourceLocation.Distance(Hero->Location) / MaxShakeDistance);
		distanceFactor = distanceFactor * distanceFactor; // square it up
		INT ffAmplitude = (INT)(100.0f * distanceFactor * shakeData.Intensity);
		CamShakeFFWaveform->Samples(0).LeftAmplitude = ffAmplitude;
		CamShakeFFWaveform->Samples(0).RightAmplitude = ffAmplitude;
		CamShakeFFWaveform->Samples(0).Duration = (shakeData.Duration > 0.0f ? shakeData.Duration : 0.3f);
		OLPC->eventClientPlayForceFeedbackWaveform(CamShakeFFWaveform);
	}
}

void UOLHeroCamera::StopCameraShake()
{
	appMemZero(ShakeData);

	AOLPlayerController* OLPC = Utils::GetOLPC();
	if (OLPC)
	{
		OLPC->eventClientStopForceFeedbackWaveform(CamShakeFFWaveform);
	}
}

void UOLHeroCamera::ActivateTargettedYawSmoothing(FLOAT targetYawWS, FLOAT smoothingTime)
{
	TargettedYawSmoothing.bActive = TRUE;
	TargettedYawSmoothing.TargetAngleWS	= targetYawWS;
	TargettedYawSmoothing.StartTime = GWorld->GetTimeSeconds();
	TargettedYawSmoothing.Duration = smoothingTime;
	TargettedYawSmoothing.StartAngleWS = ViewWS.Yaw;
	ViewCS.Yaw = 0.0f;
}

void UOLHeroCamera::ActivateCameraSmoothing(FLOAT duration)
{
	BaseViewSmoothing.bActive = TRUE;
	BaseViewSmoothing.ViewWS = ViewWS;
	BaseViewSmoothing.StartTime = GWorld->GetTimeSeconds();
	BaseViewSmoothing.Duration = duration;	
}

void UOLHeroCamera::Update(FLOAT deltaTime)
{
	if (deltaTime <= 0.0f)
	{
		return;
	}

	FCamView newViewWS = ViewWS;
	FCamView newViewCS = ViewCS;

	FLOAT inputYaw = InputYaw;
	FLOAT inputPitch = InputPitch;

	// Interpolate BoneSuppression toward target (1 when GSmoothCamera+non-cinematic, 0 otherwise)
	UBOOL bSuppressActive = GSmoothCamera && (Hero->LocomotionMode != LM_Cinematic && Hero->LocomotionMode != LM_SpecialMove && Hero->LocomotionMode != LM_Door && Hero->LocomotionMode != LM_Locker && Hero->LocomotionMode != LM_Bed && Hero->LocomotionMode != LM_Ladder && Hero->LocomotionMode != LM_Squeeze && Hero->LocomotionMode != LM_Pushing && Hero->LocomotionMode != LM_LedgeHang && Hero->LocomotionMode != LM_LedgeWalk && Hero->LocomotionMode != LM_LookBack && Hero->LocomotionMode != LM_ContextualLean);
	TWEAKABLE FLOAT SuppressionInterpSpeed = 5.0f;
	BoneSuppression = FInterpTo(BoneSuppression, bSuppressActive ? 1.0f : 0.0f, deltaTime, SuppressionInterpSpeed);

	if (bLocalSpacePlayerControl)	
	{
		GetAnimatedView(newViewWS);
		UpdateLimitsLocalRef();
		ApplyInputDelta(newViewCS, inputYaw, inputPitch);
		LimitViewCS(newViewCS);
		UpdateViewWSLocalFrame(newViewWS, newViewCS);
	}
	else
	{
		UpdateYawCorrection(deltaTime, newViewCS);
		GetAnimatedView(newViewWS);
		ApplyCameraSmoothing(newViewWS);
		ApplyLookAt(deltaTime, newViewWS, newViewCS);
		ApplyLookBack(deltaTime, newViewWS);
		ApplyProceduralLean(deltaTime, newViewWS);
		ApplyYawCorrection(deltaTime, newViewWS);
		UpdateShake(deltaTime);
		ApplyShake(deltaTime, newViewWS);
		UpdateBaseRotation(newViewWS);
		UpdateLimits(deltaTime, newViewWS);
		AttenuateInputs(deltaTime, inputYaw, inputPitch, newViewCS);
		ApplyInputDelta(newViewCS, inputYaw, inputPitch);
		ApplySoftLimits(deltaTime, inputYaw, inputPitch, newViewCS);
		LimitViewCS(newViewCS);
		UpdateNeckOffset(deltaTime);
		ApplyNeckOffset(newViewCS);
		UpdateViewWS(newViewWS, newViewCS);	
		ApplyTargettedWSSmoothing(newViewWS);
		LimitViewWS(newViewWS);
	}
	
	ViewCS = newViewCS;
	ViewWS = newViewWS;
}

void UOLHeroCamera::UpdateYawCorrection(FLOAT deltaTime, FCamView& viewCS)
{
	const FGameplayParams& gp = Hero->GP();

	if (!bUserControlsPawnRotation && gp.CameraMode == CRM_UserControlled)
	{
		// Stop controlling the pawn rotation. Copy the current CS yaw to the PendingYawCorrection
		PendingYawCorrection = viewCS.Yaw;
		viewCS.Yaw = 0.0f;
	}
	else if (!bUserControlsPawnRotation)
	{
		PendingYawCorrection = 0.0f;
	}

	bUserControlsPawnRotation = (gp.CameraMode == CRM_UserControlled);
}

void UOLHeroCamera::UpdateNeckOffset(FLOAT deltaTime)
{
	const FGameplayParams& gp = Hero->GP();

	FLOAT targetOffsetFwd = Hero->LocomotionModeParams[Hero->LocomotionMode].NeckOffsetFwd;
	FLOAT targetOffsetSide = Hero->LocomotionModeParams[Hero->LocomotionMode].NeckOffsetSide;

	if (bActiveLookAt)
	{
		targetOffsetFwd = 0.0f;
		targetOffsetSide = 0.0f;
	}

	TWEAKABLE FLOAT ApproachCoeff = 0.99f;
	NeckOffsetFwd = Utils::Approach(NeckOffsetFwd, targetOffsetFwd, ApproachCoeff, deltaTime);
	NeckOffsetSide = Utils::Approach(NeckOffsetSide, targetOffsetSide, ApproachCoeff, deltaTime);

	if (Hero->SpecialMove == SMT_BedReload && Hero->CamcorderState == CCS_ReloadingActive)
	{
		// hack - no interp
		NeckOffsetFwd = 0.0f;
		NeckOffsetSide = 0.0f;
	}

	if (targetOffsetFwd > 0.0f || targetOffsetSide > 0.0f)
	{
		NeckOffsetBaseYaw = ViewCS.Yaw;
	}
}

FLOAT UOLHeroCamera::EvaluateWave(const FCameraWaveData& wave, FLOAT t) const
{
	return wave.Amplitude * appSin(wave.Frequency * t + 2.0f*PI*wave.StartPhase);
}

void UOLHeroCamera::UpdateShake(FLOAT deltaTime)
{
	if (ShakeData.bActive)
	{
		FLOAT elapsedTime = GWorld->GetTimeSeconds() - ShakeData.StartedTime;

		if (elapsedTime >= ShakeData.Duration)
		{
			ShakeData.bActive = FALSE;
		}
		else
		{
			TWEAKABLE FLOAT MaxShakeDistance = 1500.0f;
			FLOAT distanceFactor = ShakeData.bPositionless ? 1.0f : Saturate(1.0f - ShakeData.SourceLocation.Distance(Hero->Location) / MaxShakeDistance);
			distanceFactor = distanceFactor * distanceFactor; // square it up
			FLOAT scaleFactor = ShakeData.Intensity * distanceFactor;
			FLOAT fadeInFactor = (ShakeData.FadeInTime > 0.0f && elapsedTime < ShakeData.FadeInTime) ? Utils::SmootherStep(elapsedTime / ShakeData.FadeInTime) : 1.0f;
			FLOAT fadeOutFactor = (ShakeData.FadeOutTime > 0.0f && (ShakeData.Duration - elapsedTime) < ShakeData.FadeOutTime) ? Utils::SmootherStep((ShakeData.Duration - elapsedTime) / ShakeData.FadeOutTime) : 1.0f;
			scaleFactor *= Min(fadeInFactor, fadeOutFactor);

			ShakeData.YawOffset = scaleFactor * (EvaluateWave(ShakeData.YawWaveA, elapsedTime) + EvaluateWave(ShakeData.YawWaveB, elapsedTime));
			ShakeData.PitchOffset = scaleFactor * (EvaluateWave(ShakeData.PitchWaveA, elapsedTime) + EvaluateWave(ShakeData.PitchWaveB, elapsedTime));
			ShakeData.RollOffset = scaleFactor * (EvaluateWave(ShakeData.RollWaveA, elapsedTime) + EvaluateWave(ShakeData.RollWaveB, elapsedTime));
		}
	}
}

void UOLHeroCamera::UpdateLimitsLocalRef()
{
	const FGameplayParams& gp = Hero->GP();
	const FCameraParameters& camParams = Hero->CamParams;

	FLOAT minPitchCS = camParams.MinPitchCS;
	FLOAT maxPitchCS = camParams.MaxPitchCS;	
	FLOAT minPitchWS = camParams.MinPitchWS;
	FLOAT maxPitchWS = camParams.MaxPitchWS;
	FLOAT minYawCS = camParams.MinYaw;
	FLOAT maxYawCS = camParams.MaxYaw;

	Limits.MinPitch = minPitchCS;
	Limits.MaxPitch = maxPitchCS;
	Limits.MinYaw = minYawCS;
	Limits.MaxYaw = maxYawCS;

	SoftLimits = Limits;
}

void UOLHeroCamera::UpdateLimits(FLOAT deltaTime, const FCamView& viewWS)
{
	const FGameplayParams& gp = Hero->GP();
	const FCameraParameters& camParams = Hero->CamParams;

	FViewLimits targetLimits;

	UBOOL bLocalPitchAllowed = (gp.CameraMode == CRM_Limited || gp.CameraMode == CRM_UserControlled || gp.CameraMode == CRM_Spring || gp.CameraMode == CRM_Locked);
	UBOOL bLocalYawAllowed = (gp.CameraMode == CRM_Limited || gp.CameraMode == CRM_Spring || gp.CameraMode == CRM_Locked);

	FLOAT minPitchCS = camParams.MinPitchCS;
	FLOAT maxPitchCS = camParams.MaxPitchCS;	
	FLOAT minPitchWS = camParams.MinPitchWS;
	FLOAT maxPitchWS = camParams.MaxPitchWS;
	FLOAT minYawCS = camParams.MinYaw;
	FLOAT maxYawCS = camParams.MaxYaw;

	// Give the pawn a chance to modify the limits
	Hero->ApplyDynamicCameraLimits(BaseRotation, viewWS, Limits, minYawCS, maxYawCS, minPitchCS, maxPitchCS, minPitchWS, maxPitchWS);
	
	if (!bLocalPitchAllowed)
	{
		minPitchCS = 0.0f;
		maxPitchCS = 0.0f;
		minPitchWS = -360.0f;
		maxPitchWS = 360.0f;
	}

	if (!bLocalYawAllowed)
	{
		minYawCS = 0.0f;
		maxYawCS = 0.0f;
	}	
	
	// Fold the WS limits back to cam space
	targetLimits.MinPitch = Max(minPitchCS, minPitchWS - viewWS.Pitch);
	targetLimits.MaxPitch = Min(maxPitchCS, maxPitchWS - viewWS.Pitch);	
	targetLimits.MinYaw = minYawCS;
	targetLimits.MaxYaw = maxYawCS;
	
	FLOAT LimitsApproachCoeff = Hero->GP().ViewLimitsApproachCoeff;
	Limits.MinPitch = (targetLimits.MinPitch < Limits.MinPitch) ? targetLimits.MinPitch : Utils::Approach(Limits.MinPitch, targetLimits.MinPitch, LimitsApproachCoeff, deltaTime);
	Limits.MaxPitch = (targetLimits.MaxPitch > Limits.MaxPitch) ? targetLimits.MaxPitch : Utils::Approach(Limits.MaxPitch, targetLimits.MaxPitch, LimitsApproachCoeff, deltaTime);
	Limits.MinYaw = (targetLimits.MinYaw < Limits.MinYaw) ? targetLimits.MinYaw : Utils::Approach(Limits.MinYaw, targetLimits.MinYaw, LimitsApproachCoeff, deltaTime);
	Limits.MaxYaw = (targetLimits.MaxYaw > Limits.MaxYaw) ? targetLimits.MaxYaw : Utils::Approach(Limits.MaxYaw, targetLimits.MaxYaw, LimitsApproachCoeff, deltaTime);

	SoftLimits = Limits;
	FLOAT pitchRange = (Limits.MaxPitch - Limits.MinPitch);
	FLOAT yawRange = (Limits.MaxYaw - Limits.MinYaw);
	SoftZonePitch = Min(0.33f * pitchRange, ViewLimitsSoftZone);	
	SoftZoneYaw = Min(0.33f * yawRange, ViewLimitsSoftZone);
	SoftLimits.MinPitch = Limits.MinPitch + SoftZonePitch;
	SoftLimits.MaxPitch = Limits.MaxPitch - SoftZonePitch;
	SoftLimits.MinYaw = Limits.MinYaw + SoftZoneYaw;
	SoftLimits.MaxYaw = Limits.MaxYaw - SoftZoneYaw;
}

void UOLHeroCamera::GetAnimatedView(FCamView& viewWS)
{
	FVector boneLoc = Hero->Mesh->GetBoneLocation(CameraBoneName);
	FRotator boneRot = Hero->Mesh->GetBoneQuaternion(CameraBoneName).Rotator().GetNormalized();
	if (GSmoothCamera && Hero->LocomotionMode != LM_Cinematic) {
		if (BoneSuppression > 0.0f)
		{
			FLOAT pitchSuppress = (
				(Hero->LocomotionMode == LM_Walk || Hero->LocomotionMode == LM_Fall)
				&& Hero->ActiveCSA == NULL) ? BoneSuppression : 0.0f;
			boneRot.Pitch = appRound(boneRot.Pitch * (1.0f - pitchSuppress));
		}
		boneRot.Roll  = 0; //appRound(boneRot.Roll  * (1.0f - BoneSuppression));
	}
	viewWS.Loc = boneLoc;
	viewWS.SetRot(boneRot);
}

void UOLHeroCamera::ApplyCameraSmoothing(FCamView& viewWS)
{
	if (BaseViewSmoothing.bActive)
	{
		FLOAT elapsedTime = GWorld->GetTimeSeconds() - BaseViewSmoothing.StartTime;
		if (elapsedTime >= BaseViewSmoothing.Duration)
		{
			BaseViewSmoothing.bActive = FALSE;
		}
		else
		{
			FLOAT alpha = Saturate(elapsedTime / BaseViewSmoothing.Duration);
			FVector interpLoc = Lerp(BaseViewSmoothing.ViewWS.Loc, viewWS.Loc, alpha);
			FLOAT interpYaw = Utils::LerpAngle(BaseViewSmoothing.ViewWS.Yaw, viewWS.Yaw, alpha);
			FLOAT interpPitch = Utils::LerpAngle(BaseViewSmoothing.ViewWS.Pitch, viewWS.Pitch, alpha);
			FLOAT interpRoll = Utils::LerpAngle(BaseViewSmoothing.ViewWS.Roll, viewWS.Roll, alpha);
			viewWS.Loc = interpLoc;
			viewWS.Yaw = interpYaw;
			viewWS.Pitch = interpPitch;
			viewWS.Roll = interpRoll;
		}
	}
}

void UOLHeroCamera::ApplyLookAt(FLOAT deltaTime, FCamView& viewWS, FCamView& viewCS)
{
	UOLSeqAct_HeroControl* ctrl = Hero->HeroControl;
	if (ctrl && ctrl->LookAtTarget)
	{
		FLOAT duration = ctrl->LookAtBlendInTime;
		check(duration > 0.0f);
		FLOAT alpha = Saturate(ctrl->ElapsedTime / duration);
		alpha = Utils::SmootherStep(alpha);

		FRotator targetRot = (ctrl->LookAtTarget->Location - viewWS.Loc).Rotation();
		FRotator wsRot = ctrl->OriginalCamRotation + alpha*(targetRot - ctrl->OriginalCamRotation).GetNormalized();
		viewWS.SetRot(wsRot);

		bActiveLookAt = TRUE;
	}
	else if (bActiveLookAt)
	{
		viewCS.SetRot((Hero->EyeRotation - viewWS.Rotator()).GetNormalized());
		bActiveLookAt = FALSE;
	}
}

void UOLHeroCamera::ApplyLookBack(FLOAT deltaTime, FCamView& viewWS)
{
	FLOAT targetRatio = Hero->IsConsideredLookingBack() ? 1.0f : 0.0f;

	TWEAKABLE FLOAT ApproachCoeff = 0.99999f;
	LookBackRatio = Utils::Approach(LookBackRatio, targetRatio, ApproachCoeff, deltaTime);

	TWEAKABLE FLOAT AnimatedSideOffset = 15.0f;
		
	if (LookBackRatio > 0.001f)
	{		
		FLOAT yawOffset = LookBackRatio * (Hero->bLookingBackLeftSide ? -1.0f : 1.0f) * Hero->LookBackCamRotOffset;		

		FVector sideVec =  (Hero->bLookingBackLeftSide ? -1.0f : 1.0f) * Hero->Rotation.Right();
		
		FVector posOffset(0.0f);
		if (LookBackRatio > 0.999f)
		{
			posOffset = -Hero->LookBackCamBackOffset*Hero->CharForward + Hero->LookBackCamSideOffset * sideVec;
		}
		else
		{
			FVector sideOffset = appSin(LookBackRatio * PI) * AnimatedSideOffset * sideVec;
			posOffset = LookBackRatio * (-Hero->LookBackCamBackOffset*Hero->CharForward + Hero->LookBackCamSideOffset * sideVec) + sideOffset;			
		}

		viewWS.Yaw += yawOffset;
		viewWS.Loc += posOffset;
		
		viewWS.Normalize();
	}	
}

void UOLHeroCamera::ApplyProceduralLean(FLOAT deltaTime, FCamView& viewWS)
{
	FLOAT targetRatio = 0.0f;

	if (Hero->LocomotionMode == LM_Pushing && Hero->bLeaningRightPushing)
	{
		targetRatio = 1.0f;
	}
	else if (Hero->LocomotionMode == LM_Pushing && Hero->bLeaningLeftPushing)
	{
		targetRatio = -1.0f;
	}

	TWEAKABLE FLOAT ApproachCoeff = 0.999f;
	LeanPushingRatio = Utils::Approach(LeanPushingRatio, targetRatio, ApproachCoeff, deltaTime);

	TWEAKABLE FLOAT FwdOffsetLeft = 10.0f;
	TWEAKABLE FLOAT FwdOffsetRight = 15.0f;
	TWEAKABLE FLOAT SideOffsetLeft = -23.0f;
	TWEAKABLE FLOAT SideOffsetRight = 42.0f;

	if (Abs(LeanPushingRatio) > 0.001f)
	{		
		FVector targetPosOffset(0.0f); 
		if (LeanPushingRatio > 0.0f)
		{
			targetPosOffset = FwdOffsetRight*Hero->CharForward + SideOffsetRight*Hero->Rotation.Right();
		}
		else
		{
			targetPosOffset = FwdOffsetLeft*Hero->CharForward + SideOffsetLeft*Hero->Rotation.Right();
		}
		viewWS.Loc += Abs(LeanPushingRatio) * targetPosOffset;

		viewWS.Normalize();
	}
}

void UOLHeroCamera::ApplyShake(FLOAT deltaTime, FCamView& viewWS)
{
	if (ShakeData.bActive)
	{
		viewWS.Yaw += ShakeData.YawOffset;
		viewWS.Pitch += ShakeData.PitchOffset;
		viewWS.Roll += ShakeData.RollOffset;
	}
}

void UOLHeroCamera::ApplyYawCorrection(FLOAT deltaTime, FCamView& viewWS)
{
	viewWS.Yaw = Utils::NormalizeRotAngle(viewWS.Yaw + PendingYawCorrection);
}

void UOLHeroCamera::UpdateBaseRotation(const FCamView& viewWS)
{
	BaseLocation = viewWS.Loc;
	BaseRotation = viewWS.Rotator();
}

void UOLHeroCamera::AttenuateInput(FLOAT deltaTime, FLOAT& inout_input, FLOAT currentAngle, FLOAT limitAngle, FLOAT softZone)
{
	TWEAKABLE FLOAT Exp = 3.0f;
	FLOAT attenuation = 1.0f - Abs(currentAngle - limitAngle) / softZone;
	attenuation = appPow(Clamp(attenuation, 0.0f, 1.0f), Exp);
	inout_input = Utils::NormalizeRotAngle(inout_input * attenuation);
}

void UOLHeroCamera::AttenuateInputs(FLOAT deltaTime, FLOAT& inputYaw, FLOAT& inputPitch, FCamView& viewCS)
{
	if (SoftZonePitch > 0.0f)
	{
		if (viewCS.Pitch < SoftLimits.MinPitch && inputPitch < 0.0f)
		{		
			AttenuateInput(deltaTime, inputPitch, viewCS.Pitch, SoftLimits.MinPitch, SoftZonePitch);				
		}
		else if (viewCS.Pitch > SoftLimits.MaxPitch && inputPitch > 0.0f)
		{
			AttenuateInput(deltaTime, inputPitch, viewCS.Pitch, SoftLimits.MaxPitch, SoftZonePitch);
		}
	}

	if (SoftZoneYaw > 0.0f)
	{
		if (viewCS.Yaw < SoftLimits.MinYaw && inputYaw < 0.0f)
		{		
			AttenuateInput(deltaTime, inputYaw, viewCS.Yaw, SoftLimits.MinYaw, SoftZoneYaw);
		}
		else if (viewCS.Yaw > SoftLimits.MaxYaw && inputYaw > 0.0f)
		{
			AttenuateInput(deltaTime, inputYaw, viewCS.Yaw, SoftLimits.MaxYaw, SoftZoneYaw);
		}
	}
}

void UOLHeroCamera::ApplySpring(FLOAT deltaTime, FLOAT& currentAngle, FLOAT softLimitAngle, FLOAT softZone, FLOAT stiffness)
{
	FLOAT springDist = softLimitAngle - currentAngle;
	FLOAT springForce = springDist * stiffness;
	FLOAT springVel = springForce * deltaTime;
	FLOAT deltaAngle = springVel * deltaTime;
	currentAngle += deltaAngle;
}

void UOLHeroCamera::ApplySoftLimits(FLOAT deltaTime, FLOAT inputYaw, FLOAT inputPitch, FCamView& viewCS)
{	
	TWEAKABLE UBOOL bApplySoftLimits = FALSE; // TODO - Enable when controlled with gamepad, disable for mouse

	if (!bApplySoftLimits)
	{
		return;
	}

	TWEAKABLE FLOAT SoftZoneSpringMaxInputVel = 10.0f;

	if (viewCS.Pitch < SoftLimits.MinPitch)
	{		
		ApplySpring(deltaTime, viewCS.Pitch, SoftLimits.MinPitch, SoftZonePitch, SoftZoneStiffness);
	}
	else if (viewCS.Pitch > SoftLimits.MaxPitch)
	{
		ApplySpring(deltaTime, viewCS.Pitch, SoftLimits.MaxPitch, SoftZonePitch, SoftZoneStiffness);
	}

	if (viewCS.Yaw < SoftLimits.MinYaw)
	{		
		ApplySpring(deltaTime, viewCS.Yaw, SoftLimits.MinYaw, SoftZoneYaw, SoftZoneStiffness);
	}
	else if (viewCS.Yaw > SoftLimits.MaxYaw)
	{
		ApplySpring(deltaTime, viewCS.Yaw, SoftLimits.MaxYaw, SoftZoneYaw, SoftZoneStiffness);
	}

	UBOOL bNoInput = (deltaTime <= 0.0f || (Abs(inputPitch / deltaTime) < SoftZoneSpringMaxInputVel && Abs(inputYaw / deltaTime) < SoftZoneSpringMaxInputVel));
	if (bNoInput && Hero->GP().CameraMode == CRM_Spring)
	{
		// TODO: apply 2d spring
	}

	viewCS.Normalize();
}

void UOLHeroCamera::ApplyInputDelta(FCamView& viewCS, FLOAT inputYaw, FLOAT inputPitch)
{
	if (Hero->GP().CameraMode != CRM_Locked)
	{
		if (!Hero->HeadingLockedToCamera()) // If the heading is locked, the body turns first, and camera follows
		{
			viewCS.Yaw += inputYaw;
		}
		viewCS.Pitch += inputPitch;
		viewCS.Normalize();
	}
}

void UOLHeroCamera::LimitViewCS(FCamView& viewCS)
{
	viewCS.Pitch = Clamp(viewCS.Pitch, Limits.MinPitch, Limits.MaxPitch);
	viewCS.Yaw = Clamp(viewCS.Yaw, Limits.MinYaw, Limits.MaxYaw);
	viewCS.Roll = 0.0f;
}

void UOLHeroCamera::UpdateViewWS(FCamView& viewWS, const FCamView& viewCS)
{
	FRotationTranslationMatrix mtx(viewWS.Rotator(), viewWS.Loc);
	viewWS.Loc = mtx.TransformFVector(viewCS.Loc);
	viewWS.Yaw += viewCS.Yaw;
	viewWS.Pitch += viewCS.Pitch;
	viewWS.Roll += viewCS.Roll;
	viewWS.Normalize();
}

void UOLHeroCamera::UpdateViewWSLocalFrame(FCamView& viewWS, const FCamView& viewCS)
{
	FRotationTranslationMatrix mtx(viewWS.Rotator(), viewWS.Loc);
	viewWS.Loc = mtx.TransformFVector(viewCS.Loc);

	FQuat wsQuat = Hero->Mesh->GetBoneQuaternion(CameraBoneName);
	FQuat csQuat(viewCS.Rotator());

	wsQuat = wsQuat * csQuat;

	FRotator newWsRot = wsQuat.Rotator().GetNormalized();
	if (GSmoothCamera && Hero->LocomotionMode != LM_Cinematic) {
		if (BoneSuppression > 0.0f)
		{
			FLOAT pitchSuppress = (
				(Hero->LocomotionMode == LM_Walk || Hero->LocomotionMode == LM_Fall)
				&& Hero->ActiveCSA == NULL) ? BoneSuppression : 0.0f;
			newWsRot.Pitch = appRound(newWsRot.Pitch * (1.0f - pitchSuppress));
		}
		newWsRot.Roll  = 0; //appRound(newWsRot.Roll  * (1.0f - BoneSuppression));
	}
	viewWS.SetRot(newWsRot);
}

void UOLHeroCamera::ApplyTargettedWSSmoothing(FCamView& viewWS)
{
	if (TargettedYawSmoothing.bActive)
	{
		FLOAT elapsedTime = GWorld->GetTimeSeconds() - TargettedYawSmoothing.StartTime;
		FLOAT alpha = Saturate(elapsedTime / TargettedYawSmoothing.Duration);
		FLOAT deltaYaw = Utils::NormalizeRotAngle(TargettedYawSmoothing.TargetAngleWS - TargettedYawSmoothing.StartAngleWS);
		viewWS.Yaw = Utils::NormalizeRotAngle(TargettedYawSmoothing.StartAngleWS + Utils::SmootherStep(alpha) * deltaYaw);

		if (elapsedTime > TargettedYawSmoothing.Duration)
		{
			// this is the last frame of the smoothing, consider it still active to make sure we reach the target
			TargettedYawSmoothing.bActive = FALSE;
		}
	}
}

void UOLHeroCamera::ApplyNeckOffset(FCamView& viewCS)
{
	FLOAT relYawRad = DEG_TO_RAD * NeckOffsetBaseYaw;
	FLOAT offsetFwd = NeckOffsetFwd*appCos(relYawRad);
	FLOAT offsetSide = NeckOffsetSide*appSin(relYawRad);
	FVector eyeLocOffset(offsetFwd, offsetSide, 0.0f);
	viewCS.Loc = eyeLocOffset;
}

void UOLHeroCamera::LimitViewWS(FCamView& viewWS)
{
	if (Hero->LocomotionMode == LM_Cinematic)
	{
		return;
	}

	viewWS.Pitch = Clamp(viewWS.Pitch, -85.0f, 85.0f);
}

void UOLHeroCamera::DisplayDebug(UCanvas* canvas, FLOAT& out_YL, FLOAT& out_YPos)
{
	canvas->SetDrawColor(240, 173, 239);
	canvas->SetPos(4, out_YPos);
	canvas->DrawText(FString::Printf(TEXT("View WS [yaw: %.0f, pitch: %.0f], CS [yaw: %.0f, pitch: %.0f]"), ViewWS.Yaw, ViewWS.Pitch, ViewCS.Yaw, ViewCS.Pitch));
	out_YPos += out_YL;	

	canvas->SetPos(4, out_YPos);
	canvas->DrawText(FString::Printf(TEXT("BaseRotation [yaw: %.0f, pitch: %.0f], BaseLocation: [%s]"), UNR_TO_DEG * BaseRotation.Yaw, UNR_TO_DEG * BaseRotation.Pitch, *BaseLocation.ToString()));
	out_YPos += out_YL;	

	canvas->SetPos(4, out_YPos);
	canvas->DrawText(FString::Printf(TEXT("MinYaw: %.0f, MaxYaw: %.0f, MinPitch: %.0f, MaxPitch: %.0f"), Limits.MinYaw, Limits.MaxYaw, Limits.MinPitch, Limits.MaxPitch));
	out_YPos += out_YL;	

	canvas->SetPos(4, out_YPos);
	canvas->DrawText(FString::Printf(TEXT("NeckOffsetFwd: %.2f, NeckOffsetSide: %.2f, LookBackRatio: %.2f, LeanPushingRatio: %.0f, PendingYawCorrection: %.0f"), NeckOffsetFwd, NeckOffsetSide, LookBackRatio, LeanPushingRatio, PendingYawCorrection));
	out_YPos += out_YL;		
}

void UOLHeroCamera::DrawDebug()
{
	Hero->DrawDebugSphere(BaseLocation, 2.5f, 6, 213, 5, 245);
	Hero->DrawDebugLine(BaseLocation, BaseLocation + 25.0f*BaseRotation.Vector(), 213, 5, 245);

	if (Limits.MinYaw < -0.01f)
	{
		Hero->DrawDebugLine(ViewWS.Loc, ViewWS.Loc + 25.0f*FRotator(BaseRotation.Pitch, BaseRotation.Yaw + DEG_TO_UNR * Limits.MinYaw, BaseRotation.Roll).Vector(), 227, 214, 32);
	}
	if (Limits.MaxYaw > 0.01f)
	{
		Hero->DrawDebugLine(ViewWS.Loc, ViewWS.Loc + 25.0f*FRotator(BaseRotation.Pitch, BaseRotation.Yaw + DEG_TO_UNR * Limits.MaxYaw, BaseRotation.Roll).Vector(), 227, 214, 32);
	}
}

//////////////////////////////////////////////////////////////////////////
// FCamView
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

FCamView::FCamView()
{
	appMemZero(*this);
}

void FCamView::SetRot(const FRotator& rotation)
{
	Yaw = UNR_TO_DEG * rotation.Yaw;
	Pitch = UNR_TO_DEG * rotation.Pitch;
	Roll = UNR_TO_DEG * rotation.Roll;
	Normalize();
}

FRotator FCamView::Rotator() const
{
	return FRotator((INT)(Pitch * DEG_TO_UNR), (INT)(Yaw * DEG_TO_UNR), (INT)(Roll * DEG_TO_UNR)).GetNormalized();
}

FCamView& FCamView::Normalize()
{
	Yaw = Utils::NormalizeRotAngle(Yaw);
	Pitch = Utils::NormalizeRotAngle(Pitch);
	Roll = Utils::NormalizeRotAngle(Roll);
	return *this;
}
