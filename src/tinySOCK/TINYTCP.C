/* ----- tiny tcp -------------------------------------------------------------
 * tinytcp.c - Tiny Implementation of the Transmission Control Protocol
 *
 * TinyTCP is a small implementation of the TCP and IP protocols. The
 * implementation is bare-bones. It can be compiled as either using the
 * RS232 to Ethernet converter or the ZBC Ethernet interface. The
 * implementation is based on busy-waiting, but the tcp_handler procedure
 * could easily be integrated into an interrupt driven scheme.
 *
 * IP routing is accomplished on active opens by broadcasting the tcp SYN
 * packet when ARP mapping fails. If anyone answers, the ethernet address
 * used is saved for future use. This also allows IP routing on incoming
 * connections.
 *
 * The TCP does not implement urgent pointers (easy to add), and discards
 * segments that are received out of order. It ignores the received window
 * and always offers a fixed window size on input (i.e., it is not flow
 * controlled).
 *
 * Reference is RFC-793, available through the Internet.
 * ------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#pragma hdrstop
#include "tinysock.h"

// #define DEBUG_TCP 

/* ----- static data ------------------------------------------------------- */
static int			       tcp_id;				/* TCP ID, gets incremented  */
static struct tcp_Socket *tcp_allsocs;			/* make global for sharing   */

/* ----- External data ----------------------------------------------------- */
extern struct Ethernet_Address broadcast_ethernet_address;
extern        IP_Address	local_IP_address;	/* local IP address 	     */

/* ----- Initialize the tcp implementation --------------------------------- */
void tcp_Init(void)
{
	tcp_allsocs      = (struct tcp_Socket *)0;
	tcp_id           = 0;
}

/* ----- tcp open ---------------------------------------------------------- */
/* Actively open a TCP connection to a particular destination. 		         */
/* ------------------------------------------------------------------------- */
void tcp_Open(s, lport, ina, port, datahandler)
	struct tcp_Socket *	s;
	IP_Address		ina;
	Word			lport, port;
	Procref			datahandler;
{
	s->state   = TS_SSYN;
	s->timeout = tcp_LONGTIMEOUT;
	if(lport == 0) lport = (Word)MsecClock();
	s->myport = lport;

	if(sar_MapIn2Eth(ina, &(s->hisethaddr))) {
        printf("tcp_Open with arp to %d.%d.%d.%d\n",IP_1B(ina),IP_2B(ina),IP_3B(ina),IP_4B(ina));
    }
    else {   /* Can't find the ethernet address. Blast it to everyone */
		printf("tcp_Open to %d.%d.%d.%d defaulting broadcast\n", IP_1B(ina),IP_2B(ina),IP_3B(ina),IP_4B(ina));
		Move((Byte *)&broadcast_ethernet_address, (Byte *)&s->hisethaddr, sizeof(struct Ethernet_Address));
	}

	s->hisaddr      = ina;
	s->hisport      = port;
	s->seqnum       = 0;
	s->dataSize     = 0;
	s->flags        = TCPF_SYN;		/* Looking for sync     */
	s->unhappy      = True;		    /* Flag it as unhappy   */
	s->dataHandler  = datahandler;

	s->next         = tcp_allsocs;   /* add the new socket to the linklist */
	tcp_allsocs     = s;
	tcp_Send(s);
}

/* ----- tcp listen -------------------------------------------------------- */
/* Passive open: listen for a connection on a particular port 		         */
/* ------------------------------------------------------------------------- */
void tcp_Listen(s, port, datahandler, timeout)
	struct	tcp_Socket *	s;
	Word			port;
	Procref			datahandler;
	Longword		timeout;
{
	s->state       = TS_LISTEN;

	if(timeout == 0L) s->timeout = 0x7FFFFFFL; /* forever... */
	else			  s->timeout = timeout;

	s->myport      = port;
	s->hisport     = 0;
	s->seqnum      = 0;
	s->dataSize    = 0;
	s->flags       = 0;
	s->unhappy     = 0;
	s->dataHandler = datahandler;
	s->next        = tcp_allsocs;    	 /* Add it to the linklist */
	tcp_allsocs    = s;
}

