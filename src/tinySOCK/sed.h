/* ----- sed.h ------------------------------------------------------------- */
/* Header file for very simple ethernet driver                               */
/* ------------------------------------------------------------------------- */

#define	en10size    (4*1024)	/* size of interface memory */
#define E10P_MIN	60              /* Minimum Ethernet packet size */
#define	octet		unsigned char	/*  8 bits */

/* ----- Control Definitions ----------------------------------------------- */
/* If COMMDRIVER is set to 1, then the rs232 to ethernet nic is being used   */
/* otherwise it will compile assuming the ZBC NIC interface                  */
/* ------------------------------------------------------------------------- */

#if !COMMDRIVER
/* ----- ZBC NIC IO Port Definitions --------------------------------------- */
#define PORTBASE 0x0360		  // Base Port address
#define TXSTATUS PORTBASE     // Transmit Status  Register
#define TXCONTRL PORTBASE+1   // Transmit Control and MSB Address Register
#define TXADDRSS PORTBASE+2   // Transmit Buffer  LSB Address Register
#define TXBUFFER PORTBASE+3   // Transmit Buffer  Data Register
#define RXSTATUS PORTBASE+4	  // Receive  Status  Register
#define RXCONTRL PORTBASE+5   // Receive  Control and MSB Address Register
#define RXADDRSS PORTBASE+6   // Receive  Buffer  LSB Address Register
#define RXBUFFER PORTBASE+7   // Receive  Buffer  Data Register
/* ------------------------------------------------------------------------- */
#endif

/* ----- timeouts for zbc nic ---------------------------------------------- */
#define RX_TIMEOOUT	100
#define TX_TIMEOOUT 100

/* ----------------------------------------------------------------------------
 * Below are the bit assigments for the Status and Control Registers.
 * The lower 4 bits of the Control Register are the MSB bits of the Buffer.
 *
 *  |7|6|5|4|3|2|1|0|  Transmit Status Register:
 *   | | | | | | | `-- 0       - Not used, reads zero
 *   | | | | | | `---- 0       - Not used, reads zero
 *   | | | | | `------ 0       - Not used, reads zero
 *   | | | | `-------- 0       - Not used, reads zero
 *   | | | `---------- 0       - Not used, reads zero
 *   | | `------------ 0       - Not used, reads zero
 *   | `-------------- FTTX    - if set, frame is being sent but is not done yet
 *   `---------------- FSENT   - if set, a complete Ethernet frame was sent
 *                               reset to 0 if new frame loaded, writting anything to this register clears bit7
 *
 *  |7|6|5|4|3|2|1|0|  Transmit Control Register:
 *   | | | | | | | `-- A08     - Address Bit  8 of buffer
 *   | | | | | | `---- A09     - Address Bit  9 of buffer
 *   | | | | | `------ A10     - Address Bit 10 of buffer
 *   | | | | `-------- 0       - Not used, reads zero
 *   | | | `---------- 0       - Not used, reads zero
 *   | | `------------ 0       - Not used, reads zero
 *   | `-------------- FTINT   - Enable interrupt on completion of frame transmit
 *   `---------------- FTSND   - Send contents of transmit buffer now
 *
 *
 *  |7|6|5|4|3|2|1|0|  Receive Status Register:
 *   | | | | | | | `-- 0       - Not used, reads zero
 *   | | | | | | `---- 0       - Not used, reads zero
 *   | | | | | `------ 0       - Not used, reads zero
 *   | | | | `-------- 0       - Not used, reads zero
 *   | | | `---------- 0       - Not used, reads zero
 *   | | `------------ 0       - Not used, reads zero
 *   | `-------------- FRRX    - Indicates a complete Ethernet frameis being received, but no in yet
 *   `---------------- FRCVD   - Indicates a complete Ethernet frame was, 0 if no frame waiting
 *                               writting anything to this register clears bit7, FRCVD
 *
 *  |7|6|5|4|3|2|1|0|  Receive Control Register
 *   | | | | | | | `-- A08     - Address Bit  8 of buffer
 *   | | | | | | `---- A09     - Address Bit  9 of buffer
 *   | | | | | `------ A10     - Address Bit 10 of buffer
 *   | | | | `-------- 0       - Not used, reads zero
 *   | | | `---------- 0       - Not used, reads zero
 *   | | `------------ 0       - Not used, reads zero
 *   | `-------------- FRINT   - Allow Interrupt to be generated on receipt of new frame
 *   `---------------- FRRCV   - Allow a frame to be receive into buffer, set to 0 to hold while processing
 *
 * ---------------------------------------------------------------------------*/


/* ----- end of sed.h ------------------------------------------------------ */

