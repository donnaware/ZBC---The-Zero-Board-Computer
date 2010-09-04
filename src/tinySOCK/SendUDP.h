//---------------------------------------------------------------------------
#ifndef SendUDP1H
#define SendUDP1H
//---------------------------------------------------------------------------
//   IO Port definitions
//---------------------------------------------------------------------------
#define PORTBASE 0x0360		  // Base Port address
#define TXSTATUS PORTBASE     // Transmit Status  Register
#define TXCONTRL PORTBASE+1   // Transmit Control and MSB Address Register
#define TXADDRSS PORTBASE+2   // Transmit Buffer  LSB Address Register
#define TXBUFFER PORTBASE+3   // Transmit Buffer  Data Register
#define RXSTATUS PORTBASE+4	  // Receive  Status  Register
#define RXCONTRL PORTBASE+5   // Receive  Control and MSB Address Register
#define RXADDRSS PORTBASE+6   // Receive  Buffer  LSB Address Register
#define RXBUFFER PORTBASE+7   // Receive  Buffer  Data Register
//---------------------------------------------------------------------------
#define  Protocol_IP    0x0800      // Internet Protocol (IP)
#define  Protocol_ARP   0x0806      // Address Resolution Protocol (ARP)
#define  RxBufferSize   4096        //
#define  TxBufferSize   4096        //
#define  POLYNOMIAL     0x04c11db7L // Standard CRC-32 ppolynomial
#define  BUFFER_LEN     4096L       // Length of buffer
//---------------------------------------------------------------------------
typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;
union ValRec {
	unsigned long  lw;
	struct valWord {
		unsigned short lo;
		unsigned short hi;
	} valW;
} VALL;
union valBytes {
	unsigned short  sw;
	struct valByte {
		unsigned char lo;
		unsigned char hi;
	} valB;
} VALS;
word  swap(word x)
{
	VALS v;

	v.sw = x;
	byte hi = v.valB.lo;
	byte lo = v.valB.hi;
	v.valB.lo = lo;
	v.valB.hi = hi;
	return(v.sw);
}
/* -----  udp test --------------------------------------------------------- */
word Checksum16(int v)
{
    unsigned int csum;
    ValRec val;

    val.lw = (unsigned long)v;
    val.lw = (unsigned long)(int(val.valW.lo) + int(val.valW.hi));
    csum = ~( val.valW.lo + val.valW.hi );
    return(csum);
}
/* -----  udp test --------------------------------------------------------- */
int Checksum16_BufSum(byte *buf, int buflen)
{
    unsigned int result = 0;
    while(buflen > 1) {
        result += *((word *)buf);
        buf    += 2;
        buflen -= 2;
	}
    if(buflen > 0) result += (unsigned int)*buf;
    return(result);
}
/* -----  udp test --------------------------------------------------------- */
word IPchecksum(void)
{
    return(Checksum16(Checksum16_BufSum((byte *)&pkt.IP, sizeof(pkt.IP))));

}
/* -----  udp test --------------------------------------------------------- */
word UDPchecksum(int payloadlen)
{
	word checksum = Checksum16(
        int(pkt.IP.protocol) * 0x100 +
        int(pkt.UDP.udp_length) +
        Checksum16_BufSum((byte *)&pkt.IP.srcaddr, 8 + sizeof(UDP_header) + payloadlen)
    );
    return(checksum);
}

//	word checksum = Checksum16( int(pkt.IP.protocol) * 0x100 + int(pkt.UDP.udp_length) + Checksum16_BufSum((byte *)&pkt.IP.srcaddr, 8 + sizeof(UDP_header) + payloadlen));

//---------------------------------------------------------------------------
typedef struct IPv4 { byte address[4]; }IPV4PC;
typedef struct IP_header {      // 20 bytes
	byte verlen;                // IP version & length
	byte tos;                   // IP type of service
	word totallength;           // Total length
	word id;                    // Unique identifier
	word offset;                // Fragment offset field
	byte ttl;                   // Time to live
	byte protocol;              // Protocol(TCP, UDP, etc.)	
	word checksum;              // IP checksum
	IPV4PC srcaddr;               // Source IP
	IPV4PC destaddr;              // Destination IP
}IPHEADER;
typedef struct UDP_header {     // 8 bytes
	word src_portno;            // Source port number
	word dst_portno;            // Destination port number
	word udp_length;            // UDP packet length
	word udp_checksum;          // UDP checksum (optional)
}UDPHEADER;
typedef struct EthernetIIframe {// Ethernet Frame
	byte  DestinationMAC_Address[6];
	byte  SourceMAC_Address[6];
	word  Protocol;
	byte  Data[1500];
	dword Checksum;
}ETHERNETFRAME;
//---------------------------------------------------------------------------
#define UDPPAYLOAD 18
typedef struct EthernetUDP_packet { // Ethernet UDP packet structure
	byte DestinationMAC_Address[6];
	byte SourceMAC_Address[6];
	word Protocol;
	IPHEADER IP;               // 20B
	UDPHEADER UDP;            //  8B
	byte Data[UDPPAYLOAD];     // 18B
	dword Checksum;
}ETHERNETUDPPACKET;
//---------------------------------------------------------------------------
// CRC Stuff
//---------------------------------------------------------------------------
unsigned GetFCS(byte *data, unsigned  data_size);
int  Checksum16_BufSum(byte *buf, int buflen);
word Checksum16(int v);
word IPchecksum(void);
word UDPchecksum(int payloadlen);

//---------------------------------------------------------------------------
// Packet Stuff
//---------------------------------------------------------------------------
ETHERNETUDPPACKET pkt;
int  EthernetPacketLen;
int  nMessage;
void SendPacket(void);

//---------------------------------------------------------------------------
#endif
