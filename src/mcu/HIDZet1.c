//==============================================================================
//==============================================================================
// ZBC.c -- Zet FPGA interface                                                --
// -----------------------------------------------------------------------------
//  DonnaWare International LLP Copyright (2001) All Rights Reserved          
//==============================================================================
//==============================================================================
#include "HIDZet1.h"        // Main Project Includes
//------------------------------------------------------------------------------
//    Main Function:                                                          
//------------------------------------------------------------------------------
void main()
{
    //--------------------------------------------------------------------------
    //  PIC Initialization Section
    //--------------------------------------------------------------------------
    Disable_Interrupts(GLOBAL);                 // all interrupts OFF
    Setup_adc(ADC_OFF);                         // ADC not needed
    Setup_adc_ports(NO_ANALOGS);                // No ADC ports needed 
    Setup_timer_0(RTCC_INTERNAL|RTCC_DIV_256);  // Timer 0 set for  42.6us, 
    Setup_timer_1(T1_DISABLED);                 // Timer 1  

    Set_Tris_A(TRISA_Enabled);          // SPI Disabled
    Set_Tris_B(TRISB_Disable);          // Flash SPI Disabled 
    Set_Tris_C(TRISC_Disable);          // Flash SPI Disabled

    Output_high(FLASH_SELECT);          // Slash Select High
    Output_high(FLASH_CLOCK);           // Pulse the clock
    Output_low(RTC_RST);                // Disable RTC
    Output_High(FPGALoad);              // FPGA Upload pin
    Output_High(FPGAReset);             // FPGA reset off
    Output_High(TestLED);               // indication of boot up

    spi_enabled   = False;              // ZBC to PIC SPI disabled initially
    spi_write     = False;              // ZBC to PIC SPI disabled initially
    
    Refresh_RTCSPI();                   // Refresh data from RTC into SPI buffer

    //--------------------------------------------------------------------------
    //  USB Initialization Section            
    //--------------------------------------------------------------------------
    usb_init_cs();                      // Initialize the USB Connection
    if(read_eeprom(BOOT_TYPE) > 0) {    // We want boot from FLASH
        FlashToFPGA();                  // If not hooked up to USB then try to init
        Output_Low(FPGAReset);          // FPGA reset off
        Output_High(FPGAReset);         // FPGA reset off
        Output_Low(TestLED);            // Turn off Test LED
    }
    else {
        FGPA_SPI_Init();                // Put PIC in SPI Slave mode 
    }
    
    //--------------------------------------------------------------------------
    //  Main Command Loop:            
    //--------------------------------------------------------------------------
    do {                                // Always do USB Task
        usb_task();                     // so it will detect USB pluging in 
        if(usb_enumerated()) {          // Are we plugged into the USB port ?
            usb_rcvdata_task();         // If so, check for data 
        }
        if(spi_enabled) {               // if ZBC to PIC SPI enabled, then check 
            if(SSP_HAS_DATA()) {
                Handle_SPI();           // Handle SPI request from ZBC
            }
        }                               // Otherwise... just
    } while(True);                      // Continue forever, what else can we do ?
}
//------------------------------------------------------------------------------
//  End .c
//------------------------------------------------------------------------------

