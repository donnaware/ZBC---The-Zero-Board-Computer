///////////////////////////////////////////////////////////////////////// 
////                          pic18_usb.c                            //// 
////                                                                 //// 
//// Hardware layer for CCS's USB library.  This hardware layer      //// 
//// supports the USB peripheral on the PIC18 family chips.  Current //// 
//// supported families are:                                         //// 
////     PIC18F2455/2550/4455/4550                                   //// 
////     PIC18F2450/4450                                             //// 
////     PIC18F65J50/66J50/66J55/67J50/85J50/86J50/86J55/87J50       //// 
////                                                                 //// 
//// This file is part of CCS's PIC USB driver code, which includes: //// 
////   usb_desc_*.h - an example set of config and device descriptor //// 
////   usb.c - USB token and request handler code                    //// 
////   usb.h - definitions, prototypes and global variables          //// 
////                                                                 //// 
//// The following examples are provided by CCS:                     //// 
////   ex_usb_mouse.c - A HID Mouse.                                 //// 
////   ex_usb_hid.c - A custom application using HID protocol.       //// 
////   ex_usb_kbmouse.c - A HID Mouse/Keyboard combo using multiple  //// 
////                      interfaces.                                //// 
////   ex_usb_kbmouse2.c - A HID Mouse/Keyboard combo using multiple //// 
////                      HID Reports.                               //// 
////   ex_usb_scope.c - A digital oscilloscope using a custom        //// 
////                    protocol requiring custom Windows drivers.   //// 
////   ex_usb_serial.c -                                             //// 
////   ex_usb_serial2.c - Two examples of using the CDC driver for   //// 
////     a virtual COM port.                                         //// 
////                                                                 //// 
////   *********** NOTE ABOUT 18F2450/4450 LIMITATIONS **********    //// 
////  Due to the limited USB RAM of this family, a limitation of     //// 
////  this driver is that there are only 3 endpoints (0, 1 and 2).   //// 
////  The HW actually supports more endpoints, but to simplify       //// 
////  driver development this driver will only support the first 3   //// 
////  so there is an easier memory block to work with.               //// 
////                                                                 //// 
////  USB_MAX_EP0_PACKET_LENGTH will also be set to 8 regardless     //// 
////  of USB speed, to save RAM.                                     //// 
////                                                                 //// 
////   ************** NOTE ABOUT HW REQUIREMENTS ****************    //// 
////  If you are not using internal pullups, you will need to put    //// 
////  an internal pullup resistor on C4 or C5 depending on if you    //// 
////  want to use slow speed or full speed.  This code configures    //// 
////  the device to use internal pullups, see usb_init() if you      //// 
////  want to change that.                                           //// 
////                                                                 //// 
////  You need approximately 470nF cap on C3, even if you are using  //// 
////  the internal 3.3V USB regulator.                               //// 
////                                                                 //// 
////  To run at full speed, you must use the oscillator              //// 
////  configuration (PLLx) to set the PLL divide to 4MHz.  You can   //// 
////  configure the MCU clock to any speed (up to 48MHz) but the     //// 
////  PLL must run at 4Mhz to provide the USB peripheral with a      //// 
////  96MHz clock.  See the datasheet for details.                   //// 
////                                                                 //// 
////  To run at slow speed you must configure your MCU to run at     //// 
////  24Mhz.  See the datasheet for details.                         //// 
////                                                                 //// 
////   ****************  NOTE ABOUT INTERRUPTS  ******************   //// 
//// This driver uses INT_USB.  It requires INT_USB to interrupt the //// 
//// PIC when an event has happened on the USB Bus.  Therfore        //// 
//// this code enables interrupts.  A user modification can be made  //// 
//// to poll the USB interrupt flag instead of relying on an         //// 
//// interrupt.                                                      //// 
////                                                                 //// 
////    ****************   USER FUNCTIONS  ***********************   //// 
////                                                                 //// 
//// usb_init() - Initializes the USB stack, the USB peripheral and  //// 
////              attaches the unit to the usb bus.  Enables         //// 
////              interrupts.                                        //// 
////                                                                 //// 
//// usb_init_cs() - A smaller usb_init(), does not attach unit      //// 
////              to usb bus or enable interrupts.                   //// 
////                                                                 //// 
//// usb_put_packet() - Sends one packet to the host.                //// 
////                    If you need to send a message that spans     //// 
////                    more than one packet then see usb_puts() in  //// 
////                    usb.c                                        //// 
////                                                                 //// 
//// usb_kbhit() - Returns true if OUT endpoint contains data from   //// 
////               host.                                             //// 
////                                                                 //// 
//// usb_rx_packet_size() - Returns the size of packet that was      //// 
////               received.  usb_kbhit() must return TRUE else      //// 
////               this is not valid.  Don't forget in USB there     //// 
////               are 0 len packets!                                //// 
////                                                                 //// 
//// usb_get_packet() - Gets one packet that from the host.          //// 
////                    usb_kbhit() must return true before you call //// 
////                    this routine or your data may not be valid.  //// 
////                    Once usb_kbhit() returns true you want to    //// 
////                    call this as soon as possible to get data    //// 
////                    out of the endpoint buffer so the PC can     //// 
////                    start sending more data, if needed.          //// 
////                    This only receives one packet, if you are    //// 
////                    trying to receive a multi-packet message     //// 
////                    see usb_gets() in usb.c.                     //// 
////                                                                 //// 
//// usb_detach() - De-attach USB from the system.                   //// 
////                                                                 //// 
//// usb_attach() - Attach USB to the system.                        //// 
////                                                                 //// 
//// usb_attached() - Returns TRUE if the device is attached to a    //// 
////                  USB cable.  A macro that looks at the defined  //// 
////                  connection sense pin.                          //// 
////                                                                 //// 
//// usb_task() - Keeps track of connection sense, calling           //// 
////              usb_detach() and usb_attach() when needed.         //// 
////                                                                 //// 
//// For more documentation on these functions read the comments at  //// 
//// each function.                                                  //// 
////                                                                 //// 
//// The other functions defined in this file are for use by the     //// 
//// USB code, and is not meant to be used by the user.              //// 
////                                                                 //// 
///////////////////////////////////////////////////////////////////////// 
////                                                                 //// 
//// Version History:                                                //// 
////                                                                 //// 
////   07-17-07: Added 18F4450,2450 support                          //// 
////                                                                 //// 
////   07-13-07: Added 87J50 family support                          //// 
////                                                                 //// 
////   11-01-05: usb_detach(), usb_attach() and usb_init_cs()        //// 
////               changed for the better.                           //// 
////                                                                 //// 
////   10-28-05: Added usb_rx_packet_size()                          //// 
////                                                                 //// 
////   07-13-05: usb_put_packet() changed for 16bit packet sizes     //// 
////             usb_flush_in() changed for 16bit packet sizes       //// 
////             usb_get_packet() changed for 16bit packet sizes     //// 
////             usb_flush_out() changed for 16bit packet sizes      //// 
////             usb_set_configured() changed for 16bit packet sizes //// 
////                                                                 //// 
////   06-30-05: usb_tbe() added                                     //// 
////             The way endpoint 0 DTS is set has been changed.     //// 
////                                                                 //// 
////   06-20-05: Initial Release                                     //// 
////                                                                 //// 
////   05-13-05: Beta Release (Full Speed works)                     //// 
////                                                                 //// 
////   03-21-05: Initial Alpha Release                               //// 
////                                                                 //// 
///////////////////////////////////////////////////////////////////////// 
////        (C) Copyright 1996,2005 Custom Computer Services         //// 
//// This source code may only be used by licensed users of the CCS  //// 
//// C compiler.  This source code may only be distributed to other  //// 
//// licensed users of the CCS C compiler.  No other use,            //// 
//// reproduction or distribution is permitted without written       //// 
//// permission.  Derivative programs created using this software    //// 
//// in object code form are not restricted in any way.              //// 
///////////////////////////////////////////////////////////////////////// 
#IFNDEF __USB_HARDWARE__ 
#DEFINE __USB_HARDWARE__ 

