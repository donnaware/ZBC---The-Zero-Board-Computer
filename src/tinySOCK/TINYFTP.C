/* ----- tiny ftp -------------------------------------------------------------
 * tinyftp.c - user ftp built on top of tinytcp.c
 *
 * Notes:
 *	This is the CLIENT side of the FTP connection. Currently it is only capable
 *  of requesting that files be RETRieved. It cannot retrieve more than one file.
 *  The file logic needs beefing up.
 *
 *	Names in this program often appear to have been chosen quite poorly - that is,
 *  they either are unintelligible, or don't reflect the true meaning of what they
 *  represent. I have been changing them slowly, but consider this to be a low priority.
 *
 *	There is something funny about the kbhit stuff. It seems to sometimes wait for
 *  a key even if I haven't pressed one. There is also something wrong with receiving
 *  long multi-line responses such as to HELP, like the data gets overwritten or unterminated.
 *
 * ------------------------------------------------------------------------- */
#include <stdio.h>	/* for printf, etc */
#include <stdlib.h> /* Standard C Calls */
#include <conio.h>	/* for kbhit, etc.  */
#include <string.h>	/* for strcpy, etc. */
#pragma hdrstop
#include "tinysock.h"
#include "fileio.h"

#define	RECEIVE_DATA_PORT	0x1010

#define DEBUG_FTP       0
#define	IMMEDIATE_OPEN  0

/* the following are what these seem to be */
#define	isina		kbhit
#define	busyina		getch
#define busyouta	putch

/* Sockets */
struct tcp_Socket s_og_ctl,	/* outgoing connection socket */
			s_og_data,		/* data socket */
			s_ic_ctl,		/* server control socket */
			s_ic_data;		/* server data socket */

								/* Receive buffer for client side. */
char		b_response[ 120 ];	/* response buffer */
int			i_response;			/* index into response buf */

								/* Client output command buffer */
char		b_c_command[ 82 ];	/* send buffer */

								/* Server output buffer */
char		b_s_response[ 128 ] = "";	/* server output buffer */

								/* Server file transfer buffer, index, and length */
Byte		b_s_data[ 1024 ];	/* server output buffer */
int			n_s_sent,			/* bytes sent of buffer */
			n_s_left;			/* bytes left in serv2 buffer */

char		recv_filename[ 82 ];  	/* file handle for retrieve */
FH			p_recv_file = ( FH ) 0;

char		send_filename[ 82 ];  /* file handle for server */
FH			p_send_file = ( FH ) 0;

/* ----- prototypes -------------------------------------------------------- */
static void ftp_process_response P(( void ));
static char * get_a_number P(( char *ps, unsigned short *pn ));

/* ----- ftp control handler ----------------------------------------------- */
void ftp_ctlHandler(struct tcp_Socket *s, Byte *dp, int len)
{
	static	Byte		c,
						*bp,
						data[82];
	static	int			i;

#if DEBUG_FTP
	printf( "in ftp_ctlHandler %08lx %08lx %d\n", s, dp, len ); /* 22904a58 22900a14 0 */
#endif

	if( dp == 0 ) {
		tcp_Abort( &s_og_data );
		return;
	}

	/* Received a message. What do I do with it? */
	do {
		i = len;
		if( i > sizeof( data )) i = sizeof( data );
		Move( dp, data, i );
		len -= i;

		/* Look at the buffer that was received and try to interpret it as an FTP response */
		bp = data;
		while( i-- > 0 ) {
			c = *bp++;
			if(c == '\r') continue;	/* ignore the cr? */
			if(c == '\n') { 			/* We've received the end of the response. */
				b_response[ i_response ] = '\0';

				/* Process the message received. */
				ftp_process_response();
				i_response = 0;

			}
            else if( i_response < ( sizeof( b_response ) - 1 )) {

				/* Not the end yet, keep storing. */
				b_response[ i_response++ ] = c;
			}
		}

	} while( len > 0 );
}

