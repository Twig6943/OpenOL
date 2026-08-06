class OLUIFrontEnd_Multiplayer extends OLUIFrontEnd_Screen;

var transient GFxClikWidget ApplyButton;
var transient GFxClikWidget BackButton;
var transient GFxObject     SettingsList;

var localized string ApplyText;
var localized string BackText;

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
const OST_CHECKBOX  = 0;
const OST_TEXTINPUT = 6;

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

struct SettingEntry
{
    var string Label;
    var int    Type;
    var string StringValue;
    var int    IntValue;
};
var array<SettingEntry> SettingsOptions;


function OnViewLoaded()
{
    Super.OnViewLoaded();
    BuildOptions();
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
}

function PopulateList()
{
    local int i;
    local GFxObject DataProvider, Obj;

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
            Obj.SetString("TextInputValue", SettingsOptions[i].StringValue);
            Obj.SetBool("IsPassword", SettingsOptions[i].IntValue != 0);
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
    local array<ASValue> Args;
    local ASValue RetVal;

    if (SettingsList == None)
        return;

    Args.Length = 1;
    Args[0].Type = AS_Number;

    for (i = 0; i < SettingsOptions.Length; i++)
    {
        Args[0].n = i;
        if (SettingsOptions[i].Type == OST_TEXTINPUT)
        {
            RetVal = SettingsList.Invoke("GetSelectionValueStringAt", Args);
            `log("OLUIFrontEnd_Multiplayer StoreListValues[" $ i $ "] string='" $ RetVal.s $ "'");
            SettingsOptions[i].StringValue = RetVal.s;
        }
        else
        {
            RetVal = SettingsList.Invoke("GetSelectionValueAt", Args);
            `log("OLUIFrontEnd_Multiplayer StoreListValues[" $ i $ "] int=" $ int(RetVal.n));
            SettingsOptions[i].IntValue = int(RetVal.n);
        }
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


function Press_Apply(GFxClikWidget.EventData ev)
{
    local string MapName;

    SaveSettings();

    MapName = class'WorldInfo'.static.GetWorldInfo().GetMapName(true);
    ConsoleCommand("open " $ MapName $ "?game=Multiplayer.MultiplayerGame");
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

    ApplyText="Connect"
    BackText="Back"

    UsernameText="Username"
    IPText="Server IP"
    PortText="Port"
    SyncInteractableText="Sync interactable"
    SyncEnemiesText="Sync enemies"
    SyncMatineesText="Sync matinees"
    SyncPickupsText="Sync pickups"

}
