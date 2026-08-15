/*=============================================================================
    openol_relay.h — public API for the embedded relay server static lib.

    Usage from OLGame C++ code:
      #include "openol_relay.h"  (after winsock2.h is already included)
      Server GRelayServer;
      relay_start(&GRelayServer, 7777, "MyServer", "relay.json");
      // run server_run(&GRelayServer) in a FRunnableThread
      relay_stop(&GRelayServer);   // from another thread
=============================================================================*/
#pragma once

// NOTE: winsock2.h must already be included by the caller before this header.
// In OLGame, winsock2.h is included via the Windows platform layer.
#include "server.h"

/* Start: initialise Server struct and open UDP socket. Non-blocking.
   Call server_run(s) in a dedicated thread afterwards. */
void relay_start(Server *s, unsigned short port, const char *name, const char *db_path);

/* Stop: signal server_run() to exit and close the socket. Thread-safe. */
void relay_stop(Server *s);
