/* MAIN.C - for tinytcp

|===================================================================|
|  My changes can be considered public domain.  Geof's statement    |
|  will cover everything.                                           |
|              - Rick Rodman 09/02/97                               |
|===================================================================|

	931208	rr	orig version

Note:
	I need to improve the addresses I'm using. The best way to do
	this would be to use a subnet mask between the Ethernet, Token
	Ring, and SLIP links. Each machine will have a unique address
	on each network.

	SLIP seems to be completely point-to-point: The IBM OS/2 TCP/IP
	won't route the packets; it simply rejects them.
	How do we handle this? At the FTP layer or the TCP layer?
*/

#include "tinytcp.h"
#include <stdio.h>

#ifdef PC
#include <dos.h>
#endif

/* ----- debug macros ----------------------------------------------- */

/* define DEBUG to get lots of messages */
#define	DEBUG

#ifdef DEBUG
#define DEBUGMSG(x)	printf( "main.c: %s\n", x )
#else
#define	DEBUGMSG(x)
#endif

/* ----- main program ----------------------------------------------- */

void main( argc, argv ) int argc; char *argv[];
{

	/* FUTURE: argv[ 1 ] = host to call */

	/* --- initialization --- */

	DEBUGMSG( "about to call sed_Init" );

	sed_Init();			/* init ethernet driver */

	DEBUGMSG( "about to call tcp_Init" );

	tcp_Init();			/* init TCP/IP */

	/* --- file transfer --- */

	DEBUGMSG( "about to call ftp" );

	ftp( ADDR( 192,9,201,2 ) );	/* IP addr of host to call */
			/* 0xC009C802 */ /* 192.9.201.2 tritium */

	/* if the filename is passed, the file is retrieved. */

	/* --- deinitialization --- */

	DEBUGMSG( "about to call sed_Deinit" );

	sed_Deinit();			/* deinit the interface */

	DEBUGMSG( "about to return" );
}

/* ----- millisecond clock ------------------------------------------ */

	/* Get time in milliseconds, crudely. */

Longword MsecClock( Void ) {

#ifdef PC
	static	union REGS	regs;
	unsigned long		ticks;

	/* Start with the 18ths of a second and multiply by 55. */

	static unsigned long	baseticks = 0L;		/* time zero */

	regs.h.ah = 0;
	int86( 0x1A, &regs, &regs );

	ticks = ( (( unsigned long ) regs.x.cx ) << 16L )
		+ ( unsigned long ) regs.x.dx; 

	if( baseticks == 0L ) baseticks = ticks;	/* keep time zero */

	ticks -= baseticks;			/* subtract time zero */

	return ( 55L * ticks );
#endif	/* PC */

#ifdef	XEROX820
	static	int		*p_ticker = 0xFF5A;

	/* Ticker is incremented once per second. Just multiply the value
		by 1000. */

	return ( Longword )((( long ) *p_ticker ) * 1000L );
#endif		/* XEROX820 */

	/* We'll have to find something for the Kaypro... */

#ifdef KAYPRO
	static	int		fudge = 40;
	static	Longword	ms = 1L;

	if( ! fudge-- ) {
		fudge = 40;
		++ms;
	}

	return ms;
#endif		/* KAYPRO */
}

/* end of main.c */

