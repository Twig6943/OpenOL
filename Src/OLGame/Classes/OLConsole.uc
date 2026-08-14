/**
 * Copyright 2012 Red Barrels, Inc. All Rights Reserved.
 */
class OLConsole extends Console
	native;

var string CheatBuffer;
const CHEAT_BUFFER_MAX = 32;

struct native CheatEntry
{
    var string Code;
    var string Command;
};

var array<CheatEntry> CheatCodes;

native function AddCustomCommandsToAutoComplete();

function bool InputKey( int ControllerId, name Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad = FALSE )
{
	local OLCheatManager CheatMgr;

	if ( Event == IE_Pressed )
	{
		bCaptureKeyInput = false;

		CheatMgr = class'OLCheatManager'.static.GetCheatManager();
		if (CheatMgr != None)
		{
			if ( Key == ConsoleKey )
			{
				GotoState('Open');
				bCaptureKeyInput = true;
				return true;
			}
			else if ( Key == TypeKey )
			{
				GotoState('Typing');
				bCaptureKeyInput = true;
				return true;
			}
		}
	}

	return bCaptureKeyInput;
}

function bool InputChar( int ControllerId, string Unicode )
{
	local int i, BufLen, CodeLen;
	local string Lower;

	if (Len(Unicode) == 1 && Asc(Unicode) >= 32)
	{
		Lower = Locs(Unicode);
		CheatBuffer $= Lower;

		BufLen = Len(CheatBuffer);
		if (BufLen > CHEAT_BUFFER_MAX)
			CheatBuffer = Right(CheatBuffer, CHEAT_BUFFER_MAX);

		for (i = 0; i < CheatCodes.Length; i++)
		{
			CodeLen = Len(CheatCodes[i].Code);
			if (CodeLen > 0 && Right(CheatBuffer, CodeLen) == Locs(CheatCodes[i].Code))
			{
				ConsoleCommand(CheatCodes[i].Command);
				CheatBuffer = "";
				break;
			}
		}
	}

	return Super.InputChar(ControllerId, Unicode);
}

defaultproperties
{
	CheatCodes(0)=(Code="HELP",Command="ClearView")
	CheatCodes(1)=(Code="UNLIT",Command="Unlit")
	CheatCodes(2)=(Code="LIT",Command="Lit")
	CheatCodes(3)=(Code="POOF",Command="Ghost")
	CheatCodes(4)=(Code="GOD",Command="God")
}