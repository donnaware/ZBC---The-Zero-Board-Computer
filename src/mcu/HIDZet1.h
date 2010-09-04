//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  FPGAEtchey1.c --  PIC FPGAEtchey                                         
// DonnaWare International LLP Copyright (2001) All Rights Reserved        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <18F2550.h>
//------------------------------------------------------------------------------
// Compile Switches
//------------------------------------------------------------------------------
#define DEBUGON     1   // Set to 1 to enable debugging functions
//------------------------------------------------------------------------------
#fuses HSPLL,USBDIV,PLL5,CPUDIV2,VREGEN,NOFCMEN,NOIESO,PUT,NOBROWNOUT,NOWDT,NOPROTECT,NOLVP,NODEBUG,NOPBADEN,MCLR,NOWRTD
//------------------------------------------------------------------------------
#use delay(clock=48000000)  //~~~ 20MHZ OSCILLATOR CONFIGS ~~~ FULL SPEED

//------------------------------------------------------------------------------
//  Use fast I/O for all Ports
//------------------------------------------------------------------------------
#use fast_io(A)     // Use Fast I/O for Port A
#use fast_io(B)     // Use Fast I/O for Port B
#use fast_io(C)     // Use Fast I/O for Port B

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// CCS Library dynamic defines.  For dynamic configuration of the CCS Library
// for your application several defines need to be made.  See the comments
// at usb.h for more information
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//Tells the CCS PIC USB firmware to include HID handling code.
//------------------------------------------------------------------------------
#define USB_USE_FULL_SPEED          TRUE 
#define USB_HID_DEVICE              TRUE
#define USB_REPORT_SIZE_RX          64  
#define USB_REPORT_SIZE_TX          USB_REPORT_SIZE_RX 
#define USB_MAX_EP0_PACKET_LENGTH   64

//------------------------------------------------------------------------------
// the following defines needed for the CCS USB PIC driver to enable the TX endpoint 1
// and allocate buffer space on the peripheral
//------------------------------------------------------------------------------
#define USB_EP1_TX_ENABLE  USB_ENABLE_INTERRUPT   //turn on EP1 for IN bulk/interrupt transfers
#define USB_EP1_TX_SIZE    USB_REPORT_SIZE_TX     //allocate bytes in the hardware for transmission

//------------------------------------------------------------------------------
//the following defines needed for the CCS USB PIC driver to enable the RX endpoint 1
// and allocate buffer space on the peripheral
//------------------------------------------------------------------------------
#define USB_EP1_RX_ENABLE  USB_ENABLE_INTERRUPT   //turn on EP1 for OUT bulk/interrupt transfers
#define USB_EP1_RX_SIZE    USB_REPORT_SIZE_RX     //allocate bytes in the hardware for reception

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Include the CCS USB Libraries. 
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <pic18_usb_v2.h>       // Microchip PIC18Fxx5x hardware layer for usb.c
#include <USBdescHIDTest.h>     // USB Configuration and Device descriptors
#include <usb.c>                // handles usb setup tokens and get descriptor reports
#include <string.h>

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Summary of pin definition (Pins may have multiple functions):
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define Spare_A0     PIN_A0          // Spare Pin
#define Spare_A1     PIN_A1          // Spare Pin
#define AUX2_Rx      PIN_A2          // AUX - RS232 port Receive
#define Spare_A2     PIN_A3          // Spare Pin
#define TestLED      PIN_A4          // Test LED (Active high)

#define MC_SSEL      PIN_A5          // SPI INput - FPGA to PIC Enable

#define FLASH_DI     PIN_B0          // PIC Out, Flash In  -> Pin5
#define FLASH_CLOCK  PIN_B1          // PIC Out, Flash Clk ->Pin6
#define FLASH_SELECT PIN_B2          // PIC Out, Flash /CS ->Pin1
#define FLASH_DO     PIN_C7          // PIC In,  Flash Out -> Pin2

