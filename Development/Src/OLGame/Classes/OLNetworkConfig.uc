// Stores multiplayer connection settings in OLMultiplayer.ini.
// Lives in OLGame so it's accessible before the Multiplayer package loads.
// FMpConnection::LoadConfig reads from this class's config section.
class OLNetworkConfig extends Object
    config(Multiplayer);

var config string IP;
var config string UdpPort;
var config string Username;
var config string RoomCode;
var config string Password;
var config bool   SyncInteractable;
var config bool   SyncEnemies;
var config bool   SyncMatinees;
var config bool   SyncPickups;

static function Save(string NewIP, string NewUdpPort, string NewUsername,
    bool bSyncInteractable, bool bSyncEnemies, bool bSyncMatinees, bool bSyncPickups,
    string NewRoomCode, string NewPassword)
{
    default.IP               = NewIP;
    default.UdpPort          = NewUdpPort;
    default.Username         = NewUsername;
    default.RoomCode         = NewRoomCode;
    default.Password         = NewPassword;
    default.SyncInteractable = bSyncInteractable;
    default.SyncEnemies      = bSyncEnemies;
    default.SyncMatinees     = bSyncMatinees;
    default.SyncPickups      = bSyncPickups;
    StaticSaveConfig();
}

DefaultProperties
{
    IP="127.0.0.1"
    UdpPort="7777"
    Username="Player"
    RoomCode="PUBLIC"
    SyncInteractable=true
    SyncEnemies=true
    SyncMatinees=true
    SyncPickups=true
}
