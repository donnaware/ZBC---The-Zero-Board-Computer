//============================================================================//
//============================================================================//
// RoboMouse.C --  USB PIC yittle RoboMouse                                   //
//                                                                            //
// Uses your operating system's HID drivers, which on most systems should     //
// already be installed. If everything is working, the mouse cursor will move // 
// in a circle when connected to a PC. The Button enables and disables        //                                                        
// the cirucular motion.                                                      // 
//                                                                            //
//  DonnaWare International LLP Copyright (2001) All Rights Reserved          //
//============================================================================//
//============================================================================//
#include <18F2450.h>
//------------------------------------------------------------------------------
#define CRYSTAL20MHZ 1  // Set to 1 if using 20MHZ, else assumes 12MHZ
//------------------------------------------------------------------------------
#if CRYSTAL20MHZ
    #fuses HSPLL,USBDIV,PLL5,CPUDIV1,NOWDT,NOPROTECT,NOLVP,NODEBUG,VREGEN
    #use delay(clock=20000000)
    #use rs232(baud=250000, xmit=PIN_C6, rcv=PIN_C7)
#else
    #fuses HSPLL,USBDIV,PLL3,CPUDIV1,NOWDT,NOPROTECT,NOLVP,NODEBUG,VREGEN
    #use delay(clock=12000000)
    #use rs232(baud=150000, xmit=PIN_C6, rcv=PIN_C7)
#endif
//------------------------------------------------------------------------------
// CCS Library dynamic defines.  For dynamic configuration of the CCS Library
// for your application several defines need to be made.  See the comments
// at usb.h for more information
//------------------------------------------------------------------------------
#DEFINE USB_HID_DEVICE TRUE // Tells the CCS PIC USB firmware to include HID handling code.
//the following defines needed for the CCS USB PIC driver to enable the TX endpoint 1
// and allocate buffer space on the peripheral
#define USB_EP1_TX_ENABLE  USB_ENABLE_INTERRUPT   //turn on EP1 for IN bulk/interrupt transfers
#define USB_EP1_TX_SIZE    8  // allocate 8 bytes in the hardware for transmission
//the following defines needed for the CCS USB PIC driver to enable the RX endpoint 1
// and allocate buffer space on the peripheral
#define USB_EP1_RX_ENABLE  USB_ENABLE_INTERRUPT   //turn on EP1 for OUT bulk/interrupt transfers
#define USB_EP1_RX_SIZE    8  // allocate 8 bytes in the hardware for reception
//------------------------------------------------------------------------------
// Include the CCS USB Libraries. 
//------------------------------------------------------------------------------
//#include <pic18_usb.h>          // Microchip PIC18Fxx5x hardware layer for usb.c
#include <pic18_usb_2450.h>     // Microchip PIC18Fxx5x hardware layer for usb.c
#include <USBdescHIDTest.h>     // USB Configuration and Device descriptors
#include <usb.c>                // handles usb setup tokens and get descriptor reports
//------------------------------------------------------------------------------
// Configure the demonstration I/O
//------------------------------------------------------------------------------
#define LED1    PIN_A5          //
#define LED2    PIN_B4          //
#define LED3    PIN_B5          //
#define LED4    PIN_A2          //
#define BUTTON  PIN_B0          // Button 
#define LED_ON  output_high     // Turn LED On
#define LED_OFF output_low      // Turn LED Off
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

    new_connected  = usb_attached();
    new_enumerated = usb_enumerated();

    if(new_connected)    LED_OFF(LED2);
    else                 LED_ON(LED2);

    if(new_enumerated)   LED_OFF(LED3);
    else                 LED_ON(LED3);

    if( new_connected  && !last_connected)  printf("USB connected, waiting for enumaration...\n");
    if(!new_connected  &&  last_connected)  printf("USB disconnected, waiting for connection...\n");
    if( new_enumerated && !last_enumerated) printf("USB enumerated by PC/HOST\n");
    if(!new_enumerated &&  last_enumerated) printf("USB unenumerated by PC/HOST, waiting for enumeration...\n");

    last_connected  = new_connected;
    last_enumerated = new_enumerated;
}
//------------------------------------------------------------------------------
// Get a character from RS232 Port and echo it back 
//------------------------------------------------------------------------------
int getche(void)
{
    int data;
    data= getch(); 
    putchar(data);
    return(data);
}
//------------------------------------------------------------------------------
// Receives a packet of data.  The protocol was specified in the HID
// report descriptor (see usb_desc_robomouse.h), PC->PIC and is:
//     _msg[0] = 0x00   // Report ID
//     _msg[1] = var1   // First byte of data
//     _msg[2] = var2   // Secnod byte of data 
//------------------------------------------------------------------------------
void usb_rcvdata_task(void) 
{
   static int8 leds;

   if(usb_kbhit(1)) {
      usb_get_packet(1, &leds, 1);
      if(leds & 1) LED_OFF(LED4);
      else         LED_ON(LED4);
      printf("val = %x", leds);      
   }
}
//------------------------------------------------------------------------------
// Send  data from over USB
//------------------------------------------------------------------------------
void usb_snddata_task(void)
{
    static int count = 0;
    int  out_data[4];

    out_data[0] = 2;                    // button state goes here
    out_data[1] = count;
    if(usb_put_packet(1,out_data,2,USB_DTS_TOGGLE)) count++;
}
//------------------------------------------------------------------------------
// Main System Loop
//------------------------------------------------------------------------------
void main(void) 
{
    short debug  = false;   // Default is debug off
    short GoFlag = false;   // Default Go Flag On 

    Port_B_Pullups(TRUE);   // Turn On Port B Pull ups

    LED_OFF(LED1);
    LED_ON(LED2);
    LED_ON(LED3);

    printf("USB Robo-Mouse\n");
    printf("PCH: v\n");

    usb_init_cs();

    output_bit(LED4, GoFlag);   // SHow Go Flag state                           

    while(TRUE) {
        if(kbhit()) {                                    // Test for RS232 trans
            switch(getche()) {                           // Get the command
                case 'v': {putchar(0x01);       } break; // Get version #
                case 'd': {debug = false;       } break; // Debug off
                case 'D': {debug = true;        } break; // Debug on
                case 'Z': {Output_High(LED4);   } break; // Test LED On
                case 'z': {Output_Low(LED4);    } break; // Test LED
                case 'O': {GoFlag = true;       } break; // Robo mouse On
                case 'o': {GoFlag = false;      } break; // Robo mouse Off
                default:                          break; // Invalid command
            }
        }
        if(input(BUTTON) == 0) {
            while(input(BUTTON) == 0) delay_ms(10);
            GoFlag = !GoFlag;
            output_bit(LED4, GoFlag);
            if(debug) printf("GoFlag: %d\n",GoFlag);
        }
        usb_task();
        if(debug) usb_debug_task();
        if(usb_enumerated()) {
            if(GoFlag) usb_snddata_task();
            usb_rcvdata_task(); 
            delay_ms(10);
        }
    }
}
//------------------------------------------------------------------------------
// End
//------------------------------------------------------------------------------

