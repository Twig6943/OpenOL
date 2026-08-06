class MultiplayerGame extends OLGame;

static event class<GameInfo> SetGameType(string MapName, string Options, string Portal)
{
    return Default.class;
}

event InitGame(string Options, out string ErrorMessage)
{
    super.InitGame(Options, ErrorMessage);
}


DefaultProperties
{
    PlayerControllerClass=Class'Multiplayer.MultiplayerController'
    DefaultPawnClass=Class'OLGame.OLHero'
    HUDType=Class'Multiplayer.MultiplayerHUD'
}