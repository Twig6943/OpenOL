/*=============================================================================
	OLActorFactory.cpp: 
	Copyright 2012 Red Barrels, Inc. All Rights Reserved.
=============================================================================*/

#include "OLGame.h"

IMPLEMENT_CLASS(UActorFactoryOLWaypoint);
IMPLEMENT_CLASS(UActorFactoryOLAI);

AActor* UActorFactoryOLAI::CreateActor( const FVector* const Location, const FRotator* const Rotation, const USeqAct_ActorFactory* const ActorFactoryData )
{
	AOLEnemyPawn* newPawn = NULL;
	if (PawnClass != NULL && ControllerClass != NULL)
	{
		newPawn = (AOLEnemyPawn*)Super::CreateActor( Location, Rotation, ActorFactoryData );
		if (newPawn != NULL)
		{
			newPawn->SetLocation(newPawn->Location - newPawn->CollisionComponent->Translation);

			newPawn->eventApplyModifiers(PawnModifiers);

			if (BehaviorTree != NULL)
			{
				newPawn->BehaviorTree = BehaviorTree;
			}

			if (MeshOverride != NULL)
			{
				newPawn->Mesh->SetSkeletalMesh(MeshOverride);
			}

			if (ShaderOverrides.bOverride_UniformColor) // Add All Shader Overrides to this check.
			{
				CreateOverrideMaterials(newPawn);
				newPawn->bHasUniformColorOverride = TRUE;
				newPawn->UniformColorOverride = ShaderOverrides.UniformColor;
			}

			for (INT I = 0; I < newPawn->Mesh->GetNumElements(); ++I)
			{
				UMaterialInstance* MatInstance = Cast<UMaterialInstance>(newPawn->Mesh->GetMaterial(I));
				if (MatInstance)
				{
					if (ShaderOverrides.bOverride_UniformColor)
					{
						MatInstance->SetVectorParameterValue(UniformColorName, ShaderOverrides.UniformColor);
					}
				}
			}

			if (VOAsset != NULL)
			{
				newPawn->VOAsset = VOAsset;
				newPawn->InitContextualVO();
			}

			if (bOverrideLightingFlags && newPawn->Mesh)
			{
				newPawn->Mesh->bCastDynamicShadow = bCastDynamicShadow;
				newPawn->Mesh->bCastStaticShadow = bCastStaticShadow;
				newPawn->Mesh->CastShadow = bCastStaticShadow || bCastDynamicShadow;
				newPawn->Mesh->bSelfShadowOnly = bSelfShadowOnly;

				// rcharpentier - needs a reattach? should keep data for toggling on/off because of night vision?
			}

			newPawn->bCastShadowInNV = bCastShadowInNightVision;

			if (AdditionalAnimSets.Num() > 0)
			{
				newPawn->SpawnerAnimSets.Append(AdditionalAnimSets);
				newPawn->UpdateAnimSetList();
			}

			Utils::GetFXManager()->SetFXForEnemyPawn(newPawn);

			if (ActorFactoryData && ActorFactoryData->SpawnPoints.Num() == 1)
			{
				const UActorFactoryOLAI* aiFactory = ConstCast<UActorFactoryOLAI>(ActorFactoryData->Factory);
				AOLWaypoint* waypoint = Cast<AOLWaypoint>(ActorFactoryData->SpawnPoints(0));
				AOLBot* bot = Cast<AOLBot>(newPawn->Controller);
				
				if (bot && waypoint && waypoint->AnimToPlay != NAME_None && aiFactory && aiFactory->bPlaySpawnWaypointAnim)
				{
					// let's auto-play the anim on the spawner
					FAnimationData BotAnimData;
					appMemZero(BotAnimData);
					BotAnimData.AnimationName = waypoint->AnimToPlay;
					BotAnimData.bLoop = waypoint->bLoopAnimation;
					BotAnimData.bOnWaypoint = TRUE;
					BotAnimData.Rate = 1.0f;
					BotAnimData.BlendInTime = 0.2f;
					BotAnimData.BlendOutTime = 0.2f;
					BotAnimData.StartTime = 0.f;
					BotAnimData.EndTime = 0.f;
					BotAnimData.bAlign = waypoint->bAlignAnimToWaypoint;
					BotAnimData.AlignLocation = waypoint->Location;
					BotAnimData.AlignRotation = waypoint->Rotation.Vector();

					bot->eventStartAnimating(BotAnimData, waypoint->bTurnToRotation ? waypoint->Rotation : newPawn->Rotation);
				}
			}
		}
	}
	return newPawn;
}

void UActorFactoryOLAI::PostLoad()
{
	Super::PostLoad();

	if (GetLinker() && GetLinker()->LicenseeVer() < VER_LIC_AI_FACTORY_UPDATE)
	{
		PawnModifiers.bShouldAttack = ShouldAttack_DEPRECATED;
		PawnModifiers.bUseKillingBlow = bUseKillingBlow_DEPRECATED;
		PawnModifiers.bAlwaysLookAtPlayer = bAlwaysLookAtPlayer_DEPRECATED;
	}

	if (GetLinker() && GetLinker()->LicenseeVer() < VER_LIC_AI_FACTORY_UPDATE_2)
	{
		AOLEnemyPawn* DefaultPawn = (AOLEnemyPawn*)(AOLEnemyPawn::StaticClass()->GetDefaultActor());

		for (INT i = 0; i < Weapon_MAX; ++i)
		{
			if (DefaultPawn->Weapons[i].Mesh == PawnModifiers.WeaponMeshToUse_DEPRECATED)
			{
				PawnModifiers.WeaponToUse = i;
			}
		}
	}
}

void UActorFactoryOLAI::CreateOverrideMaterials(AOLEnemyPawn* NewPawn)
{
	UMaterialInstanceConstant* Instance;

	for (INT I = 0; I < NewPawn->Mesh->GetNumElements(); ++I)
	{
		Instance = CastChecked<UMaterialInstanceConstant>(UObject::StaticConstructObject(UMaterialInstanceConstant::StaticClass(), NewPawn->Mesh));
		Instance->SetParent(NewPawn->Mesh->GetMaterial(I));

		NewPawn->Mesh->SetMaterial(I, Instance);
	}
}