#define Spare_B3     PIN_B3          // Spare Pin

#define FPGAReset    PIN_B4          // FPGA Reset
#define FPGALoad     PIN_B5          // FPGA Serial upload nConfig Line
#define FPGADOut     PIN_B6          // FPGA Serial upload data line
#define FPGAClock    PIN_B7          // FPGA Serial upload clock line

#define RTC_SCLK     PIN_C0          // RTC SCLK Line
#define RTC_SIO      PIN_C1          // RTC SIO Line
#define RTC_RST      PIN_C2          // RTC /RST Line

#define USBVCC       PIN_C3          // Reserved for USB VCC
#define USB_M        PIN_C4          // USB Negative
#define USB_P        PIN_C5          // USB Positive

#define AUX2_Tx      PIN_C6          // AUX - RS232 port Transmit

#define MC_MISO      PIN_C7          // SPI INput - FPGA to PIC 
#define MC_MOSI      PIN_B0          // SPI INput - PIC to FPGA 
#define MC_CLOCK     PIN_B1          // SPI INput - FPGA to PIC Clock
#define FLOPPY_SEL   PIN_B7          // PIC to FPGA Floppy select pin

//                      76543210     // 
#define TRISA_Enabled 0b00101111     // All PIC SPI Enabled
#define TRISB_Master  0b00001000     // PIC is Master SPI to Flash
#define TRISB_Config  0b00001111     // PIC is set up to configure the FPGA
#define TRISB_Disable 0b00001111     // All PIC SPI Disabled 
#define TRISC_Master  0b10001000     // PIC is Master SPI to Flash
#define TRISC_Slave   0b00001000     // PIC is Slave  SPI to FPGA
#define TRISC_Disable 0b10001000     // All PIC SPI Disabled

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Global definitions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define LED_ON      output_high     // LCD Enable 
#define LED_OFF     output_low      // Turn LED Off
#define blksize     USB_REPORT_SIZE_RX      // Block size for Flash Functions

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SPI Buffer window:
// This buffer contains information for the ZBC to read from as needed. The 
// memory map for this window is as follows:
//
//    Address   Description
// -----------  ----------------------------------------------------------------
// 0x00 - 0x07  Date and Time 
//        0x08  Control, version
//        0x09  Floppy boot up control
// 0x0A - 0x0F  Reserved
// 0x10 - 0x1F  IO Window for transfer of data to and from PC to ZBC
// 
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
short spi_enabled;          // Flag to indicate if PIC to ZBC SPI enabled
short spi_write;            // Flag to indicate next SPI byte is data
int   spi_buffer[32];       // 16 byte buffer for SPI message from FPGA

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Include Drivers                                                           
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include "SPIFPGA.h"            // Include FPGA routines
#include "SST25V.h"             // Flash Memory Driver
#include "DS1302.h"             // Real Time Clock

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
//  F u n c t i o n     P r o t o t y p e s:
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  E E P R O M    C o n t r o l     F u n c t i o n s
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// EEPROM Memory Map :
// PIC EEPROM is non-volital memory used store various parameters and 
// prescriptions for the boot up and control of ZBC.
//------------------------------------------------------------------------------
//   Start    End  Size Description
// ------- ------- ---- ------------------------------------------------------
//    0x00    0x02    3 Start address of Bios File
//    0x03    0x05    3 End   address of Bios File
//    0x06    0x08    3 Start address of Floppy File
//    0x09    0x0B    3 End   address of Floppy File
//    0x0C    0x0E    3 Start address of RBF File
//    0x0F    0x11    3 End   address of RBF File
//
//    0x12    0x12    1 Boot type, 0= No boot (debug), 1=HD, 2= Floppy, 
//------------------------------------------------------------------------------
#define S_ADDR_BIOS   0x00      // Start address of Bios File 
#define E_ADDR_BIOS   0x03      // End   address of Bios File
#define S_ADDR_FLOPPY 0x06      // Start address of Floppy File
#define E_ADDR_FLOPPY 0x09      // End   address of Floppy File
#define S_ADDR_RBF    0x0C      // Start address of RBF File
#define E_ADDR_RBF    0x0F      // End   address of RBF File 
#define BOOT_TYPE     0x12      // Boot type indicator

