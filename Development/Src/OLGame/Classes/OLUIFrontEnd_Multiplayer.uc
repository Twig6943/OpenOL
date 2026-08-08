class OLUIFrontEnd_Multiplayer extends OLUIFrontEnd_Screen;

var transient GFxClikWidget ApplyButton;
var transient GFxClikWidget BackButton;
var transient GFxClikWidget CopyLinkButton;
var transient GFxClikWidget InviteSteamButton;
var transient GFxObject     SettingsList;

var localized string ApplyText;
var localized string CopyLinkText;
var localized string InviteSteamText;
var localized string InviteLinkLabelText;

var localized string UsernameText;
var localized string IPText;
var localized string PortText;
var localized string RoomCodeText;
var localized string PasswordText;
var localized string SyncInteractableText;
var localized string SyncEnemiesText;
var localized string SyncMatineesText;
var localized string SyncPickupsText;

// OST_ type constants matching OptionsList.as
const OST_CHECKBOX       = 0;
const OST_BUTTON         = 4; // OST_ControllerConfigButton — single button row
const OST_TEXTINPUT      = 6;

// Fake ProfileSettingIDs used to identify our custom buttons in Press_OptionItemButton
const BTN_ID_COPY_LINK   = 9001;
const BTN_ID_INVITE_STEAM = 9002;

// Indices into SettingsOptions array
var int UsernameIdx;
var int IPIdx;
var int PortIdx;
var int RoomCodeIdx;
var int PasswordIdx;
var int SyncInteractableIdx;
var int SyncEnemiesIdx;
var int SyncMatineesIdx;
var int SyncPickupsIdx;
var int InviteLinkIdx;

struct SettingEntry
{
    var string Label;
    var int    Type;
    var string StringValue;
    var int    IntValue;
    var bool   bReadOnly;
};
var array<SettingEntry> SettingsOptions;


function OnViewLoaded()
{
    Super.OnViewLoaded();
    BuildOptions();
}

function Press_OptionItemButton(int PSID)
{
    local GFxClikWidget.EventData Dummy;

    switch (PSID)
    {
        case BTN_ID_COPY_LINK:
            Press_CopyLink(Dummy);
            break;
        case BTN_ID_INVITE_STEAM:
            Press_InviteSteam(Dummy);
            break;
    }
}


function BuildOptions()
{
    local SettingEntry E;

    SettingsOptions.Length = 0;

    E.Label = UsernameText;  E.Type = OST_TEXTINPUT; E.StringValue = GetOLPC().GetNetUsername();   E.IntValue = 0;
    UsernameIdx = SettingsOptions.Length;
    SettingsOptions.AddItem(E);

    E.Label = IPText;        E.Type = OST_TEXTINPUT; E.StringValue = GetOLPC().GetNetIP();         E.IntValue = 0;
    IPIdx = SettingsOptions.Length;
    SettingsOptions.AddItem(E);

    E.Label = PortText;      E.Type = OST_TEXTINPUT; E.StringValue = GetOLPC().GetNetPort();       E.IntValue = 0;
    PortIdx = SettingsOptions.Length;
    SettingsOptions.AddItem(E);

    E.Label = RoomCodeText;  E.Type = OST_TEXTINPUT; E.StringValue = GetOLPC().GetNetRoomCode();   E.IntValue = 0;
    RoomCodeIdx = SettingsOptions.Length;
    SettingsOptions.AddItem(E);

    E.Label = PasswordText;  E.Type = OST_TEXTINPUT; E.StringValue = GetOLPC().GetNetPassword();   E.IntValue = 1; // 1 = password field (masked)
    PasswordIdx = SettingsOptions.Length;
    SettingsOptions.AddItem(E);

    E.Label = SyncInteractableText; E.Type = OST_CHECKBOX; E.StringValue = ""; E.IntValue = GetOLPC().GetNetSyncInteractable() ? 1 : 0;
    SyncInteractableIdx = SettingsOptions.Length;
    SettingsOptions.AddItem(E);

    E.Label = SyncEnemiesText; E.Type = OST_CHECKBOX; E.StringValue = ""; E.IntValue = GetOLPC().GetNetSyncEnemies() ? 1 : 0;
    SyncEnemiesIdx = SettingsOptions.Length;
    SettingsOptions.AddItem(E);

    E.Label = SyncMatineesText; E.Type = OST_CHECKBOX; E.StringValue = ""; E.IntValue = GetOLPC().GetNetSyncMatinees() ? 1 : 0;
    SyncMatineesIdx = SettingsOptions.Length;
    SettingsOptions.AddItem(E);

    E.Label = SyncPickupsText; E.Type = OST_CHECKBOX; E.StringValue = ""; E.IntValue = GetOLPC().GetNetSyncPickups() ? 1 : 0;
    SyncPickupsIdx = SettingsOptions.Length;
    SettingsOptions.AddItem(E);

    E.Label = InviteLinkLabelText; E.Type = OST_TEXTINPUT; E.IntValue = 0; E.bReadOnly = true;
    E.StringValue = GetOLPC().NativeBuildInviteLink(
        SettingsOptions[IPIdx].StringValue,
        SettingsOptions[PortIdx].StringValue,
        SettingsOptions[RoomCodeIdx].StringValue,
        SettingsOptions[PasswordIdx].StringValue);
    InviteLinkIdx = SettingsOptions.Length;
    SettingsOptions.AddItem(E);

    E.bReadOnly = false;
    E.Label = ""; E.Type = OST_BUTTON; E.StringValue = ""; E.IntValue = BTN_ID_COPY_LINK;
    SettingsOptions.AddItem(E);

    E.Label = ""; E.Type = OST_BUTTON; E.StringValue = ""; E.IntValue = BTN_ID_INVITE_STEAM;
    SettingsOptions.AddItem(E);
}