//let the USB Stack know that we are using a PIC with internal USB peripheral 
#DEFINE __PIC__ 1 

#if ((getenv("DEVICE")=="PIC18F87J50") || (getenv("DEVICE")=="PIC18F86J55") || (getenv("DEVICE")=="PIC18F86J50") || (getenv("DEVICE")=="PIC18F85J50") || (getenv("DEVICE")=="PIC18F67J50") || (getenv("DEVICE")=="PIC18F66J55") || (getenv("DEVICE")=="PIC18F66J50") || (getenv("DEVICE")=="PIC18F65J50")) 
    #define __USB_87J50__ 
    #define USB_TOTAL_BUFFER_SPACE  ((int16)getenv("RAM")-0x500) 
    #define USB_MAX_NUM_ENDPOINTS  16 
#elif ((getenv("DEVICE")=="PIC18F2450") || (getenv("DEVICE")=="PIC18F4450")) 
    #define __USB_4450__ 
    #ifdef(USB_EP3_TX_SIZE+USB_EP3_RX_SIZE+USB_EP4_TX_SIZE+USB_EP4_RX_SIZE+USB_EP5_TX_SIZE+USB_EP5_RX_SIZE+USB_EP6_TX_SIZE+USB_EP6_RX_SIZE+USB_EP7_TX_SIZE+USB_EP7_RX_SIZE+USB_EP8_TX_SIZE+USB_EP8_RX_SIZE+USB_EP9_TX_SIZE+USB_EP9_RX_SIZE+USB_EP10_TX_SIZE+USB_EP10_RX_SIZE+USB_EP11_TX_SIZE+USB_EP11_RX_SIZE+USB_EP12_TX_SIZE+USB_EP12_RX_SIZE+USB_EP13_TX_SIZE+USB_EP13_RX_SIZE+USB_EP14_TX_SIZE+USB_EP14_RX_SIZE+USB_EP15_TX_SIZE+USB_EP15_RX_SIZE)>0 
        #error This driver only supports endpoints 0, 1 and 2 for this chip. 
    #endif
    #define USB_MAX_NUM_ENDPOINTS  3
    #define USB_TOTAL_BUFFER_SPACE (0x100 - USB_MAX_NUM_ENDPOINTS * 8) 
#else 
    #define __USB_4550__ 
    #define USB_TOTAL_BUFFER_SPACE ((int16)0x300) 
    #define USB_MAX_NUM_ENDPOINTS  16 
#endif 

#ifndef USB_USE_FULL_SPEED 
    #define USB_USE_FULL_SPEED  TRUE 
#endif 

#ifndef USB_CON_SENSE_PIN 
    #define USB_CON_SENSE_PIN  0 
#endif 

#if defined(__USB_4450__)
    #ifndef USB_MAX_EP0_PACKET_LENGTH
        #define USB_MAX_EP0_PACKET_LENGTH   8
    #endif 
#else 
   #if USB_USE_FULL_SPEED == FALSE           
      #define USB_MAX_EP0_PACKET_LENGTH   8     //slow speed requires 8byte max packet size for endpoint 0
   #else 
     #define USB_MAX_EP0_PACKET_LENGTH   64    //for full speed you can still use 8bytes, but 64 will be faster 
   #endif 
#endif 

#include <usb.h> 

#define USB_BUFFER_NEEDED (USB_EP0_TX_SIZE+USB_EP0_RX_SIZE+USB_EP1_TX_SIZE+USB_EP1_RX_SIZE+USB_EP2_TX_SIZE+USB_EP2_RX_SIZE+USB_EP3_TX_SIZE+USB_EP3_RX_SIZE+USB_EP4_TX_SIZE+USB_EP4_RX_SIZE+USB_EP5_TX_SIZE+USB_EP5_RX_SIZE+USB_EP6_TX_SIZE+USB_EP6_RX_SIZE+USB_EP7_TX_SIZE+USB_EP7_RX_SIZE+USB_EP8_TX_SIZE+USB_EP8_RX_SIZE+USB_EP9_TX_SIZE+USB_EP9_RX_SIZE+USB_EP10_TX_SIZE+USB_EP10_RX_SIZE+USB_EP11_TX_SIZE+USB_EP11_RX_SIZE+USB_EP12_TX_SIZE+USB_EP12_RX_SIZE+USB_EP13_TX_SIZE+USB_EP13_RX_SIZE+USB_EP14_TX_SIZE+USB_EP14_RX_SIZE+USB_EP15_TX_SIZE+USB_EP15_RX_SIZE) 


#if (USB_BUFFER_NEEDED > USB_TOTAL_BUFFER_SPACE) 
    #error You are trying to allocate more memory for endpoints than the PIC can handle 
#endif 

#if defined(__USB_4450__) 
    #reserve 0x400:0x4FF 
#else 
    #reserve 0x400:0x4FF+USB_BUFFER_NEEDED 
#endif 

#define debug_usb(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z) 
//  #define debug_usb printf 
//  #define debug_putc putc_tbe 
#define debug_display_ram(x,y) 
/* 
void debug_display_ram(int8 len, int8 *ptr) { 
   int8 max=16; 
   debug_usb(debug_putc,"%U - ",len); 
   if (max>len) {max=len;} 
   while(max--) { 
      debug_usb(debug_putc,"%X",*ptr); 
      len--; 
      ptr++; 
   } 
   if (len) {debug_usb(debug_putc,"...");} 
} 
*/ 

