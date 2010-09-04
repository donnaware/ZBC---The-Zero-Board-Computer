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

extern	struct Ethernet_Address	    local_ethernet_address;
extern	struct Ethernet_Address	    broadcast_ethernet_address;
extern	struct Ethernet_Address     their_ethernet_address;     /* the other guys' mac */
extern	IP_Address                  local_IP_address;

/* ---- Check Packet ------------------------------------------------------- */
/* check to see if the packet just received was an arp packet, if so, then   */
/* respond appropriately withour IP/MAC pairing information                  */
/* ------------------------------------------------------------------------- */
int arp_checkpacket(struct arp_Header *ap)
{
	struct arp_Header *op;
    Word hwType          = rev_word(ap->hwType);
    Word protType        = rev_word(ap->protType);
    Word opcode          = rev_word(ap->opcode);
    IP_Address dstIPAddr = rev_longword(ap->dstIPAddr);
    local_IP_address = MY_ADDR;     /* This is my IP Address    */

	if(hwType    != arp_TypeEther ||	/* have ethernet hardware,  */
	   protType  != Protocol_IP   ||	/* and internet software,   */
	   opcode    != ARP_REQUEST   ||	/* and be a resolution req. */
	   dstIPAddr != local_IP_address )  /* for my addr.             */
	   return(0);   			        /* .... or we ignore it. */

	/* format response. */
	op = (struct arp_Header *)sed_FormatPacket((Byte *)&ap->srcEthAddr, rev_word(Protocol_ARP));
	op->hwType          = ap->hwType;
	op->protType        = ap->protType;
	op->hwProtAddrLen   = rev_word((sizeof(struct Ethernet_Address) << 8) + sizeof(IP_Address));
	op->opcode          = rev_word(ARP_REPLY);
	op->srcIPAddr       = ap->dstIPAddr;
	ap->dstIPAddr       = ap->srcIPAddr;
	Move((Byte *)&local_ethernet_address, (Byte *)&op->srcEthAddr, sizeof(struct Ethernet_Address));
	Move((Byte *)&ap->srcEthAddr,         (Byte *)&op->dstEthAddr, sizeof(struct Ethernet_Address));
	Move((Byte *)&ap->srcEthAddr, (Byte *)&their_ethernet_address, sizeof(struct Ethernet_Address));

	sed_Send(sizeof(struct arp_Header));
	return(1);
}

/* ---- handle address resolution bit -------------------------------------- */
/* Call this function when we first start out doing stuff on ethernet, it    */
/* tell us that MAC/IP pairing information of the host we are trying to      */
/* connect to, then use that MAC from now on.                                */
/* ------------------------------------------------------------------------- */
int sar_MapIn2Eth(IP_Address ina, struct Ethernet_Address *ethap)
{
	struct arp_Header *	op;
	Longword		endTime;
	Longword		rxMitTime;

	endTime = MsecClock() + 2000;   /* allow 2 seconds for this stuff to happen */
	while(endTime > MsecClock()) {
		op = (struct arp_Header *) sed_FormatPacket(( Byte * )&broadcast_ethernet_address, rev_word(Protocol_ARP));

		op->hwType          = rev_word(arp_TypeEther);
		op->protType        = rev_word(Protocol_IP);
		op->hwProtAddrLen   = rev_word((sizeof(struct Ethernet_Address) << 8) + sizeof(IP_Address));
		op->opcode          = rev_word(ARP_REQUEST);
		op->srcIPAddr       = rev_longword(local_IP_address);
		op->dstIPAddr       = rev_longword(ina);
		Move((Byte *)&local_ethernet_address, (Byte *)&op->srcEthAddr, sizeof(struct Ethernet_Address));

		/* ...and send the packet */
		sed_Send(sizeof(struct arp_Header));

		rxMitTime = MsecClock() + 250;
		while(rxMitTime > MsecClock()) {
			op = (struct arp_Header *)sed_IsPacket();
			if(op) {
				if(sed_CheckPacket(Protocol_ARP)  == 1 &&
					rev_word(    op->protType )   == Protocol_IP &&
					rev_longword(op->srcIPAddr)   == ina &&
					rev_word(    op->opcode   )   == ARP_REPLY ) {
					Move((Byte *)&op->srcEthAddr, (Byte * )ethap, sizeof(struct Ethernet_Address));
            		Move((Byte *)&op->srcEthAddr, (Byte *)&their_ethernet_address, sizeof(struct Ethernet_Address));
					return(1);  /* Success ! */
				}
			}
		}
	}
	return(0);      /* Failure :(     */
}

/* -----  arp -------------------------------------------------------------- */
/* This routing simply finds the MAC address associated with the specified   */
/* IP address passed as a parameter. The global variable, their_ethernet_    */
/* address is set to either the address returned by the arp reply or if no   */
/* reply is received, then their address is set to the broadcast address.    */
/* ------------------------------------------------------------------------- */
int do_arp(IP_Address their_IP_address)
{
    int ret;

    if(sar_MapIn2Eth(their_IP_address, &their_ethernet_address)) ret = 1;
    else {
    	their_ethernet_address.MAC[0] = 0xFF;
        their_ethernet_address.MAC[1] = 0xFF;
        their_ethernet_address.MAC[2] = 0xFF;
        their_ethernet_address.MAC[3] = 0xFF;
        their_ethernet_address.MAC[4] = 0xFF;
        their_ethernet_address.MAC[5] = 0xFF;
        ret = 0;
    }
    return(ret);
}

/* ---- end of arp.c ------------------------------------------------------- */