/* ----- retransmitter ----------------------------------------------------- */
/* Retransmitter - called periodically to perform tcp retransmissions        */
/* ------------------------------------------------------------------------- */
static void tcp_Retransmitter( void )
{
	static	struct tcp_Socket *	s;
	static	short			x;
    int n;

	/* Go through all of the open sockets and see if there is anything to send */
    n = 0;
	for(s = tcp_allsocs; s; s = s->next) {
		x = False;
		if((s->dataSize > 0) || s->unhappy) {
			tcp_Send(s);  	/* send the data */
			x = True;
		}

		/* anything sent? */
		if(x || (s->state != TS_ESTAB)) s->timeout -= tcp_RETRANSMITTIME;

		if(s->timeout <= 0) {
			if(s->state == TS_TIMEWT) {
				printf("Closed. [222]\n" );
				s->state = TS_CLOSED;
				if(s->dataHandler != 0) (s->dataHandler)((void *) s, (Byte *) 0, 0 );
				else	printf( "got close, no handler\n" );
				tcp_Unthread(s);  /* Unthread this socket from the linklist */
			} else {
				printf("Timeout, aborting\n" );
				tcp_Abort(s);
			}
		}
        n++;
        if(n>10) {
            printf("something is wrong\n");
            break;
        }
	}
}

/* ----- Unthread a socket from the socket list, if it's there ------------- */
static void tcp_Unthread(struct tcp_Socket *ds)
{
	static	struct tcp_Socket *s, **sp;

	if(ds == (struct tcp_Socket *) 0) return;
	sp = &tcp_allsocs;			/* -> -> socket */
	for(;;) {
		s = *sp;			/* what pointer points to */
		if(s == ( struct tcp_Socket * ) 0 ) break;	/* end of list? */
		if(s == ds ) {			/* matches one to delete? */
			*sp = s->next;	    /* unlink it from the list */
			break;			    /* all done */
		}
		sp = &s->next;  		/* move to the next one */
	}
	ds->next = (struct tcp_Socket *) 0;	/* clear next pointer */
}

/* ----- busy-wait loop for tcp. Also calls an "application proc" --------- */
int tcp(Procrefv application)
{
	static	struct in_Header *ip;
	static	Longword timeout;
    Word chksum;

//	timeout = (Longword)MsecClock() + tcp_RETRANSMITTIME;
    timeout = 0L;

	for(;;) {
		ip = (struct in_Header *)sed_IsPacket();    /* check for packet received */
		if(ip) {          		                    /* We have received a packet. */

    		if(sed_CheckPacket(Protocol_ARP)) {    /* Process ARP protocol */
	    		arp_checkpacket((struct arp_Header *)ip );    /* do arp */
                    continue;         /* keep a going */
    		}

            if(sed_CheckPacket(Protocol_IP)) {   /* do IP */
    			if(rev_longword(ip->destination) != local_IP_address) {
	    		   	printf("IP address doesn't match %08x\n",rev_longword(ip->destination));
                    continue;         /* keep a going */
                }
			}

            chksum = checksum((Word *)ip, IP_HBYTES(ip));
			if(chksum != 0xFFFF) {
				printf("Checksum bad: %04x\n",chksum);
//              continue;            /* keep a going */
				printf("continuing anyhoo\n");
			}

			if(IP_PROTOCOL(ip) == 6) {  	/* Got tcp/ip   */
                tcp_Handler(ip);            /* handle it !  */
            }

			/* we could put protocol 1 for  icmp. here if we wanted to respond
               the first byte of the data is an 8 for ping (echo)
               requests; the reply Byte should have a zero */
		}
        else {                             /* No IP recvd yet */
			if(MsecClock() > timeout) {
				tcp_Retransmitter();           	/* Anything to retransmit? */
				timeout = MsecClock() + tcp_RETRANSMITTIME;  	/* Set next transmit time */
			}
			application();   /* There's nothing to do. Let the user enter a command. */
		}
	}
}