//if you are worried that the PIC is not receiving packets because a bug in the 
//DATA0/DATA1 synch code, you can set this to TRUE to ignore the DTS on 
//receiving. 
#ifndef USB_IGNORE_RX_DTS 
    #define USB_IGNORE_RX_DTS FALSE 
#endif 

#ifndef USB_IGNORE_TX_DTS 
    #define USB_IGNORE_TX_DTS FALSE 
#endif 

//if you enable this it will keep a counter of the 6 possible errors the 
//pic can detect.  disabling this will save you ROM, RAM and execution time. 
#ifndef USB_USE_ERROR_COUNTER 
   #define USB_USE_ERROR_COUNTER FALSE 
#endif 

#define USB_PING_PONG_MODE_OFF   0  //no ping pong 
#define USB_PING_PONG_MODE_E0    1  //ping pong endpoint 0 only 
#define USB_PING_PONG_MODE_ON    2  //ping pong all endpoints 

//NOTE - PING PONG MODE IS NOT SUPPORTED BY CCS! 
#ifndef USB_PING_PONG_MODE 
   #define USB_PING_PONG_MODE USB_PING_PONG_MODE_OFF 
#endif 

#if USB_USE_ERROR_COUNTER 
   int ERROR_COUNTER[6]; 
#endif 

//---pic18fxx5x memory locations 
//#if defined(__USB_4550__) 
   #byte UFRML   =  0xF66 
   #byte UFRMH   =  0xF67 
   #byte UIR     =  0xF68 
   #byte UIE     =  0xF69 
   #byte UEIR    =  0xF6A 
   #byte UEIE    =  0xF6B 
   #byte USTAT   =  0xF6C 
   #byte UCON    =  0xF6D 
   #byte UADDR   =  0xF6E 
   #byte UCFG    =  0xF6F 
   #define  UEP0_LOC 0xF70 
/*#else 
   #byte UFRML   =  0xF60 
   #byte UFRMH   =  0xF61 
   #byte UIR     =  0xF62 
   #byte UIE     =  0xF5C 
   #byte UEIR    =  0xF63 
   #byte UEIE    =  0xF5D 
   #byte USTAT   =  0xF64 
   #byte UCON    =  0xF65 
   #byte UADDR   =  0xF5E 
   #byte UCFG    =  0xF5F 
   #define  UEP0_LOC 0xF4C 
#endif 
*/ 
#byte UEP0    =  UEP0_LOC 

#if defined(__USB_4450__) 
//  #define USB_BUFFER (0x400 + (USB_MAX_NUM_ENDPOINTS*8)) 
    #define USB_BUFFER 0x418   //if you have an old compiler you will need to use this 
#else 
    #define USB_BUFFER 0x500 
#endif 

#byte BD0STAT  =  0x400 
#byte BD0CNT   =  0x401 
#byte BD0ADRL  =  0x402 
#byte BD0ADRJ  =  0x403 

#define BD0STAT_LOC 0x400 
#define BD0CNT_LOC  0x401 
#define BD0ADRL_LOC 0x402 
#define BD0ADRH_LOC 0x403 

#define UEP(x) *(UEP0_LOC+x) 

#BIT UIR_SOF   = UIR.6 
#BIT UIR_STALL = UIR.5 
#BIT UIR_IDLE  = UIR.4 
#BIT UIR_TRN   = UIR.3 
#BIT UIR_ACTV  = UIR.2 
#BIT UIR_UERR  = UIR.1 
#BIT UIR_URST  = UIR.0 

#BIT UIE_SOF   = UIE.6 
#BIT UIE_STALL = UIE.5 
#BIT UIE_IDLE  = UIE.4 
#BIT UIE_TRN   = UIE.3 
#BIT UIE_ACTV  = UIE.2 
#BIT UIE_UERR  = UIE.1 
#BIT UIE_URST  = UIE.0 

#bit UCON_PBRST  = UCON.6 
#bit UCON_SE0    = UCON.5 
#bit UCON_PKTDIS = UCON.4 
#bit UCON_USBEN  = UCON.3 
#bit UCON_RESUME = UCON.2 
#bit UCON_SUSPND = UCON.1 

#if (USB_PING_PONG_MODE==USB_PING_PONG_MODE_OFF) 
    #define EP_BDxST_O(x)  * (BD0STAT_LOC + x*8) 
    #define EP_BDxCNT_O(x) * (BD0CNT_LOC + x*8) 
    #define EP_BDxADR_O(x) * (int16 *)(BD0ADRL_LOC + x*8) 
    #define EP_BDxST_I(x)  * (BD0STAT_LOC + 4 + x*8) 
    #define EP_BDxCNT_I(x) * (BD0CNT_LOC + 4 + x*8) 
    #define EP_BDxADR_I(x) * (int16 *)(BD0ADRL_LOC + 4 + x*8) 
#else 
    #error Right now this driver only supports no ping pong 
#endif 

//See UEPn (0xF70-0xF7F) 
#define ENDPT_DISABLED      0x00    //endpoint not used 
#define ENDPT_IN_ONLY       0x02    //endpoint supports IN transactions only 
#define ENDPT_OUT_ONLY      0x04    //endpoint supports OUT transactions only 
#define ENDPT_CONTROL       0x06    //Supports IN, OUT and CONTROL transactions - Only use with EP0 
#define ENDPT_NON_CONTROL   0x0E    //Supports both IN and OUT transactions 

//Define the states that the USB interface can be in 
enum {
    USB_STATE_DETACHED  = 0, 
    USB_STATE_ATTACHED  = 1,
    USB_STATE_POWERED   = 2,
    USB_STATE_DEFAULT   = 3,
    USB_STATE_ADDRESS   = 4,
    USB_STATE_CONFIGURED= 5
} usb_state=0;

//--BDendST has their PIDs upshifed 2 
#define USB_PIC_PID_IN       0x24  //device to host transactions 
#define USB_PIC_PID_OUT      0x04  //host to device transactions 
#define USB_PIC_PID_SETUP    0x34  //host to device setup transaction 

#define USTAT_IN_E0         4 
#define USTAT_OUT_SETUP_E0  0 

#define __USB_UIF_RESET     0x01 
#define __USB_UIF_ERROR     0x02 
#define __USB_UIF_ACTIVE    0x04 
#define __USB_UIF_TOKEN     0x08 
#define __USB_UIF_IDLE      0x10 
#define __USB_UIF_STALL     0x20 
#define __USB_UIF_SOF       0x40 

#if USB_USE_ERROR_COUNTER 
    #define STANDARD_INTS 0x3F 
#else 
    #define STANDARD_INTS 0x3D 
#endif 

#define __USB_UCFG_UTEYE   0x80 
#if defined(__USB_4550__) 
    #define __USB_UCFG_UOEMON  0x40 
#endif

#define __USB_UCFG_UPUEN   0x10 
#define __USB_UCFG_UTRDIS  0x08 
#define __USB_UCFG_FSEN    0x04 

