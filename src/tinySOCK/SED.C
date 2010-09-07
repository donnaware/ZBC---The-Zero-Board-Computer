/* --- ZBC Ethernet Driver ----------------------------------------------------
 *
 * A Very Simple set of ethernet driver primitives for the ZBC Board.
 * The ethernet interface is controlled by busy-waiting, the application
 * is handed the location of on-board packet buffers, and allowed to fill in
 * the transmit buffer directly. The interface is entirely blocking.
 *
 * Primitives:
 * sed_Init() -- Initialize the package
 * sed_FormatPacket( destEAddr ) => location of transmit buffer
 * sed_Send( pkLength ) -- send the packet that is in the transmit buffer
 * sed_Receive( recBufLocation ) -- enable receiving packets.
 * sed_IsPacket() => location of packet in receive buffer
 * sed_CheckPacket( recBufLocation, expectedType )
 *
 * Global Variables:
 *	local_ethernet_address -- Ethernet address of this host.
 *	broadcast_ethernet_address -- Ethernet broadcast address.
 * ------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include <dos.h>
#pragma hdrstop
#include "tinysock.h"
#include "sed.h"
#include "comdrvr.h"   /* regular win32 comm routines, can use whatever      */

#define DEBUG_ETH_RX   0
#define DEBUG_ETH_TX   0

/* ----- globals referenced in arp ----------------------------------------- */
struct Ethernet_Address	local_ethernet_address;     /* local ethernet address */
struct Ethernet_Address	broadcast_ethernet_address; /* Ethernet broadcast address */
struct Ethernet_Address their_ethernet_address;     /* the other guys' mac   */

static	BOOL		sed_respondARPreq; /* controls responses to ARP req's    */
static	Byte        sed_tx[4096];      /* ethernet transmit Buffer           */
static	Byte        sed_rx[4096];      /* ethernet transmit Buffer           */

#if COMMDRIVER
    struct eth_Header *rcv = (struct eth_Header *)&sed_rx[1];
#else
	struct eth_Header *rcv = (struct eth_Header *)&sed_rx[0];
#endif

/* ----- Initializatoin ---------------------------------------------------- */
/*Initialize the Ethernet Interface, and this package. 						 */
int sed_Init(void)
{
	int		i;

#if COMMDRIVER
    OpenCOM();    	/* open RS232 COM driver 		*/
#endif
    
	/* just make up an address for now */
	local_ethernet_address.MAC[0] = 0x00; local_ethernet_address.MAC[1] = 0x12;
    local_ethernet_address.MAC[2] = 0x34; local_ethernet_address.MAC[3] = 0x56;
    local_ethernet_address.MAC[4] = 0x78; local_ethernet_address.MAC[5] = 0x90;

	/* and initialize the exported variable which gives the Eth broadcast
	   address, for everyone else to use. */
	for( i = 0; i < 6; i++ ) broadcast_ethernet_address.MAC[i] = 0xFF;

	outportb(RXSTATUS, 0x00);  /* Clear NIC Buf status bit		   			 */
	outportb(RXCONTRL, 0x80);  /* Set NIC Buf to address to 0 & enable recv  */
	outportb(RXADDRSS, 0x00);  /* Set NIC Buf to address to 0   			 */

	return(1);
}

/* ----- deinit the interface ---------------------------------------------- */
int sed_Deinit( void )
{
#if COMMDRIVER
    CloseCOM();     	/* Close the COM driver */
#endif
    return 1;
}

/* ----- CRC32 generation -------------------------------------------------- */
static Longword GetFCS(Byte *data, int data_size)
{
  int n;
  Longword crc = 0;
  Longword crc_table[] = {
	0x4DBDF21CL,0x500AE278L,0x76D3D2D4L,0x6B64C2B0L,0x3B61B38CL,0x26D6A3E8L,
	0x000F9344L,0x1DB88320L,0xA005713CL,0xBDB26158L,0x9B6B51F4L,0x86DC4190L,
	0xD6D930ACL, 0xCB6E20C8L,0xEDB71064L,0xF0000000L};
  for(n = 0; n < data_size; n++) {
    crc = (crc >> 4) ^ crc_table[(crc ^ (data[n] >> 0)) & 0x0F];  // lower nibble
    crc = (crc >> 4) ^ crc_table[(crc ^ (data[n] >> 4)) & 0x0F];  // upper nibble
  }
  return(crc);
}

