/*=============================================================================
    UdpLink.cpp: UDP socket for UnrealScript, wrapping FUdpLink.
=============================================================================*/

#include "UnIpDrv.h"

#if WITH_UE3_NETWORKING

IMPLEMENT_CLASS(AUdpLink);

// ---------------------------------------------------------------------------
// FUdpLinkProxy — bridges FUdpLink callbacks to AUdpLink script events
// ---------------------------------------------------------------------------

class FUdpLinkProxy : public FUdpLink
{
public:
    AUdpLink* Owner;

    FUdpLinkProxy(AUdpLink* InOwner) : Owner(InOwner) {}

    INT GetPort() const { return SocketData.Port; }

    virtual void OnReceivedData(FIpAddr SrcAddr, BYTE* Data, INT Count)
    {
        if (Owner && !Owner->IsPendingKill())
            Owner->NativeReceivedData(SrcAddr, Data, Count);
    }
};


// ---------------------------------------------------------------------------
// AUdpLink
// ---------------------------------------------------------------------------

AUdpLink::AUdpLink()
    : UdpLinkProxy(NULL)
{
}

void AUdpLink::BeginDestroy()
{
    if (UdpLinkProxy)
    {
        delete UdpLinkProxy;
        UdpLinkProxy = NULL;
    }
    Super::BeginDestroy();
}

UBOOL AUdpLink::Tick(FLOAT DeltaTime, enum ELevelTick TickType)
{
    UBOOL bResult = Super::Tick(DeltaTime, TickType);
    if (UdpLinkProxy)
        UdpLinkProxy->Poll();
    return bResult;
}

// ---------------------------------------------------------------------------
// Native implementations
// ---------------------------------------------------------------------------

INT AUdpLink::BindPort(INT PortNum)
{
    if (!UdpLinkProxy)
        UdpLinkProxy = new FUdpLinkProxy(this);

    if (UdpLinkProxy->BindPort(PortNum))
        return PortNum == 0 ? UdpLinkProxy->GetPort() : PortNum;
    return 0;
}

UBOOL AUdpLink::SendText(FIpAddr Addr, const FString& Str)
{
    if (!UdpLinkProxy)
        return FALSE;

    FTCHARToANSI Converter(*Str);
    return UdpLinkProxy->SendTo(Addr, (BYTE*)(ANSICHAR*)Converter, Converter.Length()) != 0;
}

UBOOL AUdpLink::SendBinary(FIpAddr Addr, INT Count, BYTE* B)
{
    if (!UdpLinkProxy)
        return FALSE;

    return UdpLinkProxy->SendTo(Addr, B, Count) != 0;
}

void AUdpLink::NativeReceivedData(FIpAddr SrcAddr, BYTE* Data, INT Count)
{
    // Null-terminate into a stack buffer (max datagram ~4096 bytes)
    const INT MaxBuf = 4096;
    ANSICHAR Buf[MaxBuf + 1];
    INT Len = Min(Count, MaxBuf);
    appMemcpy(Buf, Data, Len);
    Buf[Len] = '\0';

    eventReceivedText(ANSI_TO_TCHAR(Buf), SrcAddr);
}

#endif // WITH_UE3_NETWORKING
