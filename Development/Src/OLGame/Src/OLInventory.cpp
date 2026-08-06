#include "OLGame.h"

IMPLEMENT_CLASS(UOLInventoryManager);
IMPLEMENT_CLASS(AOLPickableObject);
IMPLEMENT_CLASS(AOLBatteriesPickupFactory);
IMPLEMENT_CLASS(AOLCollectiblePickup);
IMPLEMENT_CLASS(AOLGameplayItemPickup);

////////////////////////////////////////////////////////////////////////////////////////////
// AOLPickableObject
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void AOLPickableObject::Pickup(AOLHero* hero)
{
	bUsed = TRUE;

	for (INT i = 0; i < GeneratedEvents.Num(); i++)
	{
		UOLSeqEvent_Pickup* ev = Cast<UOLSeqEvent_Pickup>(GeneratedEvents(i));

		if (ev)
		{
			ev->CheckActivate(this, hero);
		}
	}

	if (hero && hero->OLPC)
		hero->OLPC->eventOnPickupKismetEvent(this);
}

////////////////////////////////////////////////////////////////////////////////////////////
// AOLBatteriesPickupFactory
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

UBOOL AOLBatteriesPickupFactory::CanPickup(AOLHero* hero)
{
	if (!Super::CanPickup(hero))
	{
		return FALSE;
	}

	if (hero->OLPC->NumBatteries == hero->OLPC->MaxNumBatteries)
	{
		return FALSE;
	}

	return TRUE;
}

void AOLBatteriesPickupFactory::Reset()
{
	AOLPlayerController* OLPC = Utils::GetOLPC();

	if (PickupMesh && OLPC && OLPC->InventoryManager)
	{
		UBOOL bShouldBeVisible = !OLPC->InventoryManager->IsBatteryCollected(this);

		if (bShouldBeVisible && PickupMesh->HiddenGame)
		{
			bUsed = FALSE;
			PickupMesh->SetHiddenGame(FALSE);
			PickupMesh->Translation = FVector(0.0f);
			PickupMesh->Rotation = FRotator(0,0,0);

			AttachComponent(PickupMesh);		
		}
		else if (!bShouldBeVisible && !PickupMesh->HiddenGame)
		{
			PickupMesh->SetHiddenGame(TRUE);
		}
	}
}

void AOLBatteriesPickupFactory::Pickup(AOLHero* hero)
{
	Super::Pickup(hero);
	hero->OLPC->NumBatteries = Min(hero->OLPC->NumBatteries + NumBatteries, hero->OLPC->MaxNumBatteries);
	hero->TriggerSoundEvent(hero->SndPickupBatteries);

	hero->OLPC->InventoryManager->MarkedBatteryAsCollected(this);	
	
	if (!hero->OLPC->TutorialManager->bBatteryTutorialComplete)
	{
		hero->OLPC->TutorialManager->BatteryTutorial();
	}
	else
	{
		FString text;

		if (NumBatteries == 1)
		{
			text = Localize(TEXT("Messages"), TEXT("PickedUpBattery"), TEXT("OLGame"));
		}
		else
		{		
			text = FString::Printf(*Localize(TEXT("Messages"), TEXT("PickedUpBatteries"), TEXT("OLGame")), NumBatteries);
		}

		hero->OLPC->HUD->AddMessage(EHMT_Generic, text);
	}
}

FString AOLBatteriesPickupFactory::GetPickableName()
{
	if (NumBatteries == 1)
	{
		return Localize(TEXT("GameplayItems"), TEXT("Battery"), TEXT("OLGame"));
	}
	else
	{
		return Localize(TEXT("GameplayItems"), TEXT("Batteries"), TEXT("OLGame"));
	}
}

void AOLBatteriesPickupFactory::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UStaticMesh* smesh = NULL;
	switch (NumBatteries)
	{
	case 1:
		smesh = Mesh1;
		break;
	case 2:
		smesh = Mesh2;
		break;
	case 3:
		smesh = Mesh3;
		break;
	default:
		smesh = Mesh4;
		break;
	}

	PickupMesh->SetStaticMesh(smesh);
}

