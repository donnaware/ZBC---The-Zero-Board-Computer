/* ------------------------------------------------------------------------- */
/* RS232 COM Driver - Super simple RS232 driver                              */
/* ------------------------------------------------------------------------- */
#include <stdio.h>
#include <windows.h>
#pragma hdrstop
#include "comdrvr.h"

#define DEBUG_COMM_RX   0
#define DEBUG_COMM_TX   0

/* ------------------------------------------------------------------------- */
static HANDLE COMFile1;
static DWORD  result;

#if COMMDRIVER
/* ------------------------------------------------------------------------- */
void OpenCOM(void)
{
    DCB DCBvar;
    COMMTIMEOUTS varCommTimeouts;
    COMFile1 = INVALID_HANDLE_VALUE;

    COMFile1 = CreateFile(ConfigStr1, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(COMFile1 == INVALID_HANDLE_VALUE) {
        printf("Cannot open port %s\n",ConfigStr1);
    }
    if(!SetupComm(COMFile1, RxBufferSize, TxBufferSize)) {
        printf("Unable to set the COM buffer sizes");
    }
    if(!GetCommState(COMFile1, &DCBvar)) {
        printf("Unable to GetCommState");
    }
    DCBvar.BaudRate = 115200;
    DCBvar.fDtrControl  = DTR_CONTROL_DISABLE;
    DCBvar.fRtsControl  = RTS_CONTROL_DISABLE;
    DCBvar.ByteSize = 8;
    DCBvar.Parity   = NOPARITY;
    DCBvar.StopBits = 0;
    if(!SetCommState(COMFile1, &DCBvar)) {
        printf("Unable to SetCommState");
    }
    varCommTimeouts.ReadIntervalTimeout         = 0;
    varCommTimeouts.ReadTotalTimeoutMultiplier  = 0;
    varCommTimeouts.ReadTotalTimeoutConstant    = 100;
    varCommTimeouts.WriteTotalTimeoutMultiplier = 0;
    varCommTimeouts.WriteTotalTimeoutConstant   = 0;
    SetCommTimeouts(COMFile1, &varCommTimeouts);
}
//---------------------------------------------------------------------------
void CloseCOM(void)
{
    if(COMFile1 != INVALID_HANDLE_VALUE) {
        CloseHandle(COMFile1);
        COMFile1 = INVALID_HANDLE_VALUE;
    }
}
//---------------------------------------------------------------------------
int  WriteCOM(unsigned char *p, int len)
{
    #if DEBUG_COMM_TX
    	int i;
	    printf("Dumping Frame content:\n");
        for(i=0; i<len; i++) printf("%02x ",p[i]);
        printf("\n");
    //  return(1);
    #endif
    if(COMFile1 == INVALID_HANDLE_VALUE) return(-1);
    return(WriteFile(COMFile1, p, len, &result, NULL));
}

//---------------------------------------------------------------------------
int  ReadCOM(unsigned char *p, int len)
{
#if DEBUG_COMM_RX
   	int i;
#endif
	int ret;
	unsigned short num;

	if(COMFile1 == INVALID_HANDLE_VALUE) return(0);
	ret = ReadFile(COMFile1, p, len, (LPDWORD)&num, NULL);
	if(num > 0) {
		#if DEBUG_COMM_RX
			printf("Dumping received content:\n");
			for(i=0; i<num; i++) printf("%02x ",p[i]);
			printf("\n");
		#endif
		return(ret);
	}
	else return(0);
}

#endif
/* ------------------------------------------------------------------------- */

