class OLUIFrontEnd_ModMaps extends OLUIFrontEnd_Screen;

var localized string TitleText;
var localized string LoadText;

var int SelectedIndex;

var transient GFxClikWidget MapList;

var transient GFxClikWidget BackButton;
var transient GFxClikWidget LoadButton;

var array<string> MapNames;

function OnTopMostView(optional bool bPlayOpenAnimation = false)
{
	PopulateMapList();
}

function SetSelectedIndex(int Index)
{
	SelectedIndex = Index;
}

function Press_Back(GFxClikWidget.EventData ev)
{
	Back();
}

function PopulateMapList()
{
	local GFxObject Obj, DataProvider;
	local int i;

	class'OLUtils'.static.GetModMaps("Mods\\Persistent", MapNames);

	// Register all found maps in GPackageFileCache so the engine can find them by name.
	// This handles maps added to the folder after the game started.
	for (i = 0; i < MapNames.length; i++)
	{
		class'OLUtils'.static.RegisterModMap(MapNames[i]);
	}

	DataProvider = CreateArray();
	for (i = 0; i < MapNames.length; i++)
	{
		Obj = CreateObject("Object");
		Obj.SetString("label", MapNames[i]);
		DataProvider.SetElementObject(i, Obj);
	}

	if (MapList != None)
	{
		MapList.SetObject("dataProvider", DataProvider);
		MapList.SetFloat("selectedIndex", 0);
	}

	SetSelectedIndex(0);
}

function MapListChanged(GFxClikWidget.EventData ev)
{
	local GFxObject Group;
	local GFxObject Button;
	local int Index;

	Group = MapList.GetObject("group");

	if (Group == None)
	{
		return;
	}

	Button = Group.GetObject("selectedButton");

	if (Button == None)
	{
		return;
	}

	Index = Button.GetInt("index");

	if (SelectedIndex != Index)
	{
		SetSelectedIndex(Index);	
	}
}

event bool WidgetInitialized(name WidgetName, name WidgetPath, GFxObject Widget)
{
	local bool bWasHandled;
	bWasHandled = TRUE;

	switch(WidgetName)
	{
		case ('loadTitle'):
			Widget.SetString("text", TitleText);
			break;
		case ('backBtn'):
			BackButton = GFxClikWidget(Widget);
			BackButton.AddEventListener('CLIK_press', Press_Back);
			BackButton.SetString("label", BackText);
			break;
		case ('loadBtn'):
			LoadButton = GFxClikWidget(Widget);
			LoadButton.AddEventListener('CLIK_press', Press_Load);
			LoadButton.SetString("label", LoadText);
			break;
		case ('saveList'):
			MapList = GFxClikWidget(Widget);
			PopulateMapList();
			MapList.AddEventListener('CLIK_change', MapListChanged);
			MapList.AddEventListener('CLIK_itemDoubleClick', Press_Load);
			break;
		default:
			bWasHandled = FALSE;
	}

	if (!bWasHandled)
	{
		bWasHandled = Super.WidgetInitialized(WidgetName, WidgetPath, Widget);  
	}
	return bWasHandled;
}

function Press_Load(GFxClikWidget.EventData ev)
{
	local int CurrentIndex;

	if (MapList != None)
	{
		CurrentIndex = int(MapList.GetFloat("selectedIndex"));
		if (CurrentIndex >= 0 && CurrentIndex < MapNames.length)
		{
			SelectedIndex = CurrentIndex;
		}
	}

	if (SelectedIndex >= 0 && SelectedIndex < MapNames.length)
	{
		GetOLPC().StopAllSounds();
		Close(true);
		GetOLPC().ConsoleCommand("open " $ MapNames[SelectedIndex]);
	}
}

defaultproperties
{
	SubWidgetBindings.Add((WidgetName="backBtn",WidgetClass=class'GFxClikWidget'))
	SubWidgetBindings.Add((WidgetName="loadBtn",WidgetClass=class'GFxClikWidget'))
	SubWidgetBindings.Add((WidgetName="saveList",WidgetClass=class'GFxClikWidget'))
}