void AOLBatteriesPickupFactory::TryShowInvalidPickupMessage()
{
	AOLPlayerController* OLPC = Utils::GetOLPC();
	if (OLPC->NumBatteries == OLPC->MaxNumBatteries)
	{
		OLPC->AddAvailableInteraction(PIT_BatteriesFull);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// AOLCollectiblePickup
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void AOLCollectiblePickup::Pickup(AOLHero* hero)
{
	Super::Pickup(hero);
	hero->OLPC->InventoryManager->AddCollectible(CollectibleName);
	hero->OLPC->HUD->SetLatestDocument(CollectibleName);

	hero->OLPC->TutorialManager->ConditionalDocumentTutorial();

	/// Don't automatically show the Document (feedback from playtest) - Henry
	//hero->OLPC->HUD->eventShowMenuType(EMT_EvidenceMenu);

	hero->TriggerSoundEvent(hero->SndPickupDocument);

	hero->OLPC->CheckCollectibleAchievement(TRUE, CollectibleName.ToString());
}

FString AOLCollectiblePickup::GetPickableName()
{
	return Localize(TEXT("GameplayItems"), TEXT("Collectible"), TEXT("OLGame"));
}

UBOOL AOLCollectiblePickup::ShouldShowCollectible()
{
	AOLPlayerController* OLPC = Utils::GetOLPC();
	return (!OLPC || !OLPC->InventoryManager->CollectedDocuments.ContainsItem(CollectibleName));
}

////////////////////////////////////////////////////////////////////////////////////////////
// AOLGameplayItemPickup
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void AOLGameplayItemPickup::Reset()
{
	if (PickupMesh && ShouldShowItem())
	{
		bUsed = FALSE;
		PickupMesh->SetHiddenGame(FALSE);
		PickupMesh->Translation = FVector(0.0f);
		PickupMesh->Rotation = FRotator(0,0,0);

		if (LastValidCheckpoint != NAME_None)
		{
			SetHidden(FALSE);
		}

		AttachComponent(PickupMesh);		
	}
	else if (LastValidCheckpoint != NAME_None)
	{
		SetHidden(TRUE);
	}
}

UBOOL AOLGameplayItemPickup::ShouldShowItem()
{
	if (LastValidCheckpoint != NAME_None && Utils::IsCheckpointCompleted(LastValidCheckpoint))
	{
		// We've completed the associated checkpoint
		return FALSE;
	}

	return TRUE;
}

void AOLGameplayItemPickup::Pickup(AOLHero* hero)
{
	Super::Pickup(hero);
	hero->OLPC->InventoryManager->AddUniqueItem(ItemName);
	FString itemText = GetPickableName();
	FString text = Localize(TEXT("Messages"), TEXT("PickedUpItem"), TEXT("OLGame"));
	hero->OLPC->HUD->AddMessage(EHMT_Generic, FString::Printf(*text, *itemText));
	
	if (PickupSound)
	{
		hero->TriggerSoundEvent(PickupSound);
	}
}

FString AOLGameplayItemPickup::GetPickableName()
{
	return Localize(TEXT("GameplayItems"), *ItemName.ToString(), TEXT("OLGame"));
}

////////////////////////////////////////////////////////////////////////////////////////////
// AOLInventoryManager
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void UOLInventoryManager::ClearGameplayItems()
{
	OwnedInventory.Empty();
}

void UOLInventoryManager::ClearUnsavedBatteries()
{
	UnsavedBatteryLocs.Empty();
}

void UOLInventoryManager::ClearAll()
{
	OwnedInventory.Empty();
	CollectedDocuments.Empty();
	UnreadDocuments.Empty();
	SavedBatteryLocs.Empty();
	UnsavedBatteryLocs.Empty();
}

UBOOL UOLInventoryManager::OwnsItem(FName itemName) const
{	
	return OwnedInventory.ContainsItem(itemName);
}

UBOOL UOLInventoryManager::ConsumeItem(FName itemName)
{
	INT nbRemoved = OwnedInventory.RemoveItem(itemName);
	if (nbRemoved > 0)
	{
		eventOnItemConsumed(itemName);
		return TRUE;
	}
	return FALSE;
}

UBOOL UOLInventoryManager::AddUniqueItem(FName itemName)
{
	OwnedInventory.AddUniqueItem(itemName);
	return TRUE;
}

void UOLInventoryManager::AddCollectible(FName collectibleName)
{
	CollectedDocuments.AddUniqueItem(collectibleName);
	UnreadDocuments.AddUniqueItem(collectibleName);
}

void UOLInventoryManager::MarkedBatteryAsCollected(AOLBatteriesPickupFactory* battery)
{
	UnsavedBatteryLocs.AddItem(battery->Location);
}

void UOLInventoryManager::SaveBatteries()
{
	SavedBatteryLocs.Append(UnsavedBatteryLocs);
	UnsavedBatteryLocs.Empty();
}

UBOOL UOLInventoryManager::IsBatteryCollected(AOLBatteriesPickupFactory* battery)
{
	const FVector& batPos = battery->Location;
	for (INT i = 0; i < SavedBatteryLocs.Num(); i++)
	{
		const FVector& testBatPos = SavedBatteryLocs(i);

		if (testBatPos.DistanceSquared(batPos) < 1.0f)
		{
			return TRUE;
		}
	}

	return FALSE;
}