/* ----- ftp data handler -------------------------------------------------- */
void ftp_dataHandler(struct tcp_Socket *s, Byte *dp, int len)
{
#if DEBUG_FTP
	printf("in ftp_dataHandler %08lx %08lx %d\n", s, dp, len);
#endif
	/* When a file is retr'd, it comes in here. */
 	if(len <= 0) {	            /* 0 = EOF, -1 = close */
		if(p_recv_file != (FH)0) {
			my_close( p_recv_file );
			p_recv_file = (FH)0;
            printf("file saved\n");
		}
		return;
	}

	/* if the file isn't open yet, try to open it */
	if((p_recv_file == (FH)0) && (strlen(recv_filename))) {

		/* open the file for retrieve */
		p_recv_file = my_open(recv_filename, MY_OPEN_WRITE);

		if(p_recv_file == (FH)0) {
			printf("** Can't open file %s **\n", recv_filename );
			recv_filename[0] = '\0';
		}
        else printf("opened file %s for write\n", recv_filename);
	}

	/* write the data to the file */
	if(p_recv_file != (FH)0) {
		my_write(p_recv_file, dp, len);
		dp += len;
		return;
	}

	/* as a last resort, dump it to the screen (Hm, to stdout?) */
	while(len > 0) {
		if(( *dp < 32 ) || ( *dp > 126 )) 	printf( " %02x ", *dp );
		else								putchar( *dp );
		++dp;
		--len;
	}
}

/* ----- process response from distant end ftp ----------------------------- */
static void ftp_process_response(void)
{
	printf( "< %s\n", b_response );
	/* Look at the fourth byte of the response message. If there is a -, there is
       more to come. If there is a blank, this is the end. */
}

/* ----- get a number (unsigned short) ------------------------------------- */
static char *get_a_number(char *ps, unsigned short *pn)
{
	static	unsigned short	us;

	us = 0;
	while( *ps == ' ' ) ++ps;   /* ignore leading blanks */

	while(( *ps >= '0' ) && ( *ps <= '9' )) 	/* convert any digits present */
		us = ( unsigned short )(( 10 * us ) + *ps++ - '0' );

	*pn = us;            	/* store the converted number */
	return ps;      	/* return pointer to next char */
}                  

/* ----- ftp server data handler ------------------------------------------- */
void ftp_server_handler(struct tcp_Socket *s, Byte *dp, int	len)
{
	static	unsigned short	l, h;
	static	char *p;

	static	int	first_time = 1;		
	static	int	his_data_port = RECEIVE_DATA_PORT;

	/* THINK I NEED TO COLLECT DATA UNTIL I RECEIVE AN ENTIRE STRING TERMINATED WITH CR/LF. */

/* #ifdef DEBUG_FTP */
	printf( "in ftp_server_handler. dp %08lx len %d, his port %04x\n",dp, len, s->hisport );
/* #endif */

	if( len < 0 ) {		/* 0 = EOF, -1 = close */
		return; 		/* Close of port. What is there to say? I suppose I need to redo the listen. */
	}
	/* Here's the usual exchange:
		(open)		220 tritium FTP server ready.
		user rickr	331 password required for rickr.
		pass grelber	230 user rickr logged on.
		retr \temp\2	150 opening blah blah
			Data port is opened. Data gets sent.
			Data port is closed.
		quit		221 goodbye. */

	/* When I enter 'get' in FTP on OS/2, he sends me a PORT message... */
	/* His port number is at s -> hisport */
	/* Set up the reply */
	if(len == 0) {
		if(first_time ) {
			strcat( b_s_response, "220 Tiny-TCP FTP server ready.\r\n" );
			first_time = 0;
		} else return;

	} else if(strncmp( ( char * ) dp, "USER", 4 ) == 0 ) {
		strcat( b_s_response, "230 No logon required.\r\n" );

	} else if(strncmp( ( char * ) dp, "PASS", 4 ) == 0 ) {
		strcat( b_s_response, "230 No logon required.\r\n" );

	} else if(strncmp( ( char * ) dp, "PORT", 4 ) == 0 ) {

		/* I get 192,168,1,2,4,7 */
		p = ( char * ) dp + 4;
		p = get_a_number( p, &h );	/* 192 */
		if( *p != ',' ) goto syntax_error;
		++p;
		p = get_a_number( p, &h );	/* 168 */
		if( *p != ',' ) goto syntax_error;
		++p;
		p = get_a_number( p, &h );	/*  1  */
		if( *p != ',' ) goto syntax_error;
		++p;
		p = get_a_number( p, &h );	/*  2 */
		if( *p != ',' ) goto syntax_error;
		++p;
		p = get_a_number( p, &h );	/*  4 */
		if( *p != ',' ) goto syntax_error;
		++p;
		p = get_a_number( p, &l );	/*  7 */

		his_data_port = ( int )(( h << 8 ) + l );

		printf( "His data port set to %04x\n", his_data_port );

		strcat( b_s_response, "200 Okay.\r\n" );

	} else if( strncmp( ( char * ) dp, "RETR", 4 ) == 0 ) {

		if( *( dp + 4 ) != ' ' ) {
syntax_error:
			strcat( b_s_response, "501 Syntax error.\r\n" );
			return;
		}

		strncpy( send_filename, ( char * ) dp + 4, len - 5 );
		send_filename[ len - 5 ] = '\0';

        printf( "retrieving file: %s\n", send_filename );

		/* Open the output file */
		p_send_file = my_open( send_filename, MY_OPEN_READ );
		if( p_send_file == ( FH ) 0 ) {
			strcat( b_s_response, "550 File doesn't exist.\r\n" );
			return;
		}

		/* Open a socket for the send data. */

		tcp_Open( &s_ic_data,		/* socket */
			RECEIVE_DATA_PORT,	/* my port */
			s -> hisaddr,		/* host to call */
			his_data_port,		/* his port */
			( Procref ) ftp_dataHandler );
						/* handler for data receive */

		strcat( b_s_response, "150 File open.\r\n" );

		/* When transfer is complete, need to send 226. */

	/* } else if( strncmp( ( char * ) dp, "STOR", 4 ) == 0 ) { */

		/* STOR is like RETR, but needs to use the recv_filename
			instead. I think I can use the same data handler... */

	} else if( strncmp( ( char * ) dp, "QUIT", 4 ) == 0 ) {
		strcat( b_s_response, "221 Goodbye.\r\n" );

	} else if( strncmp( ( char * ) dp, "HELP", 4 ) == 0 ) {
		strcat( b_s_response, "211 When we say Tiny, we mean it.\r\n" );

	} else if( len > 0 ) {
		/* dump it to the screen (Hm, to stdout?) */

		printf( "** Unrecognized command: " );
		while( len > 0 ) {
			if(( *dp < 32 ) || ( *dp > 126 ))
				printf( " %02x ", *dp );
			else	putchar( ( char ) *dp );
			++dp;
			--len;
		}
		putchar( '\n' );
		strcat( b_s_response, "502 Command not implemented.\r\n" );
	}
}

