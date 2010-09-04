/* FILEIO.H - definitions for simple file I/O */

#define	MY_OPEN_READ	0
#define	MY_OPEN_WRITE	1

/* watch the syntax on this one */
#define	FH		void *

FH	my_open P(( char *p_filename, int open_mode ));
void	my_close P(( FH fh ));
unsigned short my_read  P(( FH fh, unsigned char *p_buffer, unsigned short u_bytes ));
unsigned short my_write P(( FH fh, unsigned char *p_buffer, unsigned short u_bytes ));

/* end of fileio.h */