//------------------------------------------------------------------------------
void Get_EEPROM(int address)
{
    int Buffer[blksize];          // Buffer for data 
    Buffer[0] = read_eeprom(address);
    Buffer[1] = 'E';
    usb_put_packet(1, Buffer, blksize ,USB_DTS_TOGGLE);
}
//------------------------------------------------------------------------------
// Get a 24 bit parm from EEPROM
//------------------------------------------------------------------------------
int32 get_ee_24(int address)   
{
    return(make32(0,read_eeprom(address),read_eeprom(address+1),read_eeprom(address+2)));
}

//------------------------------------------------------------------------------
//  Upload FPGA Firware 
//  Un-comment these lines and set them according to your application.
//
// #define FPGAClock   PIN_xx      // FPGA Serial upload clock line
// #define FPGADOut    PIN_xx      // FPGA Serial upload data line
// #define FPGALoad    PIN_xx      // FPGA Serial upload select line
// #define FPGAReset   PIN_xx      // Reset
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Upload FPGA Firware 
//------------------------------------------------------------------------------
#define USEASM      1               // Use Assembly for speed
#define IOPORT      0xF81           // IO Port, A=F80, B=F81, C=F82, D=F83, E=F84
#define FPGAce      5               // FPGA Config line LO=reset HI=active
#define FPGAsio     6               // FPGA I/O data ( pin 76)
#define FPGAclk     7               // FPGA clock ( pin  75)
#define FPGA_SIO    IOPORT,FPGAsio  // FPGA SIO line LO=reset HI=active
#define FPGA_CLK    IOPORT,FPGAclk  // FPGA clock line
//--------------------------------------------------------------------------
void LoadFPGAByte(byte Sdata)
{
#if USEASM
    int8  Rbit, Rdata;
    #asm
            Movf    Sdata,W         // Store it for later
            Movwf   Rdata           // Tempstore data
            Movlw   0x08            // Number of bits to send
            Movwf   Rbit            // Save it
    WBit:   Btfss   Rdata,0         // Check next bit
            Bcf     FPGA_SIO        // If clear send a zero
            Btfsc   Rdata,0         // Check next bit
            Bsf     FPGA_SIO        // If set send a one
            Bsf     FPGA_CLK        // Clock pin high
            Rrcf    Rdata,F         // Rotate for next bit
            Bcf     FPGA_CLK        // Raise clock pin
            Decfsz  Rbit,F          // Decrement bit counter
            Goto    WBit            // Do next bit
    #endasm
#else
    int8 i;
    for(i=0; i<8; ++i) {
        output_bit(FPGADOut, shift_right(&SData,1,0));  // Send a data bit
        output_low(FPGAClock);
        output_high(FPGAClock);                         // Pulse the clock
    }
#endif
}
//------------------------------------------------------------------------------


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//  S T    F l a s h    F u n c t i  o n s                                     
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//---------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Flash RAM Memory Map 32Mb (4Mbyte) Chip:
//------------------------------------------------------------------------------
// REMEMBER - Actual Virtual Floppy File size = 1,474,560 = 0x16_8000
//
//     Start      End      Start       End      File       Hex   64k
//   Address   Address   Address   Address      Size      Size Blcks Description
// --------- --------- --------- --------- --------- --------- ----- -----------
//         0   131,071 0x00_0000 0x01_FFFF   131,071 0x02_0000   2.0 BIOS ROM
//   131,072 1,605,631 0x02_0000 0x18_7FFF 1,474,560 0x16_8000  22.5 Floppy
// 1,605,632 1,638,399 0x18_8000 0x18_FFFF    32,768 0x00_8000    .5 Round to 64k block
// 1,571,072 2,097,151 0x19_0000 0x1F_FFFF   458,752 0x07_0000   7.0 RBF, actual size varies
//
// 2,097,152 2,228,223 0x20_0000 0x21_FFFF   131,071 0x02_0000   2.0 BIOS ROM#2
// 2,228,223 3,702,783 0x22_0000 0x38_7FFF 1,474,560 0x16_8000  22.5 Floppy #2
// 3,702,784 3,735,551 0x38_8000 0x38_FFFF    32,768 0x00_8000    .5 Round to 64k block
// 3,735,552 4,097,151 0x39_0000 0x3F_FFFF   458,752 0x07_0000   7.0 RBF, actual size varies
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


