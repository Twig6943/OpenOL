class OLHeroCamera extends Object
	config(Game)
	native;

var OLHero Hero;

struct native CamView
{
	var vector Loc;
	var float Yaw; // in degrees
	var float Pitch;
	var float Roll;

	structcpptext
	{
		FCamView();
		void SetRot(const FRotator& rotation);
		FRotator Rotator() const;
		FCamView& Normalize();
	}
};

struct native ViewLimits // Degrees, camera space
{
	var float MinYaw; 
	var float MaxYaw;
	var float MinPitch;
	var float MaxPitch;
};

var CamView ViewWS; // World-space, updated every frame from the animation
var CamView ViewCS; // Camera-space, defined as a relative offset to the animated position, stateful
var ViewLimits Limits;

// Derived from Limits
var ViewLimits SoftLimits; 
var float SoftZonePitch;
var float SoftZoneYaw; 

// After animation and procedural WS (e.g. look-at)
var rotator BaseRotation; 
var vector BaseLocation;

struct native SmoothingData
{
	var bool bActive;
	var CamView ViewWS; // World-space, captured when starting a smooth
	var float StartTime;
	var float Duration;
};

var SmoothingData BaseViewSmoothing;

var float InputYaw;  // degrees
var float InputPitch;
var float NeckOffsetFwd;
var float NeckOffsetSide;
var float NeckOffsetBaseYaw;
var float LookBackRatio;
var float LeanPushingRatio;
var bool bActiveLookAt;
var float PendingYawCorrection;
var bool bUserControlsPawnRotation;
var bool bLocalSpacePlayerControl; // simplified version, for when e.g. flat on our back
var float BoneSuppression; // 0=full bone pitch/roll applied, 1=zeroed out; interpolated when GSmoothCamera toggles

// Targetted smoothing
struct native TargettedSmoothingData
{
	var bool bActive;
	var float TargetAngleWS; 
	var float StartAngleWS;
	var float StartTime;
	var float Duration;
};

var TargettedSmoothingData TargettedYawSmoothing;

struct native CameraWaveData
{
	var() float Amplitude; // degrees
	var() float Frequency; 
	var() float StartPhase; // normalized [0,1]
};

struct native CameraShakeData
{
	var() float Intensity;
	var() float Duration;
	var() float FadeInTime;
	var() float FadeOutTime;
	var() bool bPositionless;
	var() editinline CameraWaveData YawWaveA;
	var() editinline CameraWaveData YawWaveB;
	var() editinline CameraWaveData PitchWaveA;
	var() editinline CameraWaveData PitchWaveB;
	var() editinline CameraWaveData RollWaveA;
	var() editinline CameraWaveData RollWaveB;

	var transient bool bActive;
	var transient float StartedTime;
	var transient vector SourceLocation;
	var transient float YawOffset;
	var transient float PitchOffset;
	var transient float RollOffset;

	structdefaultproperties
	{
		Duration=0.4
		Intensity=1.0
		FadeInTime=0.0
		FadeOutTime=0.25

		YawWaveA=(Amplitude=4.0,Frequency=22.0,StartPhase=0.5)
		YawWaveB=(Amplitude=4.0,Frequency=122.0,StartPhase=0.0)
		PitchWaveA=(Amplitude=3.0,Frequency=37.0,StartPhase=0.3)
		PitchWaveB=(Amplitude=2.0,Frequency=90.0,StartPhase=0.0)
		RollWaveA=(Amplitude=2.0,Frequency=20.0,StartPhase=0.0)
		RollWaveB=(Amplitude=0.0,Frequency=0.0,StartPhase=0.0)
	}
};

var CameraShakeData ShakeData;
var const CameraShakeData SmallLandingShakeData;
var ForceFeedbackWaveform CamShakeFFWaveform;

var config name CameraBoneName;
var config float ViewLimitsSoftZone;
var config float SoftZoneStiffness;

