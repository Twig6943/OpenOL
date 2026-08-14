/*=============================================================================
    RelayThread.h — FRelayThread: runs the embedded OpenOL relay server in a
    dedicated UE3 thread.  The game thread calls StartRelay / StopRelay;
    the relay runs independently in the background.
=============================================================================*/
#pragma once

// winsock2.h must come before UE3/Core headers (UE3 pulls winsock.h v1 via UnVcWin32.h).
// Any .cpp that includes RelayThread.h must include winsock2.h before Multiplayer.h/Core.h.
// We guard here to make the dependency explicit.
#if defined(_WIN32) && !defined(_WINSOCK2API_)
#  error "Include <winsock2.h> before RelayThread.h (add it at the top of the .cpp file)"
#endif

#include "Core.h"
#include "openol_relay.h"   // Server struct + relay_start / relay_stop
#include "P2PBridge.h"      // GP2PBridge — started/stopped alongside the relay

// ---------------------------------------------------------------------------
// FRelayThread
// ---------------------------------------------------------------------------

class FRelayThread : public FRunnable
{
public:
	FRelayThread()
		: Thread(NULL)
		, RelayServer(NULL)
		, bRunning(FALSE)
		, PendingPort(0)
		, PendingServerName(NULL)
		, PendingDbPath(NULL)
	{}

	// Start the relay server on the given port.  Safe to call from game thread.
	// Returns TRUE if the thread was created successfully.
	UBOOL StartRelay(WORD Port, const char* ServerName, const char* DbPath)
	{
		if (bRunning)
			return TRUE; // already running

		PendingPort       = Port;
		PendingServerName = ServerName;
		PendingDbPath     = DbPath;

		Thread = GThreadFactory->CreateThread(
			this,
			TEXT("OpenOLRelayThread"),
			FALSE,  // bAutoDeleteSelf
			FALSE,  // bAutoDeleteRunnable
			0,      // default stack size
			TPri_BelowNormal);

		debugf(NAME_Log, TEXT("FRelayThread::StartRelay port=%d thread=%s"),
			(int)Port, Thread ? TEXT("OK") : TEXT("FAILED"));

		if (Thread)
			GP2PBridge.Start(Port);

		return Thread != NULL;
	}

	// Stop the relay server.  Signals shutdown and waits for the thread to exit.
	void StopRelay()
	{
		if (!bRunning && !Thread)
			return;

		// Stop the P2P bridge first so it stops forwarding to the relay.
		GP2PBridge.StopBridge();

		// Signal server_run() loop to exit.
		if (RelayServer)
			relay_stop(RelayServer);

		if (Thread)
		{
			GThreadFactory->Destroy(Thread);
			Thread = NULL;
		}
		bRunning = FALSE;
	}

	UBOOL IsRunning() const { return bRunning; }

	// Direct access to the Server struct for ImGui stats/debug display.
	const Server* GetServer() const { return RelayServer; }

	// Read one log line from the history ring (call from game thread / ImGui).
	// Returns TRUE and fills OutMsg if a line is available.
	UBOOL PopLogLine(char OutMsg[HISTORY_MSG_LEN])
	{
		if (!RelayServer) return FALSE;
		return (UBOOL)history_pop(RelayServer, OutMsg);
	}

	// ---------------------------------------------------------------------------
	// FRunnable interface — called on the worker thread
	// ---------------------------------------------------------------------------

	virtual UBOOL Init() OVERRIDE
	{
		// Allocate Server on the heap — sizeof(Server) ~140 MB as a static would bloat the exe.
		RelayServer = (Server*)appMalloc(sizeof(Server));
		if (!RelayServer) return FALSE;
		appMemzero(RelayServer, sizeof(Server));
		relay_start(RelayServer, PendingPort, PendingServerName, PendingDbPath);
		// bRunning is set in Run() after Init() completes, ensuring RelayServer
		// and its history ring are fully initialised before any other thread reads them.
		return TRUE;
	}

	virtual DWORD Run() OVERRIDE
	{
		// Signal that Init() has fully completed and RelayServer is ready.
		bRunning = TRUE;
		// Blocking loop — exits when shutdown_requested is set by StopRelay().
		server_run(RelayServer);
		return 0;
	}

	virtual void Stop() OVERRIDE
	{
		// Called by GThreadFactory->Destroy() — ensure shutdown is signalled.
		if (RelayServer)
			relay_stop(RelayServer);
	}

	virtual void Exit() OVERRIDE
	{
		appFree(RelayServer);
		RelayServer = NULL;
		bRunning    = FALSE;
	}

private:
	FRunnableThread* Thread;
	Server*          RelayServer;   // heap-allocated in Init(); sizeof(Server) ~140 MB
	UBOOL            bRunning;

	// Parameters buffered until Init() is called on the worker thread.
	WORD        PendingPort;
	const char* PendingServerName;
	const char* PendingDbPath;
};

// ---------------------------------------------------------------------------
// Global instance — one relay per game process
// ---------------------------------------------------------------------------

extern FRelayThread GRelayThread;
