/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
class OLEngine extends GameEngine
	native
	config(Engine)
	inherits(FCallbackEventDevice);


/** struct to store the date and time the checkpoint was created */
struct native CheckpointTime
{
	var int SecondsSinceMidnight;
	var int Day;
	var int Month;
	var int Year;

	structcpptext
	{
		INT Compare(const FCheckpointTime Other) const;
	}
};

struct native SaveFileInfo
{
	var string FileName;
	var name CheckpointName;
	var CheckpointTime Time;

	// dingo only (currently at least)
	var int Difficulty;
	var int SaveFileId; // 1-based [1-5]
};

var array<SaveFileInfo> SaveFiles;

/** Information set to trigger a checkpoint load/save on the next tick. */
enum ECheckpointAction
{
	Checkpoint_Default,
	Checkpoint_Load,
	Checkpoint_SaveToDisk,
	Checkpoint_SaveToMemory,	// "soft" checkpoints
};

var	name		PendingCheckpointName;
var	ECheckpointAction	PendingCheckpointAction;

var bool bRestartingActiveCheckpoint; // to differentiate between reloading last save after dying (true) and loading a save game from the menu (false)

/** current checkpoint in use (if any) */
var	protectedwrite OLCheckpointSave CurrentCheckpointSave;

var bool bPendingGraphicalSettingsChange; // set when we need to reset the system settings
var int PendingNewLanguage; // >=0 if we're pending a new language set

var config bool bDisableMotionBlur;
var config bool bSmoothCamera;

var AkEvent StartupMovieSound;

var float NextRefreshDLCTime;

// >> PS4
enum OrbisDialogType
{
	ODT_None,
	ODT_SelectSaveLocation,
	ODT_LoadGame,
	ODT_SystemError,
	ODT_LoadedBadCheckpoint,
	ODT_PSStore,
};

var OrbisDialogType ActiveOrbisDialog;
var array<byte> SaveDataBuffer;
var bool bPausedForSystemDialog;
// <<

// >> Dingo
enum Dingo_LoadGameResult
{
	LGR_LoadError,
	LGR_BadCheckpoint,
	LGR_Success
};	

var array<byte> PendingCallbacks;
// <<

native function SetSmoothCamera(bool bEnabled);

cpptext
{
	virtual void Init();
	virtual void PreExit();
	virtual void InitDLC();
	virtual void SetDefaultURL(FURL& inout_DefaultURL);

	virtual void Tick(FLOAT DeltaSeconds);
	virtual void Send( ECallbackEventType InType );
	virtual void Send( ECallbackEventType InType, const FVector& InVector );
	
	static INT LanguageStrToIdx(const TCHAR* langStr);
	static FString LanguageIdxToString(ELanguage language);

	INT DeleteAllCloudSaves();
	void DeleteAllLocalSaves();

protected:
	virtual void PlayStartupMovie();

private:
	void CreateNewSaveFile();

	// (Windows/Mac)
	void WriteSaveData(const FString& Filename, const TArray<BYTE>& SaveData);
	UBOOL ReadSaveData(const FString& Filename, TArray<BYTE>& out_SaveData);
	UBOOL WriteToSteamCloud(const char* Filename, const TArray<BYTE>& SaveData);
	UBOOL ReadFromSteamCloud(const char* Filename, TArray<BYTE>& out_SaveData);
	UBOOL DeleteSteamCloudFile(const char* Filename);
	UBOOL FindSteamCloudSaveFiles(TArray<FString>& FileNames);	

	void SaveCheckpointData(const FName& CheckpointName, UBOOL bSaveToDisk);
	UBOOL LoadCheckpointData();

	void SetCurrentSaveData(const FString& Filename, const TArray<BYTE>& CheckpointData);

#if ORBIS
	void ProcessOrbisDialogs(FLOAT deltaSeconds);
#endif

#if DINGO
	void ProcessDingoCallbacks();
#endif
};

event RestartPlayer(OLPlayerController OLPC)
{
	local WorldInfo WI;
	local OLGame CurrentGame;
	
	WI = Class'WorldInfo'.static.GetWorldInfo();
	CurrentGame = OLGame(WI.Game);

	if (CurrentGame != None)
	{	
		CurrentGame.RestartPlayer(OLPC);
	}
}

