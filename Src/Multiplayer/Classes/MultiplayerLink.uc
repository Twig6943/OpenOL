// Stub — exists only so UMake generates AMultiplayerLink C++ class (needed for IMPLEMENT_CLASS).
// Config and SaveNetworkSettings live in OLNetworkConfig (OLGame package).
class MultiplayerLink extends UdpLink
    native;

// Called by OLNetworkConfig.Save to push updated settings into the live FMpConnection singleton.
native static function NativeReloadConfig();

DefaultProperties
{
}
