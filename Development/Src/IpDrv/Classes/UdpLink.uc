// UdpLink: UDP socket for UnrealScript.
// Wraps FUdpLink from InternetLink.h.
class UdpLink extends InternetLink
    native
    transient;

// Native proxy object (FUdpLinkProxy*), stored as pointer so it survives GC.
var private native const pointer UdpLinkProxy{FUdpLinkProxy};

cpptext
{
    AUdpLink();
    void BeginDestroy();
    UBOOL Tick(FLOAT DeltaTime, enum ELevelTick TickType);
    virtual UBOOL ShouldTickInEntry() { return true; }
    virtual void NativeReceivedData(FIpAddr SrcAddr, BYTE* Data, INT Count);
}

// Bind to a local port (0 = any available port). Returns bound port or 0 on failure.
native function int BindPort(optional int PortNum);

// Send a string to the given address.
native function bool SendText(IpAddr Addr, coerce string Str);

// Send raw bytes to the given address.
native function bool SendBinary(IpAddr Addr, int Count, byte B[255]);

// Called when text data is received.
event ReceivedText(string Text, IpAddr Addr);

// Called when binary data is received.
event ReceivedBinary(int Count, byte B[255], IpAddr Addr);

defaultproperties
{
    bAlwaysTick=True
}