function PopulateList()
{
    local int i;
    local GFxObject DataProvider, Obj, LabelArr;

    if (SettingsList == None)
        return;

    DataProvider = CreateArray();
    for (i = 0; i < SettingsOptions.Length; i++)
    {
        Obj = CreateObject("Object");
        Obj.SetString("label",      SettingsOptions[i].Label);
        Obj.SetFloat ("OptionType", SettingsOptions[i].Type);
        if (SettingsOptions[i].Type == OST_TEXTINPUT)
        {
            Obj.SetFloat("ProfileSettingID", i);
            Obj.SetString("TextInputValue", SettingsOptions[i].StringValue);
            Obj.SetBool("IsPassword", SettingsOptions[i].IntValue != 0);
            Obj.SetBool("IsReadOnly", SettingsOptions[i].bReadOnly);
        }
        else if (SettingsOptions[i].Type == OST_BUTTON)
        {
            Obj.SetFloat("ProfileSettingID", SettingsOptions[i].IntValue);
            Obj.SetFloat("ButtonLabelIndex", 0);
            LabelArr = CreateArray();
            if (SettingsOptions[i].IntValue == BTN_ID_COPY_LINK)
                LabelArr.SetElementString(0, CopyLinkText);
            else
                LabelArr.SetElementString(0, InviteSteamText);
            Obj.SetObject("ButtonLabelsList", LabelArr);
        }
        else
            Obj.SetBool("CheckboxSelected", SettingsOptions[i].IntValue != 0);
        DataProvider.SetElementObject(i, Obj);
    }

    SettingsList.SetObject("dataProvider", DataProvider);
    SettingsList.SetFloat ("selectedIndex", 0);
}

function StoreListValues()
{
    local int i;
    local string Link;
    local array<ASValue> Args;
    local ASValue RetVal;

    if (SettingsList == None)
        return;

    Args.Length = 1;
    Args[0].Type = AS_Number;

    for (i = 0; i < SettingsOptions.Length; i++)
    {
        if (SettingsOptions[i].bReadOnly)
            continue;
        Args[0].n = i;
        if (SettingsOptions[i].Type == OST_TEXTINPUT)
        {
            RetVal = SettingsList.Invoke("GetSelectionValueStringAt", Args);
            SettingsOptions[i].StringValue = RetVal.s;
        }
        else if (SettingsOptions[i].Type != OST_BUTTON)
        {
            RetVal = SettingsList.Invoke("GetSelectionValueAt", Args);
            SettingsOptions[i].IntValue = int(RetVal.n);
        }
    }

    // Update invite link after reading IP/Port/Room/Pass
    Link = GetOLPC().NativeBuildInviteLink(
        SettingsOptions[IPIdx].StringValue,
        SettingsOptions[PortIdx].StringValue,
        SettingsOptions[RoomCodeIdx].StringValue,
        SettingsOptions[PasswordIdx].StringValue);
    SettingsOptions[InviteLinkIdx].StringValue = Link;
    if (SettingsList != None)
    {
        Args.Length = 2;
        Args[0].Type = AS_Number; Args[0].n = InviteLinkIdx;
        Args[1].Type = AS_String; Args[1].s = Link;
        SettingsList.Invoke("SetTextInputValueAt", Args);
    }
}

