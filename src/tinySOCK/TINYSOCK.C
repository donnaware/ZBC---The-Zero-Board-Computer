/* ----- Tiny SOCK tester -----------------------------------------------------
 * tinysock.c - Tiny SOCK Main Ppogram
 *
 * TinySOCK is a small implementation of Ethernet, IP, UDP, TCP, ARP, ICMP and
 * HTTP protocols. This implementation is writen for the PC and should compile
 * using borand, gcc or microsoft compilers. It is intended to be fairly
 * "portable" in that an attempt was made not to use an compiler specific
 * features. This coude should also be ROM-able as well and with some minor
 * modification should compile an almost any small computer or microcontroller
 * that has an RS232 Interface. This specific implementation uses a RS232 to
 * Ethernet coverter. By modifying SED.C, you can use any MAC/PHY you wish.
 * The implementation is bare-bones and borrowed heavily from TinyTcp.
 *
 * To test out each protocol, just change the defines below, make sure to
 * only set one at a time.
 * ------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <stdlib.h>
#pragma hdrstop
#include "tinysock.h"

/* ----- Test Demo Selection ----------------------------------------------- */
/* Select 1 item to change from 0 to 1 in order to demo a particular portion */
/* TinySOCK. C is a client (it is originaiting a message and S is the server */
/* it is waiting for a message to come in to handle                          */
/* ------------------------------------------------------------------------- */
#define TEST_C_UDP  0  /* Test UDP,  listener program (e.g wireshark)        */
#define TEST_S_UDP  1  /* Test UDP,  need sender program                     */
#define TEST_C_ICMP 0  /* Test ICMP, do a regular ping                       */
#define TEST_S_ICMP 0  /* Test ICMP, wait for a regular ping                 */
#define TEST_C_TCP  0  /* Test TCP, listener program (e.g wireshark)         */
#define TEST_S_TCP  0  /* Test TCP, need matching PC program                 */
#define TEST_C_FTP  0  /* Test FTP, Use any FTP server                       */
#define TEST_S_HTTP 0  /* Test HTTP, Use any Browser to test this server     */
/* ------------------------------------------------------------------------- */

/* ----- my IP address ----------------------------------------------------- */
IP_Address local_IP_address;    /* my IP address, static no DHCP             */


#if TEST_C_UDP
/* ----- UDP Client Test Demo ---------------------------------------------- */
/* This demo shows how to transmit a test message to the far end. It is      */
/* assumed that the other end has a listener program of some sort listening  */
/* to this port (you can always use wireshark of course for testing).        */
/* ------------------------------------------------------------------------- */
void TinySOCK(void)
{
    char  test_message[] = "Test Message 123";
    IP_Address dest_IP_address  = HOST_ADDR;   /* This is the Host address */
    local_IP_address = MY_ADDR;     /* This is my IP Address    */

	printf("Tiny UDP Test Trasnmit:\n");
	sed_Init();			/* init ethernet driver */
    if(do_arp(dest_IP_address)) printf("arp success\n");    /* let's try to get the destination MAC address */
    else                        printf("arp failure, using broadcast mac\n");
    do {
    	printf("Sending Test Message...\n");
        udp_send(dest_IP_address, 1024, test_message, sizeof(test_message));
        printf("Message sent, hit q to quit, Entr to send another message\n");
    } while(getch() != 'q');

	printf("Hit any key to continue...\n"); getch();
	sed_Deinit();			/* deinit the interface */
}
/* ---- end of udp client test --------------------------------------------- */
#endif


