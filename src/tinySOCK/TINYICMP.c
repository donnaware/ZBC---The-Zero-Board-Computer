/* ----- tinyicmp.c -----------------------------------------------------------
/*
 * Simple Internet Control Message Protocol Implementation
 *
 * This package implements a very simple version of the ICMP (RFC 792).
 * This simply responds to a "ping".
 *
 * ------------------------------------------------------------------------- */
#include <string.h>
#include <time.h>
#pragma hdrstop
#include "tinysock.h"

static  Word   nMsg;
extern	IP_Address		            local_IP_address;
extern	struct Ethernet_Address	    local_ethernet_address;
extern	struct Ethernet_Address     their_ethernet_address;
extern	struct Ethernet_Address	    broadcast_ethernet_address;

/* ----- icmp respond ------------------------------------------------------ */
/* call this function to see if the packet just received was an icmp packet  */
/* respond to basic echo "ping requests only, ignore others                  */
/* ------------------------------------------------------------------------- */
int icmp_check(struct icmp_packet *rp)         /* received packet */
{
	struct icmp_packet *op;                   /* output packet */
    Byte    *ima          = (Byte *)rp - 8;   /* incoming mac address */
	Longword rcv_IP       = rev_longword(rp->ip.destination);
    Byte     rcv_protocol = rev_word(rp->ip.ttlProtocol)&0xFF;
    Word     rcv_type     = rev_word(rp->icmp.type);

	if((rcv_IP        == local_IP_address) &&   /* for my addr ? */
       (rcv_protocol  == Protocol_ICMP)    &&   /* icmp ? */
       (rcv_type      == ICMP_t_echoreq))       /* echo request ? */
	   return(0);  			                    /* or we ignore it. */

    /* we are good to go */
	op = (struct icmp_packet *)sed_FormatPacket(ima, rev_word(Protocol_IP));

	/* make internet header */
	op->ip.vht             = rev_word(0x4500);      /* version 4, hdrlen 5, tos 0 */
    op->ip.length          = rev_word(sizeof(struct icmp_packet));
	op->ip.identification  = rev_word(nMsg++);      /* incrementing value */
	op->ip.frag            = 0;
	op->ip.ttlProtocol     = rev_word((128 << 8) + Protocol_ICMP);
	op->ip.source          = rp->ip.destination;
	op->ip.destination     = rp->ip.source;
    op->ip.checksum        = 0;
	op->ip.checksum        = rev_word(~checksum((Word *)&op->ip, sizeof(struct in_Header)));
	op->icmp.type          = ICMP_t_echoreply;
    op->icmp.code          = ICMP_c_echoreply;
    op->icmp.ID            = rp->icmp.ID;
    op->icmp.Sequence      = rp->icmp.Sequence;
    op->icmp.checksum      = 0;
	op->icmp.checksum      = rev_word(~checksum((Word *)&op->icmp, sizeof(struct icmp_Header)));

    sed_Send(sizeof(struct icmp_packet));
    return(1);
}

/* ----- icmp respond ------------------------------------------------------ */
/* call this function to send a ping icmp packet to the IP address specified */
/* and passed as a parameter. Once sent, we wait timeout period of time and  */
/* return 1 if the reply was received, or 0 if it was not received within    */
/* specified period of time.
/* ------------------------------------------------------------------------- */
int icmp_send(IP_Address dst, Longword timeout)
{
	struct icmp_packet *op, *rp;          /* output packet & received packet */
	IP_Address rcv_IP;
	Longword expiretime;
    int ret;

	op = (struct icmp_packet *)sed_FormatPacket((Byte *)&their_ethernet_address, rev_word(Protocol_IP));

	/* make icmp header */
	op->ip.vht             = rev_word(IPVERTOS);      /* version 4, hdrlen 5, tos 0 */
    op->ip.length          = rev_word(sizeof(struct icmp_packet));
	op->ip.identification  = rev_word(nMsg++);      /* incrementing value */
	op->ip.frag            = 0;
//	op->ip.ttlProtocol     = IP_TTLPROT(128, Protocol_ICMP);
	op->ip.ttlProtocol     = rev_word((128 << 8) + Protocol_ICMP);
	op->ip.source          = rev_longword(local_IP_address);
	op->ip.destination     = rev_longword(dst);
    op->ip.checksum        = 0;
	op->ip.checksum        = rev_word(~checksum((Word *)&op->ip, sizeof(struct in_Header)));
	op->icmp.type          = ICMP_t_echoreq;
    op->icmp.code          = ICMP_c_echoreq;
    op->icmp.ID            = 0x11;              /* arbitrary ID number */
    op->icmp.Sequence      = 0;
    op->icmp.checksum      = 0;
	op->icmp.checksum      = rev_word(~checksum((Word *)&op->icmp, sizeof(struct icmp_Header)));

    sed_Send(sizeof(struct icmp_packet)); /* send the ping out there */

    ret = 0;
    expiretime = MsecClock()  + timeout;
    do {
		rp = (struct icmp_packet *)sed_IsPacket();
		if(rp) {
		   	if(sed_CheckPacket(Protocol_IP)) {   /* IP messages ? */
                if((IP_PROTOCOL(&rp->ip) == Protocol_ICMP) &&
                   (op->ip.source        == rp->ip.destination) &&
                   (rp->ip.source        == op->ip.destination) &&
                   (ICMP_t_echoreply     == rev_word(rp->icmp.type)) &&
                   (ICMP_c_echoreply     == rev_word(rp->icmp.code)) &&
                   (rp->icmp.ID          == 0x11) &&
                   (rp->icmp.Sequence    == 0))
                {
                    ret = 1;
                    break;
                }
            }
        }
    } while(MsecClock() < expiretime);
    return(ret);
}

/* ---- end of tinyicmp.c -------------------------------------------------- */








