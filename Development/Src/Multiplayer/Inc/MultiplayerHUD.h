#pragma once
#include "Multiplayer.h"

// Helper called from C++ to add a notification to the HUD without ProcessEvent.
// Safe to call with NULL hud (no-op).
void HUD_AddNotification(AMultiplayerHUD* HUD, const FString& Msg);
