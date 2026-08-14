/*=============================================================================
    P2PBridge.cpp — FP2PBridge implementation.
=============================================================================*/
// winsock2.h before UE3 headers.
#ifdef _WIN32
#  ifndef _WINDOWS_
#    define WIN32_LEAN_AND_MEAN
#    ifndef NOMINMAX
#      define NOMINMAX
#    endif
#    include <winsock2.h>
#    include <ws2tcpip.h>
#  endif
#endif
#ifndef _WINSOCK2API_
#  define _WINSOCK2API_
#endif

#include "Multiplayer.h"
#include "..\..\OnlineSubsystemSteamworks\Inc\OnlineSubsystemSteamworks.h"
#include "P2PBridge.h"

FP2PBridge GP2PBridge;
FP2PBridge GP2PBridgeClient;

// C-callable wrapper so OnlineSubsystemSteamworks can forward P2PSessionRequest_t
// to the bridge without including P2PBridge.h (which pulls in winsock2.h).
void GP2PBridge_OnSessionRequest(QWORD SteamID)
{
    GP2PBridge.OnSessionRequest(SteamID);
}

// ---------------------------------------------------------------------------
// Server mode helpers
// ---------------------------------------------------------------------------

INT FP2PBridge::FindSlotBySteamID(QWORD SteamID) const
{
    for (INT i = 0; i < MAX_P2P_SLOTS; ++i)
        if (Slots[i].bUsed && Slots[i].SteamID == SteamID)
            return i;
    return -1;
}

INT FP2PBridge::FindSlotByVSock(UINT_PTR s) const
{
    for (INT i = 0; i < MAX_P2P_SLOTS; ++i)
        if (Slots[i].bUsed && Slots[i].VSock == s)
            return i;
    return -1;
}