/* ----- Write data to a connection. --------------------------------------- */
/* Returns number of bytes written, == 0 when connection is not in           */
/* established state.                                                        */
/* ------------------------------------------------------------------------- */
int tcp_Write(struct tcp_Socket *s, Byte *dp, int len)
{
	static	int	x;
	if(s->state != TS_ESTAB) return 0;  /* if the connection is not established, don't send anything */

	if(len > (x = TCP_MAXDATA - s->dataSize)) len = x;    	/* figure out how much I can move */
	if(len > 0 ) {
		Move(dp, &s->data[s->dataSize], len);
		s->dataSize += len;
		tcp_Flush(s);
	}
	return len;	/* return count of bytes sent */
}

/* ----- Send pending data ------------------------------------------------- */
void tcp_Flush(struct tcp_Socket *s)
{
	if((s->state == 0 ) || (s->state == TS_CLOSED)) return;
	if( s->dataSize > 0 ) {
		s->flags |= TCPF_PUSH;
		tcp_Send(s);
	}
}

/* ----- Handler for incoming packets. ------------------------------------- */
static void tcp_Handler(struct in_Header *ip)
{
	static	struct tcp_Header *	tp;
	static	struct tcp_Pseudoheader ph;
	static	int			        len;
	static	int			        diff;
	static	struct tcp_Socket   *s;
	static	Word		        flags;
	static	Longword            lw;

	len = IP_HBYTES( ip );
	tp = ( struct tcp_Header * )(( Byte * ) ip + len );
	len = rev_word( ip -> length ) - len;

	/* demux to active sockets */
	for(s = tcp_allsocs; s; s = s->next) {
		if((s-> hisport != 0 )
			&& (rev_word    (tp->dstPort ) == s->myport )
			&& (rev_word    (tp->srcPort ) == s->hisport )
			&& (rev_longword(ip->source  ) == s->hisaddr )) break;
    }
	if(s == 0) {	                    /* didn't find an matching socket */
		for(s = tcp_allsocs; s; s = s->next) { 	/* demux to passive sockets */
			if((s->hisport == 0 ) && (rev_word(tp->dstPort ) == s->myport )) break;
        }
	}

	if(s == 0) {	            /* Still didn't find a matching socket */
#ifdef DEBUG_TCP
		tcp_DumpHeader( ip, tp, "Discarding");
#endif
		return;
	}

#ifdef DEBUG_TCP
	tcp_DumpHeader(ip, tp, "Received");
#endif

	/* save his ethernet address */
//	Move( (Byte *) &(((( struct eth_Header * ) ip) - 1)->source[0]), (Byte *) &s->hisethaddr, sizeof(struct Ethernet_Address));
	Move( (Byte *) &(((( struct eth_Header * ) ip) - 1)->source), (Byte *)&s->hisethaddr, sizeof(struct Ethernet_Address));

	ph.src      = ip->source;
	ph.dst      = ip->destination;
	ph.mbz      = 0;
	ph.protocol = 6;
	ph.length   = rev_word( len );

	lw = lchecksum((Word *) &ph, sizeof(ph) - 2);
	while(lw & 0xFFFF0000L) lw = ( lw & 0xFFFFL ) + (( lw >> 16L ) & 0xFFFFL );
	lw += lchecksum( ( Word * ) tp, len );
	while(lw & 0xFFFF0000L) lw = ( lw & 0xFFFFL ) + (( lw >> 16L ) & 0xFFFFL );
	if(lw != 0x0000FFFFL) printf("bad tcp checksum (%lx), received anyway\n", lw);

	flags = rev_word(tp->flags);

	if(flags & TCPF_RST) {
		printf("connection reset\n");

		s->state = TS_CLOSED;
		if(s->dataHandler != 0) (s->dataHandler)((void *)s, (Byte *)0, -1);
		else	                printf( "got close, no handler\n" );

		tcp_Unthread(s);
		return;
	}

	switch(s->state) {

	case TS_LISTEN:                 /* Initial state of a Listen port */
		if(flags & TCPF_SYN) {      /* We expect to get Syn back */
			s-> acknum = rev_longword( tp -> seqnum ) + 1;
			s-> hisport = rev_word( tp -> srcPort );
			s-> hisaddr = rev_longword( ip -> source );
			s-> flags = TCPF_SYN | TCPF_ACK;
			tcp_Send( s );
			s-> state = TS_RSYN;
			s-> unhappy = True;
			s-> timeout = tcp_TIMEOUT;

#ifdef DEBUG_TCP
			printf("Syn from 0x%x#%d (seq 0x%lx)\n",s->hisaddr, s->hisport, rev_longword(tp->seqnum));
#endif
		}
		break;

	case TS_SSYN:     /* Initial state of a Open port */
		if(flags & TCPF_SYN) {
			s->acknum++;
			s->flags = TCPF_ACK;
			s->timeout = tcp_TIMEOUT;

			if((flags & TCPF_ACK ) && rev_longword(tp->acknum ) == (s->seqnum + 1)) {
				printf( "--- Open! ---\n" );

				s->state = TS_ESTAB;
				s->seqnum++;
				s->acknum = rev_longword(tp -> seqnum) + 1;
				s->unhappy = False;
			} else {
				s->state = TS_RSYN;
			}
		}
        else {
#ifdef DEBUG_TCP
			printf( "Sent Syn, didn't get Syn back\n" );
#endif
			/* This is the half-open condition in rfc fig. 10, p. 74 (sec. 3.4). */

			s->flags = TCPF_RST;
			s->seqnum = rev_longword( tp -> acknum );
			tcp_Send( s );
			printf("Sent RST!\n");

			/* We expect no response. Reset the flags to send SYN when the timeout occurs. */
			s->flags = TCPF_SYN;
			s->timeout = tcp_TIMEOUT;
			s->seqnum = 0;	/* Start over */

			/* Stay in SYNSENT and wait for timeout. */
		}
		break;

	case TS_RSYN:
		if( flags & TCPF_SYN ) {
			s -> flags = TCPF_SYN | TCPF_ACK;
			tcp_Send(s);
			s -> timeout = tcp_TIMEOUT;
			printf(" retransmit of original syn\n");
		}
		if(( flags & TCPF_ACK ) && rev_longword( tp -> acknum ) == ( s -> seqnum + 1L ) ) {
			s -> flags = TCPF_ACK;
			tcp_Send( s );
			s-> seqnum++;
			s-> unhappy = False;
			s-> state = TS_ESTAB;
			s-> timeout = tcp_TIMEOUT;

			printf( "Synack received - connection established\n" );
		} else {
#ifdef DEBUG_TCP
			printf( "Wrong syn. flags %04x ack %ld seq %ld\n",
				flags,
				rev_longword( tp -> acknum ),
				s -> seqnum + 1L );
#endif
		}
		break;

	case TS_ESTAB:
		if((flags & TCPF_ACK) == 0) return;

		/* process ack value in packet */
		diff = (int)(rev_longword(tp->acknum) - s->seqnum);

		if(diff > 0) {   /* The diff value is the number of bytes of MY data which he is acknowledging. */
			if(diff > TCP_MAXDATA) diff = TCP_MAXDATA;
			else {
				Move( &s -> data[ diff ], &s -> data[ 0 ], TCP_MAXDATA - diff );
			}
			s-> dataSize -= diff;	/* bytes to send remaining */
			s-> seqnum += diff;	/* my next sequence number */
		}
		s->flags = (Word)TCPF_ACK;

		tcp_ProcessData(s, tp, len);
		break;

	case TS_SFIN:
		if(( flags & TCPF_ACK ) == 0 ) return;
		diff = ( int )( rev_longword( tp -> acknum ) - s -> seqnum - 1 );
		s->flags = ( Word )( TCPF_ACK | TCPF_FIN );
		if( diff == 0 ) {
			s->state = TS_AFIN;
			s->flags = TCPF_ACK;
			printf("finack received.\n");
		}
		tcp_ProcessData(s, tp, len);
		break;

	case TS_AFIN:
		s->flags = TCPF_ACK;
		tcp_ProcessData( s, tp, len );
		break;

	case TS_RFIN:
		if(rev_longword( tp -> acknum ) == (s -> seqnum + 1) ) {
			s->state   = TS_TIMEWT;
			s->timeout = tcp_TIMEOUT;
		}
		break;

	case TS_LASTACK:
		if( rev_longword( tp -> acknum ) == (s -> seqnum + 1) ) {
			s-> state = TS_CLOSED;
			s-> unhappy = False;
			s-> dataSize = 0;

			if(s-> dataHandler != 0 )	/* cast removed */
				(s-> dataHandler )(( void * ) s, (Byte *)0, 0);
			else	printf( "got close, no handler\n" );

			tcp_Unthread(s);

			printf( "Closed. [626]\n" );
		} else {
			s -> flags = TCPF_ACK | TCPF_FIN;
			tcp_Send( s );
			s -> timeout = tcp_TIMEOUT;

			printf( "retransmitting FIN\n" );
		}
		break;

	case TS_TIMEWT:
		s->flags = TCPF_ACK;
		tcp_Send(s);
	}
}