#if TEST_S_UDP
/* ----- UDP Server Test Demo ---------------------------------------------- */
/* This program tests out the UDP receive functionality. Here we listen for  */
/* an arp message and respond to that. That means someone is looking us up.  */
/* once we supply our MAC/IP pairing, then they can send us a message and    */
/* we will just print that on the screen.                                    */
/* ------------------------------------------------------------------------- */
void TinySOCK(void)
{
    Byte *p, udp_msg[80];
    int i, udp_len;
    char quit = ' ';
    Word portno = 1024;

	printf("Tiny UDP Test Receive:\n");
    printf("Waiting for Message hit q to quit\n");
	sed_Init();			/* init ethernet driver */
    local_IP_address  = MY_ADDR;
    do {
		p = sed_IsPacket();
		if(p) {
		   	if(sed_CheckPacket(Protocol_IP)) {   /* I only want IP messages */
                udp_len = udp_receive((struct udp_packet *)p, portno, udp_msg, sizeof(udp_msg));
                if(udp_len) {                          /* received our message */
                   	printf("Dumping received UDP Message %d bytes:\n", udp_len);
                    for(i=0; i<udp_len; i++) printf("%02x ",udp_msg[i]);
                    printf("\n");
                }
			}
			if(sed_CheckPacket(Protocol_ARP)) {   /* ARP message ? */
				if(arp_checkpacket((struct arp_Header *)p)) {
					printf("Responed to an arp.\n");
				}
			}
		}
        if(kbhit()) quit = getch();
    } while(quit != 'q');

	printf("Hit any key to continue...\n"); getch();
	sed_Deinit();			/* deinit the interface */
}
/* ---- end of udp server test --------------------------------------------- */
#endif


#if TEST_S_ICMP
/* ----- ICMP Server Test Demo --------------------------------------------- */
/* This program tests out the ICMP receive functionality. Here we listen for */
/* a ping message and respond to that. Basically just respond to a ping with */
/* and echo request. We do not support a lot of other ICMP stuff             */
/* ------------------------------------------------------------------------- */
void TinySOCK(void)
{
    Byte *p;
    char quit = ' ';
    IP_Address src;

	printf("Tiny ICMP Test Receive:\n");
    printf("Waiting for Message, hit q to quit\n");
	sed_Init();			/* init ethernet driver */
    local_IP_address  = MY_ADDR;

    do {
		p = sed_IsPacket();
		if(p) {
		   	if(sed_CheckPacket(Protocol_IP)) {   /* IP messages ? */
                if(icmp_check((struct icmp_packet *)p)) {
                    src = ((struct in_Header *)p)->source;
                  	printf("Responed to a ping from %d.%d.%d.%d\n", IP_4B(src),IP_3B(src),IP_2B(src),IP_1B(src));
                }
            }
		   	if(sed_CheckPacket(Protocol_ARP)) {   /* ARP message ? */
                if(arp_checkpacket((struct arp_Header *)p)) {
                  	printf("Responed to an arp.\n");
                }
			}
        }
        if(kbhit()) quit = getch();
    } while(quit != 'q');

	sed_Deinit();			/* deinit the interface */
}
/* ---- end of icmp server test demo --------------------------------------- */
#endif


#if TEST_C_ICMP
/* ----- ICMP Client Test Demo --------------------------------------------- */
/* This program tests out the ICMP send functionality. Here we send out a    */
/* a ping message and wait for the response. Basically just do a ping.       */
/* ------------------------------------------------------------------------- */

/* ----- parse IP address string ------------------------------------------- */
IP_Address parse_IP(char *str)
{
    int i,n,l;
    char *t, *s;
    Byte ib[4];
    n = 0;
    t = str;
    s = str;
    l = (int)strlen(str) + 1;
    for(i = 0; i < l; i++) {
        if((*s == 0) || (*s == '.')) {
            *s = 0;
            ib[n] = atoi(t);
            if(n<4) n++;
            s++;
            t = s;
        }
        else s++;
    }
    return(ADDR(ib[0],ib[1],ib[2],ib[3]));
}