cpptext
{
public:
	void Init(AOLHero* hero);
	void Update(FLOAT deltaTime);
	void LatchInput(const FRotator& deltaRot);
	void SetView(const FVector& location, const FRotator& rotation);
	FLOAT ConsumeYawCorrection(FLOAT deltaTime);

	void ActivateCameraShake(FCameraShakeData& shakeData, const FVector& sourceLocation = FVector(0.0f));
	void StopCameraShake();
	void ActivateTargettedYawSmoothing(FLOAT targetYawWS, FLOAT smoothingTime);
	void ActivateCameraSmoothing(FLOAT duration = 0.25f);
	
	void DisplayDebug(UCanvas* canvas, FLOAT& out_YL, FLOAT& out_YPos);
	void DrawDebug();
	
private:
	void UpdateYawCorrection(FLOAT deltaTime, FCamView& viewCS);
	void UpdateNeckOffset(FLOAT deltaTime);
	void UpdateShake(FLOAT deltaTime);
	void UpdateLimitsLocalRef();
	void UpdateLimits(FLOAT deltaTime, const FCamView& viewWS);
	void GetAnimatedView(FCamView& viewWS);
	void ApplyCameraSmoothing(FCamView& viewWS);
	void ApplyLookAt(FLOAT deltaTime, FCamView& viewWS, FCamView& viewCS);
	void ApplyLookBack(FLOAT deltaTime, FCamView& viewWS);
	void ApplyShake(FLOAT deltaTime, FCamView& viewWS);
	void ApplyProceduralLean(FLOAT deltaTime, FCamView& viewWS);
	void ApplyYawCorrection(FLOAT deltaTime, FCamView& viewWS);
	void UpdateBaseRotation(const FCamView& viewWS);
	void AttenuateInputs(FLOAT deltaTime, FLOAT& inputYaw, FLOAT& inputPitch, FCamView& viewCS);	
	void ApplyInputDelta(FCamView& viewCS, FLOAT inputYaw, FLOAT inputPitch);
	void ApplySoftLimits(FLOAT deltaTime, FLOAT inputYaw, FLOAT inputPitch, FCamView& viewCS); // attenuates input, applied soft limits and springed rot	
	void LimitViewCS(FCamView& viewCS);	
	void ApplyNeckOffset(FCamView& viewCS);
	void UpdateViewWS(FCamView& viewWS, const FCamView& viewCS);	
	void UpdateViewWSLocalFrame(FCamView& viewWS, const FCamView& viewCS);
	void ApplyTargettedWSSmoothing(FCamView& viewWS);
	void LimitViewWS(FCamView& viewWS);

	// Helpers
	void AttenuateInput(FLOAT deltaTime, FLOAT& inout_input, FLOAT currentAngle, FLOAT limitAngle, FLOAT softZone); // Assumes that current is outside the limit
	void ApplySpring(FLOAT deltaTime, FLOAT& currentAngle, FLOAT softLimitAngle, FLOAT softZone, FLOAT stiffness);
	FLOAT EvaluateWave(const FCameraWaveData& wave, FLOAT t) const;
}

defaultproperties
{	
	Begin Object Class=ForceFeedbackWaveform Name=ForceFeedbackWaveform0
		Samples(0)=(LeftAmplitude=100,RightAmplitude=100,LeftFunction=WF_LinearDecreasing,RightFunction=WF_LinearDecreasing,Duration=0.3)
	End Object
	CamShakeFFWaveform=ForceFeedbackWaveform0

	SmallLandingShakeData=(Intensity=0.500000,Duration=0.200000,FadeOutTime=0.200000,bPositionless=True,YawWaveA=(Amplitude=1.000000,Frequency=27.000000,StartPhase=0.000000),YawWaveB=(Amplitude=0.000000,Frequency=0.000000),PitchWaveA=(Frequency=14.000000),PitchWaveB=(Frequency=87.000000),RollWaveA=(Amplitude=2.250000,Frequency=24.000000))
}