/* ----- abort ftp --------------------------------------------------------- */
void ftp_Abort(void)
{
	tcp_Abort( &s_og_ctl );
	tcp_Abort( &s_og_data );
}

/* ----- FTP application - called by TCP when there's nothing to do -------- */
void ftp_application(void)
{
	static	int		i, n;
	static	char	userbuffer[ 84 ] = "";

	fflush( stdout );	/* make sure prompt, etc. is visible */
	if(isina()) {            	/* Check the console for a keypress */
		i = busyina() & 0177;

		/* Actually, control-C will be captured by the OS and terminate everything. Use control-X. */
		if( i == ( 'X' & 037 )) {
			printf( "Closing...\n" );
			tcp_Close( &s_og_ctl );
		}
		if(( i >= ' ' ) && ( i <= 126 )) {   	/* have a user character */
			n = strlen( userbuffer );
			if( n < 81 ) {
				userbuffer[ n++ ] = ( char ) i;
				userbuffer[ n ] = '\0';
			}
			busyouta( i );		/* echo the char */
		} else if( i == '\b' ) {
			n = strlen( userbuffer );
			if( n > 0 ) {
				userbuffer[ --n ] = '\0';
				printf( "\b \b" );
			}
		} else if( i == '\022' ) {	/* control-R */
			printf( "\r\nftp> %s", userbuffer );
		} else if( i == '\r' ) {   			/* Carriage return */
			if( strlen( userbuffer )) {
				strcat( userbuffer, "\r\n" );

	/* If it's an internal command, process it locally; else concatenate it to the output buffer */
				if( userbuffer[ 0 ] == '@' ) ftp_local_command( userbuffer );
				else {
					/* See if it is a RETR command */
					if(( strncmp( userbuffer, "RETR ", 5 ) == 0 ) || ( strncmp( userbuffer, "retr ", 5 ) == 0 ))
						strcpy( recv_filename, &userbuffer[ 5 ] );
					strcat( b_c_command, userbuffer );
				}
				userbuffer[ 0 ] = '\0';
			}
			printf( "\r\nftp> " );
		}
	}

	/* send any data on the control port which is waiting to be sent */
	i = strlen(b_c_command);
	if(i) {
		i = tcp_Write(&s_og_ctl,(Byte *) b_c_command, i);

		/* move down the data which remains to be sent */
		if(i) strcpy( &b_c_command[ 0 ], &b_c_command[ i ] );
		tcp_Flush( &s_og_ctl );
	}

	/* Look at the SERVER buffer and send anything that's there */
	i = strlen(b_s_response);
	if(i) {
		i = tcp_Write( &s_ic_ctl,( Byte * ) b_s_response,i );

		/* move down the data which remains to be sent */
		strcpy( &b_s_response[ 0 ], &b_s_response[ i ] );
		tcp_Flush( &s_ic_ctl );
	}

	/* If the server (or the client?) is sending a file, send the file */
	if( p_send_file != ( FH ) 0 ) {
		/* if buffer is empty, read some more data */
		if( n_s_left == 0 ) {
			n_s_left = ( int ) my_read( p_send_file,&b_s_data[ 0 ],sizeof( b_s_data ));
			if( n_s_left == 0 ) {		/* EOF! */
				my_close( p_send_file );
				p_send_file = ( FH ) 0;
				printf( "Closing...\n" );
				tcp_Close( &s_ic_data );
				strcat( b_s_response, "226 File sent.\r\n" );
				goto done_serv2;
 			}
			n_s_sent = 0;
		}

		/* send as much as will fit */
		i = tcp_Write( &s_ic_data,&b_s_data[ n_s_sent ],n_s_left );

		/* if buffer is empty, read some more data */
		n_s_sent += i;
		n_s_left -= i;
		tcp_Flush( &s_ic_data );
	}
done_serv2: ;
}

