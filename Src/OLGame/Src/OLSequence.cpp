/*=============================================================================
	OLSequence.cpp: 
	Copyright 2012 Red Barrels, Inc. All Rights Reserved.
=============================================================================*/

#include "OLGame.h"
#include "OLGameSequenceClasses.h"
#include "OLUtilities.h"

IMPLEMENT_CLASS(UOLSeqAct_AIStartPatrol);
IMPLEMENT_CLASS(UOLSeqEvent_NightVision);
IMPLEMENT_CLASS(UOLSeqAct_NightVisionStatus);
IMPLEMENT_CLASS(UOLSeqEvent_SpawnedAtCheckpoint);
IMPLEMENT_CLASS(UOLSeqEvent_ApplyCheckpointState);
IMPLEMENT_CLASS(UOLSeqAct_Checkpoint);
IMPLEMENT_CLASS(UOLSeqCond_Checkpoint);
IMPLEMENT_CLASS(UOLSeqEvent_AILostTarget);
IMPLEMENT_CLASS(UOLSeqAct_AIAbortPatrol);
IMPLEMENT_CLASS(UOLSeqEvent_RecordingComplete);
IMPLEMENT_CLASS(UOLSeqAct_Door);
IMPLEMENT_CLASS(UOLSeqAct_DoorStatus);
IMPLEMENT_CLASS(UOLSeqEvent_Door);
IMPLEMENT_CLASS(UOLSeqAct_GameplayItem);
IMPLEMENT_CLASS(UOLSeqEvent_Pickup);
IMPLEMENT_CLASS(UOLSeqEvent_CSAActivated);
IMPLEMENT_CLASS(UOLSeqAct_HeroControl);
IMPLEMENT_CLASS(UOLSeqCond_IsTouching);
IMPLEMENT_CLASS(UOLSeqAct_SetObjective);
IMPLEMENT_CLASS(UOLSeqAct_SplashScreen);
IMPLEMENT_CLASS(UOLSeqCond_AIState);
IMPLEMENT_CLASS(UOLSeqAct_Tutorial);
IMPLEMENT_CLASS(UOLSeqAct_GameOver);
IMPLEMENT_CLASS(UOLSeqCond_CompareDeaths);
IMPLEMENT_CLASS(UOLSeqCond_IsDemo);
IMPLEMENT_CLASS(UOLSeqCond_DetailMode);
IMPLEMENT_CLASS(UOLSeqAct_Struggle);
IMPLEMENT_CLASS(UOLSeqAct_Blur);
IMPLEMENT_CLASS(UOLSeqAct_Fade);
IMPLEMENT_CLASS(UOLSeqAct_PushableObject);
IMPLEMENT_CLASS(UOLSeqAct_CameraShake);
IMPLEMENT_CLASS(UOLSeqAct_Camcorder);
IMPLEMENT_CLASS(UOLSeqAct_ToggleNightVision);
IMPLEMENT_CLASS(UOLSeqAct_ConditionalHideBatteries);
IMPLEMENT_CLASS(UOLSeqAct_OpenMainMenu);
IMPLEMENT_CLASS(UOLSeqAct_RainEffect);
IMPLEMENT_CLASS(UOLSeqAct_ChokePoint);
IMPLEMENT_CLASS(UOLSeqEvent_AIWaypointAction);
IMPLEMENT_CLASS(UOLSeqAct_LoadingScreen);
IMPLEMENT_CLASS(UOLSeqAct_ActivateGameState);
IMPLEMENT_CLASS(UOLSeqEvent_ApplyGameState);
IMPLEMENT_CLASS(UOLSeqCond_GameState);
IMPLEMENT_CLASS(UOLSeqEvent_Pushable);
IMPLEMENT_CLASS(UOLSeqAct_Pushable);
IMPLEMENT_CLASS(UOLSeqAct_ZoomImpulse);
IMPLEMENT_CLASS(UOLSeqAct_ReadDocument);
IMPLEMENT_CLASS(UOLSeqAct_CrackCameraGlass);
IMPLEMENT_CLASS(UOLSeqAct_CameraGlitch);
IMPLEMENT_CLASS(UOLSeqAct_DarkLightControl);
IMPLEMENT_CLASS(UOLSeqAct_Limp);
IMPLEMENT_CLASS(UOLSeqAct_IgnoreBaseRotation);
IMPLEMENT_CLASS(UOLSeqAct_AdjustToFloor);
IMPLEMENT_CLASS(UOLSeqAct_Achievement);
IMPLEMENT_CLASS(UOLSeqAct_AnimControl);
IMPLEMENT_CLASS(UOLSeqAct_ForceStance);
IMPLEMENT_CLASS(UOLSeqAct_WaitForPlayerInput);
IMPLEMENT_CLASS(UOLSeqAct_SetEnemyMoveSpeed);
IMPLEMENT_CLASS(UOLSeqAct_CameraParticleEffect);
IMPLEMENT_CLASS(UOLSeqAct_MovieEffect);
IMPLEMENT_CLASS(UOLSeqAct_MonitorPlayerSpeed);
IMPLEMENT_CLASS(UOLSeqAct_SetPlayerSpeed);
IMPLEMENT_CLASS(UOLSeqCond_PlayerSpeed);
IMPLEMENT_CLASS(UOLSeqAct_StopCameraShake);
IMPLEMENT_CLASS(UOLSeqAct_SetPlayerWalkingStyle);

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_AIStartPatrol
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Overloaded to see if any new inputs have been activated for this
 * op, so as to add them to the list.  This is needed since scripters tend
 * to use a single action for multiple AI, and the default will only work
 * for the first activation.
 */
UBOOL UOLSeqAct_AIStartPatrol::UpdateOp(FLOAT DeltaTime)
{
	UBOOL bNewInput = 0;
	for (INT Idx = 0; Idx < InputLinks.Num(); Idx++)
	{
		if (InputLinks(Idx).bHasImpulse)
		{
			KISMET_LOG( TEXT("Latent move %s detected new input - Idx %i"), *GetName(), Idx );
			bNewInput = 1;
			break;
		}
	}
	if (bNewInput)
	{
		USeqAct_Latent::Activated();
		OutputLinks(2).bHasImpulse = TRUE;
	}
	return Super::UpdateOp(DeltaTime);
}