//--------------------------------------------------------------------------
// Initialize ST Flash RAM with retries, 
// tries = number of times to retry befor giving up
//--------------------------------------------------------------------------
short Try_Init_Flash(int tries)
{
    short ret = 1;
    do {
        Init_STFlash();                 // Initialize ST Card
        if(STFlash_readStatus() == 0) { // Check Status of ST FLASH
            ret = 0;
            break;
        }
    } while(tries--);
    return(ret);
}

//--------------------------------------------------------------------------
// Initialize ST Flash RAM
//--------------------------------------------------------------------------
void Init_Flash(void)
{
    int   Buffer[blksize];          // Buffer for data 
    Buffer[0] = Try_Init_Flash(3); 
    Buffer[1] = Buffer[0]; 
    Buffer[2] = 'I'; 
    usb_put_packet(1, Buffer, blksize ,USB_DTS_TOGGLE);
}

//--------------------------------------------------------------------------
//    Get Status Register
//--------------------------------------------------------------------------
void Get_Status(void)
{
    int   Buffer[blksize];          // Buffer for data 
    Buffer[0] = STFlash_readStatus(); 
    Buffer[1] = Buffer[0]; 
    Buffer[2] = 'S'; 
    usb_put_packet(1, Buffer, blksize ,USB_DTS_TOGGLE);
}

//--------------------------------------------------------------------------
//    Get Status Register
//--------------------------------------------------------------------------
void Get_ID(void)
{
    int   Buffer[blksize];          // Buffer for data 
    
    Buffer[0] = 'J';
    output_low(FLASH_SELECT);            // Enable select line
    STFlash_SendByte(0x9F);              // Send JDEC command
    Buffer[1] = STFlash_GetByte();       // Get the ID;
    Buffer[2] = STFlash_GetByte();       // Get the ID; 
    Buffer[3] = STFlash_GetByte();       // Get the ID; 
    output_high(FLASH_SELECT);           // Disable select line
    usb_put_packet(1, Buffer, blksize ,USB_DTS_TOGGLE);
}

