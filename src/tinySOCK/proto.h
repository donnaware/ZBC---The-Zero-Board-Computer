/* ----- proto.h ----------------------------------------------------------- */
/* PROTO.H - function prototypes for TinySOCK 	                             */
/* ------------------------------------------------------------------------- */

/* ----- in arp.c  --------------------------------------------------------- */
int arp_checkpacket P(( struct arp_Header *ap ));
int sar_MapIn2Eth   P(( Longword ina, struct Ethernet_Address *ethap ));
int do_arp(IP_Address their_IP_address);

/* ----- in sed.c or sedslip.c --------------------------------------------- */
int   sed_Init P(( void ));
int   sed_Deinit P(( void ));
Byte *sed_FormatPacket(Byte *destEAddr, Word ethType);
int   sed_Send P(( int pkLengthInOctets ));
Byte *sed_Receive P(( Word Protocol ));
Byte *sed_IsPacket P(( void ));
int   sed_CheckPacket(Word expectedType);

/* ----- byte order reversal crap ------------------------------------------ */
#ifdef BIG_ENDIAN
#define     rev_word( w )       ((w))
#define     rev_longword( l )   ((l))
#else
Word        swapBytes(Word x);
Longword    swapWords(Longword x);
#define     rev_word( w )       (swapBytes(w))
#define     rev_longword( l )   (swapWords(l))
#endif

/* ----- used all over the place ------------------------------------------- */
#define	Move(s,d,n)	memcpy(d,s,n)  /* redefine move as memcpy for some reason*/

/* ----- in tinyudp.c ------------------------------------------------------ */
Word UDPChecksum16(Longword v);  /* weird UDP checksum fix */
void udp_send(IP_Address dstIP, Word portno, Byte *message, Word payloadlen);
Word udp_receive(struct udp_packet *p, Word portno, Byte *rcv_message, Word maxlen);

/* ----- in icmp.c --------------------------------------------------------- */
int icmp_check(struct icmp_packet *rp);
int icmp_send(IP_Address dst, Longword timeout);


/* ----- in tinytcp.c ------------------------------------------------------ */
void tcp_Init   P(( void ));
void tcp_Open   P(( struct tcp_Socket *s, Word lport, IP_Address ina, Word port, Procref datahandler ));
void tcp_Listen P(( struct tcp_Socket *s, Word port, Procref datahandler, Longword timeout ));
void tcp_Close  P(( struct tcp_Socket *s ));
void tcp_Abort  P(( struct tcp_Socket *s ));
int  tcp        P(( Procrefv application ));

int tcp_Write P(( struct tcp_Socket *s, Byte *dp, int len ));
void tcp_Flush P(( struct tcp_Socket *s ));
void tcp_Send P(( struct tcp_Socket *s ));

/* the following are internal to TCP, and hence, static                      */
static void tcp_Unthread P(( struct tcp_Socket *ds ));
static void tcp_Retransmitter P(( void ));
static void tcp_ProcessData P(( struct tcp_Socket *s, struct tcp_Header *tp, int len ));
static void tcp_DumpHeader P(( struct in_Header *ip, struct tcp_Header *tp, char *mesg ));
static void tcp_Handler P(( struct in_Header *ip ));

Word checksum P(( Word *dp, int length ));
Longword lchecksum P(( Word *dp, int length ));

/* ----- in tinyftp.c ------------------------------------------------------ */
void ftp_ctlHandler P(( struct tcp_Socket *s, Byte *dp, int len ));
void ftp_dataHandler P(( struct tcp_Socket *s, Byte *dp, int len ));
void ftp_commandLine P(( void ));
void ftp_Abort P(( void ));
void ftp_application P(( void ));
void ftp P(( IP_Address host ));
void ftp_server_handler P(( struct tcp_Socket *s, Byte *dp, int len ));
void ftp_local_command P(( char *s ));

/* ----- in anyplace ------------------------------------------------------- */
/* sometimes we need a timer, this is a call to a ms timer                   */
Longword MsecClock P((void));

/* ----- end of proto.h ---------------------------------------------------- */

