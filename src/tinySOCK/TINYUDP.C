/* ----- tiny udp -------------------------------------------------------------
 * tinyudp.c - Tiny Implementation of the User Datagram Protocol
 *
 * This code is a small implementation of the UDP and IP protocols. The user
 * Simply calls send_udp to send a packet and receive_udp to wait for an
 * incoming packet.
 *
 * ------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include <dos.h>
#pragma hdrstop
#include "tinysock.h"

/* ----- static data ------------------------------------------------------- */
static Word                     nMessage;           /* current message number        */
extern	IP_Address		        local_IP_address;
extern	struct Ethernet_Address their_ethernet_address;     /* the other guys' mac */

/* -----  udp checksum + pseudo header ------------------------------------- */
/* The UDP checksum is weird, it is base on this funky pseudo header that is */
/* included in the checksum calculation but the pseudo header is never       */
/* actually sent, what ever dude, it is funky, but this works.               */
/* ------------------------------------------------------------------------- */
Word udp_checksum(struct udp_packet *p, int payloadlen, Word Protocol)
{
    int buflen = payloadlen + sizeof(struct udp_Header) + 8;
    Longword result = 0;
    Byte *buf = (Byte *)&p->ip.source;
    union ValWords val;

    while(buflen > 1) {
        result += *((Word *)buf);
        buf    += 2;
        buflen -= 2;
	}
    if(buflen > 0) result += (Longword)*buf;   /* 129490 - 118482  = 11008*/
    result += (Longword)Protocol *0x100;
    result += (Longword)p->udp.udp_length;

    val.lw = result;
    result = (Longword)val.valW.lo + (Longword)val.valW.hi;
    return(result);
}

/* ----- udp send ---------------------------------------------------------- */
void udp_send(IP_Address dstIP, Word portno, Byte *message, Word payloadlen)
{
    static struct udp_packet *pkt;
    Byte *data;
    int  i, pktsize;
    Word protocol = Protocol_UDP;     /* the usual protocol number for UDP */

	pkt  = (struct udp_packet *)sed_FormatPacket((Byte *) &(their_ethernet_address), rev_word(Protocol_IP));
    data = (Byte *)pkt + sizeof(struct udp_packet);
    pktsize = sizeof(struct udp_packet) + payloadlen;

	/* make internet header */
	pkt->ip.vht             = rev_word(IPVERTOS);      /* version 4, hdrlen 5, tos 0 */
    pkt->ip.length          = rev_word(pktsize);
	pkt->ip.identification  = rev_word( nMessage++ );  /* incrementing value */
	pkt->ip.frag            = 0;
	pkt->ip.ttlProtocol     = rev_word((128 << 8) + protocol);
	pkt->ip.checksum        = 0;
	pkt->ip.source          = rev_longword(local_IP_address);
	pkt->ip.destination     = rev_longword(dstIP);
    pkt->ip.checksum        = 0;
	pkt->ip.checksum        = rev_word(~checksum((Word *)&pkt->ip, sizeof(struct in_Header)));
	pkt->udp.src_portno     = rev_word(portno);
    pkt->udp.dst_portno     = rev_word(portno);
    pkt->udp.udp_length     = rev_word(sizeof(struct udp_Header) + payloadlen);

    for(i=0; i < payloadlen; i++) data[i] = 0;
    for(i=0; i < payloadlen; i++) data[i] = message[i];

    pkt->udp.udp_checksum = 0;
    pkt->udp.udp_checksum = ~(udp_checksum(pkt, payloadlen, protocol));
    sed_Send(pktsize);
}

/* ----- udp receive ------------------------------------------------------- */
Word udp_receive(struct udp_packet *p, Word portno, Byte *rcv_message, Word maxlen)
{
	Longword rcv_IP;
    Word     rcv_portno;
    Byte     rcv_protocol;
    Word     udp_len;

    rcv_protocol = rev_word(p->ip.ttlProtocol)&0xFF;
    rcv_IP       = rev_longword(p->ip.destination);
    rcv_portno   = rev_word(p->udp.dst_portno);
    udp_len      = rev_word(p->udp.udp_length) - sizeof(struct udp_Header);

    if((rcv_protocol == Protocol_UDP)      &&
       (rcv_portno   == portno)            &&
       (rcv_IP       == local_IP_address) ) {
        if(udp_len > maxlen) udp_len = maxlen;
        Move((Byte  *)p + sizeof(struct udp_packet), rcv_message, udp_len);
    }
    else udp_len = 0;
    return(udp_len);
}

/* ----- End of TinyUDP.c -------------------------------------------------- */