INT FP2PBridge::AllocSlot(QWORD SteamID)
{
    INT Existing = FindSlotBySteamID(SteamID);
    if (Existing >= 0) return Existing;

    for (INT i = 0; i < MAX_P2P_SLOTS; ++i)
    {
        if (!Slots[i].bUsed)
        {
            // Bind a dedicated loopback socket for this client.
            // Store as UINT_PTR (== SOCKET on Win64) to avoid winsock header in P2PBridge.h.
            SOCKET RawSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (RawSock == INVALID_SOCKET) return -1;

            u_long nb = 1;
            ioctlsocket(RawSock, FIONBIO, &nb);

            int yes = 1;
            setsockopt(RawSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

            WORD VPort = (WORD)(RelayPort + BRIDGE_VPORT_OFFSET + i);

            struct sockaddr_in sa;
            appMemzero(&sa, sizeof(sa));
            sa.sin_family      = AF_INET;
            sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            sa.sin_port        = htons(VPort);

            if (bind(RawSock, (struct sockaddr*)&sa, sizeof(sa)) < 0)
            {
                debugf(NAME_Log, TEXT("FP2PBridge: bind vport %d failed (%d)"),
                    (int)VPort, WSAGetLastError());
                closesocket(RawSock);
                return -1;
            }

            Slots[i].bUsed   = TRUE;
            Slots[i].SteamID = SteamID;
            Slots[i].VSock   = (UINT_PTR)RawSock;
            Slots[i].VPort   = VPort;

            debugf(NAME_Log, TEXT("FP2PBridge: slot %d -> SteamID %llu vport %d"),
                i, (unsigned long long)SteamID, (int)VPort);
            return i;
        }
    }
    return -1;
}

void FP2PBridge::FreeSlot(INT idx)
{
    if (idx < 0 || idx >= MAX_P2P_SLOTS || !Slots[idx].bUsed) return;
    if (Slots[idx].VSock != (UINT_PTR)INVALID_SOCKET)
    {
        closesocket((SOCKET)Slots[idx].VSock);
        Slots[idx].VSock = (UINT_PTR)INVALID_SOCKET;
    }
    appMemzero(&Slots[idx], sizeof(Slots[idx]));
}

void FP2PBridge::CleanupSlots()
{
    for (INT i = 0; i < MAX_P2P_SLOTS; ++i)
        FreeSlot(i);

    // Client mode: close the listen socket.
    if (ClientSock != (UINT_PTR)INVALID_SOCKET)
    {
        closesocket((SOCKET)ClientSock);
        ClientSock = (UINT_PTR)INVALID_SOCKET;
    }
}

void FP2PBridge::DrainPendingAccepts()
{
    FScopeLock Lock(&PendingLock);
    for (INT i = 0; i < PendingAcceptCount; ++i)
    {
        QWORD SID = PendingAccept[i];
        if (GSteamNetworking)
            GSteamNetworking->AcceptP2PSessionWithUser(CSteamID((uint64)SID));
        AllocSlot(SID);
    }
    PendingAcceptCount = 0;
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

UBOOL FP2PBridge::Init()
{
    bClientReturnKnown = FALSE;

    if (bClientMode)
    {
        // Client mode: create a single UDP socket on 127.0.0.1:(RelayPort+1).
        // FMpConnection sends here; we forward to HostSteamID via Steam P2P.
        SOCKET RawSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (RawSock == INVALID_SOCKET)
        {
            debugf(NAME_Log, TEXT("FP2PBridge client: socket() failed (%d)"), WSAGetLastError());
            return FALSE;
        }

        u_long nb = 1;
        ioctlsocket(RawSock, FIONBIO, &nb);

        WORD ListenPort = (WORD)(RelayPort + BRIDGE_CLIENT_PORT_OFFSET);
        struct sockaddr_in sa;
        appMemzero(&sa, sizeof(sa));
        sa.sin_family      = AF_INET;
        sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        sa.sin_port        = htons(ListenPort);

        if (bind(RawSock, (struct sockaddr*)&sa, sizeof(sa)) < 0)
        {
            debugf(NAME_Log, TEXT("FP2PBridge client: bind port %d failed (%d)"),
                (int)ListenPort, WSAGetLastError());
            closesocket(RawSock);
            return FALSE;
        }

        ClientSock = (UINT_PTR)RawSock;

        debugf(NAME_Log, TEXT("FP2PBridge client: listen port %d, host SteamID %llu"),
            (int)ListenPort, (unsigned long long)HostSteamID);
    }
    else
    {
        // Server mode: prepare relay destination address.
        appMemzero(&RelayAddr, sizeof(RelayAddr));
        RelayAddr.sin_family      = AF_INET;
        RelayAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        RelayAddr.sin_port        = htons(RelayPort);

        debugf(NAME_Log, TEXT("FP2PBridge server: relay port %d, vports start at %d"),
            (int)RelayPort, (int)(RelayPort + BRIDGE_VPORT_OFFSET));
    }

    bRunning = TRUE;
    return TRUE;
}

// ---------------------------------------------------------------------------
// Run
// ---------------------------------------------------------------------------

DWORD FP2PBridge::Run()
{
    if (bClientMode)
        RunClient();
    else
        RunServer();
    return 0;
}

// ---------------------------------------------------------------------------
// RunServer — server (host) mode
// ---------------------------------------------------------------------------

void FP2PBridge::RunServer()
{
    BYTE Buf[BRIDGE_BUF_SIZE];

    while (!bStopRequested)
    {
        UBOOL bDidWork = FALSE;

        // Accept any pending sessions queued by the game-thread callback.
        DrainPendingAccepts();

        // P2P → relay: drain ALL channels 0-9 unconditionally.
        // Accept every sender immediately; allocate a slot on first contact.
        if (GSteamNetworking)
        {
            for (int ch = 0; ch <= 9; ++ch)
            {
                uint32 MsgSize = 0;
                while (GSteamNetworking->IsP2PPacketAvailable(&MsgSize, ch))
                {
                    if (MsgSize > BRIDGE_BUF_SIZE) MsgSize = BRIDGE_BUF_SIZE;

                    CSteamID RemoteID;
                    uint32   Read = 0;
                    if (!GSteamNetworking->ReadP2PPacket(Buf, MsgSize, &Read, &RemoteID, ch))
                        break;

                    // Always accept — idempotent for active sessions, required after re-open.
                    GSteamNetworking->AcceptP2PSessionWithUser(RemoteID);

                    QWORD SID  = RemoteID.ConvertToUint64();
                    INT   Slot = FindSlotBySteamID(SID);
                    if (Slot < 0)
                    {
                        debugf(NAME_Log, TEXT("[P2PBridge] server: new client SteamID=%llu ch=%d"),
                            (unsigned long long)SID, ch);
                        Slot = AllocSlot(SID);
                    }
                    if (Slot < 0) continue;

                    // Discard keepalive probes (< 2 bytes) — no relay payload.
                    if (Read < 2) { bDidWork = TRUE; continue; }

                    // Forward to relay via the client's virtual loopback socket.
                    sendto((SOCKET)Slots[Slot].VSock, (const char*)Buf, (int)Read, 0,
                        (struct sockaddr*)&RelayAddr, sizeof(RelayAddr));

                    bDidWork = TRUE;
                } // while IsP2PPacketAvailable
            } // for ch
        } // if GSteamNetworking

        // Relay → P2P: poll each vport socket for replies, forward to the Steam client.
        for (INT i = 0; i < MAX_P2P_SLOTS; ++i)
        {
            if (!Slots[i].bUsed) continue;

            int Received;
            struct sockaddr_in From;
            int FromLen = sizeof(From);

            while ((Received = recvfrom((SOCKET)Slots[i].VSock, (char*)Buf, BRIDGE_BUF_SIZE,
                0, (struct sockaddr*)&From, &FromLen)) > 0)
            {
                if (GSteamNetworking)
                {
                    CSteamID DestID((uint64)Slots[i].SteamID);
                    bool bSent = GSteamNetworking->SendP2PPacket(DestID, Buf, (uint32)Received,
                        k_EP2PSendUnreliable, BRIDGE_P2P_CHANNEL);
                    if (!bSent)
                        debugf(NAME_Log, TEXT("[P2PBridge] server: SendP2PPacket FAILED slot=%d SteamID=%llu"),
                            i, (unsigned long long)Slots[i].SteamID);
                }
                bDidWork = TRUE;
                FromLen = sizeof(From);
            }
        }

        if (!bDidWork)
            appSleep(0.001f);
    }
}

// ---------------------------------------------------------------------------
// RunClient — client mode: FMpConnection ↔ Steam P2P ↔ remote relay
// ---------------------------------------------------------------------------

void FP2PBridge::RunClient()
{
    BYTE     Buf[BRIDGE_BUF_SIZE];
    CSteamID HostID((uint64)HostSteamID);
    INT      DiagTick = 0;
    UBOOL    bConnected = FALSE;

    while (!bStopRequested)
    {
        UBOOL bDidWork = FALSE;

        // Every ~5 seconds: log state and send Reliable keepalive while not connected.
        if (GSteamNetworking && (++DiagTick % 5000) == 0)
        {
            P2PSessionState_t State;
            if (GSteamNetworking->GetP2PSessionState(HostID, &State))
            {
                debugf(NAME_Log, TEXT("[P2PBridge] client: P2PState conn=%d relay=%d bytes=%u err=%d"),
                    (int)State.m_bConnectionActive, (int)State.m_bUsingRelay,
                    (unsigned)State.m_nBytesQueuedForSend, (int)State.m_eP2PSessionError);
                bConnected = State.m_bConnectionActive;
            }
            else
            {
                debugf(NAME_Log, TEXT("[P2PBridge] client: no session yet"));
                bConnected = FALSE;
            }

            // Send Reliable keepalive while not yet connected to trigger NAT traversal.
            if (!bConnected)
            {
                BYTE dummy = 0;
                GSteamNetworking->SendP2PPacket(HostID, &dummy, 1,
                    k_EP2PSendReliable, BRIDGE_P2P_CHANNEL);
            }
        }

        // 1. FMpConnection → P2P: forward all UDP to host unconditionally.
        {
            int Received;
            struct sockaddr_in From;
            int FromLen = sizeof(From);

            while ((Received = recvfrom((SOCKET)ClientSock, (char*)Buf, BRIDGE_BUF_SIZE,
                0, (struct sockaddr*)&From, &FromLen)) > 0)
            {
                if (!bClientReturnKnown)
                {
                    ClientReturnAddr   = From;
                    bClientReturnKnown = TRUE;
                    debugf(NAME_Log, TEXT("[P2PBridge] client: first UDP from FMpConn, port=%d"),
                        (int)ntohs(From.sin_port));
                }

                if (GSteamNetworking)
                    GSteamNetworking->SendP2PPacket(HostID, Buf, (uint32)Received,
                        k_EP2PSendUnreliable, BRIDGE_P2P_CHANNEL);

                bDidWork = TRUE;
                FromLen  = sizeof(From);
            }
        }

        // 2. P2P → FMpConnection: read all channels, forward to local UDP socket.
        if (GSteamNetworking && bClientReturnKnown)
        {
            for (int ch = 0; ch <= 9; ++ch)
            {
                uint32 MsgSize = 0;
                while (GSteamNetworking->IsP2PPacketAvailable(&MsgSize, ch))
                {
                    if (MsgSize > BRIDGE_BUF_SIZE) MsgSize = BRIDGE_BUF_SIZE;

                    CSteamID RemoteID;
                    uint32   Read = 0;
                    if (!GSteamNetworking->ReadP2PPacket(Buf, MsgSize, &Read, &RemoteID, ch))
                        break;

                    if (RemoteID.ConvertToUint64() != HostSteamID) continue;
                    if (Read < 2) continue; // discard probes

                    sendto((SOCKET)ClientSock, (const char*)Buf, (int)Read, 0,
                        (struct sockaddr*)&ClientReturnAddr, sizeof(ClientReturnAddr));

                    bDidWork = TRUE;
                }
            }
        }

        if (!bDidWork)
            appSleep(0.001f);
    }
}
