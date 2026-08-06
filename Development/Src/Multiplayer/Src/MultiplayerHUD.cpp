#include "Multiplayer.h"
#include "MultiplayerHUD.h"

// AMultiplayerHUD::AddNotification — native implementation.
// Appends a timed notification to the HUD's Notifications array.
void AMultiplayerHUD::AddNotification(const FString& Msg)
{
    FLOAT Now = (GWorld && GWorld->GetWorldInfo()) ? GWorld->GetWorldInfo()->TimeSeconds : 0.0f;
    FNotificationEntry E;
    E.Text       = Msg;
    E.ExpireTime = Now + 5.0f; // NOTIF_DURATION
    Notifications.AddItem(E);
}

void HUD_AddNotification(AMultiplayerHUD* HUD, const FString& Msg)
{
    if (HUD)
        HUD->AddNotification(Msg);
}
