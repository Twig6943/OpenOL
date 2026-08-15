class OLInventoryManager extends Object
	native
	config(Game);

// Note: This class has nothing to do with unreal's InventoryManager

var OLPlayerController OwnerPC;

var array<name> OwnedInventory;
var array<name> CollectedDocuments;
var array<name> UnreadDocuments;

var array<vector> SavedBatteryLocs; // saved from previous save game or checkpoint
var array<vector> UnsavedBatteryLocs; // newly collected, reset upon death, but saved upon reaching a new checkpoint

cpptext
{
	UBOOL OwnsItem(FName itemName) const;
	UBOOL AddUniqueItem(FName itemName);
	void AddCollectible(FName collectibleName);

	void MarkedBatteryAsCollected(class AOLBatteriesPickupFactory* battery);
	UBOOL IsBatteryCollected(class AOLBatteriesPickupFactory* battery);
	void SaveBatteries();

	void ClearAll();
}

native function ClearUnsavedBatteries();
native function ClearGameplayItems();
native function bool ConsumeItem(name ItemName);

event OnItemConsumed(name ItemName)
{
    if (OwnerPC != None)
        OwnerPC.OnInventoryItemConsumed(ItemName);
}