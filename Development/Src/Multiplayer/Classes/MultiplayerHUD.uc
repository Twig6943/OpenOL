class MultiplayerHUD extends OLHUD
    config(Multiplayer)
    native;

const NOTIF_DURATION = 5.0;
const NOTIF_FADE_IN  = 0.25;
const NOTIF_FADE_OUT = 0.6;

// Max remote player nicks shown in the HUD nick list (0 = unlimited).
var int MaxNickDisplay;

struct NotificationEntry
{
    var string Text;
    var float  ExpireTime;
};
var array<NotificationEntry> Notifications;

native function AddNotification(string Msg);

Event OnLostFocusPause(Bool bEnable) {
    return;
}

exec function ShowMenu()
{
    ShowMenuType(EMT_PauseMenu);
}

event ShowMenuType(EMenuType MenuType)
{
    local OLGame TheGame;

    if (MenuType != EMT_MainMenu && !CanShowSubMenu())
        return;

    TheGame = OLGame(WorldInfo.Game);
    if (MenuManager == None || !MenuManager.bMovieIsOpen)
    {
        if (PlayerOwner != None)
            PlayerOwner.PlayerInput.ResetInput();

        if (MenuType != EMT_MainMenu)
        {
            if (MenuType == EMT_Credits)
                TheGame.bSoundOnPause = FALSE;
            else
                TheGame.bSoundOnPause = TRUE;
            // Skip SetPause — game keeps running in multiplayer.
        }

        if (MenuManager == None)
        {
            if (class'OLUtils'.static.IsConsole())
            {
                if (class'OLUtils'.static.IsDingo())
                    MenuManager = new(self) class'OLUIFrontEnd_ConsoleXbox';
                else
                    MenuManager = new(self) class'OLUIFrontEnd_Console';
            }
            else
            {
                MenuManager = new(self) class'OLUIFrontEnd';
            }
            MenuManager.MenuType = MenuType;
        }

        if (MenuManager != None)
        {
            if (MenuType == EMT_MainMenu)
                MuteSelectSound(2.5);
            else
                MuteSelectSound();

            MenuManager.Start(false);

            if (CamcorderHUD != None)
                CamcorderHUD.SetVisible(false);
            HideHUDMessages();
        }
    }
}