#if USB_USE_FULL_SPEED 
   #define __UCFG_VAL_ENABLED__ (__USB_UCFG_UPUEN | __USB_UCFG_FSEN | USB_PING_PONG_MODE) 
#else 
   #define __UCFG_VAL_ENABLED__ (__USB_UCFG_UPUEN | USB_PING_PONG_MODE); 
#endif 

#define __UCFG_VAL_DISABLED__ 0x08 

char usb_ep0_rx_buffer[USB_MAX_EP0_PACKET_LENGTH]; 
#locate usb_ep0_rx_buffer=USB_BUFFER 

char usb_ep0_tx_buffer[USB_MAX_EP0_PACKET_LENGTH]; 
#locate usb_ep0_tx_buffer=USB_BUFFER+USB_MAX_EP0_PACKET_LENGTH 

char usb_data_buffer[USB_TOTAL_BUFFER_SPACE-USB_MAX_EP0_PACKET_LENGTH-USB_MAX_EP0_PACKET_LENGTH]; 
#locate usb_data_buffer=USB_BUFFER+USB_MAX_EP0_PACKET_LENGTH+USB_MAX_EP0_PACKET_LENGTH 

int8 __setup_0_tx_size; 

//interrupt handler, specific to PIC18Fxx5x peripheral only 
void usb_handle_interrupt(); 
void usb_isr_rst(); 
void usb_isr_uerr(); 
void usb_isr_sof(void); 
void usb_isr_activity(); 
void usb_isr_uidle(); 
void usb_isr_tok_dne(); 
void usb_isr_stall(void); 
void usb_init_ep0_setup(void); 

//following functions standard part of CCS PIC USB driver, and used by usb.c 
void usb_init(); 
void usb_detach(); 
int1 usb_put_packet(int endpoint, int * ptr, int16 len, USB_DTS_BIT tgl); 
int1 usb_flush_in(int8 endpoint, int16 len, USB_DTS_BIT tgl); //marks the transmit buffer as ready for transmission 
int16 usb_get_packet(int8 endpoint, int8 * ptr, int16 max); 
int16 usb_rx_packet_size(int8 endpoint); 
int16 usb_get_packet_buffer(int8 endpoint, int8 *ptr, int16 max); 
void usb_flush_out(int8 endpoint, USB_DTS_BIT tgl); 
void usb_stall_ep(int8 endpoint); 
void usb_unstall_ep(int8 endpoint); 
int1 usb_endpoint_stalled(int8 endpoint); 
void usb_set_address(int8 address); 
void usb_set_configured(int config); 
void usb_disable_endpoints(void); 

//// BEGIN User Functions: 

/****************************************************************************** 
/* usb_attached() 
/* 
/* Summary: Returns TRUE if the device is attached to a USB cable 
/* 
/*****************************************************************************/ 
#if USB_CON_SENSE_PIN 
    #define usb_attached() input(USB_CON_SENSE_PIN) 
#else 
    #define usb_attached() TRUE 
#endif 

/****************************************************************************** 
/* usb_detach() 
/* 
/* Summary: Remove the D+/D- lines from the USB bus.  Basically, disable USB. 
/* 
/*****************************************************************************/ 
void usb_detach(void) {  //done 
   UCON=0;  //disable USB hardware 
   UIE=0;   //disable USB interrupts 
   UCFG = __UCFG_VAL_DISABLED__; 
   set_tris_c(*0xF94 | 0x30); 
   usb_state=USB_STATE_DETACHED; 
   usb_token_reset();              //clear the chapter9 stack 
   __usb_kbhit_status=0; 
} 

/****************************************************************************** 
/* usb_attach() 
/* 
/* Summary: Attach the D+/D- lines to the USB bus.  Enable the USB peripheral. 
/* 
/* You should wait until UCON_SE0 is clear before enabling reset/idle interrupt 
/* 
/*****************************************************************************/ 
void usb_attach(void) { 
   usb_token_reset(); 
    UCON = 0; 
   UCFG = __UCFG_VAL_ENABLED__; 
    UIE = 0;                                // Mask all USB interrupts 
    UCON_USBEN = 1;                     // Enable module & attach to bus 
    usb_state = USB_STATE_ATTACHED;      // Defined in usbmmap.c & .h 
} 

/***************************************************************************** 
/* usb_init_cs() 
/* 
/* Summary: Resets and initalizes USB peripheral.  Does not attach the peripheral 
/*          to the USB bus.  See usb_attach() and usb_task() on how to 
/*          attach to the USB bus. 
/* 
/*          You must call this before any other USB code. 
/* 
/*          NOTE: an alternative function, usb_init(), is provided that 
/*                initializes the USB and then connects. 
/* 
/*****************************************************************************/ 
#define usb_init_cs usb_detach 

/***************************************************************************** 
/* usb_task() 
/* 
/* Summary: Keeps an eye on the connection sense pin to determine if we are 
/*          attached to a USB cable or not.  If we are attached to a USB cable, 
/*          initialize the USB peripheral if needed.  If we are disconnected 
/*          from the USB cable, disable the USB peripheral. 
/* 
/*          NOTE: If you are not using a connection sense pin, will automatically 
/*                enable the USB peripheral. 
/* 
/*          NOTE: this enables interrupts once the USB peripheral is ready 
/* 
/*****************************************************************************/ 
void usb_task(void) 
{
   if (usb_attached()) { 
      if (UCON_USBEN==0) { 
         debug_usb(debug_putc, "\r\n\nUSB TASK: ATTACH"); 
         usb_attach(); 
      } 
   } 
   else { 
      if (UCON_USBEN==1)  { 
         debug_usb(debug_putc, "\r\n\nUSB TASK: DE-ATTACH"); 
         usb_detach(); 
      } 
   } 

   if ((usb_state == USB_STATE_ATTACHED)&&(!UCON_SE0)) { 
      UIR=0; 
      UIE=0; 
      enable_interrupts(INT_USB); 
      enable_interrupts(GLOBAL); 
      UIE=__USB_UIF_IDLE | __USB_UIF_RESET;  //enable IDLE and RESET USB interrupt 
      usb_state=USB_STATE_POWERED; 
      debug_usb(debug_putc, "\r\n\nUSB TASK: POWERED"); 
   } 
} 

/***************************************************************************** 
/* usb_init() 
/* 
/* Summary: Resets and initalizes USB hardware.  You must call this first before 
/*          using code.  Will attach the USB periperhal to the USB bus. 
/* 
/*          NOTE: If you are using a connection sense pin, this will wait in 
/*                an infinite loop until the device is connected to a USB cable. 
/* 
/*          NOTE: If you are not using a connection sense pin, this will wait 
/*                in an infinte loop until the SE0 condition clears, which usually 
/*                doesn't take long 
/* 
/*          NOTE: this enables interrupts. 
/* 
/*****************************************************************************/ 
void usb_init(void) { 
   usb_init_cs(); 

   do { 
      usb_task(); 
   } while (usb_state != USB_STATE_POWERED); 
} 