//--------------------------------------------------------------------------
//    Read 256 bytes from flash and return on comm line
//--------------------------------------------------------------------------
void Read_Flash(int32 Address) 
{
    int   Buffer[blksize];          // Buffer for data 
    STFlash_ReadBlock(Address, Buffer, blksize);
    usb_put_packet(1, Buffer, blksize ,USB_DTS_TOGGLE);
}
//--------------------------------------------------------------------------
//    Write 64 bytes to Flash
//--------------------------------------------------------------------------
void Write_Flash(int32 Address) 
{
    int   Buffer[blksize];          // Buffer for data 
    while(!usb_kbhit(1)) usb_task();
    usb_get_packet(1, Buffer, blksize);
    STFlash_WriteBlock(Address, Buffer, blksize);
    Buffer[0] = '1';
    Buffer[1] = 'F';
    usb_put_packet(1, Buffer, blksize ,USB_DTS_TOGGLE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// FLASH TO FPGA Upload Functions:
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Upload FPGA Firmware from flash, Stored in FLASH as follows:
//------------------------------------------------------------------------------
//     Start      End      Start       End      File    Actual
//   Address   Address   Address   Address     Space      Size Comment
// --------- --------- --------- --------- --------- --------- -----------------
// 1,571,072 2,097,151 0x18_0000 0x1F_FFFF   524,288    Varies FPGA RBF
//
// example:  actual rbf file size = 218,713 bytes = 0x35659
// start = 0x180000
// end   = 0x180000 + 0x35659 = 1B_56_59  
//----------------------------------------------------------------------------
void FlashToFPGA(void)
{
    int   Data;
    int32 Address, End;
    
    Address = get_ee_24(S_ADDR_RBF);    // Start Address
    End     = get_ee_24(E_ADDR_RBF);    // End Address

    Disable_FGPA_SPI();                 // Disable SPI slave mode
    Set_Tris_B(TRISB_Config);           // turn on output pin
    
    delay_ms(5);                        // Short delay, settling    
    Init_Flash();                       // Now take over, PIC is master of flash

    Output_Low(FPGALoad);               // FPGA Upload pin
    delay_ms(50);                       // 50 ms delay to put FPGA into load mode
    Output_High(FPGALoad);              // FPGA Upload pin
    delay_ms(2);                        // Short delay, FPGA is disabled now

    output_low(FLASH_SELECT);           // Enable select line
    STFlash_sendByte(0x03);                 // Send opcode to read
    STFlash_sendByte(Make8(Address, 2));    // Send address 
    STFlash_sendByte(Make8(Address, 1));    // Send address
    STFlash_sendByte(Make8(Address, 0));    // Send address
    do {
        Data = STFlash_GetByte();
        LoadFPGAByte(Data);        
        Address++;
    } while(Address <= End);
    output_high(FLASH_SELECT);      // Disable select line, we are done reading
    delay_ms(5);                    // Short delay, settling    

    Disable_STFlash();              // Disable Flash, yield to FPGA
    Set_Tris_B(TRISB_Disable);      // turn off output pin
    delay_ms(5);                    // Short delay, settling    
    FGPA_SPI_Init();                // Enable SPI Interface   
}

//------------------------------------------------------------------------------
// Loads RBF from USB to FPGA 
//------------------------------------------------------------------------------
void USBToFPGA(int16 Blks, int Rmdr)
{
    int16 i;
    int8  Buffer[blksize], j, n;

    Set_Tris_B(TRISB_Config);           // turn on output pin
    
    Output_Low(FPGALoad);            // FPGA Upload pin
    delay_ms(50);                    // 50 ms delay to put FPGA into load mode
    Output_High(FPGALoad);           // FPGA Upload pin
    delay_ms(2);                     // Short delay
    for(i = 0; i < Blks; i++) {
        usb_get_packet(1, Buffer, blksize);
        if(i == Blks-1) n = Rmdr;       // Last block
        else            n = blksize;    // regular block
        for(j = 0; j < n; j++) LoadFPGAByte(Buffer[j]);
        while(!usb_kbhit(1)) usb_task();
    }    
    
    Set_Tris_B(TRISB_Disable);      // turn off output pin
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// R T C   Control Routines
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Refresh data from RTC into SPI buffer
//------------------------------------------------------------------------------
void Refresh_RTCSPI(void)
{
    int i; 
    for(i=0; i < 8; i++) spi_buffer[i] = RTCRead(i);  // Update the spi window
}

//------------------------------------------------------------------------------
// Reads all data from RTC and send back on USB
//------------------------------------------------------------------------------
void Read_RTC(void)
{
    int i, Buffer[blksize];          // Buffer for data 
    
    for(i=0; i <32; i++) Buffer[i] = RTCRead(i);
    Buffer[32] = 'R';
    usb_put_packet(1, Buffer, blksize ,USB_DTS_TOGGLE);
}    
    
//------------------------------------------------------------------------------
// Write all data to RTC 
//------------------------------------------------------------------------------
void Write_RTC(int data[])
{
    int i; 
    for(i=0; i <32; i++) RTCWrite(i, data[i+1]);
}
    
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// ZBC FPGA to PIC SPI Interface Section:
// ZBC is the master, PIC is the slave in this case. PIC (this program) waits
// in main loop for a command. When received takes action and then returns the 
// results. The command structure is a 3 bit command followed by a 5 bit address.
// The commands are as follows, first 3 bit are the command, last 5 are the address.
//
//     Command  Description
//    --------  ------------------------
// 0  000xxxxx  No Operation
// 1  001xxxxx  Read from SPI buffer window at specified address
// 2  010xxxxx  Write to SPI buffer window  at specified address
// 3  011xxxxx  Refresh contents of SPI buffer window
// 4  100xxxxx  Reserved for future use
// 5  101xxxxx  Reserved for future use
// 6  110xxxxx  Reserved for future use
// 7  111xxxxx  No Operation
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Handle SPI Request from ZBC:
//------------------------------------------------------------------------------
void Handle_SPI(void)
{
    int data, cmd, addr;

    data = ssp_get();       // Get current request from ZBC and return last
    cmd  = data>>5;
    addr = data & 0x1F;
   
    if(cmd == 1) {         // User requested a read
        ssp_put(spi_buffer[addr]);
        spi_write = false;
    }
    else {
        ssp_put(0xFF); 
    }
    
    if(cmd == 2) {         // User requested a write
        spi_write = true;
    }
    
    if(cmd == 3) Refresh_RTCSPI();  // Refresh data from RTC into SPI buffer

}


//------------------------------------------------------------------------------
// Exchange data between SPI and USB
//------------------------------------------------------------------------------
void FPGA_SPI_Xfer(int data[])
{
    int i, Buffer[blksize];          // Buffer for data 

    for(i=0; i <32; i++) Buffer[i]     = spi_buffer[i]; // from spi buffer to usb
    for(i=0; i <32; i++) spi_buffer[i] = data[i];       // From USB to spi buffer
    Buffer[34] = 'C';
    usb_put_packet(1, Buffer, blksize ,USB_DTS_TOGGLE);
}    

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  U S B  C o n t r o l   F u n c t i o n s
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// When called, displays debugging information over serial to display enumeration 
// and connection states.  Also lights LED2 and LED3 based upon enumeration and connection status.
//------------------------------------------------------------------------------
void usb_debug_task(void) 
{
    static int8 last_connected;
    static int8 last_enumerated;
    int8 new_connected;
    int8 new_enumerated;

    new_connected   = usb_attached();
    new_enumerated  = usb_enumerated();
    last_connected  = new_connected;
    last_enumerated = new_enumerated;
}

//------------------------------------------------------------------------------
// Receives a packet of data.  The protocol was specified in the HID
// report descriptor (see usb_desc_robomouse.h), PC->PIC and is:
//     msg[0] = 0x00   // Report ID
//     msg[1] = var1   // First byte of data
//     msg[2] = var2   // Secnod byte of data
//      .
//      .
//  Valid 
//  Commands  Description
//  --------  ------------------------------------------------------------------
//      0x09  Turn test LED on or off, if var1 = 1, turn on, var1 = 0, turn off
//      0x0B  Set or reset floppy boot option
//      0x0F  Set or clear FPGA load pin and or FPGA Reset Pin
//      0x10  Upload RBF file from USB line, var1, 2 & 3 are the number of bytes
//      0x11  Command to configure FPGA from a file stored in FLASH
//      0x20  Write 1 byte to EEPROM, var1 is address and var2 is the data
//      0x21  Read 1 byte from EEPROM, var1 is address, data returned in USB report
//      0x90  Initialize Flash RAM (Makes PIC the SPI master)
//      0x91  Returns status of Flash RAM in a USB report
//      0x92  Erase a 64K block from Flash, var1, 2 & 3 make the address  
//      0x93  Read a 64 byte block from Flash, var1,2&3 address, data returned USB
//      0x94  Write 64 bytes from USB, var1,2&3 address, data in next report
//      0x95  Write to Flash Status register, var1 is value to write
//      0x96  Get the Flash Chip ID return in USB report
//      0x9F  Diables the Flash, makes PIC an SPI Slave
//      0xA1  Read 32 bytes from RTC, var1 is address, return 32 bytes data in USB report 
//      0xA2  Write 32 byte to RTC, var1 is address, var2 on is data
//      0xA3  Write 1 byte to RTC, var1 is address, var2 is data
//      0xB1  FPGA data transfer, 
//      0xB2  FPGA SPI enabled if var=1, elase disabled 
//
//------------------------------------------------------------------------------
void usb_rcvdata_task(void) 
{
    int data[blksize];
    if(usb_kbhit(1)) {
        usb_get_packet(1, data, 40);  
        switch(data[0]) {

            //------------------------------------------------------------------
            // Basic Functions
            //------------------------------------------------------------------
            case 0x09: if(Bit_Test(data[1],0)) LED_OFF(TestLED);
                       else                    LED_ON(TestLED);
                       break;
                       
            case 0x0B: if(Bit_Test(data[1],0)) Output_Low (FLOPPY_SEL);
                       else                    Output_High(FLOPPY_SEL);
                       break;

            case 0x0F: if(Bit_Test(data[1],0)) Output_Low(FPGALoad);  // FPGA Upload pin
                       else                    Output_High(FPGALoad); // FPGA Upload pin
                       if(Bit_Test(data[1],1)) Output_Low(FPGAReset);  // FPGA Reset pin
                       else                    Output_High(FPGAReset); // FPGA Reset pin                       
                       break;

            case 0x10: USBToFPGA(make16(data[1],data[2]),data[3]);  // Loads USB to FPGA
                       break;

            case 0x11: FlashToFPGA();  // Loads RBF from Flash to FPGA
                       break;

            //------------------------------------------------------------------
            // EE PROM Functions
            //------------------------------------------------------------------
            case 0x20: write_eeprom(data[1], data[2]); // Write a value to EEPROM
                       break; 

            case 0x21: Get_EEPROM(data[1]); // Read a value from EEPROM
                       break;

            //------------------------------------------------------------------
            // ST FLASh RAM Functions
            //------------------------------------------------------------------
            case 0x90: Init_Flash();
                       break; 

            case 0x91: Get_Status();
                       break; 

            case 0x92: STFlash_EraseBlock(Make32(data[1],data[2],data[3],data[4]));
                       break; 
                       
            case 0x93: Read_Flash(Make32(data[1],data[2],data[3],data[4]));
                       break; 

            case 0x94: Write_Flash(Make32(data[1],data[2],data[3],data[4]));
                       break; 

            case 0x95: STFlash_writeStatus(data[1]);
                       break; 

            case 0x96: Get_ID();
                       break; 
                       
            case 0x9F: Disable_STFlash();           // Disable Flash, yield to FPGA
                       break;


            //------------------------------------------------------------------
            // RTC Functions
            //------------------------------------------------------------------
            case 0xA1: Read_RTC();                 // Read all data from RTC
                       break; 

            case 0xA2: Write_RTC(data);            // Write all data to RTC
                       break; 

            case 0xA3: RTCWrite(data[1], data[2]); // Write 1 Byte of data to RTC
                       break; 

            //------------------------------------------------------------------
            // FPGA SPI Functions
            //------------------------------------------------------------------
            case 0xB1: FPGA_SPI_Xfer(data);     // Transfer data over SPI
                       break; 

            case 0xB2: if(data[1]==0x01) FGPA_SPI_Init();   
                       else              Disable_FGPA_SPI();
                       break; 


            default:   break;
        }
    }
}

//------------------------------------------------------------------------------
//  End .h
//------------------------------------------------------------------------------