event PostRender()
{
    local MultiplayerController TC;
    local int i;
    local float X, Y, XL, YL;
    local string Line;
    local vector ScreenPos, HeadPos, CamLoc;
    local rotator CamRot;
    local int ValidCount;
    local float PanelW, PanelH, Padding, BarsWidth;
    local float TiltDeg, TiltPhase;
    local rotator TiltRot;
    local float TagScale, Dist;
    local float HealthRatio;
    local byte CR, CG, CB;
    local bool bPingStale;
    local int NickLimit, NickShown, NickTotal;
    local int PingBars;
    local float NotifAge, NotifRemain, NotifAlpha;
    local string PingLine;
    local float PingYL, ServerYL, PanelX, PanelY, PanelWidth, PanelHeight, RowY;
    local bool bShowServerLine;

    super.PostRender();

    TC = MultiplayerController(PlayerOwner);
    if (TC == None || Canvas == None)
        return;

    Canvas.Font = class'Engine'.Static.GetSmallFont();
    Padding = 8.0;
    BarsWidth = 24.0;

    // --- Top-left: unified server/ping status panel ---
    bShowServerLine = Len(TC.ServerName) > 0;

    bPingStale = (TC.LastPongTime > 0 && WorldInfo.TimeSeconds - TC.LastPongTime > 10.0)
              || (TC.CurrentPingMs <= 0);

    if (bPingStale)
    {
        CR = 160; CG = 160; CB = 160;
        PingBars = 0;
        PingLine = "Ping: ...";
    }
    else if (TC.CurrentPingMs <= 200)
    {
        CR = 100; CG = 220; CB = 100;
        PingBars = 4;
        PingLine = "Ping:" @ Round(TC.CurrentPingMs) @ "ms";
    }
    else if (TC.CurrentPingMs <= 500)
    {
        CR = 230; CG = 140; CB = 60;
        PingBars = 2;
        PingLine = "Ping:" @ Round(TC.CurrentPingMs) @ "ms";
    }
    else
    {
        CR = 220; CG = 80; CB = 80;
        PingBars = 1;
        PingLine = "Ping:" @ Round(TC.CurrentPingMs) @ "ms";
    }

    Canvas.TextSize(PingLine, XL, PingYL);
    PanelWidth = XL + BarsWidth;

    if (bShowServerLine)
    {
        Line = TC.ServerName $ ":" @ TC.OnlineCount @ (TC.OnlineCount == 1 ? "player" : "players");
        Canvas.TextSize(Line, XL, ServerYL);
        if (XL > PanelWidth)
            PanelWidth = XL;
    }

    PanelHeight = PingYL + Padding * 1.5;
    if (bShowServerLine)
        PanelHeight += ServerYL + 4.0;

    PanelX = Padding;
    PanelY = Padding;

    DrawAccentPanel(PanelX, PanelY, PanelWidth + Padding * 2, PanelHeight, CR, CG, CB, 150);

    // Ease-in-out tilt: sin mapped so range is -2..0..+2 degrees.
    TiltPhase = WorldInfo.TimeSeconds * (2.0 * Pi / 3.5);
    TiltDeg   = Sin(TiltPhase) * 2.0;
    TiltRot.Yaw = int(TiltDeg * (65536.0 / 360.0));

    RowY = PanelY + Padding * 0.5;
    if (bShowServerLine)
    {
        Canvas.SetPos(PanelX + Padding * 1.5, RowY);
        Canvas.SetDrawColor(190, 210, 255, 255);
        Canvas.PushRotationMatrix(TiltRot, XL, ServerYL, 0.5, 0.5);
        Canvas.DrawText(Line, false);
        Canvas.PopTransform();
        RowY += ServerYL + 4.0;
    }

    DrawSignalBars(PanelX + Padding * 1.5, RowY, CR, CG, CB, PingBars);
    Canvas.SetPos(PanelX + Padding * 1.5 + BarsWidth, RowY);
    Canvas.SetDrawColor(CR, CG, CB, 255);
    Canvas.DrawText(PingLine, false);

    PanelH = PanelHeight + Padding * 2;

    // --- Top-left: player nick list below ping (self first, then remotes) ---
    NickLimit = MaxNickDisplay;
    NickTotal = 1 + TC.RemotePlayers.Length;  // 1 for self
    NickShown = (NickLimit > 0 && NickTotal > NickLimit) ? NickLimit : NickTotal;

    // Self entry
    if (NickShown > 0)
    {
        Line = TC.GetNetUsername() @ "(You)";
        Canvas.TextSize(Line, XL, YL);
        Y = Padding + PanelH + 2;
        Canvas.SetPos(Padding, Y);
        Canvas.SetDrawColor(0, 0, 0, 140);
        Canvas.DrawRect(XL + Padding * 2, YL + Padding);
        Canvas.SetPos(Padding * 2, Y + Padding * 0.5);
        HealthRatio = (TC.Pawn != None) ? FClamp(float(TC.Pawn.Health) / 100.0, 0.0, 1.0) : 1.0;
        Canvas.SetDrawColor(255, byte(255.0 * HealthRatio), byte(255.0 * HealthRatio), 230);
        Canvas.DrawText(Line, false);
        PanelH = PanelH + YL + Padding + 2;
    }

    // Remote entries
    for (i = 0; i < NickShown - 1; i++)
    {
        Line = (Len(TC.RemotePlayers[i].PlayerNick) > 0)
            ? TC.RemotePlayers[i].PlayerNick
            : ("Player" $ TC.RemotePlayers[i].PlayerID);
        Canvas.TextSize(Line, XL, YL);
        Y = Padding + PanelH + 2;
        Canvas.SetPos(Padding, Y);
        Canvas.SetDrawColor(0, 0, 0, 140);
        Canvas.DrawRect(XL + Padding * 2, YL + Padding);
        Canvas.SetPos(Padding * 2, Y + Padding * 0.5);
        HealthRatio = FClamp(float(TC.RemotePlayers[i].LastRemoteHealth) / 100.0, 0.0, 1.0);
        Canvas.SetDrawColor(255, byte(255.0 * HealthRatio), byte(255.0 * HealthRatio), 230);
        Canvas.DrawText(Line, false);
        PanelH = PanelH + YL + Padding + 2;
    }

    if (NickLimit > 0 && NickTotal > NickLimit)
    {
        Line = "and" @ (NickTotal - NickLimit) @ "more...";
        Canvas.TextSize(Line, XL, YL);
        Y = Padding + PanelH + 2;
        Canvas.SetPos(Padding, Y);
        Canvas.SetDrawColor(0, 0, 0, 140);
        Canvas.DrawRect(XL + Padding * 2, YL + Padding);
        Canvas.SetPos(Padding * 2, Y + Padding * 0.5);
        Canvas.SetDrawColor(140, 140, 140, 200);
        Canvas.DrawText(Line, false);
        PanelH = PanelH + YL + Padding + 2;
    }

    // --- Bottom-left: connection notifications (fade in/out + accent by type) ---
    ValidCount = 0;
    for (i = 0; i < Notifications.Length; i++)
    {
        if (Notifications[i].ExpireTime > WorldInfo.TimeSeconds)
            ValidCount++;
    }
    if (ValidCount > 0)
    {
        Canvas.Font = class'Engine'.Static.GetSmallFont();
        Canvas.TextSize("X", XL, YL);
        Y = Canvas.ClipY - Padding - ValidCount * (YL + Padding);
        for (i = 0; i < Notifications.Length; i++)
        {
            NotifRemain = Notifications[i].ExpireTime - WorldInfo.TimeSeconds;
            if (NotifRemain <= 0)
                continue;

            NotifAge = NOTIF_DURATION - NotifRemain;
            NotifAlpha = 1.0;
            if (NotifAge < NOTIF_FADE_IN)
                NotifAlpha = FClamp(NotifAge / NOTIF_FADE_IN, 0.0, 1.0);
            else if (NotifRemain < NOTIF_FADE_OUT)
                NotifAlpha = FClamp(NotifRemain / NOTIF_FADE_OUT, 0.0, 1.0);

            Line = Notifications[i].Text;
            Canvas.TextSize(Line, XL, YL);

            if (InStr(Line, "disconnected") != INDEX_NONE)
            {
                CR = 220; CG = 90; CB = 90;
            }
            else
            {
                CR = 120; CG = 220; CB = 140;
            }

            DrawAccentPanel(Padding, Y, XL + Padding * 2, YL + Padding, CR, CG, CB, byte(150 * NotifAlpha));

            Canvas.SetPos(Padding * 1.5 + 4.0, Y + Padding * 0.5);
            Canvas.SetDrawColor(235, 235, 235, byte(255 * NotifAlpha));
            Canvas.DrawText(Line, false);
            Y += YL + Padding;
        }
    }

    // --- Nametags above dummy pawn heads ---
    Canvas.Font = class'Engine'.Static.GetLargeFont();
    PlayerOwner.GetPlayerViewPoint(CamLoc, CamRot);
    for (i = 0; i < TC.RemotePlayers.Length; i++)
    {
        if (TC.RemotePlayers[i].DummyPlayer == None)
            continue;

        Line = (Len(TC.RemotePlayers[i].PlayerNick) > 0)
            ? TC.RemotePlayers[i].PlayerNick
            : ("Player" $ TC.RemotePlayers[i].PlayerID);

        if (OLPawn(TC.RemotePlayers[i].DummyPlayer) != None
            && OLPawn(TC.RemotePlayers[i].DummyPlayer).Mesh != None)
            HeadPos = OLPawn(TC.RemotePlayers[i].DummyPlayer).Mesh.GetBoneLocation('Hero-Head');
        else
            HeadPos = TC.RemotePlayers[i].DummyPlayer.Location;
        HeadPos.Z += 15.0;
        ScreenPos = Canvas.Project(HeadPos);

        if (ScreenPos.Z <= 0)
            continue;

        Dist = VSize(HeadPos - CamLoc);
        TagScale = FClamp(600.0 / FMax(Dist, 1.0), 0.1, 1.2);

        Canvas.TextSize(Line, XL, YL);
        XL *= TagScale;
        YL *= TagScale;
        X = ScreenPos.X - XL * 0.5;
        Y = ScreenPos.Y - YL;
        X = FClamp(X, 0, Canvas.ClipX - XL);

        Canvas.SetPos(X - Padding * TagScale, Y - 2);
        Canvas.SetDrawColor(0, 0, 0, 120);
        Canvas.DrawRect(XL + Padding * 2 * TagScale, YL + 4);

        HealthRatio = FClamp(float(TC.RemotePlayers[i].LastRemoteHealth) / 100.0, 0.0, 1.0);
        CR = 255;
        CG = byte(255.0 * HealthRatio);
        CB = byte(255.0 * HealthRatio);
        Canvas.SetPos(X, Y);
        Canvas.SetDrawColor(CR, CG, CB, 230);
        Canvas.DrawText(Line, false, TagScale, TagScale);
    }
}