/* ----- Process the data in an incoming packet. --------------------------- */
/* Called from all states where incoming data can be received: established,  */
/*	fin-wait-1, fin-wait-2                                                   */
/* ------------------------------------------------------------------------- */
static void tcp_ProcessData( s, tp, len )
	struct tcp_Socket *s;
	struct tcp_Header *tp;
	int len;
{
	static	int		 diff, x;
	static	Word	 flags;
	static	Byte    *dp;

	flags = ( Word ) rev_word( tp -> flags );

	/* Look at the difference between the value I'm acking and the
		sequence number he's sending. The difference (if any) is the
		length of data in the buffer which we have already seen. */

	diff = ( int )( s -> acknum - rev_longword( tp -> seqnum ));

	/* I don't understand the following statement, unless it is supposed
		to be compensating for options. There should not be data on a SYN packet. */

	if( flags & TCPF_SYN ) diff--;

	x = TCP_DATAOFFSET( tp ) << 2;	/* mult. times 4 */
	dp = (( Byte * ) tp ) + x;		/* point to data */
	len -= x;				        /* subtract offset */

	if( diff >= 0 ) {
		dp += diff;		/* Ignore stuff we've already seen */
		len -= diff;		/* subtract length of data we've already seen */
		s -> acknum += len;	/* Add len to acknum to acknowledge new data */

		if( s -> dataHandler != 0 )	( s -> dataHandler )(( void * ) s, dp, len );
		else	printf( "got data, %d bytes\n", len );

		if( flags & TCPF_FIN ) {
			s -> acknum++;
#ifdef DEBUG_TCP
			printf("consumed fin.\n" );
#endif
			switch(s -> state) {
			case TS_ESTAB:
				/* note: skip state CLOSEWT by automatically closing conn */
				x = TS_LASTACK;
				s -> flags |= TCPF_FIN;
				s -> unhappy = True;
#ifdef DEBUG_TCP
				printf( "sending fin.\n" );
#endif
				break;
			case TS_SFIN:
				x = TS_RFIN;
				break;
			case TS_AFIN:
				x = TS_TIMEWT;
				break;
			}
			s -> state = x;
		}
	} else {
#ifdef DEBUG_TCP
		printf( "diff was negative, %d\n", diff );
#endif
	}
	s -> timeout = tcp_TIMEOUT;
	tcp_Send( s );
}

