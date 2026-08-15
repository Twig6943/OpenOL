/**
 * Copyright 1998-2012 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_ToggleCinematicMode extends SequenceAction
	native(Sequence);

var() bool bDisableMovement;
var() bool bDisableTurning;
var() bool bHidePlayer;
/** Don't allow input */
var() bool bDisableInput;
/** Whether to hide the HUD during cinematics or not */
var() bool bHideHUD;

/** Destroy dead GearPawns */
var() bool bDeadBodies;
/** Destroy dropped weapons and pickups */
var() bool bDroppedPickups;

var(Outlast) bool bAllowCameraOffset;
var(Outlast) bool bLocalSpacePlayerControl;
var(Outlast) float MinYaw;
var(Outlast) float MaxYaw;
var(Outlast) float MinPitch; // camera-space
var(Outlast) float MaxPitch;
var(Outlast) float NeckOffsetFwd;
var(Outlast) float NeckOffsetSide;
var(Outlast) float CameraSmoothingTime;

var(Outlast) bool bDisableCollision;
var(Outlast) bool bDeactivateCamcorder;
var(Outlast) bool bRestoreCamcorder;
var(Outlast) actor NewBase;

/** Set by remote OLCSA activation to suppress this node from affecting the local player. Cleared in OnToggleCinematicMode. */
var transient bool bObserverOnly;

/** Delete objects we don't want to keep around during cinematics */
event Activated()
{
	local Actor A;

	if (!InputLinks[1].bHasImpulse && (bDeadBodies || bDroppedPickups))
	{
		foreach GetWorldInfo().DynamicActors(class'Actor', A)
		{
			if ( (bDeadBodies && A.IsA('GamePawn') && A.bTearOff) ||
				(bDroppedPickups && A.IsA('DroppedPickup')) )
			{
				A.Destroy();
			}
		}
	}
}


defaultproperties
{
	ObjName="Toggle Cinematic Mode"
	ObjCategory="Toggle"

	InputLinks(0)=(LinkDesc="Enable")
	InputLinks(1)=(LinkDesc="Disable")
	InputLinks(2)=(LinkDesc="Toggle")

	bDisableMovement=TRUE
	bDisableTurning=TRUE
	bHidePlayer=TRUE
	bDisableInput=TRUE
	bHideHUD=TRUE
	bDeadBodies=TRUE
	bDroppedPickups=TRUE

	MinYaw=-180.0
	MaxYaw=180.0
	MinPitch=-80.0
	MaxPitch=80.0

	CameraSmoothingTime=0.0
	bDeactivateCamcorder=true
}