/************************************************************** 
/* usb_flush_in() 
/* 
/* Input: endpoint - which endpoint to mark for transfer 
/*        len - length of data that is being tramsferred 
/*        tgl - Data toggle synchronization for this packet 
/* 
/* Output: TRUE if success, FALSE if error (we don't control the endpoint) 
/* 
/* Summary: Marks the endpoint ready for transmission.  You must 
/*          have already loaded the endpoint buffer with data. 
/*          (IN is PIC -> PC) 
/***************************************************************/ 
int1 usb_flush_in(int8 endpoint, int16 len, USB_DTS_BIT tgl) { 
   int8 i; 

   debug_usb(debug_putc,"\r\nPUT %X %U %LU",endpoint, tgl, len); 

   i=EP_BDxST_I(endpoint); 
   if (!bit_test(i,7)) { 

      EP_BDxCNT_I(endpoint)=len; 

     debug_display_ram(len, EP_BDxADR_I(endpoint)); 

     #if USB_IGNORE_TX_DTS 
      i=0x80; 
     #else 
      if (tgl == USB_DTS_TOGGLE) { 
         i=EP_BDxST_I(endpoint); 
         if (bit_test(i,6)) 
            tgl=USB_DTS_DATA0;  //was DATA1, goto DATA0 
         else 
            tgl=USB_DTS_DATA1;  //was DATA0, goto DATA1 
      } 
      else if (tgl == USB_DTS_USERX) { 
         i=EP_BDxST_O(endpoint); 
         if (bit_test(i,6)) 
            tgl=USB_DTS_DATA1; 
         else 
            tgl=USB_DTS_DATA0; 
      } 
      if (tgl == USB_DTS_DATA1) { 
         i=0xC8;  //DATA1, UOWN 
      } 
      else if (tgl == USB_DTS_DATA0) { 
         i=0x88; //DATA0, UOWN 
      } 
     #endif 

      //set BC8 and BC9 
      if (bit_test(len,8)) {bit_set(i,0);} 
      if (bit_test(len,9)) {bit_set(i,1);} 

      debug_usb(debug_putc," %X",i); 

      EP_BDxST_I(endpoint)=i;//save changes 

      return(1); 
   } 
    else { 
         debug_usb(debug_putc,"\r\nPUT ERR"); 
    } 
   return(0); 
} 

/******************************************************************************* 
/* usb_put_packet(endpoint,*ptr,len,toggle) 
/* 
/* Input: endpoint - endpoint to send packet to 
/*        ptr - points to data to send 
/*        len - amount of data to send 
/*        toggle - whether to send data with a DATA0 pid, a DATA1 pid, or toggle from the last DATAx pid. 
/* 
/* Output: TRUE if data was sent correctly, FALSE if it was not.  The only reason it will 
/*         return FALSE is if because the TX buffer is still full from the last time you 
/*         tried to send a packet. 
/* 
/* Summary: Sends one packet out the EP to the host.  Notice that there is a difference 
/*          between a packet and a message.  If you wanted to send a 512 byte message you 
/*          would accomplish this by sending 8 64-byte packets, followed by a 0 length packet. 
/*          If the last (or only packet) being sent is less than the max packet size defined 
/*          in your descriptor then you do not need to send a 0 length packet to identify 
/*          an end of message. 
/* 
/*          usb_puts() (provided in usb.c) will send a multi-packet message correctly. 
/* 
/********************************************************************************/ 
int1 usb_put_packet(int8 endpoint, int8 * ptr, int16 len, USB_DTS_BIT tgl) { //done 
   int16 j; 
   int8 i; 
   int8 * buff_add;    

   i=EP_BDxST_I(endpoint); 
   if (!bit_test(i,7)) { 

      buff_add=EP_BDxADR_I(endpoint); 

      for (j=0;j<len;j++) { 
         *buff_add=*ptr; 
         buff_add++; 
         ptr++; 
      } 

      return(usb_flush_in(endpoint, len, tgl)); 
    } 
    else { 
        debug_usb(debug_putc,"\r\nPUT ERR"); 
    } 
    return(0); 
} 

/// END User Functions 


/// BEGIN Hardware layer functions required by USB.C 

/************************************************************** 
/* usb_flush_out() 
/* 
/* Input: endpoint - which endpoint to mark for transfer 
/*        tgl - Data toggle synchronization to expect in the next packet 
/* 
/* Output: NONE 
/* 
/* Summary: Clears the previously received packet, and then marks this 
/*          endpoint's receive buffer as ready for more data. 
/*          (OUT is PC -> PIC) 
/***************************************************************/ 
void usb_flush_out(int8 endpoint, USB_DTS_BIT tgl) { 
   int8 i; 
   int16 len; 

     #if USB_IGNORE_RX_DTS 
      if (tgl == USB_DTS_STALL) { 
         debug_usb(debug_putc, '*'); 
         i=0x84; 
         EP_BDxST_I(endpoint)=0x84; 
         return; 
      } 
      else 
         i=0x80; 
     #else 
      i=EP_BDxST_O(endpoint); 
      if (tgl == USB_DTS_TOGGLE) { 
         if (bit_test(i,6)) 
            tgl=USB_DTS_DATA0;  //was DATA1, goto DATA0 
         else 
            tgl=USB_DTS_DATA1;  //was DATA0, goto DATA1 
      } 
      if (tgl == USB_DTS_STALL) { 
         i=0x84; 
         EP_BDxST_I(endpoint)=0x84; //stall both in and out endpoints 
      } 
      else if (tgl == USB_DTS_DATA1) { 
         i=0xC8;  //DATA1, UOWN 
      } 
      else if (tgl == USB_DTS_DATA0) { 
         i=0x88; //DATA0, UOWN 
      } 
     #endif 

   bit_clear(__usb_kbhit_status,endpoint); 

   len=usb_ep_rx_size[endpoint]; 
   EP_BDxCNT_O(endpoint)=len; 
   if (bit_test(len,8)) {bit_set(i,0);} 
   if (bit_test(len,9)) {bit_set(i,1);} 


   EP_BDxST_O(endpoint)=i; 
} 

int16 usb_rx_packet_size(int8 endpoint) { 
   return(EP_BDxCNT_O(endpoint)); 
} 

