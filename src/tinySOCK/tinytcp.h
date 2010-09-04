/* ----- address macro ---------------------------------------------- */
/* tinytcp.h - header file for tinytcp.c
 *
 * Note: the structures herein must guarantee that the code only performs Word
 * fetches, since the imagenether card doesn't accept byte accesses.
 * minor mods,remove all typedefs. NOT TESTED SINCE.
*/
#define True		1
#define False		0

/* ----- address macro ---------------------------------------------- */
#define	ADDR(a,b,c,d)	(((long)a<<24)|((long)b<<16)|((long)c<<8)|((long)d))

/* ----- options ---------------------------------------------------- */
#include "options.h" 

/* ----- Prototype -------------------------------------------------- */
/* uncomment following for compilers allowing prototypes 			  */
#define P(x) x

/* ----- Canonically-sized data ------------------------------------- */
//#define Longword	unsigned long	/* 32 bits */
///#define	Word		unsigned short	/* 16 bits */
//#define	Byte		unsigned char	/*  8 bits */
//#define	BOOL        unsigned char   /*  1 bits */

typedef  unsigned long  Longword;
typedef  unsigned short Word;
typedef  unsigned char  Byte;
typedef  unsigned char  BOOL;

/* ----- protocol address definitions --------------------------------- */
#define IP_Address	Longword

struct Ethernet_Address { Byte   MAC[6]; };

/* The true type of 's' should be 'tcp_Socket *'. But Microsoft C won't let us
   do that, because 'Procref' is a type used in the definition of that structure. */

typedef void ( *Procref  ) P(( void *s, Byte *dp, int len ));
typedef void ( *Procrefv ) P(( void ));

/* The Ethernet header */
struct eth_Header {
	struct	Ethernet_Address destination;	/* 48 bits = 6 bytes */
	struct	Ethernet_Address source;		/* 48 bits = 6 bytes */
	Word				 	 type;			/* 16 bits = 2 bytes */
};											/* total 14 bytes */

/* The Internet Header: */
struct in_Header {
	Word		vht;		/* version, hdrlen, tos */
							/* 2 bytes */
							/* version = first nybble = 4. */
    /* hdrlen = low nybble of 1st byte = length of header in dwords = 5 */

	Word		length;			/* 2 bytes */
	Word		identification;	/* 2 bytes */
	Word		frag;			/* 2 bytes */
	Word		ttlProtocol;	/* 2 bytes */
								/* first byte is TTL = 'time to lose' */
								/* second byte is protocol */
	Word		checksum;		/* 2 bytes */
	IP_Address	source;			/* IP address - 4 bytes */
	IP_Address	destination;	/* IP address - 4 bytes */
};								/* total 20 bytes */

/* macros which parse parts of the ip message */
#define IP_VERSION(ip)		(( rev_word((ip)->vht) >> 12) & 0xf)
#define IP_HLEN(ip)		    (( rev_word((ip)->vht) >> 8) & 0xf)
#define IP_HBYTES(ip)		(( rev_word((ip)->vht) >> 6) & 0x3c)
/* #define IP_TOS(ip)		( rev_word((ip)->vht) & 0xff) */
/* #define IP_TTL(ip)		( rev_word((ip)->ttlProtocol) >> 8) */
#define IP_PROTOCOL(ip)	    ( rev_word((ip)->ttlProtocol) & 0xff)

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

/* UDP header is only 8 bytes */

/* The TCP/UDP Pseudo Header. Used for computing checksum. */

struct tcp_Pseudoheader {
	IP_Address	src;		/* 4 bytes */
	IP_Address	dst;		/* 4 bytes */
	Byte		mbz;		/* must be zero. 1 byte */
	Byte		protocol;	/* 6 = tcp. 1 byte */
	Word		length;		/* 2 bytes */
	Word		checksum;	/* 2 bytes */
};					/* 14 bytes */

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
/* #define TS_CLOSEWT	7 */	/* received FIN waiting for close */
#define TS_RFIN		8	/* (closing) sent FIN, received FIN */
				/* (waiting for FINACK) */
#define TS_LASTACK	9	/* fin received, finack+fin sent */
#define TS_TIMEWT	10	/* dally after sending final FINACK */
#define TS_CLOSED	11	/* (closed) finack received */

/* ------ TCP Socket definition ------------------------------------- */
#define TCP_MAXDATA	512		/* maximum bytes to buffer on output */

struct tcp_Socket {
	struct tcp_Socket * next;	/* pointer to next socket */
	short		state;			/* connection state */
	Procref		dataHandler;	/* called with incoming data */
	struct Ethernet_Address hisethaddr;	/* ethernet address of peer */
	IP_Address	hisaddr;		/* internet address of peer */
	Word		myport, hisport; /* tcp ports for this connection */
	Longword	acknum, seqnum; /* data ack'd and sequence num */
	Longword	timeout;		/* timeout, in milliseconds */
	short		unhappy;		/* flag, retransmitting segt's */
	Word		flags;			/* flags Word for last packet sent */
	short		dataSize;		/* number of bytes of data to send */
	Byte		data[ TCP_MAXDATA ]; /* data to send */
} ;

/* ----- ARP definitions -------------------------------------------- */
#define arp_TypeEther	1		/* ARP type of Ethernet address */

/* harp op codes */

#define ARP_REQUEST	1
#define ARP_REPLY	2

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

/* ----- Timer definitions ------------------------------------------ */

#ifdef ETHERNET		/* ETHERNET VALUES */
#define tcp_RETRANSMITTIME 1000	/* interval at which retransmitter is called */
					/* 1 SECOND */
#define tcp_LONGTIMEOUT 31000	/* timeout for opens */
#define tcp_TIMEOUT	10000	/* timeout during a connection */
#else	/* GUESSING SLIP VALUES */
#define tcp_RETRANSMITTIME 10000 /* interval at which retransmitter is called */
					/* 10 SECONDS */
#define tcp_LONGTIMEOUT 31000	/* timeout for opens */
#define tcp_TIMEOUT	20000	/* timeout during a connection */
#endif

#include "proto.h"

/* ----- end of tinytcp.h -------------------------------------------- */