void UOLSeqAct_AIStartPatrol::Activated()
{
	USequenceOp::Activated(); // Required for triggering Kismet visual debugger breakpoints
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_Door
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL UOLSeqAct_Door::UpdateOp(FLOAT DeltaTime)
{
	// When triggered by a remote DOOR_STATE event, skip physical door movement
	// (angle is already driven by DOOR_STATE packets) but still run OutputLinks
	// so the rest of the Kismet chain executes normally.
	AOLPlayerController* OLPC = Utils::GetOLPC();
	if (OLPC && OLPC->bApplyingRemoteDoorEvent)
		return Super::UpdateOp(DeltaTime);

	AOLHero* hero = Utils::GetHero();

	TArray<UObject**>	ObjVars;
	GetObjectVars(ObjVars,TEXT("Door"));

	for (INT Idx = 0; Idx < ObjVars.Num(); Idx++)
	{
		AOLDoor* door = Cast<AOLDoor>(*(ObjVars(Idx)));

		if (door)
		{
			if (InputLinks(0).bHasImpulse) // Open
			{
				door->Open(NULL, RotationSpeedOverride, 0.0f, bNoSound);
			}
			else if (InputLinks(1).bHasImpulse) // Close
			{
				door->Close(NULL, RotationSpeedOverride, bNoSound ? AOLDoor::CST_NoSound : AOLDoor::CST_Normal);

				if (hero)
				{
					hero->NotifyDoorClosing(door);
				}
			}
			else if (InputLinks(2).bHasImpulse) // Force Open
			{
				door->ForceOpen();
			}
			else if (InputLinks(3).bHasImpulse) // Force Close
			{
				door->ForceClose();
				
				if (hero)
				{
					hero->NotifyDoorClosing(door);
				}
			}
			else if (InputLinks(4).bHasImpulse) // Set Open Angle
			{
				door->ForceOpenRatio(OpenAngle/door->MaxOpenAngle);
			}
			else if (InputLinks(5).bHasImpulse) // Lock
			{
				door->bLocked = TRUE;
			}
			else if (InputLinks(6).bHasImpulse) // Unlock
			{
				door->bLocked = FALSE;
			}
			else if (InputLinks(7).bHasImpulse) // Set Max Angle
			{
				door->MaxOpenAngle = OpenAngle;
			}
			else if (InputLinks(8).bHasImpulse) // Allow Break
			{
				door->bBreakTriggered = TRUE;
			}
			else if (InputLinks(9).bHasImpulse) // Shake
			{
				door->StartShaking(ShakeParams, bSwitchToBreakingMesh, bBashReversed);
			}
			else if (InputLinks(10).bHasImpulse) // Shake
			{
				door->StopShaking();
			}
			else if (InputLinks(11).bHasImpulse)
			{
				door->InstantBreak();
			}
			else if (InputLinks(12).bHasImpulse) // force break fwd
			{
				door->ForceBreakDoor(TRUE);
			}
			else if (InputLinks(13).bHasImpulse) // force break bwd
			{
				door->ForceBreakDoor(FALSE);
			}
			else if (InputLinks(14).bHasImpulse) // Cancel Bash
			{
				door->CancelBash((ECancelBashDirection)CancelBashDirection);
			}
			else if (InputLinks(15).bHasImpulse) // Destroy
			{
				door->SoftDestroy();
			}
		}
	}	

	return Super::UpdateOp(DeltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_DoorStatus
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_DoorStatus::Activated()
{
	TArray<UObject**>	ObjVars;
	GetObjectVars(ObjVars,TEXT("Door"));

	for (INT Idx = 0; Idx < ObjVars.Num(); Idx++)
	{
		AOLDoor* door = Cast<AOLDoor>(*(ObjVars(Idx)));

		if (door)
		{
			OpenAngle = door->OpenRatio * door->MaxOpenAngle;

			ActivateOutputLink(0);

			if (door->bLocked)
			{
				ActivateOutputLink(3);
			}
			else if (door->IsOpened())
			{
				ActivateOutputLink(1);
			}
			else
			{
				ActivateOutputLink(2);
			}	

			break;
		}
	}	

	USequenceOp::Activated();	
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_NightVisionStatus
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_NightVisionStatus::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero)
	{
		RemainingBatteries = hero->OLPC->NumBatteries;

		ActivateOutputLink(0);

		if (hero->IsCamcorderActive())
		{
			ActivateOutputLink(2);

			switch (hero->CamcorderMode)
			{
			case CCM_Default:
				ActivateOutputLink(3);
				break;
			case CCM_NightVision:
				ActivateOutputLink(4);
				break;
			case CCM_PoweredNightVision:
				ActivateOutputLink(5);
				break;
			}
		}
		else
		{
			ActivateOutputLink(1);
		}
	}

	USequenceOp::Activated();	
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_GameplayItem
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_GameplayItem::Activated()
{
	UBOOL success = FALSE;

	AOLPlayerController* OLPC = Utils::GetOLPC();

	if (OLPC && OLPC->InventoryManager)
	{	
		if (InputLinks(3).bHasImpulse) // give to player
		{
			OLPC->InventoryManager->AddUniqueItem(ItemName);
			success = TRUE;
		}
		else if (OLPC->InventoryManager->OwnsItem(ItemName))
		{
			if (InputLinks(2).bHasImpulse) // use and consume
			{
				OLPC->InventoryManager->ConsumeItem(ItemName);
			}

			success = TRUE;
		}
	}

	if (success)
	{
		ActivateOutputLink(0); // success
	}
	else
	{
		ActivateOutputLink(1); // failure
	}

	USequenceOp::Activated();	
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_HeroControl
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_HeroControl::Activated()
{
	bCompleted = FALSE;
	bPendingKill = FALSE;
	ElapsedTime = 0.0f;

	TArray<UObject**>	GoToTargetVars;
	GetObjectVars(GoToTargetVars, TEXT("GoToTarget"));

	for (INT Idx = 0; Idx < GoToTargetVars.Num(); Idx++)
	{
		AActor* target = Cast<AActor>(*(GoToTargetVars(Idx)));

		if (target)
		{
			GoToTarget = target;
			break;
		}
	}

	TArray<UObject**>	LookAtTargetVars;
	GetObjectVars(LookAtTargetVars, TEXT("LookAtTarget"));

	for (INT Idx = 0; Idx < LookAtTargetVars.Num(); Idx++)
	{
		AActor* target = Cast<AActor>(*(LookAtTargetVars(Idx)));

		if (target)
		{
			LookAtTarget = target;
			break;
		}
	}

	AOLHero* hero = Utils::GetHero();

	if (hero)
	{	
		if (hero->HeroControlActivated(this))
		{
			ActivateOutputLink(0); // success			
		}
		else
		{
			ActivateOutputLink(1); // failure
		}
	}
}

UBOOL UOLSeqAct_HeroControl::UpdateOp(FLOAT deltaTime)
{
	if (bCompleted)
	{
		ActivateOutputLink(2); // completed
		return TRUE;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqCond_IsTouching
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqCond_IsTouching::Activated()
{
	Super::Activated();
	bResult = FALSE;

	AActor* Actor1 = NULL;
	AActor* Actor2 = NULL;
	TArray<UObject**> objectVars;

	GetObjectVars(objectVars, TEXT("Actor 1"));
	if (objectVars.Num() > 0 && objectVars(0) != NULL)
	{
		Actor1 = Cast<AActor>( *(objectVars(0)) );

		if (Actor1 && Actor1->IsA(AController::StaticClass()))
		{
			Actor1 = Cast<AController>(Actor1)->Pawn;
		}
	}

	objectVars.Reset();

	GetObjectVars(objectVars, TEXT("Actor 2"));
	if (objectVars.Num() > 0 && objectVars(0) != NULL)
	{
		Actor2 = Cast<AActor>( *(objectVars(0)) );

		if (Actor2 && Actor2->IsA(AController::StaticClass()))
		{
			Actor2 = Cast<AController>(Actor2)->Pawn;
		}
	}

	if (Actor1 && Actor2)
	{	
		for (INT TouchingIndex = 0; TouchingIndex < Actor1->Touching.Num(); ++TouchingIndex)
		{
			if (Actor1->Touching(TouchingIndex) == Actor2)
			{
				bResult = TRUE;
				break;
			}
		}
	}

	OutputLinks(bResult ? 0 : 1).ActivateOutputLink();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_SetObjective
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL UOLSeqAct_SetObjective::UpdateOp(FLOAT DeltaTime)
{
	if (InputLinks(0).bHasImpulse)
	{
		if (!bHasBeenActivated || !bActivateOnlyOnce)
		{
			AOLPlayerController* OLPC = Utils::GetOLPC();

			if (OLPC)
			{
				OLPC->SetNewObjective(ObjectiveTextId, !bActivateOnlyOnce);
				bHasBeenActivated = TRUE;
			}
		}

		OutputLinks(0).ActivateOutputLink();
	}	
	return Super::UpdateOp(DeltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_SplashScreen
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_SplashScreen::Activated()
{
	AOLPlayerController* OLPC = Utils::GetOLPC();

	if (OLPC)
	{
		if (InputLinks(0).bHasImpulse) // show
		{
			OLPC->ShowSplashScreen(this);
		}
	}

	USequenceOp::Activated();		

	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

UBOOL UOLSeqAct_SplashScreen::UpdateOp(FLOAT deltaTime)
{
	AOLPlayerController* OLPC = Utils::GetOLPC();

	if (bDone)
	{
		for (INT i = 0; i < 3; i++)
		{
			OutputLinks(i).bHasImpulse = FALSE;
		}
		return TRUE;
	}

	if (OLPC && InputLinks(1).bHasImpulse) // hide
	{
		OLPC->HideSplashScreen();
	}
	else if (OLPC && OLPC->HUD && InputLinks(2).bHasImpulse) // set ready
	{
		OLPC->HUD->bSplashScreenReady = TRUE;
	}

	if (!OLPC || !OLPC->HUD || !OLPC->HUD->ShowingFullScreenOverlay())
	{
		ActivateOutputLink(1); // hidden
		bDone = TRUE;
		return TRUE;
	}
	return FALSE;
}

void UOLSeqAct_SplashScreen::Continue()
{
	AOLPlayerController* OLPC = Utils::GetOLPC();

	if (OLPC)
	{
		OLPC->HideSplashScreen();
	}

	ActivateOutputLink(2); // continue
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_GameOver
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_GameOver::Activated()
{
	AOLPlayerController* OLPC = Utils::GetOLPC();

	if (OLPC)
	{
		OLPC->GameOver();
	}

	USequenceOp::Activated();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqCond_AIState
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqCond_AIState::Activated()
{
	Super::Activated();

	AActor* Actor = NULL;
	AOLBot* Bot = NULL;
	TArray<UObject**> objectVars;

	GetObjectVars(objectVars, TEXT("Bot"));
	if (objectVars.Num() > 0 && objectVars(0) != NULL)
	{
		Actor = Cast<AActor>( *(objectVars(0)) );
		if (Actor && Actor->IsA(AOLEnemyPawn::StaticClass()))
		{
			Bot = Cast<AOLEnemyPawn>(Actor)->Bot;
		}
		else
		{
			Bot = Cast<AOLBot>(Actor);
		}
	}

	if (Bot != NULL)
	{
		OutputLinks(Bot->BehaviorState).ActivateOutputLink();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_Tutorial
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_Tutorial::Activated()
{
	USequenceOp::Activated();
}

UBOOL UOLSeqAct_Tutorial::UpdateOp(FLOAT deltaTime)
{
	AOLPlayerController* OLPC = Utils::GetOLPC();

	if (!OLPC)
	{
		return FALSE;
	}

	UOLPlayerInput* playerInput = Cast<UOLPlayerInput>(OLPC->PlayerInput);
	UBOOL bUsingAnalogLeans = playerInput && playerInput->bUsingGamepad && playerInput->GamepadConfig != GBT_C;

	if (InputLinks(0).bHasImpulse && TutorialId < 0) // show
	{
		FString text;

		if (TutorialTextIdGamepad != NAME_None && OLPC->PlayerInput->bUsingGamepad)
		{
			FName effectiveId = TutorialTextIdGamepad;

			if (TutorialTextIdGamepad == FName(TEXT("Tutorial_Peak_Gamepad")) && !bUsingAnalogLeans)
			{
				effectiveId = FName(TEXT("Tutorial_Peak_Keyboard"));
			}

			text = Localize(TEXT("Tutorials"), *effectiveId.ToString(), TEXT("OLGame"));
		}
		else if (TutorialTextId != NAME_None)
		{
			FName effectiveId = TutorialTextId;

			// last minute exceptions
			if (TutorialTextId == FName(TEXT("Tutorial_Crouch_Keyboard")) && OLPC->bToggleCrouch)
			{
				effectiveId = FName(TEXT("Tutorial_Crouch_Keyboard_Toggle"));
			}
			else if (TutorialTextId == FName(TEXT("Tutorial_LookBehind")) && bUsingAnalogLeans)
			{
				effectiveId = FName(TEXT("Tutorial_LookBehind_Analog"));
			}
			else if (TutorialTextId == FName(TEXT("Tutorial_DropFromLedge")) && OLPC->PlayerInput->bUsingGamepad)
			{
				effectiveId = FName(TEXT("Tutorial_DropFromLedge_Gamepad"));
			}
			
			text = Localize(TEXT("Tutorials"), *effectiveId.ToString(), TEXT("OLGame"));
		}
		else
		{
			text = FString(TEXT("[UNKNOWN TUTORIAL]"));
		}

		FString translatedText = Utils::TranslateKeyBindings(text);

		TutorialId = OLPC->TutorialManager->AddItem(translatedText);
		ElapsedTime = 0.0f;
		bIsDone = FALSE;
		ActivateOutputLink(0);
		USeqAct_Latent::Activated();
	}

	ElapsedTime += deltaTime;

	if ((TutorialId > 0) && (InputLinks(1).bHasImpulse || (DisplayTime > 0.0f && ElapsedTime > DisplayTime)))
	{
		OLPC->TutorialManager->RemoveItem(TutorialId);
		TutorialId = -1;
		bIsDone = TRUE;
	}

	if (bIsDone)
	{
		ActivateOutputLink(1);
		return TRUE;
	}
		
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqCond_CompareDeaths
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqCond_CompareDeaths::Activated()
{
	AOLPlayerController* PC = Utils::GetOLPC();

	if (PC != NULL)
	{
		INT Deaths = PC->NumDeathsSinceLastCheckpoint;

		// compare the values and set appropriate output impulse
		if (Deaths <= Value)
		{
			OutputLinks(0).bHasImpulse = TRUE;
		}
		if (Deaths > Value)
		{
			OutputLinks(1).bHasImpulse = TRUE;
		}
		if (Deaths == Value)
		{
			OutputLinks(2).bHasImpulse = TRUE;
		}
		if (Deaths < Value)
		{
			OutputLinks(3).bHasImpulse = TRUE;
		}
		if (Deaths >= Value)
		{
			OutputLinks(4).bHasImpulse = TRUE;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqCond_IsDemo
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqCond_IsDemo::Activated()
{
	Super::Activated();
	bResult = Utils::IsDemo();
	OutputLinks(bResult ? 1 : 0).ActivateOutputLink();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqCond_DetailMode
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqCond_DetailMode::Activated()
{
	Super::Activated();

	if (GSystemSettings.DetailMode == DM_Low)
	{
		OutputLinks(0).ActivateOutputLink();
	}
	else if (GSystemSettings.DetailMode == DM_Medium)
	{
		OutputLinks(1).ActivateOutputLink();
	}
	else
	{
		OutputLinks(2).ActivateOutputLink();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_Struggle
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_Struggle::Init()
{
	if (bObserverOnly)
	{
		ActivateOutputLink(0);
		return;
	}

	Enemy = NULL;

	TArray<UObject**>	EnemyVars;
	GetObjectVars(EnemyVars, TEXT("Enemy"));

	for (INT Idx = 0; Idx < EnemyVars.Num(); Idx++)
	{
		Enemy = Cast<ASkeletalMeshActor>(*(EnemyVars(Idx)));

		if (Enemy)
		{
			break;
		}
	}

	bSucceeded = FALSE;
	bFailed = FALSE;

	Utils::GetOLPC()->InitStruggleOnEnemy(this);
}

void UOLSeqAct_Struggle::Start()
{
	if (bObserverOnly)
	{
		ActivateOutputLink(0);
		return;
	}

	UBOOL bValidRefLoc = FALSE;
	TArray<UObject**>	RefLocationVars;
	GetObjectVars(RefLocationVars, TEXT("RefLocation"));

	for (INT Idx = 0; Idx < RefLocationVars.Num(); Idx++)
	{
		AActor* actor = Cast<AActor>(*(RefLocationVars(Idx)));

		if (actor)
		{
			RefLocation = actor->Location;
			RefDirection = actor->Rotation.Vector();
			bValidRefLoc = TRUE;
			break;
		}
	}

	if (!bValidRefLoc)
	{
		RefLocation = Utils::GetHero()->Location;
		RefDirection = Utils::GetHero()->Rotation.Vector();
	}

	bSucceeded = FALSE;
	bFailed = FALSE;

	Utils::GetOLPC()->ActivateStruggle(this);
}

UBOOL UOLSeqAct_Struggle::UpdateOp(FLOAT deltaTime)
{
	// bObserverOnly is set by MarkChainRemoteObserver when this action is triggered
	// for a remote player (MultiplayerController). Skip entirely — the remote side
	// has no local OLPC to drive and no enemy anim node to update.
	if (bObserverOnly)
		return FALSE;

	AOLPlayerController* OLPC = Utils::GetOLPC();
	if (!OLPC)
		return FALSE;

	if (InputLinks(0).bHasImpulse) // Init
	{
		if (!OLPC->bExcludeFromKismetPlayer)
			Init();
		ActivateOutputLink(0); // Out
	}
	else if (InputLinks(1).bHasImpulse) // Start
	{
		if (OLPC->bExcludeFromKismetPlayer)
		{
			// Observer mode: skip the struggle entirely and report success immediately.
			bSucceeded = TRUE;
		}
		else
		{
			if (Enemy == NULL)
				Init(); // in case the init link wasn't called
			Start();
		}
		ActivateOutputLink(0); // Out
	}
	else if (bSucceeded)
	{
		ActivateOutputLink(1);
		return TRUE;
	}
	else if (bFailed)
	{
		ActivateOutputLink(2);
		return TRUE;
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_Blur
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_Blur::Activated()
{
	Utils::GetFXManager()->TriggerBlur(Amount, Duration, Desaturation, BlendInTime, BlendOutTime);

	USequenceOp::Activated();
	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_MovieEffect
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_MovieEffect::Activated()
{
	Utils::GetFXManager()->TriggerMovieEffect(this);

	USequenceOp::Activated();
	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_Fade
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_Fade::Activated()
{
	AOLPlayerController* OLPC = Utils::GetOLPC();
	if (OLPC)
	{
		FVector2D fadeAlpha;		
		fadeAlpha.X = (bForceStartValue ? (bFadeIn ? 0.0f : Opacity) : OLPC->FadeAmount);
		fadeAlpha.Y = bFadeIn ? Opacity : 0.0f;		

		OLPC->eventClientSetCameraFade(TRUE, FadeColor, fadeAlpha, Duration, FALSE);
	}

	USequenceOp::Activated();
	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_CameraShake
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_CameraShake::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero && hero->Camera)
	{
		TArray<UObject**>	ObjVars;
		GetObjectVars(ObjVars,TEXT("ShakeLocation"));

		FVector srcLocation = hero->Location;

		for (INT Idx = 0; Idx < ObjVars.Num(); Idx++)
		{
			AActor* locationActor = Cast<AActor>(*(ObjVars(Idx)));

			if (locationActor)
			{
				srcLocation = locationActor->Location;
				break;
			}
		}
	
		hero->Camera->ActivateCameraShake(Params, srcLocation);
	}	

	USequenceOp::Activated();
	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_StopCameraShake
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_StopCameraShake::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero && hero->Camera)
	{
		hero->Camera->StopCameraShake();
	}	

	USequenceOp::Activated();
	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_PushableObject
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL UOLSeqAct_PushableObject::UpdateOp(FLOAT DeltaTime)
{
	TArray<UObject**>	ObjVars;
	GetObjectVars(ObjVars,TEXT("PushableObject"));

	for (INT Idx = 0; Idx < ObjVars.Num(); Idx++)
	{
		AOLPushableObject* pushable = Cast<AOLPushableObject>(*(ObjVars(Idx)));

		if (pushable)
		{
			if (InputLinks(0).bHasImpulse) // Reset
			{
				pushable->Reset();
			}
			else if (InputLinks(1).bHasImpulse) // Enable
			{
				pushable->bEnabled = TRUE;
			}
			else if (InputLinks(2).bHasImpulse) // Disable
			{
				pushable->bEnabled = FALSE;
			}
			else if (InputLinks(3).bHasImpulse) // PushMaxFwd
			{
				pushable->SetDisplacement(pushable->MaxFwdDist);
			}
			else if (InputLinks(4).bHasImpulse) // PushMaxBack
			{
				pushable->SetDisplacement(-pushable->MaxBackDist);
			}
		}
	}

	return Super::UpdateOp(DeltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_Camcorder
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_Camcorder::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero)
	{
		if (InputLinks(0).bHasImpulse) // Remove
		{
			hero->RemoveCamcorder();
		}
		else if (InputLinks(1).bHasImpulse) // Give
		{
			hero->GiveCamcorder(FALSE, bWithNightVision);
		}		
		else if (InputLinks(2).bHasImpulse) // Give and use
		{
			if (NewMinFOV > 0.0f)
			{
				hero->OverriddenMinCamcorderFOV = NewMinFOV;
			}

			hero->CurrentBatterySetEnergy = Max(hero->CurrentBatterySetEnergy, 0.2f);
			
			hero->GiveCamcorder(TRUE, bWithNightVision);
		}
		else if (InputLinks(3).bHasImpulse) // Discard all batteries
		{
			hero->CurrentBatterySetEnergy = 0.0f;
			Utils::GetOLPC()->NumBatteries = 0;
		}
		else if (InputLinks(4).bHasImpulse) // Set Batteries
		{
			if (NewNumBatteries >= 0)
			{
				Utils::GetOLPC()->NumBatteries = NewNumBatteries;
			}

			if (NewCurrentEnergy >= 0.0f)
			{
				hero->CurrentBatterySetEnergy = NewCurrentEnergy;
			}			
		}
	}

	USequenceOp::Activated();
	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_CrackCameraGlass
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_CrackCameraGlass::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero && InputLinks(0).bHasImpulse)
	{
		hero->CrackCameraGlass();
	}

	USequenceOp::Activated();
	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_CameraGlitch
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_CameraGlitch::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero && InputLinks(0).bHasImpulse)
	{
		hero->StartElectricGlitch(GlitchIntensity, GlitchFrequency, GlitchDuration);
	}

	USequenceOp::Activated();
	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_CameraParticleEffect
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_CameraParticleEffect::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero && InputLinks(0).bHasImpulse)
	{
		hero->AttachCameraEffect(ParticleSystemTemplate, Duration, PlaneDist);
	}

	USequenceOp::Activated();
	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_ZoomImpulse
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_ZoomImpulse::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero)
	{
		if (InputLinks(0).bHasImpulse) // Zoom In
		{
			hero->ZoomImpulse(ImpulseScale);
		}
		else if (InputLinks(1).bHasImpulse) // Zoom Out
		{
			hero->ZoomImpulse(-ImpulseScale);
		}
	}

	USequenceOp::Activated();
	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_Limp
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_Limp::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero)
	{
		if (InputLinks(0).bHasImpulse) // activate
		{
			{
				UBOOL wasHobbling = hero->bHobbling;
				hero->bHobbling = TRUE;

				switch (HobbleIntensity)
				{
				case HI_VeryBad:
					hero->TargetHobblingIntensity = 1.0f;
					break;
				case HI_Bad:
					hero->TargetHobblingIntensity = 0.75f;
					break;
				case HI_NotSoBad:
					hero->TargetHobblingIntensity = 0.5f;
					break;
				case HI_RatherOk:
					hero->TargetHobblingIntensity = 0.25f;
					break;
				case HI_Healed:
					hero->TargetHobblingIntensity = 0.0f;
					hero->HobblingIntensity = 0.0f;
					hero->bHobbling = FALSE;
					break;
				}

				if (!wasHobbling)
				{
					// if first set (because just spawned), snap to target
					hero->HobblingIntensity = hero->TargetHobblingIntensity;
				}
			}
		}
		else if (InputLinks(1).bHasImpulse) // deactivate
		{
			hero->bHobbling = FALSE;			
		}
	}

	USequenceOp::Activated();
	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_IgnoreBaseRotation
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_IgnoreBaseRotation::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero)
	{
		if (InputLinks(0).bHasImpulse) // don't ignore
		{
			hero->bIgnoreBaseRotation = FALSE;
		}
		else if (InputLinks(1).bHasImpulse) // ignore
		{
			hero->bIgnoreBaseRotation = TRUE;
		}
	}

	USequenceOp::Activated();
	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_ReadDocument
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_ReadDocument::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero && InputLinks(0).bHasImpulse && DocumentName != NAME_None) // Read
	{
		hero->OLPC->InventoryManager->AddCollectible(DocumentName);
		hero->OLPC->HUD->SetLatestDocument(DocumentName);
		hero->OLPC->HUD->eventShowMenuType(EMT_EvidenceMenu);
		hero->TriggerSoundEvent(hero->SndPickupDocument);
	}

	USequenceOp::Activated();
	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqEvent_SpawnedAtCheckpoint
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqEvent_SpawnedAtCheckpoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyThatChanged != NULL && PropertyThatChanged->GetFName() == FName(TEXT("CheckpointName")))
	{
		UpdateStatus();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UOLSeqEvent_SpawnedAtCheckpoint::UpdateStatus()
{
	bStatusIsOk = (Utils::GetCheckpointFromName(CheckpointName) != NULL);
}

#if WITH_EDITOR	
void UOLSeqEvent_SpawnedAtCheckpoint::DrawExtraInfo(FCanvas* Canvas, const FVector& BoxCenter)
{
	if(!GEngine->TickMaterial || !GEngine->CrossMaterial)
	{
		return;
	}

	const UMaterial* UseMaterial = bStatusIsOk ? GEngine->TickMaterial : GEngine->CrossMaterial;
	DrawTile(Canvas, appTrunc(BoxCenter.X-16), appTrunc(BoxCenter.Y-8), 32, 32, 0.f, 0.f, 1.f, 1.f, UseMaterial->GetRenderProxy(0) );
}

FString UOLSeqEvent_SpawnedAtCheckpoint::GetDisplayTitle() const
{
	FString Title = Super::GetDisplayTitle();

	if (CheckpointName != NAME_None)
	{
		Title += FString::Printf( TEXT(" \"%s\""), *CheckpointName.ToString() );
	}

	return Title;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_Checkpoint
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

AOLCheckpoint* UOLSeqAct_Checkpoint::GetCheckpointFromName(FName cpName)
{
	return Utils::GetCheckpointFromName(cpName);
}

void UOLSeqAct_Checkpoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyThatChanged != NULL && PropertyThatChanged->GetFName() == FName(TEXT("CheckpointName")))
	{
		UpdateStatus();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UOLSeqAct_Checkpoint::UpdateStatus()
{
	bStatusIsOk = (Utils::GetCheckpointFromName(CheckpointName) != NULL);
}

#if WITH_EDITOR	
void UOLSeqAct_Checkpoint::DrawExtraInfo(FCanvas* Canvas, const FVector& BoxCenter)
{
	if(!GEngine->TickMaterial || !GEngine->CrossMaterial)
	{
		return;
	}

	const UMaterial* UseMaterial = bStatusIsOk ? GEngine->TickMaterial : GEngine->CrossMaterial;
	DrawTile(Canvas, appTrunc(BoxCenter.X-16), appTrunc(BoxCenter.Y-8), 32, 32, 0.f, 0.f, 1.f, 1.f, UseMaterial->GetRenderProxy(0) );
}

FString UOLSeqAct_Checkpoint::GetDisplayTitle() const
{
	FString Title = Super::GetDisplayTitle();

	if (CheckpointName != NAME_None)
	{
		Title += FString::Printf( TEXT(" \"%s\""), *CheckpointName.ToString() );
	}

	return Title;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqEvent_ApplyCheckpointState
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqEvent_ApplyCheckpointState::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyThatChanged != NULL && PropertyThatChanged->GetFName() == FName(TEXT("CheckpointName")))
	{
		UpdateStatus();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UOLSeqEvent_ApplyCheckpointState::UpdateStatus()
{
	bStatusIsOk = Utils::IsCheckpointValid(CheckpointName);
}

#if WITH_EDITOR	
void UOLSeqEvent_ApplyCheckpointState::DrawExtraInfo(FCanvas* Canvas, const FVector& BoxCenter)
{
	if(!GEngine->TickMaterial || !GEngine->CrossMaterial)
	{
		return;
	}

	const UMaterial* UseMaterial = bStatusIsOk ? GEngine->TickMaterial : GEngine->CrossMaterial;
	DrawTile(Canvas, appTrunc(BoxCenter.X-16), appTrunc(BoxCenter.Y-8), 32, 32, 0.f, 0.f, 1.f, 1.f, UseMaterial->GetRenderProxy(0) );
}

FString UOLSeqEvent_ApplyCheckpointState::GetDisplayTitle() const
{
	FString Title = Super::GetDisplayTitle();

	if (CheckpointName != NAME_None)
	{
		Title += FString::Printf( TEXT(" \"%s\""), *CheckpointName.ToString() );
	}

	return Title;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqCond_Checkpoint
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqCond_Checkpoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyThatChanged != NULL && PropertyThatChanged->GetFName() == FName(TEXT("CheckpointName")))
	{
		UpdateStatus();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UOLSeqCond_Checkpoint::UpdateStatus()
{
	bStatusIsOk = Utils::IsCheckpointValid(CheckpointName);
}

#if WITH_EDITOR	
void UOLSeqCond_Checkpoint::DrawExtraInfo(FCanvas* Canvas, const FVector& BoxCenter)
{
	if(!GEngine->TickMaterial || !GEngine->CrossMaterial)
	{
		return;
	}

	const UMaterial* UseMaterial = bStatusIsOk ? GEngine->TickMaterial : GEngine->CrossMaterial;
	DrawTile(Canvas, appTrunc(BoxCenter.X-16), appTrunc(BoxCenter.Y-8), 32, 32, 0.f, 0.f, 1.f, 1.f, UseMaterial->GetRenderProxy(0) );
}

FString UOLSeqCond_Checkpoint::GetDisplayTitle() const
{
	FString Title = Super::GetDisplayTitle();

	if (CheckpointName != NAME_None)
	{
		Title = FString::Printf( TEXT("Checkpoint \"%s\"?"), *CheckpointName.ToString() );
	}

	return Title;
}
#endif

void UOLSeqCond_Checkpoint::Activated()
{
	Super::Activated();

	AOLGame* currentGame = Cast<AOLGame>(GWorld->GetGameInfo());
	if (currentGame && currentGame->CurrentCheckpointName != NAME_None)
	{	
		if (currentGame->CurrentCheckpointName == CheckpointName)
		{
			OutputLinks(2).ActivateOutputLink(); // Current
		}
		else if (AOLCheckpointList::IsUnreached(CheckpointName, currentGame->CurrentCheckpointName))
		{
			OutputLinks(0).ActivateOutputLink(); // Unreached
		}
	
		if (AOLCheckpointList::IsReached(CheckpointName, currentGame->CurrentCheckpointName))
		{
			OutputLinks(1).ActivateOutputLink(); // Reached
		}

		if (AOLCheckpointList::IsCompleted(CheckpointName, currentGame->CurrentCheckpointName))
		{
			OutputLinks(3).ActivateOutputLink(); // Completed
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_ToggleNightVision
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL UOLSeqAct_ToggleNightVision::UpdateOp(FLOAT deltaTime)
{
	AOLHero* hero = Utils::GetHero();
	if (!hero)
	{
		return FALSE;
	}

	if (InputLinks(0).bHasImpulse) // enable
	{
		if (Utils::IsPlayingDLC() && hero->IsCamcorderActive())
		{
			if (!hero->IsInNightVision())
			{
				hero->ActivateNightVision();
			}
		}
		else
		{
			hero->ForceActivateNightVision();
		}
		ActivateOutputLink(0);
	}
	else if (InputLinks(1).bHasImpulse) // disable
	{
		hero->ForceDeactivateNightVision();
		ActivateOutputLink(0);
	}

	return Super::UpdateOp(deltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_DarkLightControl
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL UOLSeqAct_DarkLightControl::UpdateOp(FLOAT deltaTime)
{
	AOLHero* hero = Utils::GetHero();
	if (!hero)
	{
		return FALSE;
	}

	if (InputLinks(0).bHasImpulse) // enable
	{
		hero->SetDarkLightOverride(DarkLightBrightness, DarkLightRadius);
		ActivateOutputLink(0);
	}
	else if (InputLinks(1).bHasImpulse) // disable
	{
		hero->ResetDarkLightOverride();
		ActivateOutputLink(0);
	}

	return Super::UpdateOp(deltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_ConditionalHideBatteries
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_ConditionalHideBatteries::Activated()
{
	AOLPlayerController* OLPC = Utils::GetOLPC();
	AOLGame* TheGame = Utils::GetOLGame();

	UBOOL bHideBattery = FALSE;

	if (TheGame == NULL || TheGame->DifficultyMode == EDM_Normal)
	{
		bHideBattery = (OLPC && OLPC->NumBatteries > MaxNumBatteries);
	} 
	else if (TheGame->DifficultyMode == EDM_Hard)
	{
		bHideBattery = bHideInHardMode || (OLPC && OLPC->NumBatteries > MaxNumBatteries);
	}
	else // Nightmare Mode
	{
		bHideBattery = bHideInHardMode || (OLPC && OLPC->NumBatteries > 0);
	}

	if (bHideBattery)
	{
		TArray<UObject**>	BatVars;
		GetObjectVars(BatVars, TEXT("Batteries"));

		for (INT Idx = 0; Idx < BatVars.Num(); Idx++)
		{
			AOLBatteriesPickupFactory* battery = Cast<AOLBatteriesPickupFactory>(*(BatVars(Idx)));

			if (battery)
			{
				battery->SetHidden(TRUE);
			}
		}
	}

	USequenceOp::Activated();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_OpenMainMenu
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_OpenMainMenu::Activated()
{
	if (Utils::GetOLPC() && Utils::GetOLPC()->HUD)
	{
		Utils::GetOLPC()->HUD->eventShowMenuType(EMT_MainMenu);
	}

	USequenceOp::Activated();
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_RainEffect
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL UOLSeqAct_RainEffect::UpdateOp(FLOAT deltaTime)
{
	AOLHero* hero = Utils::GetHero();
	if (!hero)
	{
		return FALSE;
	}

	if (InputLinks(0).bHasImpulse) // enable
	{
		hero->ActivateRainEffect();
		ActivateOutputLink(0);
	}
	else if (InputLinks(1).bHasImpulse) // disable
	{
		hero->DeactivateRainEffect();
		ActivateOutputLink(0);
	}

	return Super::UpdateOp(deltaTime);
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_ChokePoint
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL UOLSeqAct_ChokePoint::IsLoadingComplete()
{
	UBOOL bWaiting = FALSE;

	for (INT levelIdx = 0; levelIdx < LevelNames.Num(); levelIdx++)
	{
		UBOOL bNoMatch = TRUE;
		
		for (INT i = 0; i < GWorld->GetWorldInfo()->StreamingLevels.Num(); i++)
		{
			ULevelStreaming* levelStreamingObject = GWorld->GetWorldInfo()->StreamingLevels(i);

			if (levelStreamingObject && levelStreamingObject->PackageName.ToString().EndsWith(LevelNames(levelIdx).ToString()))
			{
				UBOOL bLevelLoading = (levelStreamingObject->bHasLoadRequestPending) || (levelStreamingObject->LoadedLevel == NULL && levelStreamingObject->bShouldBeLoaded);
				UBOOL bLevelWaitingForVisibility = bLevelLoading || !levelStreamingObject->bIsVisible;

				bNoMatch = FALSE;
				
				if ((bLevelLoading && bWaitingForLoaded) || (bLevelWaitingForVisibility && bWaitingForVisible))
				{
					bWaiting = TRUE;
				}

				break;
			}
		}

#if !SHIPPING_PC_GAME && !FINAL_RELEASE
		if (bNoMatch)
		{
			warnf(TEXT("##### UOLSeqAct_ChokePoint : Level %s not in streaming level list! StreamingLevels:"), *LevelNames(levelIdx).ToString());
			for (INT i = 0; i < GWorld->GetWorldInfo()->StreamingLevels.Num(); i++)
			{
				ULevelStreaming* levelStreamingObject = GWorld->GetWorldInfo()->StreamingLevels(i);
				if (levelStreamingObject)
				{
					debugf(TEXT("##### -- %s"), *levelStreamingObject->PackageName.ToString());
				}
			}
		}
#endif

		if (bWaiting)
		{
			break;
		}
	}

	AOLPlayerController* OLPC = Utils::GetOLPC();

	if (!bNonBlocking && bWaiting && OLPC && !OLPC->bBlockedOnLoading)
	{
		OLPC->StartBlockOnLoading();
	}
	else if (!bNonBlocking && !bWaiting && OLPC && OLPC->bBlockedOnLoading)
	{
		OLPC->StopBlockOnLoading();
	}

	return !bWaiting;
}

UBOOL UOLSeqAct_ChokePoint::UpdateOp(FLOAT deltaTime)
{
	if (InputLinks(0).bHasImpulse) // BlockUntilLoaded
	{
		bWaitingForVisible = FALSE;
		bWaitingForLoaded = TRUE;
		ActivateOutputLink(0); // Out
	}
	else if (InputLinks(1).bHasImpulse) // BlockUntilVisible
	{
		bWaitingForLoaded = FALSE;
		bWaitingForVisible = TRUE;
		ActivateOutputLink(0); // Out
	}

	if ((bWaitingForLoaded || bWaitingForVisible) && IsLoadingComplete())
	{
		bWaitingForLoaded = FALSE;
		bWaitingForVisible = FALSE;
		ActivateOutputLink(1); // Done
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_LoadingScreen
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL UOLSeqAct_LoadingScreen::UpdateOp(FLOAT deltaTime)
{
	AOLPlayerController* OLPC = Utils::GetOLPC();

	if (!OLPC)
	{
		return TRUE;
	}

	if (InputLinks(0).bHasImpulse) // Show
	{
		OLPC->KismetRequestedShowLoadingScreen();

		if (this->ObjComment != FString(TEXT("finished")))
		{
			ActivateOutputLink(0); // Out
		}
	}
	else if (InputLinks(1).bHasImpulse) // Hide
	{
		OLPC->KismetRequestedHideLoadingScreen();
		ActivateOutputLink(0); // Out
	}

	return Super::UpdateOp(deltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqEvent_AIWaypointAction
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL UOLSeqEvent_AIWaypointAction::CheckActivate(AActor *InOriginator, AActor *InInstigator, UBOOL bTest/*=FALSE*/, TArray<INT>* ActivateIndices/* = NULL*/, UBOOL bPushTop/* = FALSE*/)
{
	UBOOL bFoundTarget = FALSE;
	UBOOL bPassed = FALSE;

	if (InInstigator != NULL)
	{
		// Check that Instigator is one of the targets or there are no targets
		TArray<UObject**> TargetVars;
		GetObjectVars(TargetVars,TEXT("Target"));
		for (INT Idx = 0; Idx < TargetVars.Num(); Idx++)
		{
			AActor* TestActor = Cast<AActor>( *(TargetVars(Idx)) );
			if (TestActor != NULL)
			{
				AController* TestController = Cast<AController>(TestActor);
				if( TestController != NULL && TestController->Pawn != NULL )
				{
					TestActor = TestController->Pawn;
				}


				if (TestActor == InInstigator)
				{
					bFoundTarget = TRUE;
					break;
				}
			}
		}

		if (bFoundTarget || TargetVars.Num() == 0)
		{
			bPassed = Super::CheckActivate(InOriginator, InInstigator, bTest, ActivateIndices, bPushTop);
		}
	}

	return bPassed;
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqEvent_ApplyGameState
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqEvent_ApplyGameState::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyThatChanged != NULL && PropertyThatChanged->GetFName() == FName(TEXT("GameStateName")))
	{
		UpdateStatus();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UOLSeqEvent_ApplyGameState::UpdateStatus()
{
	bStatusIsOk = AOLGameStateList::IsGameStateValid(GameStateName);
}

#if WITH_EDITOR	
void UOLSeqEvent_ApplyGameState::DrawExtraInfo(FCanvas* Canvas, const FVector& BoxCenter)
{
	if(!GEngine->TickMaterial || !GEngine->CrossMaterial)
	{
		return;
	}

	const UMaterial* UseMaterial = bStatusIsOk ? GEngine->TickMaterial : GEngine->CrossMaterial;
	DrawTile(Canvas, appTrunc(BoxCenter.X-16), appTrunc(BoxCenter.Y-8), 32, 32, 0.f, 0.f, 1.f, 1.f, UseMaterial->GetRenderProxy(0) );
}

FString UOLSeqEvent_ApplyGameState::GetDisplayTitle() const
{
	FString Title = Super::GetDisplayTitle();

	if (GameStateName != NAME_None)
	{
		Title += FString::Printf( TEXT(" \"%s\""), *GameStateName.ToString() );
	}

	return Title;
}
#endif



////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqEvent_ApplyGameState
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_ActivateGameState::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyThatChanged != NULL && PropertyThatChanged->GetFName() == FName(TEXT("GameStateName")))
	{
		UpdateStatus();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UOLSeqAct_ActivateGameState::UpdateStatus()
{
	bStatusIsOk = AOLGameStateList::IsGameStateValid(GameStateName);
}

#if WITH_EDITOR	
void UOLSeqAct_ActivateGameState::DrawExtraInfo(FCanvas* Canvas, const FVector& BoxCenter)
{
	if(!GEngine->TickMaterial || !GEngine->CrossMaterial)
	{
		return;
	}

	const UMaterial* UseMaterial = bStatusIsOk ? GEngine->TickMaterial : GEngine->CrossMaterial;
	DrawTile(Canvas, appTrunc(BoxCenter.X-16), appTrunc(BoxCenter.Y-8), 32, 32, 0.f, 0.f, 1.f, 1.f, UseMaterial->GetRenderProxy(0) );
}

FString UOLSeqAct_ActivateGameState::GetDisplayTitle() const
{
	FString Title = Super::GetDisplayTitle();

	if (GameStateName != NAME_None)
	{
		Title += FString::Printf( TEXT(" \"%s\""), *GameStateName.ToString() );
	}

	return Title;
}
#endif

void UOLSeqAct_ActivateGameState::Activated()
{
	AOLGameStateList::ActivateGameState(GameStateName, bTriggerApplyEvent);
	ActivateOutputLink(0);
	USequenceOp::Activated();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqCond_GameState
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqCond_GameState::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyThatChanged != NULL && PropertyThatChanged->GetFName() == FName(TEXT("GameStateName")))
	{
		UpdateStatus();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UOLSeqCond_GameState::UpdateStatus()
{
	bStatusIsOk = AOLGameStateList::IsGameStateValid(GameStateName);
}

#if WITH_EDITOR	
void UOLSeqCond_GameState::DrawExtraInfo(FCanvas* Canvas, const FVector& BoxCenter)
{
	if(!GEngine->TickMaterial || !GEngine->CrossMaterial)
	{
		return;
	}

	const UMaterial* UseMaterial = bStatusIsOk ? GEngine->TickMaterial : GEngine->CrossMaterial;
	DrawTile(Canvas, appTrunc(BoxCenter.X-16), appTrunc(BoxCenter.Y-8), 32, 32, 0.f, 0.f, 1.f, 1.f, UseMaterial->GetRenderProxy(0) );
}

FString UOLSeqCond_GameState::GetDisplayTitle() const
{
	FString Title = Super::GetDisplayTitle();

	if (GameStateName != NAME_None)
	{
		Title = FString::Printf( TEXT("Game State \"%s\"?"), *GameStateName.ToString() );
	}

	return Title;
}
#endif

void UOLSeqCond_GameState::Activated()
{
	Super::Activated();

	if (AOLGameStateList::IsGameStateActivated(GameStateName))
	{
		OutputLinks(1).ActivateOutputLink(); // ON
	}
	else
	{
		OutputLinks(0).ActivateOutputLink(); // OFF
	}
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_Pushable
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL UOLSeqAct_Pushable::UpdateOp(FLOAT DeltaTime)
{
	TArray<UObject**>	ObjVars;
	GetObjectVars(ObjVars,TEXT("Pushable"));

	for (INT Idx = 0; Idx < ObjVars.Num(); Idx++)
	{
		AOLPushableObject* pushable = Cast<AOLPushableObject>(*(ObjVars(Idx)));

		if (pushable)
		{
			if (InputLinks(0).bHasImpulse) // Reset
			{
				pushable->Reset();
			}
			else if (InputLinks(1).bHasImpulse) // Force Push Left
			{
				pushable->ForcePushLeft();
			}
			else if (InputLinks(2).bHasImpulse) // Force Push Right
			{
				pushable->ForcePushRight();
			}			
		}
	}	

	return Super::UpdateOp(DeltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_AdjustToFloor
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_AdjustToFloor::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero)
	{
		FProceduralAnimData animData;
		animData.PositionDelta = VecZ(HeightOffset);

		check(Duration > 0.0f);
		FLOAT effectiveVel = Abs(HeightOffset) / Duration;

		hero->QueueProceduralAnim(animData, effectiveVel, 360.0f);
	}

	Super::Activated();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_ForceStance
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_ForceStance::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero)
	{
		if (bCrouch)
		{
			hero->bWantsToCrouch = TRUE;
			hero->bForcedCrouch = TRUE;
		}
		else
		{
			hero->bWantsToCrouch = FALSE;
			Utils::GetOLPC()->bDuck = 0;
		}
	}

	Super::Activated();
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_Achievement
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_Achievement::Activated()
{
	AOLPlayerController* OLPC = Utils::GetOLPC();
	check(OLPC);

#if DINGO
	OLPC->TryDingoUnlockAchievement((EOutlastAchievement)achievement);
#else
	OLPC->eventUnlockAchievement(achievement);
#endif

	Super::Activated();
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_AnimControl
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL UOLSeqAct_AnimControl::UpdateOp(FLOAT DeltaTime)
{
	TArray<UObject**>	ObjVars;
	GetObjectVars(ObjVars, TEXT("SkelMeshActor"));

	INT nbValidAnims = Min(InputLinks.Num() - 1, ExtraAnims.Num());

	for (INT Idx = 0; Idx < ObjVars.Num(); Idx++)
	{
		ASkeletalMeshActor* sma = Cast<ASkeletalMeshActor>(*(ObjVars(Idx)));

		if (sma && sma->SkeletalMeshComponent)
		{
			for (INT linkIdx = 0; linkIdx <= nbValidAnims; linkIdx++)
			{
				if (InputLinks(linkIdx).bHasImpulse)
				{
					if (linkIdx == 0)
					{
						StartBaseAnim(sma);
						PlayingIdx = -1;
					}
					else
					{
						StartNewAnim(sma, ExtraAnims(linkIdx-1));						
						PlayingIdx = linkIdx-1;
					}

					ActivateOutputLink(0);

					break;
				}
			}			
		}
	}	

	CheckAnimEnd();

	return (PlayingIdx < 0);
}

void UOLSeqAct_AnimControl::CheckAnimEnd()
{
	if (PlayingIdx >= 0)
	{
		TArray<UObject**>	ObjVars;
		GetObjectVars(ObjVars, TEXT("SkelMeshActor"));

		if (ObjVars.Num() >= 1)
		{
			ASkeletalMeshActor* sma = Cast<ASkeletalMeshActor>(*(ObjVars(0))); // only check the first sma

			if (sma && sma->SkeletalMeshComponent)
			{
				UAnimNodeSlot* slot = Cast<UAnimNodeSlot>(sma->SkeletalMeshComponent->FindAnimNode(SlotName));

				if (!slot)
				{
					return;
				}

				UAnimNodeSequence* playingSeqNode = slot->GetCustomAnimNodeSeq();

				UBOOL bAnimDone = FALSE;

				if (playingSeqNode)
				{
					FName playingAnimName = playingSeqNode->AnimSeqName;
					FName triggeredAnimName = ExtraAnims(PlayingIdx).AnimName;

					bAnimDone = (playingAnimName != triggeredAnimName);
				}
				else
				{
					bAnimDone = TRUE;					
				}

				if (bAnimDone)
				{
					ActivateOutputLink(PlayingIdx + 1);
					PlayingIdx = -1;
				}
			}
		}
	}
}

void UOLSeqAct_AnimControl::StartBaseAnim(ASkeletalMeshActor* sma)
{
	UAnimNodeSlot* slot = Cast<UAnimNodeSlot>(sma->SkeletalMeshComponent->FindAnimNode(SlotName));

	if (!slot)
	{
		if (Utils::GetOLPC())
		{
			Utils::GetOLPC()->eventClientMessage(FString::Printf(TEXT("Cannot find anim slot %s on %s"), *SlotName.ToString(), *sma->GetName()));
		}
		return;
	}

	if (PlayingIdx >= 0)
	{
		slot->StopCustomAnim(ExtraAnims(PlayingIdx).BlendOutTime);
	}

	UAnimNodeSequence* animNode = Cast<UAnimNodeSequence>(sma->SkeletalMeshComponent->FindAnimNode(BaseAnimNodeName));

	if (!animNode)
	{
		if (Utils::GetOLPC())
		{
			Utils::GetOLPC()->eventClientMessage(FString::Printf(TEXT("Cannot find anim node %s on %s"), *BaseAnimNodeName.ToString(), *sma->GetName()));
		}
		return;
	}

	animNode->SetAnim(BaseAnimName);
	animNode->PlayAnim(TRUE);

	slot->SetActiveChild(0, 0.25f);

	PlayingIdx = -1;
}

void UOLSeqAct_AnimControl::StartNewAnim(ASkeletalMeshActor* sma, const FAnimParams& params)
{
	UAnimNodeSlot* slot = Cast<UAnimNodeSlot>(sma->SkeletalMeshComponent->FindAnimNode(SlotName));

	if (!slot)
	{
		if (Utils::GetOLPC())
		{
			Utils::GetOLPC()->eventClientMessage(FString::Printf(TEXT("Cannot find anim slot %s on %s"), *SlotName.ToString(), *sma->GetName()));
		}
		return;
	}

	slot->PlayCustomAnim(params.AnimName, 1.0f, params.BlendInTime, params.BlendOutTime, params.bLooping);	
}

void UOLSeqAct_AnimControl::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	INT nbInputLinks = ExtraAnims.Num() + 1;
	INT nbOutputLinks = ExtraAnims.Num() + 1;

	if (OutputLinks.Num() < nbOutputLinks)
	{
		while (OutputLinks.Num() < nbOutputLinks)
		{
			INT idx = OutputLinks.AddZeroed();
			OutputLinks(idx).LinkDesc = FString::Printf(TEXT("Finished %c"),'A'+(idx-1));
		}
	}

	if (InputLinks.Num() < nbInputLinks)
	{
		while (InputLinks.Num() < nbInputLinks)
		{
			INT idx = InputLinks.AddZeroed();
			InputLinks(idx).LinkDesc = FString::Printf(TEXT("Play %c"),'A'+(idx-1));
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}



////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_WaitForPlayerInput
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL UOLSeqAct_WaitForPlayerInput::UpdateOp(FLOAT DeltaTime)
{
	if (bWaiting)
	{
		AOLPlayerController* OLPC = Utils::GetOLPC();
		if (!OLPC)
		{
			return TRUE;
		}

		if (MaxTime > 0.0f && GWorld->GetTimeSeconds() > StartedTime + MaxTime)
		{
			debugf(TEXT("Waiting for player input - TIME OUT"));

			bWaiting = FALSE;
			ActivateOutputLink(1);
		}
		else
		{
			UBOOL bHasInput = (bOnClick && OLPC->Inputs.bPressedUseButton);
			bHasInput = bHasInput || (bOnPrototypeActionA && OLPC->Inputs.bPressedPrototypeActionA);
			bHasInput = bHasInput || (bOnPrototypeActionB && OLPC->Inputs.bPressedPrototypeActionB);
			bHasInput = bHasInput || (bOnPrototypeActionC && OLPC->Inputs.bPressedPrototypeActionC);
			bHasInput = bHasInput || (bOnPrototypeActionD && OLPC->Inputs.bPressedPrototypeActionD);
			bHasInput = bHasInput || (bOnPrototypeActionE && OLPC->Inputs.bPressedPrototypeActionE);

			if (bHasInput)
			{
				debugf(TEXT("Waiting for player input done"));

				bWaiting = FALSE;
				ActivateOutputLink(0);
			}
			else if (PromptTextId != NAME_None)
			{
				if (TranslatedPrompt.Len() == 0)
				{
					FString text = Localize(TEXT("Messages"), *PromptTextId.ToString(), TEXT("OLGame"));
					TranslatedPrompt = Utils::TranslateKeyBindings(text);
				}
				OLPC->HUD->AddMessage(EHMT_Interaction, TranslatedPrompt, 0.1f);
			}
		}
	}
	else
	{
		for (INT linkIdx = 0; linkIdx < InputLinks.Num(); linkIdx++)
		{
			if (InputLinks(linkIdx).bHasImpulse)
			{
				debugf(TEXT("Waiting for player input started"));

				bWaiting = TRUE;
				StartedTime = GWorld->GetTimeSeconds();
				break;
			}
		}
	}

	return (!bWaiting);
}


////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_SetEnemyMoveSpeed
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_SetEnemyMoveSpeed::Activated()
{
	Super::Activated();

	TArray<UObject**> objectVars;
	GetObjectVars(objectVars, TEXT("Bot"));
	if (objectVars.Num() > 0 && objectVars(0) != NULL)
	{
		AActor* Actor = Cast<AActor>( *(objectVars(0)) );
		AOLEnemyPawn* enemyPawn = Cast<AOLEnemyPawn>(Actor);
		
		if (!enemyPawn)
		{
			AOLBot* bot = Cast<AOLBot>(Actor);
			if (bot)
			{
				enemyPawn = bot->EnemyPawn;
			}
		}

		if (enemyPawn)
		{
			enemyPawn->MoveSpeedMode = MoveSpeedMode;
			enemyPawn->MoveSpeed_Override.PatrolSpeed = PatrolSpeedOverride;
			enemyPawn->MoveSpeed_Override.InvestigateSpeed = InvestigateSpeedOverride;
			enemyPawn->MoveSpeed_Override.ChaseSpeed = ChaseSpeedOverride;
			enemyPawn->MoveSpeed_AccelRate = AccelRate;
			enemyPawn->MoveSpeed_DecelRate = DecelRate;
			enemyPawn->MoveSpeed_SpeedNoVisibility = SpeedNoVisibility;
			enemyPawn->MoveSpeed_ChaseSpeedAtMinDist = ChaseSpeedAtMinDist;
			enemyPawn->MoveSpeed_ChaseSpeedAtMaxDist = ChaseSpeedAtMaxDist;
			enemyPawn->MoveSpeed_ChaseDistMin = ChaseDistMin;
			enemyPawn->MoveSpeed_ChaseDistMax = ChaseDistMax;
		}
		else
		{
			Utils::GetOLPC()->eventClientMessage("SetEnemyMoveSpeed has no valid target");
		}
	}

	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqCond_PlayerSpeed
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqCond_PlayerSpeed::Activated()
{
	Super::Activated();

	AOLHero* hero = Utils::GetHero();

	if (hero)
	{
		FLOAT heroSpeed = hero->RealVelocity.Size2D();

		if (heroSpeed < MaxSpeedForIdle)
		{
			OutputLinks(0).ActivateOutputLink();
		}
		else if (heroSpeed < MaxSpeedForWalk)
		{
			OutputLinks(1).ActivateOutputLink();
		}
		else if (heroSpeed < MaxSpeedForRun)
		{
			OutputLinks(2).ActivateOutputLink();
		}
		else
		{
			OutputLinks(3).ActivateOutputLink();
		}

		if (SpecificThreshold > 0.0f)
		{
			if (heroSpeed < SpecificThreshold)
			{
				OutputLinks(4).ActivateOutputLink();
			}
			else
			{
				OutputLinks(5).ActivateOutputLink();
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_SetPlayerSpeed
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_SetPlayerSpeed::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero)
	{
		hero->GroundSpeed = TargetSpeed;
		hero->CurrentRunSpeed = TargetSpeed;
	}

	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_MonitorPlayerSpeed
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL UOLSeqAct_MonitorPlayerSpeed::UpdateOp(FLOAT deltaTime)
{
	UBOOL bFirstRun = FALSE;

	if (InputLinks(0).bHasImpulse) // Start
	{		
		bRunning = TRUE;
		bBelowThreshold = TRUE;
		bFirstRun = TRUE;
		CurrentStateIdx = -1;
	}
	else if (InputLinks(1).bHasImpulse) // Stop
	{
		bRunning = FALSE;
		return TRUE;
	}

	if (bRunning)
	{
		INT targetIdx = -1;

		AOLHero* hero = Utils::GetHero();

		if (hero)
		{
			FLOAT heroSpeed = hero->RealVelocity.Size2D();

			if (SpecificThreshold > 0.0f)
			{
				UBOOL isBelowTreshold = (heroSpeed < SpecificThreshold);

				if ((bFirstRun || isBelowTreshold != bBelowThreshold) && GWorld->GetTimeSeconds() > LastEventTime + 1.0f)
				{
					LastEventTime = GWorld->GetTimeSeconds();
					bBelowThreshold = isBelowTreshold;
					ActivateOutputLink(bBelowThreshold ? 4 : 5);
				}
			}
			else
			{
				if (heroSpeed < MaxSpeedForIdle)
				{
					targetIdx = 0;
				}
				else if (heroSpeed < MaxSpeedForWalk)
				{
					targetIdx = 1;
				}
				else if (heroSpeed < MaxSpeedForRun)
				{
					targetIdx = 2;
				}
				else
				{
					targetIdx = 3;
				}

				if (targetIdx != CurrentStateIdx && GWorld->GetTimeSeconds() > LastEventTime + 1.0f)
				{
					LastEventTime = GWorld->GetTimeSeconds();
					CurrentStateIdx = targetIdx;
					ActivateOutputLink(CurrentStateIdx);
				}
			}			
		}
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// UOLSeqAct_SetPlayerWalkingStyle
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLSeqAct_SetPlayerWalkingStyle::Activated()
{
	AOLHero* hero = Utils::GetHero();

	if (hero)
	{
		hero->ForcedWalkingStyle = WalkingStyle;
		hero->WalkSpeedOverride = WalkSpeedOverride;
		hero->RunSpeedOverride = RunSpeedOverride;

		AOLHero* defaultHero = Cast<AOLHero>(hero->GetClass()->GetDefaultObject());

		if (AccelApproachFactorOverride > 0.0f)
		{
			hero->AccelApproachFactor = AccelApproachFactorOverride;
		}
		else
		{
			hero->AccelApproachFactor = defaultHero->AccelApproachFactor;
		}

		if (DecelApproachFactorOverride > 0.0f)
		{
			hero->DecelApproachFactor = DecelApproachFactorOverride;
		}
		else
		{
			hero->DecelApproachFactor = defaultHero->DecelApproachFactor;
		}
	}

	OutputLinks(0).bHasImpulse = InputLinks(0).bHasImpulse;
}