/**
 * Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
 */

#include "EnginePrivate.h"
#include "DownloadableContent.h"
#include "UnIpDrv.h"

#if WITH_UE3_NETWORKING
FLOAT FOnlineAsyncTask::DefaultMinCompletionTime = 0.0f;
#endif

#define EMERGENCY_PINGCAST 1

IMPLEMENT_CLASS(UOnlineSubsystem);
IMPLEMENT_CLASS(UOnlineGameSettings);
IMPLEMENT_CLASS(UOnlineGameSearch);
IMPLEMENT_CLASS(UOnlineMatchmakingStats);
IMPLEMENT_CLASS(UOnlineAuthInterface);
IMPLEMENT_CLASS(UOnlineEventTracker);

#define EVENT_TRACKER_INTERFACE_NAME FName(TEXT("OnlineEventTracker"))

/**
 * Loads and creates any registered named interfaces
 */
UBOOL UOnlineSubsystem::Init(void)
{
#if WITH_UE3_NETWORKING
	// Set default minimum completion time for all asynch tasks
	FOnlineAsyncTask::DefaultMinCompletionTime = AsyncMinCompletionTime;
#endif
	// Iterate through each configured named interface load it and create an instance
	for (INT InterfaceIndex = 0; InterfaceIndex < NamedInterfaceDefs.Num(); InterfaceIndex++)
	{
		const FNamedInterfaceDef& Def = NamedInterfaceDefs(InterfaceIndex);
		// Load the specified interface class name
		UClass* Class = LoadClass<UObject>(NULL,*Def.InterfaceClassName,NULL,LOAD_None,NULL);
		if (Class)
		{
			INT AddIndex = NamedInterfaces.AddZeroed();
			FNamedInterface& Interface = NamedInterfaces(AddIndex);
			// Set the object and interface names
			Interface.InterfaceName = Def.InterfaceName;
			Interface.InterfaceObject = ConstructObject<UObject>(Class);
			debugf(NAME_DevOnline,
				TEXT("Created named interface (%s) of type (%s)"),
				*Def.InterfaceName.ToString(),
				*Def.InterfaceClassName);
		}
		else
		{
			debugf(NAME_DevOnline,
				TEXT("Failed to load class (%s) for named interface (%s)"),
				*Def.InterfaceClassName,
				*Def.InterfaceName.ToString());
		}
	}

	UOnlineEventTracker* EventTracker = Cast<UOnlineEventTracker>(eventGetNamedInterface(EVENT_TRACKER_INTERFACE_NAME));
	if (EventTracker)
	{
		EventTracker->eventInit();
	}

	return TRUE;
}

/**
* Called from the engine shutdown code to allow the subsystem to release any
* resources that may have been allocated
*/
void UOnlineSubsystem::Exit()
{
	UOnlineEventTracker* EventTracker = Cast<UOnlineEventTracker>(eventGetNamedInterface(EVENT_TRACKER_INTERFACE_NAME));
	if (EventTracker)
	{
		EventTracker->eventShutDown();
	}
}

/**
 * Signals the online service that the given event occurred
 *
 * @param EventName the name of the event to signal
 * @param EventParams the parameters to pass with the event
 */
UBOOL UOnlineSubsystem::RaiseEvent(const FString& EventName, const TArray<FString>& EventParams)
{
	UOnlineEventTracker* EventTracker = Cast<UOnlineEventTracker>(eventGetNamedInterface(EVENT_TRACKER_INTERFACE_NAME));
	if (EventTracker)
	{
		return EventTracker->eventRaiseEvent(EventName, EventParams);
	}
	
	warnf(TEXT("Attempting to RaiseEvent %s with no OnlineEventTracker in the OnlineSubsystem NamedInterfaces"), *EventName);
	return FALSE;
}

/**
 * Generates a string representation of a UniqueNetId struct.
 *
 * @param	IdToConvert		the unique net id that should be converted to a string.
 *
 * @return	the specified UniqueNetId represented as a string.
 */
FString UOnlineSubsystem::UniqueNetIdToString( const FUniqueNetId& IdToConvert, UBOOL Hex /*= TRUE*/ )
{
#if PS3 || IPHONE
	FString Result = FString::Printf(TEXT("%llu"), (QWORD&)IdToConvert.Uid);
#else
	FString Result = Hex?
		FString::Printf(TEXT("0x%016I64X"), (QWORD&)IdToConvert.Uid):
		FString::Printf(TEXT("%llu"), (QWORD&)IdToConvert.Uid);
#endif
	return Result;
}

