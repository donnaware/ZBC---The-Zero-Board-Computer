/* SEDSLIP.C - Use slip for tinytcp by emulating the sed entry points
	This code is not derived in any way from the original Tiny-TCP
	package.

|===================================================================|
|  My changes can be considered public domain.  Geof's statement    |
|  will cover everything.                                           |
|              - Rick Rodman 09/02/97                               |
|===================================================================|

	931213	rr	orig attempt
	941010	rr	return int on all calls
*/

/* #define DEBUG_SED */

#include "tinytcp.h"
/* #include "sed.h" */

#include "casyncms.h"			/* async drivers for PC */

#ifdef PC
/* #include <conio.h> */
#endif
#include <stdio.h>

/* ----- internal definitions --------------------------------------- */

#define	FR_END		0xC0	/* 0300 */
#define	FR_ESC		0xDB	/* 0333 */
#define	T_FR_END	0xDC	/* 0334 */
#define	T_FR_ESC	0xDD	/* 0335 */

#define	BUFFERSIZE	8192

static	unsigned char	xmitbuffer[ BUFFERSIZE ];
static	unsigned char	recvbuffer[ BUFFERSIZE ];

/* ----- initialize the slip interface ------------------------------ */

int sed_Init() {
	init_comm();		/* initialize the communications interface */
	inp_flush();		/* flush the input fifo */

	return 1;
}

/* ----- deinit the interface --------------------------------------- */

int sed_Deinit() {
	uninit_comm();
	return 1;
}

/* ----- Format an ethernet header in the transmit buffer ----------- */
	/* return pointer to where to build the IP message */

Byte * sed_FormatPacket( destEAddr, ethType )
	Byte *destEAddr; int ethType; {

	return ( Byte * ) &xmitbuffer[ 0 ];	/* return ptr to 1st Byte */
}

/* ----- send a packet on the link ---------------------------------- */

int sed_Send( pkLengthInBytes ) int pkLengthInBytes; {
	int		i;
	unsigned char *	p_uc;

	if( pkLengthInBytes <= 0 ) return 1;	/* nothing to do */ 

	/* While sending, we have to process the escapes.
		If we find an FR_END, change it to FR_ESC + T_FR_END.
		If we find an FR_ESC, change it to FR_ESC + T_FR_ESC. */

	outp_char( FR_END );		/* send FR_END. */
	p_uc = &xmitbuffer[ 0 ];

	for( i = 0; i < pkLengthInBytes; ++i ) {
		switch( *p_uc ) {
		case FR_END:
			outp_char( FR_ESC );
			outp_char( T_FR_END );
			break;
		case FR_ESC:
			outp_char( FR_ESC );
			outp_char( T_FR_ESC );
			break;
		default:
			outp_char( *p_uc );
		}
		++p_uc;
	}
	outp_char( FR_END );

#ifdef DEBUG_SED
	printf( "SEDSLIP: Sent a packet %d bytes.\n", pkLengthInBytes );
#endif
	return 1;
}

/* ----- prepare to receive packets --------------------------------- */

	/* If the argument is zero, enable both buffers.
		If the argument is nonzero,
		take it as the address of the buffer to be enabled.
		(i.e. that message has been processed) */

int sed_Receive( recBufLocation ) Byte *recBufLocation; {

	/* This call can be ignored. */
	return 1;
}

/* ----- check for packet received ---------------------------------- */

	/* Test for the arrival of a packet. The location of the packet is
		returned. If no packet is returned, the routine returns
		zero. (Timeout is irrelevant here.) */

Byte * sed_IsPacket() {
	unsigned char		uc;

	static	unsigned char *	p_begin_packet = &recvbuffer[ 0 ];
	static	unsigned char *	p_recv_next = &recvbuffer[ 0 ];
#ifdef DEBUG_SED
	unsigned char *		p;
#endif
	static	int	end_count = 0;
	static	int	b_esc_flag = 0;

	/* See if there are more packets in the receiver */

	while( inp_status() != 0 ) { 	/* any chars to receive? */
		uc = ( unsigned char ) inp_char();	/* get one */

		if( b_esc_flag ) {
			switch( uc ) {
			case T_FR_ESC:
				*p_recv_next++ = FR_ESC;
				break;
			case T_FR_END:
				*p_recv_next++ = FR_END;
				break;
			default:
				*p_recv_next++ = uc;
			}
			b_esc_flag = 0;
		} else {
			switch( uc ) {
			case FR_ESC:
				b_esc_flag = 1;
				break;
			case FR_END:
				++end_count;
				if( end_count == 1 )
					p_begin_packet = p_recv_next;
				break;
			default:
				*p_recv_next++ = uc;
			}
		}
		if( end_count == 2 ) break;
	}

	/* do we have a packet in the buffer? */

	if( end_count == 2 ) {

#ifdef DEBUG_SED
		printf( "SEDSLIP: Received a packet.\n" );

		p = p_begin_packet;
		while( p != p_recv_next ) {
			printf( "%02x ", *p++ );
		}
		printf( "\n" );
#endif

		p_recv_next = &recvbuffer[ 0 ];
		end_count = 0;

		return p_begin_packet;

	} else	return ( Byte * ) 0;
}

/* ----- check received packet for ethernet type -------------------- */

int sed_CheckPacket( recBufLocation, expectedType )
	Word *recBufLocation; Word expectedType; {

	if( expectedType != 0x800 )		/* Must be IP */
		return 0;

	return 1;			/* Yes, it's an IP */
}

/* end of sed.c */