function SaveSettings()
{
    StoreListValues();

    GetOLPC().SaveNetworkSettings(
        SettingsOptions[IPIdx].StringValue,
        SettingsOptions[PortIdx].StringValue,
        SettingsOptions[UsernameIdx].StringValue,
        SettingsOptions[SyncInteractableIdx].IntValue != 0,
        SettingsOptions[SyncEnemiesIdx].IntValue != 0,
        SettingsOptions[SyncMatineesIdx].IntValue != 0,
        SettingsOptions[SyncPickupsIdx].IntValue != 0,
        SettingsOptions[RoomCodeIdx].StringValue,
        SettingsOptions[PasswordIdx].StringValue);
}

function string BuildInviteLink()
{
    local string IP, Port, Room, Pass;

    StoreListValues();
    IP   = SettingsOptions[IPIdx].StringValue;
    Port = SettingsOptions[PortIdx].StringValue;
    Room = SettingsOptions[RoomCodeIdx].StringValue;
    Pass = SettingsOptions[PasswordIdx].StringValue;

    return GetOLPC().NativeBuildInviteLink(IP, Port, Room, Pass);
}

function bool ParseInviteLink(string Link)
{
    local string OutIP, OutPort, OutRoom, OutPass;

    if (!GetOLPC().NativeParseInviteLink(Link, OutIP, OutPort, OutRoom, OutPass))
        return false;

    if (Len(OutIP)   > 0) SettingsOptions[IPIdx].StringValue       = OutIP;
    if (Len(OutPort) > 0) SettingsOptions[PortIdx].StringValue      = OutPort;
    if (Len(OutRoom) > 0) SettingsOptions[RoomCodeIdx].StringValue  = OutRoom;
    SettingsOptions[PasswordIdx].StringValue = OutPass;
    return true;
}

function DoConnect()
{
    local string MapName;

    SaveSettings();
    MapName = class'WorldInfo'.static.GetWorldInfo().GetMapName(true);
    ConsoleCommand("open " $ MapName $ "?game=Multiplayer.MultiplayerGame");
}

function Press_Apply(GFxClikWidget.EventData ev)
{
    DoConnect();
}

function CopyWidgetTextByName(string WidgetLabel)
{
    local int i;
    for (i = 0; i < SettingsOptions.Length; i++)
    {
        if (SettingsOptions[i].Label == WidgetLabel)
        {
            GetOLPC().CopyToClipboard(SettingsOptions[i].StringValue);
            return;
        }
    }
}

function Press_CopyLink(GFxClikWidget.EventData ev)
{
    CopyWidgetTextByName(InviteLinkLabelText);
}

function Press_InviteSteam(GFxClikWidget.EventData ev)
{
    // Steam invite: not implemented yet (Relay GUI handles this, Etap 3)
}

function Press_Back(GFxClikWidget.EventData ev)
{
    MenuManager.PopView();
}

function bool Back()
{
    MenuManager.PopView();
    return true;
}

event bool WidgetInitialized(name WidgetName, name WidgetPath, GFxObject Widget)
{
    local bool bWasHandled;
    bWasHandled = false;

    switch (WidgetName)
    {
        case ('applyBtn'):
            ApplyButton = GFxClikWidget(Widget);
            ApplyButton.AddEventListener('CLIK_press', Press_Apply);
            ApplyButton.SetString("label", ApplyText);
            bWasHandled = true;
            break;
        case ('backBtn'):
            BackButton = GFxClikWidget(Widget);
            BackButton.AddEventListener('CLIK_press', Press_Back);
            BackButton.SetString("label", BackText);
            bWasHandled = true;
            break;
        case ('copyLinkBtn'):
            CopyLinkButton = GFxClikWidget(Widget);
            CopyLinkButton.AddEventListener('CLIK_press', Press_CopyLink);
            CopyLinkButton.SetString("label", CopyLinkText);
            bWasHandled = true;
            break;
        case ('inviteSteamBtn'):
            InviteSteamButton = GFxClikWidget(Widget);
            InviteSteamButton.AddEventListener('CLIK_press', Press_InviteSteam);
            InviteSteamButton.SetString("label", InviteSteamText);
            bWasHandled = true;
            break;
        case ('gameplayList'):
            SettingsList = Widget;
            PopulateList();
            bWasHandled = true;
            break;
        default:
            bWasHandled = false;
    }

    if (!bWasHandled)
        bWasHandled = Super.WidgetInitialized(WidgetName, WidgetPath, Widget);

    return bWasHandled;
}

defaultproperties
{
    SubWidgetBindings.Add((WidgetName="applyBtn",WidgetClass=class'GFxClikWidget'))
    SubWidgetBindings.Add((WidgetName="backBtn",WidgetClass=class'GFxClikWidget'))
    SubWidgetBindings.Add((WidgetName="copyLinkBtn",WidgetClass=class'GFxClikWidget'))
    SubWidgetBindings.Add((WidgetName="inviteSteamBtn",WidgetClass=class'GFxClikWidget'))

}