FORCEINLINE INT HexDigit(TCHAR c)
{
	INT Result = 0;

	if (c >= '0' && c <= '9')
	{
		Result = c - '0';
	}
	else if (c >= 'a' && c <= 'f')
	{
		Result = c + 10 - 'a';
	}
	else if (c >= 'A' && c <= 'F')
	{
		Result = c + 10 - 'A';
	}
	else
	{
		Result = 0;
	}

	return Result;
}

/**
 * Converts a string representing a UniqueNetId into a UniqueNetId struct.
 *
 * @param	UniqueNetIdString	the string containing the text representation of the unique id.
 * @param	out_UniqueId		will receive the UniqueNetId generated from the string.
 *
 * @return	TRUE if the string was successfully converted into a UniqueNetId; FALSE if the string was not a valid UniqueNetId.
 */
UBOOL UOnlineSubsystem::StringToUniqueNetId( const FString& UniqueNetIdString, FUniqueNetId& out_UniqueId )
{
	UBOOL bResult=FALSE;

	// strip off the leading 0x, if it was included.
	INT Start=0;
	if ( UniqueNetIdString.Left(2) == TEXT("0x") )
	{
		Start=2;
	}

	QWORD& ConvertedValue = (QWORD&)out_UniqueId.Uid;
	ConvertedValue = 0;
	for ( INT Idx = Start; Idx < UniqueNetIdString.Len(); Idx++ )
	{
		INT NextDigit = HexDigit(UniqueNetIdString[Idx]);
		if ( NextDigit == 0 && UniqueNetIdString[Idx] != TEXT('0') )
		{
			break;
		}

		if ( Idx != Start )
		{
			ConvertedValue <<= 4;
		}
		ConvertedValue |= NextDigit;
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Generates a unique number based off of the current script compilation
 *
 * @return the unique number from the current script compilation
 */
INT UOnlineSubsystem::GetBuildUniqueId(void)
{
	INT Crc = 0;
	if (bUseBuildIdOverride == FALSE)
	{
		UPackage* EnginePackage = UEngine::StaticClass()->GetOutermost();
		if (EnginePackage)
		{
			// Use the GUID of the engine package to determine a unique CRC
			Crc = appMemCrc(&EnginePackage->Guid,sizeof(FGuid));
		}
	}
	else
	{
		Crc = BuildIdOverride;
	}
	return Crc;
}

/**
 * Returns the number of players that can be signed in on this platform
 */
INT UOnlineSubsystem::GetNumSupportedLogins(void)
{
#if _XBOX
	return 4;
#elif DINGO
	return MAX_NUM_PLAYERS_DINGO;
#else
	return 1;
#endif
}

//Decodes an array that is base-64 encoded
TArray<BYTE> UOnlineSubsystem::DecodeBase64(const TArray<BYTE>& Encoded)
{
	TArray<BYTE> DecodedArray;
	DecodedArray.AddZeroed(((Encoded.Num()) / 4) * 3);
	BYTE* Decoded = DecodedArray.GetData();

	FString Base64Map(TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"));
	BYTE ch = '\0';
	INT i=0, j=0;
	TCHAR Junk[2] = {0, 0};
	INT idx = 0;

	for(INT idx=0; idx<Encoded.Num(); idx++)
	{
		ch = Encoded(idx);
		if (ch == '=')
			break;

		Junk[0] = ch;
		ch = Base64Map.InStr(Junk);
		if( ch == -1 )
		{
			return TArray<BYTE>();
		}

		switch(i % 4) {
		case 0:
			Decoded[j] = ch << 2;
			break;
		case 1:
			Decoded[j++] |= ch >> 4;
			Decoded[j] = (ch & 0x0f) << 4;
			break;
		case 2:
			Decoded[j++] |= ch >>2;
			Decoded[j] = (ch & 0x03) << 6;
			break;
		case 3:
			Decoded[j++] |= ch;
			break;
		}
		i++;
	}

	//Restore the original size of the data
	check(j <= DecodedArray.Num());
	DecodedArray.SetNum(j);

	return DecodedArray;
}

//Encodes a data array into base-64
TArray<BYTE> UOnlineSubsystem::EncodeBase64(const TArray<BYTE>& Decoded)
{
	TArray<BYTE> EncodedArray(((Decoded.Num() + 2) / 3) * 4);
	BYTE* Encoded = EncodedArray.GetData();

	FString Base64Map(TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"));

	INT i=0, j=0;
	BYTE x,y,z;

	while (i < Decoded.Num())
	{
		x = Decoded(i);
		y = (i < Decoded.Num()-1 ) ? (INT) Decoded(i+1) : 0;
		z = (i < Decoded.Num()-2 ) ? (INT) Decoded(i+2) : 0;
		Encoded[j++] = (BYTE)Base64Map[x >> 2];
		Encoded[j++] = (BYTE)Base64Map[((x & 3) << 4) | (y >> 4)];
		Encoded[j++] = (BYTE)Base64Map[((y & 15) << 2) | (z >> 6)];
		Encoded[j++] = (BYTE)Base64Map[(z & 63)];
		i+=3;
	}

	switch (Decoded.Num() % 3)
	{
	case 1:
		Encoded[j-2] = '=';
	case 2:
		Encoded[j-1] = '=';
	}

	check(j == EncodedArray.Num());
	return EncodedArray;
}

/**
 * Mark timer as started and store current offset in Secs
 */
void UOnlineMatchmakingStats::StartTimer(struct FMMStats_Timer& Timer)
{
	Timer.MSecs = appSeconds();
	Timer.bInProgress = TRUE;
}
/**
 * Mark timer as stopped and store delta in MSecs
 */
void UOnlineMatchmakingStats::StopTimer(struct FMMStats_Timer& Timer)
{
	if (Timer.bInProgress)
	{
		Timer.MSecs = (appSeconds() - Timer.MSecs) * 1000;
		Timer.bInProgress = FALSE;
	}
}

#if WITH_UE3_NETWORKING
UBOOL FLanBeacon::BroadcastPacket(BYTE* Packet,INT Length)
{
#if WITH_IPV6
	warnf(TEXT("FLanBeacon::BroadcastPacket not supported with IPV6"));
	return FALSE;
#else
	INT BytesSent = 0;
	UBOOL bTotalSuccess = TRUE;

	// For right now let's try hitting every in our area and see if we can get a response on our port
#if EMERGENCY_PINGCAST
	// If we weren't totally successful with sending the broadcast, and we want to use this hack, do it!
	FInternetIpAddr localAddr;
	GSocketSubsystem->GetLocalHostAddr(*GLog, localAddr);
	FIpAddr Addr = localAddr.GetAddress();

	debugf(TEXT("Got IP: Address is:%s Major bytes [0]{%#x} [1]{%#x} [2]{%#x} [3]{%#x}"),
		*localAddr.ToString(FALSE),
		(Addr.Addr & 0xFF),
		(Addr.Addr & 0xFF00) >> 8,
		(Addr.Addr & 0xFF0000) >> 16,
		(Addr.Addr & 0xFF000000) >> 24);

	// Don't do it if we got ANY or BROADCAST - waste of time
	if( Addr.Addr != 0x0 && Addr.Addr != 0xFFFFFFFF)
	{
		for(BYTE i = 2; i < 255; ++i)
		{
			FInternetIpAddr sendToAddr;	

			sendToAddr.SetIp(
				(Addr.Addr & 0xFF000000) >> 24,
				(Addr.Addr & 0xFF0000) >> 16,
				(Addr.Addr & 0xFF00) >> 8,
				i);
#if ORBIS
			sendToAddr.SetVPort(BroadcastAddr.GetVPort());
#endif
			sendToAddr.SetPort(BroadcastAddr.GetPort());
			debugf(TEXT("Sending LanBeacon Ping ip: %s"), *sendToAddr.ToString(TRUE));
			UBOOL bSuccess = ListenSocket->SendTo(Packet,Length,BytesSent,sendToAddr) && BytesSent == Length;
			if(!bSuccess)
			{
				debugf(TEXT("Failed to send to ip: %s"), *sendToAddr.ToString(TRUE));
			}

			bTotalSuccess &= bSuccess;
		}
	}
#else
	debugf(TEXT("Sending broadcast to ip: %s"), *BroadcastAddr.ToString(TRUE));

	UBOOL bSuccess = ListenSocket->SendTo(Packet,Length,BytesSent,BroadcastAddr) && BytesSent == Length;
	if(!bSuccess)
	{
		debugf(TEXT("Failed to send broadcast packet"));
		bTotalSuccess = FALSE;
	}
#endif
	return bTotalSuccess;

#endif
}
#endif // WITH_UE3_NETWORKING