/******************************************************************************* 
/* usb_get_packet_buffer(endpoint, *ptr, max) 
/* 
/* Input: endpoint - endpoint to get data from 
/*        ptr - where to save data to local PIC RAM 
/*        max - max amount of data to receive from buffer 
/* 
/* Output: the amount of data taken from the buffer. 
/* 
/* Summary: Gets a packet of data from the USB buffer and puts into local PIC RAM. 
/*          Does not mark the endpoint as ready for more data.  Once you are 
/*          done with data, call usb_flush_out() to mark the endpoint ready 
/*          to receive more data. 
/* 
/********************************************************************************/ 
int16 usb_get_packet_buffer(int8 endpoint, int8 *ptr, int16 max) 
{
   int8 * al; 
   int8 st; 
   int16 i; 

   al=EP_BDxADR_O(endpoint); 
   i=EP_BDxCNT_O(endpoint); 
   st=EP_BDxST_O(endpoint); 

   //read BC8 and BC9 
   if (bit_test(st,0)) {bit_set(i,8);} 
   if (bit_test(st,1)) {bit_set(i,9);} 

   if(i<max) { max=i; } 

   i=0; 

   while (i<max) { 
      *ptr=*al; 
       ptr++; 
       al++; 
       i++; 
   } 

   return(max); 
} 

/******************************************************************************* 
/* usb_get_packet(endpoint, *ptr, max) 
/* 
/* Input: endpoint - endpoint to get data from 
/*        ptr - where to save data to local PIC RAM 
/*        max - max amount of data to receive from buffer 
/* 
/* Output: the amount of data taken from the buffer. 
/* 
/*         NOTE - IF THERE IS NO PACKET TO GET YOU WILL GET INVALID RESULTS! 
/*                VERIFY WITH USB_KBHIT() BEFORE YOU CALL USB_GET_PACKET()! 
/* 
/* Summary: Gets a packet of data from the USB buffer and puts into local PIC RAM. 
/*          Until you call usb_get_packet() the data will sit in the endpoint 
/*          buffer and the PC will get NAKs when it tries to write more data 
/*          to the endpoint. 
/* 
/********************************************************************************/ 
int16 usb_get_packet(int8 endpoint, int8 * ptr, int16 max) { 

   max=usb_get_packet_buffer(endpoint,ptr,max); 
   usb_flush_out(endpoint, USB_DTS_TOGGLE); 

   return(max); 
} 

/******************************************************************************* 
/* usb_tbe(endpoint) 
/* 
/* Input: endpoint - endpoint to check 
/*        ptr - where to save data to local PIC RAM 
/*        max - max amount of data to receive from buffer 
/* 
/* Output: returns TRUE if this endpoint's IN buffer (PIC-PC) is empty and ready 
/*         returns FALSE if this endpoint's IN buffer is still processing the last 
/*         transmit or if this endpoint is invalid. 
/* 
/********************************************************************************/ 
int8 usb_tbe(int8 endpoint) { 
   int8 st; 
   st=EP_BDxST_I(endpoint); 
   if (!bit_test(st,7)) 
      return(TRUE); 
   return(FALSE); 
} 

/******************************************************************************* 
/* usb_stall_ep(endpoint,direction) 
/* 
/* Input: endpoint - endpoint to stall. 
/*                   top most bit indicates direction (set is IN, clear is OUT) 
/* 
/* Summary: Stalls specified endpoint.  If endpoint is stalled it will send STALL packet 
/*          if the host tries to access this endpoint's buffer. 
/* 
/* 
/* NOTE: WE ASSUME ENDPOINT IS VALID.  USB.C SHOULD CHECK THIS 
/* 
/********************************************************************************/ 
void usb_stall_ep(int8 endpoint) {  //done 
   int1 direction; 
   direction=bit_test(endpoint,7); 
   endpoint&=0x7F; 
   if (direction) { 
      EP_BDxST_I(endpoint)=0x84; 
   } 
   else { 
      EP_BDxST_O(endpoint)=0x84; 
   } 
} 

/******************************************************************************* 
/* usb_unstall_ep(endpoint, direction) 
/* 
/* Input: endpoint - endpoint to un-stall. 
/*                   top most bit indicates direction (set is IN, clear is OUT) 
/* 
/* Summary: Un-stalls endpoint. 
/* 
/* NOTE: WE ASSUME ENDPOINT IS VALID.  USB.C SHOULD CHECK THIS 
/********************************************************************************/ 
void usb_unstall_ep(int8 endpoint) {   //done 
   int1 direction; 
   direction=bit_test(endpoint,7); 
   endpoint&=0x7F; 
   if (direction) { 
      #if USB_IGNORE_RX_DTS 
      EP_BDxST_I(endpoint)=0x80; 
      #else 
      EP_BDxST_I(endpoint)=0x88; 
      #endif 
   } 
   else { 
      EP_BDxST_O(endpoint)=0x00; 
   } 
} 

/******************************************************************************* 
/* usb_endpoint_stalled(endpoint) 
/* 
/* Input: endpoint - endpoint to check 
/*                   top most bit indicates direction (set is IN, clear is OUT) 
/* 
/* Output: returns a TRUE if endpoint is stalled, FALSE if it is not. 
/* 
/* Summary: Looks to see if an endpoint is stalled, or not.  Does not look to 
/*          see if endpoint has been issued a STALL, just whether or not it is 
/*          configured to STALL on the next packet.  See Set_Feature and Clear_Feature 
/*          Chapter 9 requests. 
/* 
/* NOTE: WE ASSUME ENDPOINT IS VALID.  USB.C SHOULD CHECK THIS 
/********************************************************************************/ 
int1 usb_endpoint_stalled(int8 endpoint) {   //done 
   int1 direction; 
   int8 st; 
   direction=bit_test(endpoint,7); 
   endpoint&=0x7F; 
   if (direction) { 
      st=EP_BDxST_I(endpoint); 
   } 
   else { 
      st=EP_BDxST_O(endpoint); 
   } 
   return(bit_test(st,7) && bit_test(st,2)); 
} 


/******************************************************************************* 
/* usb_set_address(address) 
/* 
/* Input: address - address the host specified that we use 
/* 
/* Summary: Configures the USB Peripheral for the specified device address.  The host 
/*          will now talk to use with the following address. 
/* 
/********************************************************************************/ 
void usb_set_address(int8 address) {   //done 
   UADDR=address; 
   if (address) { 
      usb_state=USB_STATE_ADDRESS; 
   } 
   else { 
      usb_state=USB_STATE_POWERED; 
   } 
} 


