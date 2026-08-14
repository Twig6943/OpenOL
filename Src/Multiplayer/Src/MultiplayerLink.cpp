/*=============================================================================
    MultiplayerLink.cpp — FMpConnection: persistent UDP connection singleton.
=============================================================================*/
#include "Multiplayer.h"
#include "HeroChannelPackets.h"
#include "ServerPackets.h"
#include "WorldChannelPackets.h"
#include "UnSocket.h"

#if WITH_UE3_NETWORKING

IMPLEMENT_CLASS(AMultiplayerLink);

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

FMpConnection        GMpConn;
static FMpConnectionTicker* GMpTicker = NULL;

// Global pointers — set each map load by AMultiplayerController::NativeInit.
AMultiplayerController* GMultiplayerController = NULL;
AOLHero*                GMultiplayerHero       = NULL;

FResolveInfo* GResolveInfo = NULL;

// NativeReloadConfig — called from UC SaveNetworkSettings to push updated
// config strings into the live FMpConnection without restarting the process.
void AMultiplayerLink::NativeReloadConfig()
{
    GMpConn.bResolved = FALSE;
    GResolveInfo          = NULL;
    GMpConn.LoadConfig();
}


// ---------------------------------------------------------------------------
// FMpConnection — config
// ---------------------------------------------------------------------------

void FMpConnection::LoadConfig()
{
    // Read exclusively from OLGame.OLNetworkConfig — single source of truth.
    const FString IniFile = appGameConfigDir() + TEXT("OLMultiplayer.ini");
    GConfig->LoadFile(*IniFile);
    const TCHAR* Ini  = *IniFile;
    const TCHAR* Sect = TEXT("OLGame.OLNetworkConfig");

    FString Tmp;
    if (GConfig->GetString(Sect, TEXT("IP"),       Tmp, Ini)) IP       = Tmp;
    if (GConfig->GetString(Sect, TEXT("UdpPort"),  Tmp, Ini)) UdpPort  = Tmp;
    if (GConfig->GetString(Sect, TEXT("UserName"), Tmp, Ini)) Username = Tmp;
    if (GConfig->GetString(Sect, TEXT("RoomCode"), Tmp, Ini)) RoomCode = Tmp;
    if (GConfig->GetString(Sect, TEXT("Password"), Tmp, Ini)) Password = Tmp;

    UBOOL B = TRUE;
    if (GConfig->GetBool(Sect, TEXT("SyncInteractable"), B, Ini)) SyncInteractable = B; B = TRUE;
    if (GConfig->GetBool(Sect, TEXT("SyncEnemies"),      B, Ini)) SyncEnemies      = B; B = TRUE;
    if (GConfig->GetBool(Sect, TEXT("SyncMatinees"),     B, Ini)) SyncMatinees     = B; B = TRUE;
    if (GConfig->GetBool(Sect, TEXT("SyncPickups"),      B, Ini)) SyncPickups      = B;

    if (IP.IsEmpty())       IP       = TEXT("127.0.0.1");
    if (UdpPort.IsEmpty())  UdpPort  = TEXT("7777");
    if (Username.IsEmpty()) Username = TEXT("Player");
    if (RoomCode.IsEmpty()) RoomCode = TEXT("PUBLIC");


}

// ---------------------------------------------------------------------------
// FMpConnection::Connect — idempotent; starts DNS resolve on first call.
// ---------------------------------------------------------------------------

void FMpConnection::Connect()
{
    FString OldIP       = IP;
    FString OldUdpPort  = UdpPort;
    FString OldRoomCode = RoomCode;
    FString OldPassword = Password;

    LoadConfig();

    // If connection params changed, drop the existing connection and re-resolve.
    if (bResolved && (IP != OldIP || UdpPort != OldUdpPort || RoomCode != OldRoomCode || Password != OldPassword))
    {
        Disconnect();
        bResolved     = FALSE;
        bIsConnected  = FALSE;
        bIsHandshaked = FALSE;
        HelloTimer    = 0.f;
        if (GResolveInfo) { delete GResolveInfo; GResolveInfo = NULL; }
    }

    if (bResolved || GResolveInfo)
        return;

    // GetHostByName handles both dotted IPs (returns cached immediately)
    // and hostnames (async). We poll IsComplete() in Tick.
    GResolveInfo = GSocketSubsystem->GetHostByName(TCHAR_TO_ANSI(*IP));
}

// ---------------------------------------------------------------------------
// FMpConnection::SendBinary
// ---------------------------------------------------------------------------

void FMpConnection::SendBinary(BYTE* Data, INT Count)
{
    if (Count <= 0 || Count > 4091)
        return;

    BYTE Out[4096];
    Out[0] = Data[0];
    Out[1] = (BYTE)(LocalPlayerID);
    Out[2] = (BYTE)(LocalPlayerID >>  8);
    Out[3] = (BYTE)(LocalPlayerID >> 16);
    Out[4] = (BYTE)(LocalPlayerID >> 24);
    appMemcpy(Out + 5, Data + 1, Count - 1);

    SendTo(ServerAddr, Out, Count + 4);
}