/* ----- format the Ethernet Frame ----------------------------------------- */
/* Format an ethernet header in the transmit buffer. Note that because of the*/
/* way the interface works, we need to know how long the packet is before we */
/* know where to put it. The solution is that we format the packet at the    */
/* BEGINNING of the transmit buffer, and later copy it (carefully) to where  */
/* it belongs. Another hack would be to be inefficient about the size of the */
/* packet to be sent (always send a larger ethernet packet than you need to, */
/* but this works fine for now. 						                     */
/* ------------------------------------------------------------------------- */
Byte *sed_FormatPacket(Byte *destEAddr, Word ethType)
{
    int i;
	Byte *xMitBuf;

	xMitBuf = sed_tx;

    for(i = 0; i < 7; i++) *xMitBuf++ = 0x55;   /* Ethernet preamble */
    *xMitBuf++ = 0xD5;                          /* Ethernet SFD */

	Move((Byte *)destEAddr, (Byte *)xMitBuf, 6);
	Move((Byte *)&local_ethernet_address, (Byte *)(xMitBuf + 6), 6);
	*((short *)(xMitBuf + 12)) = ethType;
	return(xMitBuf + 14);
}

/* ----- transmit the ethernet frame  -------------------------------------- */
#if !COMMDRIVER
void xmt_frame(int frameLength)
{
	int i;
	outportb(TXCONTRL, 0x00);    /* Set NIC Buf to address to 0 */
	outportb(TXADDRSS, 0x00);    /* Set NIC Buf to address to 0 */
	for(i = 0; i < frameLength; i++) {
		outportb(TXBUFFER, sed_tx[i]);
		outportb(TXCONTRL, (i>>8)&0x07);    /* Set NIC Buf to address to 0   */
		outportb(TXADDRSS,  i    &0xFF);    /* Set NIC Buf to address to 0   */
	}
	outportb(TXCONTRL, inportb(TXCONTRL)|0x80); /* trigger transmit of buffer*/
	outportb(TXCONTRL, inportb(TXCONTRL)&0xEF); /* Turn of triger            */
	while(!(inportb(TXSTATUS)&0x80)); 			/* wait for it to comlpete */
	outportb(TXCONTRL, 0x00);					/* clear status for next frame */
}
#endif

/* ----- Send Packet ------------------------------------------------------- */
/* Send a packet out over the ethernet. The packet is sitting at the         */
/* beginning of the transmit buffer. The routine returns when the packet has */
/* been successfully sent.                                                   */
/* ------------------------------------------------------------------------- */
int sed_Send(int pkLengthInBytes)
{
    Longword Checksum, *ptr;
	int	   pkLength, i;

	pkLengthInBytes += 14;		/* account for Ethernet header */
	if(pkLengthInBytes < E10P_MIN) {
/*      use this code if you want numbers in the padding instead of whatever */
        for(i=pkLengthInBytes; i<E10P_MIN; i++) sed_tx[8+i] = (Byte)i;
        pkLengthInBytes = E10P_MIN; /* and min. ethernet len */
    }

    Checksum = GetFCS((Byte *)&sed_tx[8], pkLengthInBytes);   /* exclude preamble & SFD */
    ptr = (Longword *)(((Byte *)&sed_tx) + 8 + pkLengthInBytes);
    *ptr = Checksum;

#if COMMDRIVER
    WriteCOM(sed_tx, (pkLengthInBytes + 12));  /* this will wait until it has really been sent. */
#else
    xmt_frame(pkLengthInBytes+12);
#endif
	return(1);   	/* else we sent the packet ok. */
}

/* ----- Receive Packet ---------------------------------------------------- */
/* Receive a packet of only the protocol requested. This is usefule when we  */
/* know we know we only want IP messages for example, if the desired packet  */
/* has arrived, then the ethernet length is returned, otherwise it returns 0 */
/* ------------------------------------------------------------------------- */
Byte *sed_Receive(Word Protocol)
{
    Byte *p;
	p = sed_IsPacket();							/* get the packet			*/
	if(p) {                                     /* if we got a packet, then */
		if(!sed_CheckPacket(Protocol)) p = 0; 	/* check  messages protocol */
	}
	return(p);
}

