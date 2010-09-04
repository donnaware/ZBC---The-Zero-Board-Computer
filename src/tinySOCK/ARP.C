/* ----- Simple ARP -----------------------------------------------------------
/*
 * SAR: Simple Address Resolution Protocol Implementation
 *
 * This package implements a very simple version of the Plummer Address
 * Resolution Protocol (RFC 826). It allows clients to resolve Internet
 * addresses into Ethernet addresses, and knows how to respond to an
 * address resolution request (when the transmit buffer is free).
 *
 * Routines:
 * sar_CheckPacket( pb ) => 1, if ARP packet and processed, 0 otherwise
 * sar_MapIn2Eth( ina, ethap ) => 1 if did it, 0 if couldn't.
 *
 * ------------------------------------------------------------------------- */
#include <string.h>
#include <time.h>
#pragma hdrstop
#include "tinysock.h"

extern	IP_Address		            local_IP_address;
extern	struct Ethernet_Address	    local_ethernet_address;
extern	struct Ethernet_Address	    broadcast_ethernet_address;

/* ---- Check Packet ------------------------------------------------------- */
int sar_CheckPacket(struct arp_Header *ap)
{
	struct arp_Header *op;

	if( ap->hwType != arp_TypeEther ||	    /* have ethernet hardware, */
		ap->protType != 0x800 ||	        /* and internet software, */
		ap->opcode != ARP_REQUEST ||	    /* and be a resolution req. */
		ap->dstIPAddr != local_IP_address ) /* for my addr. */
		return ( 0 );			            /* .... or we ignore it. */

	/* format response. */

	op = (struct arp_Header *)sed_FormatPacket((Byte *)&ap->srcEthAddr, 0x806);
	op->hwType = arp_TypeEther;
	op->protType = 0x800;
	op->hwProtAddrLen = (sizeof(struct Ethernet_Address) << 8) + sizeof(IP_Address);
	op->opcode = ARP_REPLY;
	op->srcIPAddr = local_IP_address;
	Move((Byte *)&local_ethernet_address, (Byte *)&op->srcEthAddr, sizeof(struct Ethernet_Address));
	ap->dstIPAddr = op->srcIPAddr;
	Move( ( Byte * )&ap->srcEthAddr, ( Byte * )&op->dstEthAddr, sizeof(struct Ethernet_Address));

	sed_Send(sizeof(struct arp_Header));

	return ( 1 );
}

/* ---- handle address resolution bit -------------------------------------- */
int sar_MapIn2Eth( ina, ethap ) Longword ina; struct Ethernet_Address *ethap;
{
	struct arp_Header *	op;
	Longword		endTime;
	Longword		rxMitTime;

	sed_Receive( ( Byte * ) 0 );
	endTime = MsecClock() + 2000;
	while( endTime > MsecClock() ) {
		op = (struct arp_Header *) sed_FormatPacket(( Byte * )&broadcast_ethernet_address, Protocol_ARP);

		op->hwType = arp_TypeEther;
		op->protType = 0x800;
		op->hwProtAddrLen = (sizeof(struct Ethernet_Address) << 8) + sizeof( IP_Address );
		op->opcode = ARP_REQUEST;
		op->srcIPAddr = local_IP_address;
		Move( ( Byte * )&local_ethernet_address, ( Byte * )&op->srcEthAddr, sizeof(struct Ethernet_Address));
		op->dstIPAddr = ina;

		/* ...and send the packet */
		sed_Send( sizeof(struct arp_Header) );

		rxMitTime = MsecClock() + 250;
		while ( rxMitTime > MsecClock() ) {
			op = (struct arp_Header *)sed_IsPacket();
			if ( op ) {
				if ( sed_CheckPacket( ( Word * ) op, 0x806 )
					== 1 &&
					op->protType == 0x800 &&
					op->srcIPAddr == ina &&
					op->opcode == ARP_REPLY ) {
					Move((Byte * )&op->srcEthAddr, (Byte * ) ethap, sizeof(struct Ethernet_Address));
					return ( 1 );
				}
				sed_Receive( ( Byte * ) op );
			}
		}
	}
	return ( 0 );
}

/* ---- end of arp.c ------------------------------------------------------- */