/* ----- Format and send an outgoing segment ------------------------------- */
void tcp_Send(struct tcp_Socket *s)
{
	static	struct tcp_Pseudoheader	ph;

	static	struct _pkt {
			struct in_Header	in;
			struct tcp_Header	tcp;
			Longword		maxsegopt;
		} * pkt;

	static	Byte *			dp;
	static	Longword		lw;

	/* don't do it if the state is Closed or the socket is not on the linklist */
	if((s->state == 0) || (s -> state == TS_CLOSED)) return;

	pkt = (struct _pkt *)sed_FormatPacket((Byte *) &(s->hisethaddr.MAC[0] ), rev_word(Protocol_IP));
	dp  = (Byte *) &(pkt->maxsegopt );

	if(s->flags & TCPF_SYN) {       /* Should not send data on SYN */
		pkt->in.length = rev_word(sizeof(struct in_Header) + sizeof(struct tcp_Header));
	}
    else {
		pkt->in.length = rev_word(sizeof(struct in_Header) + sizeof(struct tcp_Header) + s->dataSize);
	}

	/* tcp header */

	pkt->tcp.srcPort = rev_word(s->myport );
	pkt->tcp.dstPort = rev_word(s->hisport );
	pkt->tcp.seqnum  = rev_longword(s->seqnum );
	pkt->tcp.acknum  = rev_longword(s->acknum );
	pkt->tcp.window  = rev_word(1024);  /* This 1024 is the size of data which I am willing to receive */
	pkt->tcp.flags         = rev_word(s->flags | 0x5000);
	pkt->tcp.checksum      = 0;
	pkt->tcp.urgentPointer = 0;

	if(s->flags & TCPF_SYN) {
		/* If sending the options, add 1 DWORD to the data offset */
		pkt->tcp.flags = rev_word(rev_word(pkt->tcp.flags) + 0x1000);

		/* Add 4 to the length */
		pkt->in.length = rev_word(rev_word(pkt->in.length) + 4 );

		/* Options. This is really: kind 02, length 04, value 1400B. See page 42. */
		pkt-> maxsegopt = rev_longword(0x02040578);

		dp += 4;
	} else {
		Move(s->data, dp, s -> dataSize );  	/* Only send the data if NOT a SYN. */
	}

	/* internet header */
	pkt->in.vht            = rev_word(0x4500);       /* version 4, hdrlen 5, tos 0 */
	pkt->in.identification = rev_word( tcp_id++ );
	pkt->in.frag           = 0;
	pkt->in.ttlProtocol    = rev_word((200 << 8) + 6);
	pkt->in.checksum       = 0;
	pkt->in.source         = rev_longword(local_IP_address);
	pkt->in.destination    = rev_longword(s->hisaddr);
	pkt->in.checksum       = rev_word((Word) ~checksum((Word *)&pkt->in, sizeof(struct in_Header)));

	/* compute tcp checksum */
	ph.src      = pkt->in.source;
	ph.dst      = pkt->in.destination;
	ph.mbz      = 0;
	ph.protocol = 6;
	ph.length   = rev_word(rev_word(pkt->in.length ) - sizeof(struct in_Header));

	/* Actually, the idea is to compute the checksum of the pseudoheader plus that of the
       tcp header and the data portion. By sticking the *unreversed* checksum in the
       pseudoheader itself, we hope to achieve the same result. I'm rather doubtful of this,
       because the overflows will have been mangled. It seems to work on receive, though. */

	lw = lchecksum((Word *)&ph, sizeof(ph) - 2);
	while( lw & 0xFFFF0000L) lw = ( lw & 0xFFFFL ) + (( lw >> 16L ) & 0xFFFFL);

	lw += lchecksum( ( Word * ) &pkt -> tcp, rev_word( ph.length ));
	while( lw & 0xFFFF0000L) lw = ( lw & 0xFFFFL ) + (( lw >> 16L ) & 0xFFFFL);

	pkt->tcp.checksum = rev_word(~(Word)(lw & 0xFFFFL));

#ifdef DEBUG_TCP
	tcp_DumpHeader(&pkt->in, &pkt -> tcp, "Sending");
#endif

	sed_Send(rev_word(pkt->in.length));
}