/* ----- receive the ethernet frame  --------------------------------------- */
/* Waits for a period of time set by the RX_TIMEOOUT for a frame. If no      */
/* frame has been received then we return 0, otherwise return the bytes      */
/* received                                                                  */
/* ------------------------------------------------------------------------- */
#if !COMMDRIVER
int rcv_frame(int maxLength)
{
	int i;
    Word addr, len;
	Longword endTime;

	outportb(RXCONTRL, 0x00);  /* Set NIC Buf to enable recv  */
	endTime = MsecClock() + RX_TIMEOOUT ;   /* allow time for frame to come */
	do {
		if(MsecClock() > endTime) return(0);
	} while(!(inportb(RXSTATUS)&0x80));

	len  = ((Word)(inportb(RXCONTRL)&0x07))*256;
	len +=  (Word)inportb(RXADDRSS);
	#if DEBUG_ETH_RX
		printf("\nlen = %d\n",len);
	#endif

	if(len > maxLength) len = maxLength;
	outportb(RXCONTRL, 0x00);  /* Set NIC Buf to address to 0 & disable rcv  */
	outportb(RXADDRSS, 0x00);  /* Set NIC Buf to address to 0   			 */
	addr = 0;
	for(i = 0; i < len; i++) {
		outportb(RXCONTRL, (addr>>8)&0x07); /* Set NIC Buf address  */
		outportb(RXADDRSS,  addr    &0xFF); /* Set NIC Buf address  */
		sed_rx[i] = inportb(RXBUFFER);
		addr++;
		#if DEBUG_ETH_RX
			printf("%02x ",sed_tx[i]);
		#endif
	}
	outportb(RXSTATUS, 0x00);  /* Clear NIC Buf status bit	 */
	outportb(RXCONTRL, 0x80);  /* Rest NIC Buf   */
	return(addr);
}
#endif

/* ----- sed_checkMAC ------------------------------------------------------ */
/* Checks to make sure the packet is for me                                  */
/* ------------------------------------------------------------------------- */
int sed_checkMAC(void)
{
	int i;
	int ret = 1;
	for(i=0; i<6; i++) {
		if((rcv->destination.MAC[i] != local_ethernet_address.MAC[i]) &&
		   (rcv->destination.MAC[i] != 0xFF)) {
			ret = 0;
			#if DEBUG_ETH_RX
				printf("arp mis %d %02x\n",i,rcv->destination.MAC[i]);
			#endif
			break;
        }
    }
    return(ret);
}

/* ----- is Packet --------------------------------------------------------- */
/* Test for the arrival of a packet on the Ethernet interface. The packet    */
/* may arrive in either buffer A or buffer B; the location of the packet is  */
/* returned. If no packet is returned withing 'timeout' milliseconds, then   */
/* the routine returns zero. 												 */
/*                                                                           */
/* Note: ignores ethernet errors. may occasionally return something which    */
/* was received in error.                                                    */
/* ------------------------------------------------------------------------- */
Byte *sed_IsPacket(void)
{
#if COMMDRIVER
   	int ret;
	Byte *pb = &sed_rx[1];
   	ret = ReadCOM(sed_rx, 1518);    /* read up to the MTU           */
	if(ret) {
		if(sed_checkMAC()) pb += 14;   /* get past the ethernet header */
		else               pb = 0; 	   /* was not for em         */
	}
	else pb = 0; 				/* nothing was received         */
	return(pb);
#else
	int ret;
	Byte *pb = &sed_rx[0];
	ret = rcv_frame(1518);			/* receive up to MTU			*/
	if(ret) {
		if(sed_checkMAC()) pb += 14;   /* get past the ethernet header */
		else               pb = 0; 	   /* was not for em         */
	}
	else pb = 0; 				/* nothing was received         */
	return(pb);
#endif
}

/* ----- Check Packet ------------------------------------------------------ */
/* Check to make sure packet received was the one that you expected to get.  */
/* ------------------------------------------------------------------------- */
int sed_CheckPacket(Word expectedType)
{
	if(rcv->type != rev_word(expectedType)) return(0);
	return(1);
}

/* ----- millisecond clock ------------------------------------------------- */
/* Get time in milliseconds. Set the usex86call define to control whether to */
/* use the built in borland clock() or to use the int86 bios call			 */
/* ------------------------------------------------------------------------- */
#define usex86call 0
Longword MsecClock(void)
{
#if usex86call
	static union REGS regs;
	Longword		  ticks;

	/* Start with the 18ths of a second and multiply by 55. */
	static Longword baseticks = 0L;		/* time zero */
	regs.h.ah = 0;
	int86(0x1A, &regs, &regs);
	ticks = (((Longword) regs.x.cx ) << 16L ) + (Longword) regs.x.dx;
	if( baseticks == 0L ) baseticks = ticks;	/* keep time zero */
	ticks -= baseticks;			/* subtract time zero */
	return(55L * ticks);
#else
	#include <time.h>
    return((Longword)clock());
#endif
}

/* ---- end of sed.c ------------------------------------------------------- */

