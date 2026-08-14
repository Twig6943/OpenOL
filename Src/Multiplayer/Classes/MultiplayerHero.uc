class MultiplayerHero extends OLHero
    native;


// Dummies must never trigger the cracked-lens effect — they have no local camera.
event Tick(float DeltaTime)
{
    if (bIsDummyPawn)
        bCameraCracked = false;
    super.Tick(DeltaTime);
    if (bIsDummyPawn)
        bCameraCracked = false;
}

simulated event InterpolationStarted(SeqAct_Interp InterpAction, InterpGroupInst GroupInst)
{
    super.InterpolationStarted(InterpAction, GroupInst);
}

simulated event InterpolationFinished(SeqAct_Interp InterpAction)
{
    super.InterpolationFinished(InterpAction);
}

simulated event PlayFootStepSound(int FootDown, AnimNotify_Footstep footstepNotify)
{
    local name SurfaceMat;
    local bool bRun;
    local float DecalAlpha;

    if (bIsDummyPawn)
    {
        if (WorldInfo.TimeSeconds < LastFootstepTime + 0.2)
            return;
        LastFootstepTime = WorldInfo.TimeSeconds;
        LastFootDown = FootDown;
        // Use surface synced from the remote player's STATE packet (collision disabled on dummy).
        SurfaceMat = LastFootstepSurface;
        bRun = footstepNotify.bForceRunEvent || (IsRunning() && !footstepNotify.bForceWalkEvent);
        PlayDummyFootstep(bRun, SurfaceMat);

        if (WorldInfo.GetDetailMode() != DM_Low)
        {
            if (SurfaceMat == WaterMaterial)
            {
                ActivateWaterFootstepParticles(FootDown == 1);
            }
            else if (SurfaceMat == BloodMaterial)
            {
                RemainingBloodyFootsteps = NumBloodyFootsteps;
            }

            if (RemainingBloodyFootsteps > 0)
            {
                DecalAlpha = (RemainingBloodyFootsteps > 0.75 * NumBloodyFootsteps)
                    ? 1.0
                    : RemainingBloodyFootsteps / (0.75 * NumBloodyFootsteps);
                SpawnBloodFootstepDecal(FootDown == 0, DecalAlpha);
                RemainingBloodyFootsteps--;
            }
        }
        return;
    }

    // Local player: play normally, then send surface material to remote for decals/particles.
    Super.PlayFootStepSound(FootDown, footstepNotify);
    SendFootstep(FootDown);
}

function SendFootstep(int FootDown)
{
    local MultiplayerController MC;
    MC = MultiplayerController(Controller);
    if (MC == None || !MC.IsConnected())
        return;
    // FOOTSTEP packet removed — server does not process it.
}

function PlayDummyFootstep(bool bRun, name SurfaceMat)
{
    SetSwitch(FloorMaterialGroup, SurfaceMat);
    if (bRun)
        PlayAkEvent(FootStepSound_Run);
    else
        PlayAkEvent(FootStepSound_Walk);
}

DefaultProperties
{
    bIsDummyPawn=true
}
