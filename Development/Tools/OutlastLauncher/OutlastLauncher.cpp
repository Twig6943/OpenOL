// OutlastLauncher.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "OutlastLauncher.h"


int _tmain(int argc, _TCHAR* argv[])
{
	bool bNoSteam = false;

	for (int i = 1; i < argc; i++)
	{
		if (_tcsicmp(argv[i], TEXT("-nosteam")) == 0)
		{
			bNoSteam = true;
		}
	}

#ifdef NOSTEAM
	bNoSteam = true;
#endif

	TCHAR cmdLine[MAX_PATH * 2];
	if (bNoSteam)
		lstrcpy(cmdLine, TEXT(" -installed -nohomedir -seekfreeloadingpcconsole -nosteam"));
	else
		lstrcpy(cmdLine, TEXT(" -installed -nohomedir -seekfreeloadingpcconsole"));

	PROCESS_INFORMATION processInformation;
	STARTUPINFO startupInfo;
	memset(&processInformation, 0, sizeof(processInformation));
	memset(&startupInfo, 0, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);

	BOOL bOk = CreateProcess(TEXT("Binaries\\Win64\\OLGame.exe"), cmdLine,
		NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInformation);

	if (!bOk)
	{
		DWORD errCode = GetLastError();
		fprintf(stderr, "OutlastLauncher: CreateProcess failed with error code: 0x%08x", errCode);
		return errCode;
	}

	CloseHandle(processInformation.hProcess);
	CloseHandle(processInformation.hThread);

	return 0;
}