native function bool CheckReloadForDLC(); // refresh DLC periodically, and reload if appropriate
native function bool RefreshDLC(); // updates and installs DLC if possible. Returns true if newly installed
native function bool ShouldShowNewDLCGame();
native function bool TryStartDLCGame(); // check for dlc ownership. return true if DLC is available, otherwise brings up steam/psn store and returns false.
native function bool UsingFixedSaveLocation(); // true on ps4
// >> PS4

delegate SaveDataDialogDoneCallback(bool bSuccess);
event FinishedSaveDataDialog(bool bSuccess)
{
	if (SaveDataDialogDoneCallback != None)
	{
		SaveDataDialogDoneCallback(bSuccess);
		SaveDataDialogDoneCallback = None;
	}
}

native function NativeSelectSaveLocation(string startCP);
function SelectSaveLocation(string startCP, delegate<SaveDataDialogDoneCallback> Callback)
{
	SaveDataDialogDoneCallback = Callback;
	NativeSelectSaveLocation(startCP);
}

native function NativeSelectAndLoadGame();
function SelectAndLoadGame(delegate<SaveDataDialogDoneCallback> Callback)
{
	SaveDataDialogDoneCallback = Callback;
	NativeSelectAndLoadGame();
}

// <<

//
// Checkpoints
// 

native function StartCurrentCheckpoint();
native function SaveCheckpoint(name CheckpointName, bool bSaveToDisk);
native function SaveCheckpointImmediate(name CheckpointName);
native function SaveAllCheckpoints();

//
// Save files 
//

native function bool HasValidSaveGame();
native function DebugSaveGame(string Filename);
native function DebugLoadGame(string Filename);

// (Windows/Mac)
native function bool LoadSaveFile(string Filename); // windows/mac
native function DeleteSaveFile(string Filename);
native function RefreshSaveFiles();

// (PS4)
native function bool PS4_LoadCurrentSaveFile(); 
native function bool PS4_LoadMostRecentSaveFile(); 

// (XboxOne)
native function bool ShouldOpenPressStartScreen();

native function Dingo_LoadGameResult Dingo_LoadSaveGame(string SaveFilename);
native function Dingo_LoadGameResult Dingo_LoadMostRecentSaveFile();
native function bool Dingo_StartNewGameWithSave(string startCP, string SaveFilename);
native function bool Dingo_StartNewGameWithNewSave(string startCP);
native function bool Dingo_SaveGameImmediate(string SaveFilename);
native function bool Dingo_SaveGameImmediateToNewSave();
native function bool Dingo_DeleteSaveGame(string SaveFilename);
native function bool Dingo_ShowHelpUI();
native function string Dingo_GetSaveFileIconName(name CheckpointName);
native function string Dingo_GetCheckpointTag(name CheckpointName);
native function string Dingo_GetActiveGamertag();

delegate DingoUserInitializedCallback(bool bSuccess);
native function DingoNative_StartInitializeUser();

function Dingo_InitializeUser(delegate<DingoUserInitializedCallback> Callback)
{
	DingoUserInitializedCallback = Callback;
	DingoNative_StartInitializeUser();
}

event OnDingoUserInitialized(bool bSuccess)
{
	`log("## OnDingoUserInitialized - " $ bSuccess);

	if (DingoUserInitializedCallback != None)
	{
		DingoUserInitializedCallback(bSuccess);
		DingoUserInitializedCallback = None;
	}
}

native function Dingo_AllowAllControllersInput();
native function int Dingo_OnInitialPressStart(int ControllerId);
native function bool Dingo_ShouldShowLoginUI(int ControllerId);
native function bool DingoNative_ShowLoginUIAndInitializeUser();
function bool Dingo_ShowLoginUIAndInitializeUser(delegate<DingoUserInitializedCallback> Callback)
{
	DingoUserInitializedCallback = Callback;
	return DingoNative_ShowLoginUIAndInitializeUser();
}

native function ReturnToPressStartScreen();


//
// System settings configuration
// 

native function bool ApplySystemSettings(OLProfileSettings ProfileSettings);
native function bool ApplyKeyBindings(OLProfileSettings ProfileSettings);
native function bool UpdateProfileFromSystemSettings(OLProfileSettings ProfileSettings);
native function bool UpdateProfileKeyBindingsFromSystem(OLProfileSettings ProfileSettings);

defaultproperties
{
	PendingNewLanguage=-1

	StartupMovieSound=AkEvent'Intro_Logo.SFX_Logo_intro'
}