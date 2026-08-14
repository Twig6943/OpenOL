/*=============================================================================
    RelayThread.cpp — global FRelayThread instance definition.
=============================================================================*/
// server.h (via openol_relay.h) handles the winsock2/ws2tcpip includes
// correctly for both unity and non-unity builds.  We only need to satisfy
// RelayThread.h's compile-time guard that winsock2 will be available.
#ifndef _WINSOCK2API_
#  define _WINSOCK2API_
#endif
#include "Multiplayer.h"
#include "RelayThread.h"

// One relay server per process.
FRelayThread GRelayThread;