/* ----- ping test --------------------------------------------------------- */
void TinySOCK(void)
{
    char ipstr[64], *ips;
    static IP_Address dst;
    int i;

	printf("IP to ping: ");
    dst = parse_IP(gets(ipstr));
	sed_Init();			/* init ethernet driver */
    local_IP_address  = MY_ADDR;
    if(do_arp(dst)) printf("arp success\n");    /* let's try to get the destination MAC address */
    else            printf("arp failure, using broadcast mac\n");
    printf("pinging %d.%d.%d.%d\n", IP_1B(dst),IP_2B(dst),IP_3B(dst),IP_4B(dst));

    for(i = 0; i<4; i++) {
        if(icmp_send(dst, 2000L)) {
            printf("received reply from %d.%d.%d.%d\n", IP_1B(dst),IP_2B(dst),IP_3B(dst),IP_4B(dst));
            break;
        }
        else {
            printf("no reply on attempt %d\n",i+1);
        }
    }
	printf("Hit any key to continue...\n"); getch();
	sed_Deinit();			/* deinit the interface */
}
/* ---- end of icmp server test demo --------------------------------------- */
#endif



/* ----- TCP Client Test Demo ---------------------------------------------- */
/* This program demonstrates how to send a message using TCP. You will need  */
/* TCP listener program on the other end or something like TCP CHAT          */
/* ------------------------------------------------------------------------- */
#if TEST_C_TCP
static struct tcp_Socket s_data; 	    /* data socket */
static IP_Address host;

/* ----- control handler --------------------------------------------------- */
void msg_Handler(struct tcp_Socket 	*s, Byte *dp, int len)
{
	printf("in msg_Handler: %08lx %08lx %d\n", s, dp, len ); /* just show it worked */
}

/* ----- my application - called by TCP when there's nothing to do -------- */
void my_application(void)
{
    int   n;
    Byte  test_message[] = "Test Message 123";

    if(kbhit()) {
        switch(getch()) {
            case 'q':
            	printf("quitting...\n");
            	sed_Deinit();			/* deinit the interface */
                exit(0);
                break;
            case 's':
            	printf("Sending Test Messages\n");
            	n = tcp_Write(&s_data, test_message, strlen(test_message));
                printf("bytes sent %d\n",n);
                break;
            case 'o':
            	printf("Opening connection\n");
                tcp_Open(&s_data,			/* socket           */
		    	    1025,				    /* my data port     */
    		    	host,				    /* host to call     */
	    		    1025,				    /* his port         */
    		    	(Procref) msg_Handler   /* message handler  */
                );
                break;
            case 'c':
            	printf("Closing connection\n");
                tcp_Close(&s_data);
                break;

            default:
                printf("Commands:\n"
                       "  o   - opens connection\n"
                       "  c   - closes connection\n"
                       "  s   - send test message\n"
                       "  q   - quits tcp test program\n"
                       "> ");
                break;
        }
    }
}

/* ----- tcp tester -------------------------------------------------------- */
void TinySOCK(void)
{
	printf("Tiny TCP Client Test Program:\n> ");
	sed_Init();					    /* init ethernet driver */
    local_IP_address  = MY_ADDR;
    tcp_Init();         		    /* Initialize TCP  */
    host = (IP_Address) HOST_ADDR;
	tcp(my_application);
}
/* ---- end of TinySOCK.c -------------------------------------------------- */
#endif



#if TEST_C_FTP
/* ----- ftp tester -------------------------------------------------------- */
void TinySOCK(void)
{
	/* --- initialization --- */
	sed_Init();			/* init ethernet driver */
	sed_Init();			/* init ethernet driver */
	tcp_Init();			/* init TCP/IP */

	/* --- file transfer --- */
	printf("starting ftp, (@h for help)\n");
	ftp(ADDR(192,16,1,4));		/* IP addr of host to call , 192.9.201.2 tritium */

	/* if the filename is passed, the file is retrieved. */

	/* --- deinitialization --- */
	sed_Deinit();			/* deinit the interface */
	printf("exiting ftp...\n" );
}

/* ---- end of TinySOCK.c -------------------------------------------------- */
#endif



/* ---- end of Main.c ------------------------------------------------------ */