// ---------------------------------------------------------------------------
// FMpConnection::Disconnect — send DISCONNECT packet, reset state.
// ---------------------------------------------------------------------------

void FMpConnection::Disconnect()
{
    if (!bResolved || !bIsConnected)
        return;

    BYTE B[1] = { MPKT_WORLD_DISCONNECT };
    SendBinary(B, 1);

    bIsConnected  = FALSE;
    bIsHandshaked = FALSE;
}

// ---------------------------------------------------------------------------
// FMpConnection::SendText — legacy path for SendToServer(string).
// ---------------------------------------------------------------------------

void FMpConnection::SendText(const FString& Msg)
{
    if (!bResolved || !bIsConnected)
        return;
    FString Line = FString::Printf(TEXT("%d,%s\n"), LocalPlayerID, *Msg);
    FTCHARToANSI Conv(*Line);
    SendTo(ServerAddr, (BYTE*)(ANSICHAR*)Conv, Conv.Length());
}

// ---------------------------------------------------------------------------
// FMpConnection::OnReceivedData — FUdpLink callback (game thread via Poll).
// ---------------------------------------------------------------------------

void FMpConnection::OnReceivedData(FIpAddr SrcAddr, BYTE* Data, INT Count)
{
    if (Count <= 0)
        return;

    if (GWorld && GWorld->GetWorldInfo())
        LastReceivedTime = GWorld->GetWorldInfo()->TimeSeconds;

    // All packets are binary. Layout: [type(1)][sender_id LE4][payload...]
    if (Count >= 1)
    {
        if (Count < 5)
            return;

        BYTE  PktType  = Data[0];
        INT   SenderID = (INT)((DWORD)Data[1] | ((DWORD)Data[2]<<8) |
                               ((DWORD)Data[3]<<16) | ((DWORD)Data[4]<<24));
        BYTE* Payload    = Data + 5;
        INT   PayloadLen = Count - 5;

        // PING echo: [type(1)][player_id(4)][sent_ms LE u32(4)]
        if (PktType == MPKT_PING)
        {
            if (PayloadLen >= 4 && GMultiplayerController)
            {
                DWORD SentMs = (DWORD)Payload[0] | ((DWORD)Payload[1] << 8)
                             | ((DWORD)Payload[2] << 16) | ((DWORD)Payload[3] << 24);
                DWORD NowMs  = (DWORD)(appSeconds() * 1000.0);
                FLOAT RTT    = (FLOAT)(NowMs - SentMs);
                if (RTT > 0.f && RTT < 10000.f)
                {
                    GMultiplayerController->CurrentPingMs = RTT;
                    AWorldInfo* WI = GWorld ? GWorld->GetWorldInfo() : NULL;
                    GMultiplayerController->LastPongTime = WI ? WI->TimeSeconds : 0.f;
                }
            }
            return;
        }

        // SRV_READY (0xE0) — always parse server name/player-id into GMpConn.
        if (PktType == 0xE0 && PayloadLen >= 4)
        {
            INT PID = (INT)((DWORD)Payload[0] | ((DWORD)Payload[1]<<8) |
                            ((DWORD)Payload[2]<<16) | ((DWORD)Payload[3]<<24));
            LocalPlayerID = PID;

            // Parse server name from payload: [player_id LE4][name_len(1)][name...][token(32)]
            if (PayloadLen >= 5)
            {
                BYTE NLen = Payload[4];
                INT  NBytes = Min((INT)NLen, PayloadLen - 5);
                TCHAR TmpName[256] = {0};
                for (INT i = 0; i < NBytes && i < 255; i++)
                    TmpName[i] = (TCHAR)Payload[5 + i];
                ServerName = FString(TmpName);

                // Parse session token appended after name (32 bytes)
                INT TokenOffset = 5 + NBytes;
                if (PayloadLen >= TokenOffset + 32)
                {
                    appMemcpy(SessionToken, Payload + TokenOffset, 32);
                    bHasSessionToken = TRUE;
                }
            }
            else
            {
                ServerName = TEXT("Server");
            }

            if (!bIsConnected)
            {
                bIsConnected = TRUE;
                HelloAttempt = 0;
                if (GMultiplayerController)
                    GMultiplayerController->OnConnected();
            }
            // Sync UC controller with current persistent state.
            if (GMultiplayerController)
            {
                GMultiplayerController->ServerName  = ServerName;
                GMultiplayerController->OnlineCount = OnlineCount;
            }
        }

        // SRV_ONLINE_COUNT — always update GMpConn so it survives map transitions.
        if (PktType == SRV_ONLINE_COUNT && PayloadLen >= 4)
        {
            INT Count = (INT)((DWORD)Payload[0] | ((DWORD)Payload[1]<<8) |
                              ((DWORD)Payload[2]<<16) | ((DWORD)Payload[3]<<24));
            OnlineCount = Max(Count, 1);
        }

        if (GMultiplayerController)
            GMultiplayerController->OnReceiveBinaryData(PktType, SenderID, Payload, PayloadLen);
        return;
    }

}