/* ----- calculate Word checksum ------------------------------------------- */
Word checksum(Word *dp, int length)
{
	static Longword sum;

	sum = lchecksum(dp, length);
	while(sum & 0xFFFF0000L) sum = (sum & 0xFFFFL) + ((sum >> 16L) & 0xFFFFL);
	return((Word)(sum & 0xFFFF));
}

/* ----- compute a Longword sum of words in message ------------------------ */
/* (partial checksum)                                                        */
/* ------------------------------------------------------------------------- */
Longword lchecksum(Word *dp, int length)
{
	static	int		    len;
	static	Longword	sum;

	len = length >> 1;
	sum = 0L;

	while ( len-- > 0 ) sum += rev_word( *dp++ );
	if( length & 1 ) sum += ( rev_word( *dp ) & 0xFF00 );
	return sum;
}

/* ----- Dump tcp protocol header of a packet ------------------------------ */
static void tcp_DumpHeader(struct in_Header *ip, struct tcp_Header *tp, char *mesg)
{
	static char *flags[] = { "FIN", "SYN", "RST", "PUSH", "ACK", "URG" };
	static	int		len;
	static	Word		f;

	len = rev_word(ip->length) - ((TCP_DATAOFFSET(tp) + IP_HLEN(ip)) << 2);
	printf("TCP: %s packet:\nSrcP: %x; DstP: %x; SeqN=%lx AckN=%lx Wind=%d DLen=%d\n",
		mesg,
		rev_word( tp -> srcPort ),
		rev_word( tp -> dstPort ),
		rev_longword(tp-> seqnum ),
		rev_longword(tp-> acknum ),
		rev_word( tp -> window ),
		len );
	printf( "DO=%d, C=%x U=%d",
		TCP_DATAOFFSET( tp ),
		rev_word(tp -> checksum),
		rev_word(tp -> urgentPointer));

	/* output flags */
	f = rev_word( tp -> flags );
	for(len = 0; len < 6; len++) {
		if(f & ( 1 << len )) printf( " %s", flags[ len ]);
    }
	printf( "\n" );
}

