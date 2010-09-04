//==============================================================================
//==============================================================================
// SPI  FPGA Controler Routines                                        SPIFPGA.H
//
// This driver controls the FPGA via an SPI interface. These are the low level
// driver routines to access the FPGA functions.
//
// DonnaWare International LLP Copyright (2001) All Rights Reserved        
//==============================================================================
//==============================================================================


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Library for FPGA SPI Interface                              
//
// void FGPA_SPI_Init()  - Initializes the pins that control the device.   
// int  SPI_xferByte()  - Writes and Reads a register from the device                     
//                                                               
//   Pin Assignments:                         
//   SSEL    Device Select  
//   MOSI    Device Data Oout   
//   SCK     Device Clock              
//   MISO    Device Data Input              
//
//   #define DEVICE_SELECT       PIN_xx        // Device /CS
//   #define DEVICE_CLOCK        PIN_xx        // Device Clk
//   #define DEVICE_DO           PIN_xx        // Device to PIC
//   #define DEVICE_DI           PIN_xx        // PIC to Device
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define DEVICE_SELECT   MC_SSEL     // Device /CS
#define DEVICE_CLOCK    MC_CLOCK    // Device Clk
#define DEVICE_DI       MC_MISO     // PIC to Device
#define DEVICE_DO       MC_MOSI     // Device to PIC

//------------------------------------------------------------------------------
//     * * * USER CONFIGURATION Section, set these per Hardware set up * * *
//------------------------------------------------------------------------------
#define UseHWSPI       1  // Set to 1 for HW, 0 for SW, must use correct pins for HW 
#define MCU_SPI_MASTER 0  // 
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// HW SPI SSP Routines                                                             
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#if UseHWSPI
    #byte SSPBUF  = 0x0FC9          // MSSP Receive Buffer/Transmit Register 
    #byte SSPCON  = 0x0FC6          // MSSP CONTROL REGISTER 1 (SPI MODE)
    #byte SSPSTAT = 0x0FC7          // MSSP STATUS REGISTER (SPI MODE)
    #bit  SSPBF   = SSPSTAT.0       // Status Bit, when set, data is ready 
    #bit  SSPSMP  = SSPSTAT.7       // SMP: Sample bit Must be 0 in slave mode
    #bit  SSPWCOL = SSPCON.7        // Collision detect
    #bit  SSPCKP  = SSPCON.4        // Clock Polarity Select bit, 1 = Idle state for clock is a high level

    #define  READ_SSP()     (SSPBUF) 
    #define  SSP_HAS_DATA() (SSPBF) 
    #define  WAIT_FOR_SSP()  while(!SSP_HAS_DATA()) 
    #define  WRITE_SSP(chr)  SSPBUF=(chr)

    #if MCU_SPI_MASTER
        int8 ssp_xfer(int8 data) 
        {
            SSPWCOL = 0;                // Precautionary Measure
            WRITE_SSP(data);
            WAIT_FOR_SSP();
            return(READ_SSP());
        }
    #else
        int8 ssp_xfer(int8 data) 
        {
            int ret;
            ret = READ_SSP();
            WRITE_SSP(data);
            SSPWCOL = 0;                // Precautionary Measure
            return(ret);
        }
    
    #endif
    
    #define ssp_get()        READ_SSP()
    #define ssp_put(data)    WRITE_SSP(data)
    #define ssp_clear()      SSPWCOL = 0;
#endif 

//------------------------------------------------------------------------------
// Purpose:       Initialize the pins that control the flash device.
//                This must be called before any other flash function is used.
// Inputs:        None
// Outputs:       None
// Dependencies:  None
//------------------------------------------------------------------------------
void FGPA_SPI_Init(void)
{
    Set_Tris_B(TRISB_Disable);           // Flash Disabled, turn on output pins
    Set_Tris_C(TRISC_Slave);           // Set up for SPI  
#if UseHWSPI
    setup_spi(SPI_SLAVE | spi_h_to_l);
    SSPCKP = 1;     // Clock Polarity Select bit, 1 = Idle state for clock is a high level
    SSPSMP = 0;     // SMP: Sample bit Must be 0 in slave mode
#else
    output_low(DEVICE_CLOCK);
#endif    
    output_high(DEVICE_SELECT);
    
    spi_enabled = True;             // Enable polling of SPI input     
}

//------------------------------------------------------------------------------
// Purpose:       Disables Flash Functions to yeild to FPGA by tri-stating 
//                FLASH_SELECT Line
// Inputs:        None
// Outputs:       None
// Dependencies:  None
//------------------------------------------------------------------------------
void Disable_FGPA_SPI(void)
{
    setup_spi(spi_ss_disabled);
    Set_Tris_B(TRISB_Disable);     // Flash Disabled, output pins Tristated 
    Set_Tris_C(TRISC_Disable);     // Flash Disabled 
#if UseHWSPI
    setup_spi(SPI_SS_DISABLED);
#endif  

    spi_enabled = False;           // Disables polling of SPI input 
}

//------------------------------------------------------------------------------
// Purpose:       Send data Byte to the flash device
// Inputs:        1) byte of data
// Outputs:       None
// Dependencies:  None
//------------------------------------------------------------------------------
int8 FGPA_SPI_xferByte(int8 outdata)
{
#if UseHWSPI
    return(ssp_xfer(outdata));
#else
   int8 i, indata8;
   
   for(i=0; i<8; ++i) {
      output_bit(DEVICE_DI, shift_left(&outdata,1,0));    // Send a data bit
      output_high(DEVICE_CLOCK);                       // Pulse the clock
      shift_left(&indata8, 1, input(DEVICE_DO));
      output_low(DEVICE_CLOCK);
   }
   return(indata8);   
#endif
}

//------------------------------------------------------------------------------
// Purpose:       Send data Byte to the flash device
// Inputs:        1) byte of data
// Outputs:       None
// Dependencies:  None
//------------------------------------------------------------------------------
void FGPA_SPI_sendByte(int8 data)
{
#if UseHWSPI
    ssp_put(data);
#else
   int8 i;
   
   for(i=0; i<8; ++i) {
      output_bit(DEVICE_DI, shift_left(&data,1,0));    // Send a data bit
      output_high(DEVICE_CLOCK);                       // Pulse the clock
      output_low(DEVICE_CLOCK);
   }
#endif
}

//------------------------------------------------------------------------------
// Purpose:       Get a byte of data from the device. 
// Inputs:        None
// Outputs:       1) A byte of data
// Dependencies:  None
//------------------------------------------------------------------------------
int FGPA_SPI_getByte(void)
{
#if UseHWSPI
    return(ssp_get());
#else
   int8 i, data;

   for(i=0; i<8; ++i) {                       // Get 8 bits of data
      output_high(DEVICE_CLOCK);
      shift_left(&data, 1, input(DEVICE_DO));
      output_low(DEVICE_CLOCK);
   }
   return(data);
#endif
}


//------------------------------------------------------------------------------
//    End .h
//------------------------------------------------------------------------------