// ---------------------------------------------------------------------------
// FMpConnection::Tick — polls socket, runs HELLO retries, heartbeat check.
// ---------------------------------------------------------------------------

void FMpConnection::Tick(FLOAT DeltaTime)
{
    // Check async DNS resolve.
    if (GResolveInfo)
    {
        if (GResolveInfo->IsComplete())
        {
            if (GResolveInfo->GetErrorCode() != SE_NO_ERROR)
            {
                // DNS failed — notify and retry next connect attempt
                delete GResolveInfo;
                GResolveInfo = NULL;
                if (GMultiplayerController)
                    GMultiplayerController->OnDisconnected();
                return;
            }
            FInternetIpAddr Resolved = GResolveInfo->GetResolvedAddress();
            delete GResolveInfo;
            GResolveInfo = NULL;

            // FIpAddr has a constructor from FInternetIpAddr (Core.h).
            ServerAddr      = FIpAddr(Resolved);
            ServerAddr.Port = appAtoi(*UdpPort);
            bResolved = TRUE;
            // Global FMpConnection is constructed before GSocketSubsystem is ready,
            // so the socket must be created here, after subsystem init.
            if (SocketData.Socket == NULL && GSocketSubsystem)
            {
                SocketData.Socket = GSocketSubsystem->CreateDGramSocket(TEXT("MpConn"), TRUE);
                if (SocketData.Socket)
                {
                    SocketData.Socket->SetReuseAddr();
                    SocketData.Socket->SetNonBlocking();
                    SocketData.Socket->SetRecvErr();
                }
            }
            BindPort(0);
        }
        // Else: still resolving, wait.
        return;
    }

    // Poll socket for incoming datagrams.
    if (bResolved)
        Poll();

    if (!bResolved)
    {
        if (!GResolveInfo)
            Connect();
        return;
    }

    // HELLO retry loop — send until connected.
    if (!bIsConnected)
    {
        HelloTimer -= DeltaTime;
        if (HelloTimer <= 0.f)
        {
            HelloTimer = 2.0f;
            // Build HELLO: include session token (hex64) if we have one for NAT rebind
            FString Msg;
            if (bHasSessionToken)
            {
                // Encode 32-byte token as 64 lowercase hex chars
                TCHAR HexToken[65] = {0};
                const TCHAR HexChars[] = TEXT("0123456789abcdef");
                for (INT i = 0; i < 32; i++)
                {
                    HexToken[i * 2]     = HexChars[(SessionToken[i] >> 4) & 0xF];
                    HexToken[i * 2 + 1] = HexChars[SessionToken[i] & 0xF];
                }
                Msg = FString::Printf(TEXT("HELLO,%s,%s,%s\n"), *RoomCode, *Password, HexToken);
            }
            else
            {
                Msg = FString::Printf(TEXT("HELLO,%s,%s\n"), *RoomCode, *Password);
            }
            FTCHARToANSI Conv(*Msg);
            SendTo(ServerAddr, (BYTE*)(ANSICHAR*)Conv, Conv.Length());
        }
        return;
    }

    // PING — send every 5 seconds while handshaked.
    if (bIsHandshaked)
    {
        PingTimer -= DeltaTime;
        if (PingTimer <= 0.f)
        {
            PingTimer = 5.f;
            DWORD NowMs = (DWORD)(appSeconds() * 1000.0);
            BYTE B[5];
            B[0] = MPKT_PING;
            B[1] = (BYTE)(NowMs);
            B[2] = (BYTE)(NowMs >> 8);
            B[3] = (BYTE)(NowMs >> 16);
            B[4] = (BYTE)(NowMs >> 24);
            SendBinary(B, 5);
        }
    }

    // Heartbeat check — disconnect if server went silent.
    HeartbeatTimer -= DeltaTime;
    if (HeartbeatTimer <= 0.f)
    {
        HeartbeatTimer = 5.0f;
        AWorldInfo* WI = GWorld ? GWorld->GetWorldInfo() : NULL;
        FLOAT Now = WI ? WI->TimeSeconds : 0.f;
        if (Now - LastReceivedTime > 30.0f)
        {
            bIsConnected  = FALSE;
            bIsHandshaked = FALSE;
            HelloAttempt  = 0;
            HelloTimer    = 0.f;
            if (GMultiplayerController)
                GMultiplayerController->OnDisconnected();
        }
    }
}

#endif // WITH_UE3_NETWORKING
