/* ----- TinySOCK.h -----------------------------------------------------------
 *
 * TinySOCK.h - header file for all modules in TinySOCK.
 *
 * Note: the structures herein must guarantee that the code only performs Word
 * fetches, since the imagenether card doesn't accept byte accesses.
 * minor mods,remove all typedefs. NOT TESTED SINCE.
 *
 * ------------------------------------------------------------------------- */

/* ----- debug macros ------------------------------------------------------ */
/* define DEBUG to get lots of messages 									 */
/* #define	DEBUG */
#ifdef DEBUG
#define DEBUGMSG(x)	printf( "main.c: %s\n", x )
#else
#define	DEBUGMSG(x)
#endif

/* ----- macros and definitions -------------------------------------------- */
#define True		1
#define False		0

#define  Protocol_IP    0x0800      // Internet Protocol (IP)                */
#define  Protocol_ARP   0x0806      /* Address Resolution Protocol (ARP)     */
#define  Protocol_ICMP    0x01      /* ICMP/IP protocol                      */
#define  Protocol_UDP     0x11      /* UDP/IP protocol                       */


/* ----- address macro ----------------------------------------------------- */
#define	ADDR(a,b,c,d)	(((long)a<<24)|((long)b<<16)|((long)c<<8)|((long)d))
#define IP_1B(x)        (int)(x>>24)
#define IP_2B(x)        (int)(x>>16)&0xFF
#define IP_3B(x)        (int)(x>> 8)&0xFF
#define IP_4B(x)        (int)(x    )&0xFF

/* ----- options ----------------------------------------------------------- */
#include "options.h"

/* ----- Prototype -------------------------------------------------------- */
/* uncomment following for compilers allowing prototypes 			        */
#define P(x) x

/* ----- Canonically-sized data ------------------------------------------- */
typedef  unsigned long  Longword;
typedef  unsigned short Word;
typedef  unsigned char  Byte;
typedef  unsigned char  BOOL;

/* ----- byte order reversal crap ------------------------------------------ */
union ValWords {
    Longword lw;
    struct valWord {
        Word lo;
        Word hi;
    } valW;
} ;
union valBytes {
    Word  sw;
    struct valByte {
        Byte lo;
        Byte hi;
    } valB;
} ;

/* ----- protocol address definitions ------------------------------------- */
#define IP_Address	Longword

struct Ethernet_Address { Byte   MAC[6]; };

/* ----- protocol address definitions ------------------------------------- */
/* The true type of 's' should be 'tcp_Socket *'. But Microsoft C won't let */
/* us do that, because 'Procref' is a type used in the definition of that structure. */

typedef void ( *Procref  ) P(( void *s, Byte *dp, int len ));
typedef void ( *Procrefv ) P(( void ));

/* ----- The Ethernet header ----------------------------------------------- */
struct eth_Header {
	struct	Ethernet_Address destination;	/* 48 bits = 6 bytes */
	struct	Ethernet_Address source;		/* 48 bits = 6 bytes */
	Word				 	 type;			/* 16 bits = 2 bytes */
};											/* total 14 bytes */

/* ----- Standar IP header Definitions ------------------------------------- */
#define IPVERTOS 0x4500 				       /* version 4, hdrlen 5, tos 0 */

/* ----- The Internet header ----------------------------------------------- */
struct in_Header {
	Word		vht;		/* version, hdrlen, tos */
							/* 2 bytes */
							/* version = first nybble = 4. */
                            /* hdrlen = low nybble of 1st byte = length of header in dwords = 5 */
	Word		length;			/* 2 bytes */
	Word		identification;	/* 2 bytes */
	Word		frag;			/* 2 bytes */
	Word		ttlProtocol;	/* 2 bytes, 1st byte is TTL, 2nd byte is protocol */
	Word		checksum;		/* 2 bytes */
	IP_Address	source;			/* IP address - 4 bytes */
	IP_Address	destination;	/* IP address - 4 bytes */
};								/* total 20 bytes */

/* ----- Macros ------------------------------------------------------------ */
/* macros that parse or set parts of the ip message                          */
/* ----- Macros ------------------------------------------------------------ */
#define IP_VERSION(ip)		(( rev_word((ip)->vht) >> 12) & 0x0f)
#define IP_HLEN(ip)		    (( rev_word((ip)->vht) >>  8) & 0x0f)
#define IP_HBYTES(ip)		(( rev_word((ip)->vht) >>  6) & 0x3c)
#define IP_TOS(ip)		    (  rev_word((ip)->vht)        & 0xff)
#define IP_TTL(ip)		    (  rev_word((ip)->ttlProtocol) >> 8)
#define IP_PROTOCOL(ip)	    (  rev_word((ip)->ttlProtocol) & 0xff)
#define IP_TTLPROT(t,p)     (  rev_word(((t) << 8) + (p))

/* ----- UDP header -------------------------------------------------------- */
typedef struct udp_Header {     /* 8 bytes                  */
	Word src_portno;            /* Source port number       */
	Word dst_portno;            /* Destination port number  */
	Word udp_length;            /* UDP packet length        */
	Word udp_checksum;          /* UDP checksum (optional)  */
};                              /* 8 bytes long always      */

/* ----- UDP Packet -------------------------------------------------------- */
struct udp_packet {
	struct in_Header	ip;
	struct udp_Header	udp;
};