/******************************************************************************* 
/* usb_set_configured(config) 
/* 
/* Input: config - Configuration to use.  0 to uncofigure device. 
/* 
/* Summary: Configures or unconfigures device.  If configuring device it will 
/*          enable all the endpoints the user specified for this configuration. 
/*          If un-configuring device it will disable all endpoints. 
/* 
/*          NOTE: CCS only provides code to handle 1 configuration. 
/* 
/********************************************************************************/ 
void usb_set_configured(int config) { 
   int8 en; 
   int16 addy; 
   int8 new_uep; 
   int16 len; 
   int8 i; 
      if (config==0) { 
         //if config=0 then set addressed state 
         usb_state=USB_STATE_ADDRESS; 
         usb_disable_endpoints(); 
      } 
      else { 
         usb_state=USB_STATE_CONFIGURED; //else set configed state 
         addy=(int16)USB_BUFFER+(2*USB_MAX_EP0_PACKET_LENGTH); 
         for (en=1;en<16;en++) { 
            new_uep=0; 
            if (usb_ep_rx_type[en]!=USB_ENABLE_DISABLED) { 
               new_uep=0x04; 
               len=usb_ep_rx_size[en]; 
               EP_BDxCNT_O(en)=len; 
               EP_BDxADR_O(en)=addy; 
               addy+=usb_ep_rx_size[en]; 
               #if USB_IGNORE_RX_DTS 
                  i=0x80; 
               #else 
                  i=0x88; 
               #endif 
               if (bit_test(len,8)) {bit_set(i,0);} 
               if (bit_test(len,9)) {bit_set(i,1);} 
               EP_BDxST_O(en)=i; 
            } 
            if (usb_ep_tx_type[en]!=USB_ENABLE_DISABLED) { 
               new_uep|=0x02; 
               EP_BDxADR_I(en)=addy; 
               addy+=usb_ep_tx_size[en]; 
               EP_BDxST_I(en)=0x40; 
            } 
            if (new_uep==0x06) {new_uep=0x0E;} 
            if (usb_ep_tx_type[en]!=USB_ENABLE_ISOCHRONOUS) { 
               new_uep|=0x10; 
            } 
            UEP(en)=new_uep; 
         } 
      } 
} 

/// END Hardware layer functions required by USB.C 


/// BEGIN USB Interrupt Service Routine 

/******************************************************************************* 
/* usb_handle_interrupt() 
/* 
/* Summary: Checks the interrupt, and acts upon event.  Processing finished 
/*          tokens is the majority of this code, and is handled by usb.c 
/* 
/* NOTE: If you wish to change to a polling method (and not an interrupt method), 
/*       then you must call this function rapidly.  If there is more than 10ms 
/*       latency the PC may think the USB device is stalled and disable it. 
/*       To switch to a polling method, remove the #int_usb line above this fuction. 
/*       Also, goto usb_init() and remove the code that enables the USB interrupt. 
/********************************************************************************/ 
#int_usb 
void usb_isr() { 
   if (usb_state==USB_STATE_DETACHED) return;   //should never happen, though 
   if (UIR) { 
      debug_usb(debug_putc,"\r\n\n[%X] ",UIR); 
      if (UIR_ACTV && UIE_ACTV) {usb_isr_activity();}  //activity detected.  (only enable after sleep) 

      if (UCON_SUSPND) return; 

      if (UIR_UERR && UIE_UERR) {usb_isr_uerr();}          //error has been detected 

      if (UIR_URST && UIE_URST) {usb_isr_rst();}        //usb reset has been detected 

      if (UIR_IDLE && UIE_IDLE) {usb_isr_uidle();}        //idle time, we can go to sleep 
      if (UIR_SOF && UIE_SOF) {usb_isr_sof();} 
      if (UIR_STALL && UIE_STALL) {usb_isr_stall();}        //a stall handshake was sent 

      if (UIR_TRN && UIE_TRN) { 
         usb_isr_tok_dne(); 
         UIR_TRN=0;    // clear the token done interrupt., 0x190.3 
      }    //a token has been detected (majority of isrs) 
   } 
} 

//SOF interrupt not handled.  user must add this depending on application 
void usb_isr_sof(void) { 
   debug_usb(debug_putc,"\r\nSOF"); 
   UIR_SOF=0; 
} 

/******************************************************************************* 
/* usb_disable_endpoints() 
/* 
/* Summary: Disables endpoints 1 thru 15 
/* 
/********************************************************************************/ 
void usb_disable_endpoints(void) { 
   int8 i; 
   for (i=1;i<16;i++) { 
      UEP(i)=ENDPT_DISABLED; 
   } 
   __usb_kbhit_status=0; 
} 

/******************************************************************************* 
/* usb_isr_rst() 
/* 
/* Summary: The host (computer) sent us a RESET command.  Reset USB device 
/*          and token handler code to initial state. 
/* 
/********************************************************************************/ 
void usb_isr_rst() { 
   debug_usb(debug_putc,"R"); 

   UEIR=0; 
   UIR=0; 
   UEIE=0x9F; 
   UIE=STANDARD_INTS & ~__USB_UIF_ACTIVE; 

   UADDR=0; 

   usb_token_reset(); 

   usb_disable_endpoints(); 

   UEP(0)=ENDPT_CONTROL | 0x10; 

   while (UIR_TRN) { 
      UIR_TRN=0;    //do this to clear out the ustat fifo 
   } 

   UCON_PKTDIS=0; //SIE token and packet processing enabled 

   usb_init_ep0_setup(); 

   usb_state=USB_STATE_DEFAULT; //put usb mcu into default state 
} 

/***************************************************************************** 
/* usb_init_ep0_setup() 
/* 
/* Summary: Configure EP0 to receive setup packets 
/* 
/*****************************************************************************/ 
void usb_init_ep0_setup(void) { 
    EP_BDxCNT_O(0) = USB_MAX_EP0_PACKET_LENGTH; 
    EP_BDxADR_O(0) = USB_BUFFER; 
   #if USB_IGNORE_RX_DTS 
    EP_BDxST_O(0) = 0x80; //give control to SIE, data toggle synch off 
   #else 
    EP_BDxST_O(0) = 0x88; //give control to SIE, DATA0, data toggle synch on 
   #endif 

    EP_BDxST_I(0) = 0; 
    EP_BDxADR_I(0) = USB_BUFFER + (int16)USB_MAX_EP0_PACKET_LENGTH; 
} 

/******************************************************************************* 
/* usb_isr_uerr() 
/* 
/* Summary: The USB peripheral had an error.  If user specified, error counter 
/*          will incerement.  If having problems check the status of these 8 bytes. 
/* 
/* NOTE: This code is not enabled by default. 
/********************************************************************************/ 
void usb_isr_uerr() { 
#if USB_USE_ERROR_COUNTER 
   int ints; 
#endif 

   debug_usb(debug_putc,"E %X ",UEIR); 

#if USB_USE_ERROR_COUNTER 

   ints=UEIR & UEIE; //mask off the flags with the ones that are enabled 

   if ( bit_test(ints,0) ) { //increment pid_error counter 
      debug_usb(debug_putc,"PID "); 
      ERROR_COUNTER[0]++; 
   } 

   if ( bit_test(ints,1) ) {  //increment crc5 error counter 
      debug_usbdebug_putc,"CRC5 "); 
      ERROR_COUNTER[1]++; 
   } 

   if ( bit_test(ints,2) ) {  //increment crc16 error counter 
      debug_usb(debug_putc,"CRC16 "); 
      ERROR_COUNTER[2]++; 
   } 

   if ( bit_test(ints,3) ) {  //increment dfn8 error counter 
      debug_usb(debug_putc,"DFN8 "); 
      ERROR_COUNTER[3]++; 
   } 

   if ( bit_test(ints,4) ) {  //increment bto error counter 
      debug_usb(debug_putc,"BTO "); 
      ERROR_COUNTER[4]++; 
   } 

   if ( bit_test(ints,7) ) { //increment bts error counter 
      debug_usb(debug_putc,"BTS "); 
      ERROR_COUNTER[5]++; 
   } 
