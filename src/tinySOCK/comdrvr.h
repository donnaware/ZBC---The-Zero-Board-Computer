//---------------------------------------------------------------------------
// RS232 COM Driver
//---------------------------------------------------------------------------
#ifndef COMDRIVER1H
#define COMDRIVER1H

/* ----- Control Definitions ----------------------------------------------- */
/* If COMMDRIVER is set to 1, then the rs232 to ethernet nic is being used   */
/* otherwise it will compile assuming the ZBC NIC interface                  */
/* ------------------------------------------------------------------------- */
#define COMMDRIVER 	0  /* set this to 1 to use the RS232 NIC                 */

//---------------------------------------------------------------------------
#define  ConfigStr1     "COM1:"     // For sending data
#define  RxBufferSize   4096        //
#define  TxBufferSize   4096        //

#if COMMDRIVER
/* ------------------------------------------------------------------------- */
void OpenCOM(void);
void CloseCOM(void);
int  WriteCOM(unsigned char *p, int len);
int  ReadCOM( unsigned char *p, int len);
#endif

//---------------------------------------------------------------------------
#endif