/* ----- TCP header -------------------------------------------------------- */
struct tcp_Header {
	Word		srcPort;
	Word		dstPort;
	Longword	seqnum;
	Longword	acknum;
	Word		flags;
	Word		window;
	Word		checksum;
	Word		urgentPointer;
};

#define TCPF_FIN	0x0001
#define TCPF_SYN	0x0002
#define TCPF_RST	0x0004
#define TCPF_PUSH	0x0008
#define TCPF_ACK	0x0010
#define TCPF_URG	0x0020
#define TCPF_DO		0xF000
#define TCP_DATAOFFSET(tp) ( rev_word((tp)->flags) >> 12)

/* The TCP/UDP Pseudo Header. Used for computing checksum. */
struct tcp_Pseudoheader {
	IP_Address	src;		/* 4 bytes */
	IP_Address	dst;		/* 4 bytes */
	Byte		mbz;		/* must be zero. 1 byte */
	Byte		protocol;	/* 6 = tcp. 1 byte */
	Word		length;		/* 2 bytes */
	Word		checksum;	/* 2 bytes */
};					        /* 14 bytes */

/* TCP states, from tcp manual.
Note: close-wait state is bypassed by automatically closing a connection
	when a FIN is received. This is easy to undo.

rr 940529 There should not be a state 0 in here. When a socket is unlinked
	the state should be set to zero, indicating an invalid socket.
*/

#define TS_LISTEN	1	/* listening for connection */
#define TS_SSYN		2	/* syn sent, active open */
#define TS_RSYN		3	/* syn received, synack+syn sent. */
#define TS_ESTAB	4	/* established */
#define TS_SFIN		5	/* sent FIN */
#define TS_AFIN		6	/* sent FIN, received FINACK */
#define TS_CLOSEWT	7 	/* received FIN waiting for close */
#define TS_RFIN		8	/* (closing) sent FIN, received FIN */
				        /* (waiting for FINACK) */
#define TS_LASTACK	9	/* fin received, finack+fin sent */
#define TS_TIMEWT	10	/* dally after sending final FINACK */
#define TS_CLOSED	11	/* (closed) finack received */

/* ------ TCP Socket definition -------------------------------------------- */
#define TCP_MAXDATA	512		/* maximum bytes to buffer on output             */

struct tcp_Socket {
	struct tcp_Socket * next;	        /* pointer to next socket           */
	short		state;			        /* connection state                 */
	Procref		dataHandler;	        /* called with incoming data        */
	struct Ethernet_Address hisethaddr;	/* ethernet address of peer         */
	IP_Address	hisaddr;		        /* internet address of peer         */
	Word		myport, hisport;        /* tcp ports for this connection    */
	Longword	acknum;                 /* data ack'd number                */
    Longword	seqnum;                 /* data sequence num                */
	Longword	timeout;		        /* timeout, in milliseconds         */
	short		unhappy;		        /* flag, retransmitting segt's      */
	Word		flags;			        /* flags Word for last packet sent  */
	short		dataSize;		        /* number of bytes of data to send  */
	Byte		data[ TCP_MAXDATA ];    /* data to send                     */
} ;

/* ----- ARP definitions --------------------------------------------------- */
#define arp_TypeEther	1		/* ARP type of Ethernet address */
#define ARP_REQUEST	    1       /* harp op codes */
#define ARP_REPLY	    2

struct arp_Header {
	Word		hwType;
	Word		protType;
	Word		hwProtAddrLen;  /* hw and prot addr len */
	Word		opcode;
	struct Ethernet_Address srcEthAddr;
	IP_Address	srcIPAddr;
	struct Ethernet_Address dstEthAddr;
	IP_Address	dstIPAddr;
};

/* ----- icmp definitions -------------------------------------------------- */
#define ICMP_t_echoreply      0     /* type - an Echo is being requested     */
#define ICMP_c_echoreply      0     /* code - an Echo is being requested     */
#define ICMP_t_echoreq        8     /* type - echo request                   */
#define ICMP_c_echoreq        0     /* code - echo request                   */
#define ICMP_t_destunreach    3     /* type - Destination Unreachable        */
#define ICMP_c_routefail      5     /* code - combine with above             */
struct icmp_Header {
	Byte		type;       /* Type or request                               */
    Byte        code;       /* response code                                 */
	Word		checksum;   /* checksum                                      */
	Word		ID;         /* message ID, return with echo                  */
	Word		Sequence;   /* should be returned in case of ECHO REPLY      */
};

/* ----- icmp Packet ------------------------------------------------------- */
struct icmp_packet {
	struct in_Header	ip;
	struct icmp_Header	icmp;
};

/* ----- TCP Timer definitions ----------------------------------------------*/
#define ETHERNET 1
#if     ETHERNET					/* ETHERNET VALUES */
#define tcp_RETRANSMITTIME  1000L  	/* interval retransmitter is called 1sec */
#define tcp_LONGTIMEOUT    20000L   /* timeout for opens 					 */
#define tcp_TIMEOUT	       20000L   /* timeout during a connection 			 */
#else
#define tcp_RETRANSMITTIME 10000 	/* interval retransmitter is called 10sec*/
#define tcp_LONGTIMEOUT    30000    /* timeout for opens 					 */
#define tcp_TIMEOUT	       20000    /* timeout during a connection 			 */
#endif

/* ----- function prototype definitions ------------------------------------ */
#include "proto.h"

/* ----- end of tinytcp.h -------------------------------------------------- */


