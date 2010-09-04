/* ----- FILEIO.C - simple file I/O routines for portability --------------- */

/* ----- include files ----------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#pragma hdrstop
#include "tinysock.h"
#include "fileio.h"

/* ----- internals --------------------------------------------------------- */
static char *fixup_filename P((char *p_filename));

/* ----- my_open ----------------------------------------------------------- */
FH	my_open(char *p_filename, int open_mode)
{
	FILE	*p_file;
	p_filename = fixup_filename( p_filename );
	switch( open_mode ) {
		case MY_OPEN_READ:
			p_file = fopen( p_filename, "rb" );	/* Microsoft */
			return ( FH ) p_file;

		case MY_OPEN_WRITE:
			p_file = fopen( p_filename, "wb" );	/* Microsoft */
			return ( FH ) p_file;

		default:
			return ( FH ) 0;
	}
}

/* ----- close the file ---------------------------------------------------- */
void my_close(FH fh)
{
	fclose((FILE *)fh);
}

/* ----- read bytes of file ------------------------------------------------ */
unsigned short my_read(FH fh, unsigned char *p_buffer, unsigned short u_bytes)
{
	return fread( p_buffer, 1, u_bytes, ( FILE * ) fh );
}

/* ----- write bytes of file ----------------------------------------------- */
unsigned short my_write(FH fh, unsigned char *p_buffer, unsigned short u_bytes)
{
	return fwrite( p_buffer, 1, u_bytes, ( FILE * ) fh );
}

/* ----- internal to fix up filename --------------------------------------- */
static char *fixup_filename(char *p_filename)
{
	char *		p;
	char		c;
	static	char	fixed_filename[ 42 ];
	int		dots;

	/* Here, we just move to the character after the last slash  or forward slash. */
	p = strrchr( p_filename, '/' );
	if( p != ( char * ) 0 ) p_filename = p + 1;

	p = strrchr( p_filename, '\\' );
	if( p != ( char * ) 0 ) p_filename = p + 1;

	/* Force it into 8.3 style, uppercase only, one dot only */
	dots = 0;
	p = &fixed_filename[ 0 ];
	while( *p_filename ) {
		c = *p_filename++;
		if(( c <= ' ' ) || ( c > 126 )) continue;
		if(( c >= 'a' ) && ( c <= 'z' )) c -= ( 'a' - 'A' );
		/* else if( c == '/' ) c = '\\'; */
		else if( c == '.' ) {
			++dots;
			if( dots > 1 ) break;
		}
		*p++ = c;    /* we could check for bad characters too. maybe later */
	}
	return &fixed_filename[ 0 ];
}

/* ----- end of fileio.c --------------------------------------------------- */