/* ----- ftp --------------------------------------------------------------- */
void ftp(IP_Address host)
{
	i_response = 0;

	/* Set up listen for FTP server */
	tcp_Listen(&s_ic_ctl,				/* socket */
		21,								/* my port */
		( Procref ) ftp_server_handler,	/* handler */
		0L );							/* timeout = forever */

	/* Set up listen for data coming back. (Do we have to
		do a new Listen everytime this socket gets closed?) */
	tcp_Listen(&s_og_data,				/* socket */
		RECEIVE_DATA_PORT,				/* my port */
		( Procref ) ftp_dataHandler,	/* handler */
		0L );							/* timeout = forever */

#if IMMEDIATE_OPEN          /* If a host IP address was passed, open the other end. */
	if( host != 0L ) {   	/* Open connection for messages and commands going out */
							/* NOTE: This is usually only done when an 'Open' command is issued. */
		tcp_Open( &s_og_ctl,		/* socket */
			RECEIVE_DATA_PORT,		/* my port */
			host,					/* host to call */
			21,								/* his port (hailing freq.) */
			( Procref ) ftp_ctlHandler );	/* handler */
	}
#endif /* IMMEDIATE OPEN */
	tcp(ftp_application);
}

/* ----- process local command -------------------------------------- */
void ftp_local_command(char *s)
{
	static	IP_Address	host;

	if(*s++ != '@') return;
	if(*s   == 'o') {				  /* @open */
		host = (IP_Address)HOST_ADDR; /* IP addr of host to call eg. 192.168.1.4 */

#if !IMMEDIATE_OPEN
		/* Open connection for messages and commands going out */
		tcp_Open(&s_og_ctl,					/* socket */
			RECEIVE_DATA_PORT,				/* my port */
			host,							/* host to call */
			21,								/* his port (hailing freq.) */
			( Procref ) ftp_ctlHandler );	/* handler */
#endif	/* IMMEDIATE_OPEN */

	}
    else if(*s == 'c') {					/* @close */
		if(p_recv_file != ( FH ) 0 ) {
		   my_close( p_recv_file );
		   p_recv_file = ( FH ) 0;
		}
		tcp_Abort(&s_og_ctl );
		tcp_Abort(&s_og_data );

	}
    else if(*s == 'q') {					/* @quit */
		if( p_recv_file != ( FH ) 0 ) {
			my_close( p_recv_file );
			p_recv_file = ( FH ) 0;
		}
		exit(0);		/* get out quick! */
	}
    else if(*s == 'h') {
        printf("ftp Help:\n");
        printf("Local Commands:\n"
               "@c   - closes ftp\n"
               "@o   - opens ftp\n"
               "@q   - quits ftp\n");
        printf("Remote Commands:\n"
               "ABOR - abort a file transfer\n"
               "CWD  - change working directory\n"
               "DELE - delete a remote file\n"
               "LIST - list remote files\n"
               "MDTM - return the modification time of a file\n"
               "MKD  - make a remote directory\n"
               "NLST - name list of remote directory\n"
               "PASS - send password\n"
               "PASV - enter passive mode\n"
               "PORT - open a data port\n"
               "PWD  - print working directory\n"
               "QUIT - terminate the connection\n"
               "RETR - retrieve a remote file\n"
               "RMD  - remove a remote directory\n"
               "RNFR - rename from\n"
               "RNTO - rename to\n"
               "SITE - site-specific commands\n"
               "SIZE - return the size of a file\n"
               "STOR - store a file on the remote host\n"
               "TYPE - set transfer type\n"
               "USER - send username\n"
        );
    }
}

/* ---- end of tinyftp.c --------------------------------------------------- */
