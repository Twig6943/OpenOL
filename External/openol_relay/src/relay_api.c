/*=============================================================================
    relay_api.c — thin convenience wrappers around server_init / server_shutdown.
=============================================================================*/
#include "server.h"
#include "relay_compat.h"  /* polyfills after winsock2 is available */

#define RELAY_DEFAULT_ROOM "DEFAULT"

void relay_start(Server *s, unsigned short port, const char *name, const char *db_path) {
    /* UE3 initialises winsock v1 via GSocketSubsystem, but the relay needs
       winsock2 (sendto/recvfrom/select on SOCKET).  Call WSAStartup here so
       the embedded relay works regardless of UE3's socket state. */
    platform_init();
    server_init(s, port, name, DEFAULT_IP, db_path);

    /* Always ensure a default room exists — clients join it without needing
       a room code (they send "DEFAULT" as the room name, no password). */
    if (!server_find_room(s, RELAY_DEFAULT_ROOM))
        server_add_room(s, RELAY_DEFAULT_ROOM, "");
}

void relay_stop(Server *s) {
    s->shutdown_requested = 1;
    server_shutdown(s);
    platform_cleanup();
}