#endif 

   UEIR=0; 
   UIR_UERR=0; 
} 

/******************************************************************************* 
/* usb_isr_uidle() 
/* 
/* Summary: USB peripheral detected IDLE.  Put the USB peripheral to sleep. 
/* 
/********************************************************************************/ 
void usb_isr_uidle() { 
   debug_usb(debug_putc,"I"); 

   UIE_ACTV=1;   //enable activity interrupt flag. (we are now suspended until we get an activity interrupt. nice) 
   UIR_IDLE=0; //clear idle interrupt flag 
   UCON_SUSPND=1; //set suspend. we are now suspended 
} 


/******************************************************************************* 
/* usb_isr_activity() 
/* 
/* Summary: USB peripheral detected activity on the USB device.  Wake-up the USB 
/*          peripheral. 
/* 
/********************************************************************************/ 
void usb_isr_activity() { 
   debug_usb(debug_putc,"A"); 

   UCON_SUSPND=0; //turn off low power suspending 
   UIE_ACTV=0; //clear activity interupt enabling 
   UIR_ACTV=0; 
} 

/******************************************************************************* 
/* usb_isr_stall() 
/* 
/* Summary: Stall handshake detected. 
/* 
/********************************************************************************/ 
void usb_isr_stall(void) { 
   debug_usb(debug_putc,"S"); 

   if (bit_test(UEP(0),0)) { 
      usb_init_ep0_setup(); 
      bit_clear(UEP(0),0); 
   } 
   UIR_STALL=0; 
} 


/******************************************************************************* 
/* usb_isr_tok_dne() 
/* 
/* Summary: A Token (IN/OUT/SETUP) has been received by the USB peripheral. 
/*          If a setup token on EP0 was received, run the chapter 9 code and 
/*          handle the request. 
/*          If an IN token on EP0 was received, continue transmitting any 
/*          unfinished requests that may take more than one packet to transmit 
/*          (if necessary). 
/*          If an OUT token on any other EP was received, mark that EP as ready 
/*          for a usb_get_packet(). 
/*          Does not handle any IN or OUT tokens on EP0. 
/* 
/********************************************************************************/ 
void usb_isr_tok_dne() { 
   int8 en; 

   en=USTAT>>3; 

         debug_usb(debug_putc,"T "); 
         debug_usb(debug_putc,"%X ", USTAT); 

      if (USTAT==USTAT_OUT_SETUP_E0) {   //new out or setup token in the buffer 
         debug_usb(debug_putc,"%X ", EP_BDxST_O(0)); 
         if ((EP_BDxST_O(0) & 0x3C)==USB_PIC_PID_SETUP) { 
            EP_BDxST_I(0)=0;   // return the in buffer to us (dequeue any pending requests) 

            debug_usb(debug_putc,"(%U) ", EP_BDxCNT_O(0)); 
            debug_display_ram(EP_BDxCNT_O(0), usb_ep0_rx_buffer); 

            usb_isr_tok_setup_dne(); 

            //if setup_0_tx_size==0xFF - stall ep0 (unhandled request) 
            //if setup_0_tx_size==0xFE - get EP0OUT ready for a data packet, leave EP0IN alone 
            //else setup_0_tx_size=size of response, get EP0OUT ready for a setup packet, mark EPOIN ready for transmit 
            if (__setup_0_tx_size==0xFF) 
               usb_flush_out(0,USB_DTS_STALL); 
            else { 
               usb_flush_out(0,USB_DTS_TOGGLE); 
               if (__setup_0_tx_size!=0xFE) { 
                  usb_flush_in(0,__setup_0_tx_size,USB_DTS_USERX); 
               } 
            } 
            UCON_PKTDIS=0;       // UCON,PKT_DIS ; Assuming there is nothing to dequeue, clear the packet disable bit 
         } 
         else if ((EP_BDxST_O(0) & 0x3C)==USB_PIC_PID_OUT) { 
            usb_isr_tok_out_dne(0); 
            usb_flush_out(0,USB_DTS_TOGGLE); 
            if ((__setup_0_tx_size!=0xFE)&&(__setup_0_tx_size!=0xFF)) { 
               usb_flush_in(0,__setup_0_tx_size,USB_DTS_DATA1);   //send response (usually a 0len) 
            } 
         } 
      } 

      else if (USTAT==USTAT_IN_E0) {   //pic -> host transfer completed 
         __setup_0_tx_size=0xFF; 
         usb_isr_tok_in_dne(0); 
         if (__setup_0_tx_size!=0xFF) 
            usb_flush_in(0,__setup_0_tx_size,USB_DTS_TOGGLE); 
         else 
            usb_init_ep0_setup(); 
      } 

      else { 
         if (!bit_test(USTAT,2)) { 
            usb_isr_tok_out_dne(en); 
         } 
         else { 
            usb_isr_tok_in_dne(en); 
         } 
      } 
} 

/************************************************************** 
/* usb_request_send_response(len) 
/* usb_request_get_data() 
/* usb_request_stall() 
/* 
/* Input: len - size of packet to send 
/* 
/* Summary: After we process a SETUP request, we have 1 of three responses: 
/*            1.) send a response IN packet 
/*            2.) wait for followup OUT packet(s) with data 
/*            3.) stall because we don't support that SETUP request 
/* 
/*          If we are sending data, the array usb_ep0_tx_buffer[] will hold 
/*          the response and the USB Request handler code will call 
/*          usb_request_send_response() to let us know how big the packet is. 
/* 
/*          If we are waiting for more data, usb_request_get_data() will 
/*          be called by the USB request handler code to configure the EP0 OUT 
/*          endpoint to be ready for more data 
/* 
/*          If we don't support a request, usb_request_stall() will be called 
/*          by the USB request handler code to stall the endpoint 0. 
/* 
/***************************************************************/ 
void usb_request_send_response(int len) { 
   __setup_0_tx_size=len; 
} 

void usb_request_get_data(void) { 
   __setup_0_tx_size=0xFE; 
} 

void usb_request_stall(void) { 
   __setup_0_tx_size=0xFF; 
} 

/// END USB Interrupt Service Routine 

#ENDIF