function DrawAccentPanel(float X, float Y, float W, float H, byte AR, byte AG, byte AB, byte FillAlpha)
{
    local float AccentW;
    AccentW = 3.0;

    Canvas.SetPos(X, Y);
    Canvas.SetDrawColor(12, 12, 14, FillAlpha);
    Canvas.DrawRect(W, H);

    Canvas.SetPos(X, Y);
    Canvas.SetDrawColor(255, 255, 255, 16);
    Canvas.DrawRect(W, 1);
    Canvas.SetPos(X, Y + H - 1);
    Canvas.DrawRect(W, 1);

    Canvas.SetPos(X, Y);
    Canvas.SetDrawColor(AR, AG, AB, 255);
    Canvas.DrawRect(AccentW, H);
}

function DrawSignalBars(float X, float Y, byte AR, byte AG, byte AB, int FilledCount)
{
    local int i;
    local float BarW, Gap, BarH;
    BarW = 3.0;
    Gap = 2.0;

    for (i = 0; i < 4; i++)
    {
        BarH = 4.0 + i * 3.0;
        Canvas.SetPos(X + i * (BarW + Gap), Y + (13.0 - BarH));
        if (i < FilledCount)
            Canvas.SetDrawColor(AR, AG, AB, 255);
        else
            Canvas.SetDrawColor(AR, AG, AB, 45);
        Canvas.DrawRect(BarW, BarH);
    }
}

DefaultProperties
{
    MaxNickDisplay=6
}