/* ----- tcp close --------------------------------------------------------- */
/* Send a FIN on a particular port -- only works if it is open               */
/* ------------------------------------------------------------------------- */
void tcp_Close(struct tcp_Socket *s)
{
	if(s->state == TS_ESTAB || s -> state == TS_RSYN ) {
	   s->flags   = TCPF_ACK | TCPF_FIN;
	   s->state   = TS_SFIN;
	   s->unhappy = True;
	}
}

/* ----- Abort a tcp connection -------------------------------------------- */
void tcp_Abort(struct tcp_Socket *s)
{
	if(s->state != TS_LISTEN && s -> state != TS_CLOSED ) {
	   s->flags = TCPF_RST | TCPF_ACK;
	   tcp_Send(s);
	}
	s->unhappy  = 0;
	s->dataSize = 0;
	s->state    = TS_CLOSED;

	if(s->dataHandler != 0) (s->dataHandler )((void *)s, (Byte *)0, -1);
	else	printf( "got abort, no handler\n" );

	tcp_Unthread( s );
}

/* ----- Little Endian Byte Reversal --------------------------------------- */
Word  swapBytes(Word x)
{
    Byte hi, lo;
    union valBytes v;
    v.sw = x;
    hi = v.valB.lo;
    lo = v.valB.hi;
    v.valB.lo = lo;
    v.valB.hi = hi;
    return(v.sw);
}
Longword  swapWords(Longword x)
{
    Word hi,lo;
    union ValWords v;
    v.lw = x;
    hi = swapBytes(v.valW.lo);
    lo = swapBytes(v.valW.hi);
    v.valW.hi = hi;
    v.valW.lo = lo;
    return(v.lw);
}

/* ----- End of TinyTCP.c -------------------------------------------------- */

