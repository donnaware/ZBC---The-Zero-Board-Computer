/* ----- simple HTTP Server ---------------------------------------------------
 *
 * Super simple HTTP Server, host on port 80, Hard Coded Host IP, compiles with
 * borland 4.5. Supports almost nothing, responds to a GET and shows a page.
 *
 * by D. Polehn
 * ------------------------------------------------------------------------- */
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <conio.h>
#pragma hdrstop
#include "tinysock.h"

/* ----- static data ------------------------------------------------------- */
#define webroot ".\\"   /* This is the pathname the HTML files are stored in */


/* ----- get file size ----------------------------------------------------- */
/* this function just finds the size of the requested html file              */
static int get_file_size(int fd)
{
	struct stat stat_struct;     /* fstat stores information in the stat structure about the file or directory */
	if(fstat(fd, &stat_struct) == -1) return(1);
	return (int)stat_struct.st_size;
}

/* ----- Send string ------------------------------------------------------- */
static void send_str(struct tcp_Socket *s, char *msg) /* This function just sends a canned string to the client */
{
	if(tcp_Write(s, msg, strlen(msg)) == 0) printf("Error sending\n");
}

/* ----- Receive new data -------------------------------------------------- */
/* This function recieves the buffer untill a "End of line" byte is recieved */
static int recv_new(struct tcp_Socket *s)
{
	#define EOL "\r\n"  	/* this the end of line indicator for HTTP */
	#define EOL_SIZE 2

	char *p = s->data ;		/* we'll be using a pointer to the buffer than to mess with buffer directly */
	int eol_matched = 0;    /* this is used to see that the recieved byte matched the buffer byte or not */

	while(s->dataSize ) {		    /* start recv bytes 1 by 1 */
		if(*p == EOL[eol_matched]) {	/* if the byte matched the first eol byte that is '\r' */
			++eol_matched;
			if(eol_matched == EOL_SIZE) {	/* if both the bytes matches the EOL */
				*(p+1 - EOL_SIZE) = '\0';	/* End the string */
				return(strlen(p));	  /* return the bytes recieved */
			}
		}
		else eol_matched = 0;
		p++;					/* increment the pointer to recv next byte */
	}
	return(0);  	/* return with sucess */
}

/* ----- process connection ------------------------------------------------ */
/* this function is called when a client request is received                 */
void http_connect( s, dp, len )
	struct tcp_Socket 	*s;
	Byte 				*dp;
	int					len;
{
    char *request, resource[512], *ptr;
	int fd1, length;

    request = dp;
	if(len < 32) printf("not HTTP message\n");
	else {                       /* Checking for a valid browser request */
        ptr = strstr(request," HTTP/");
	    if(ptr == NULL) printf("Not an HTTP message.\n");
        else if(strncmp(request,"GET ", 4) != 0) {
            request[32] = 0; printf("Not an HTTP GET Request> %s\n", request);
        }
		else {


    		*ptr = 0;    		/* string terminate the request line */
            ptr = request + 4;
			if(ptr[strlen(ptr) - 1] == '/' ) {  /* user wants the default page */
                ptr  = "index.html";
            }
			strcpy(resource, webroot);   /* make the full path name for the file */
			strcat(resource, ptr);
			fd1 = open(resource,O_RDONLY,0);  /* open the requested file */
			if(fd1 == -1) {                   /* on error, say this junk */
				printf("404 File not found Error\n");
				send_str(s,"HTTP/1.0 404 Not Found\r\n");
				send_str(s,"Server : zbc/Private\r\n\r\n");
				send_str(s,"<html><head><title>404 not found error!! :( </head></title>");
				send_str(s,"<body><h1>Url not found</h1><br><p>Sorry user the url you were searching for was not found on this server!!</p><br><br><br><h1>ZBC Server</h1></body></html>");
			}
			else {                               /* if file opened OK */
                printf("Opened \"%s\"\n",resource);
				printf("200 OK!!!\n");             
				send_str(s, "HTTP/1.0 200 OK\r\n");      /* Send HTTP 200 message */
				send_str(s, "Server : zbc/Private\r\n\r\n");
 				if((length = get_file_size(fd1)) == -1 ) printf("Error getting size \n");
                else                                     printf("File size = %d\n", length);
 				if((ptr = (char *)malloc(length)) == NULL ) printf("Error allocating memory!!\n");
 				read(fd1, ptr, length);                  /* read the file into memory */
				if(tcp_Write(s, ptr, strlen(ptr)) == 0) printf("Send err!!\n");  /* send it to the client */
				free(ptr);
			}
			close(fd1);
		}
	}

	tcp_Flush(s);		/* flush out queue */
}

/* ----- idle application - called by TCP when there's nothing to do ------ */
void idle_application( void )
{
	if(kbhit()) {
		if(getch() == 'q') exit(0);		/* get out quick! */
	}
}

/* ----- Receive new data -------------------------------------------------- */
/* Main HTTP Program loop, just call this from main                          */
void http(void)
{
	int i, err;

	struct tcp_Socket s_http; 		/* http socket 		*/
   	static	IP_Address	host;
    Word Port = 80;					/* http port number	*/

	printf("Tiny HTTP Server Program:\n");

	sed_Init();					/* init ethernet driver */
    tcp_Init();         		/* Initialize TCP  */

    host = ( IP_Address ) MY_ADDR;  /* I am the host in this app */

	/* register to listen for TCP messages */
	tcp_Listen(&s_http,					/* socket 			*/
		Port,					   		/* http port 		*/
		(Procref) http_connect,			/* handler 			*/
		0L );							/* timeout = forever */

	printf("Server is open for listening on port 80\n");
	printf("Hit control C to stop the server...\n");
	tcp(idle_application);
    
	tcp_Close(&s_http);     /* close down tcp  */
	sed_Deinit();			/* deinit the interface */
}

/* ----- End of TinyHTTP.c ------------------------------------------------- */


