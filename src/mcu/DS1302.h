//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// D S 1 3 0 2      R T C    C o n t r o l     R o u t i n e s :             
//
// Interface to the Dallas Maxim DS1302
//
// Use Standard IO (Let CCS determine TRIS Setup)
// Define IO pins as needed.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define RtcRstBit     RTC_RST                   // RTC reset line LO=reset HI=active
#define RtcClkBit     RTC_SCLK                  // RTC clock pin
#define RtcSioBit     RTC_SIO                   // RTC I/O data pin
//------------------------------------------------------------------------------
//   DS1302 Register definintions:                                             
//------------------------------------------------------------------------------
#define RTC_Reg      0x80                       // Real Time Clock base register
#define RTC_RD       0x01                       // Read bit
#define RTC_WR       0x00                       // Write bit
//------------------------------------------------------------------------------
#define RTC_Sec      0x00                       // Seconds
#define RTC_Min      0x01                       // Minutes Write
#define RTC_Hrs      0x02                       // Hours Write
#define RTC_Dat      0x03                       // Date Write
#define RTC_Mon      0x04                       // Month Write
#define RTC_Day      0x05                       // Day Write
#define RTC_Yrs      0x06                       // Years Write
#define RTC_Ctl      0x07                       // Control Write
#define RTC_Trc      0x08                       // Trickle Charge Control Write
#define RTC_Bst      0x1F                       // RAM Burst Control Write
#define RTC_RAMS     0x20                       // Scratch Pad Start
//------------------------------------------------------------------------------
//  Alarm Time Locations                                                     
//------------------------------------------------------------------------------
#define Alm_ENB      0x20                        // Flag to enable/disable alrms
#define Alm_RHr      0x21                        // Sunrise Time Hours
#define Alm_RMn      0x22                        // Sunrise Time Minutes
#define Alm_SHr      0x23                        // Sunset  Time Hours
#define Alm_SMn      0x24                        // Sunset  Time Minutes

//------------------------------------------------------------------------------
//   Write 1 byte to RTC Pipe:                                                 
//------------------------------------------------------------------------------
void RtcWrByte(int SData)
{
    int i;
    for(i=0; i<8; ++i) {
        output_bit(RtcSioBit, shift_right(&SData,1,0));  // Send a data bit
        output_low(RtcClkBit);
        output_high(RtcClkBit);                          // Pulse the clock
    }
}
//------------------------------------------------------------------------------
//   Read 1 byte from RTC Pipe:                                                 
//------------------------------------------------------------------------------
int RtcRdByte(void)
{
    int i, SData;
    for(i=0; i<8; ++i) {                        // Get 8 bits of data
        output_low(RtcClkBit);
        set_tris_C(get_tris_C() | 0b00000010);
        shift_right(&SData, 1, input(RtcSioBit));
        set_tris_C(get_tris_C() & 0b11111101);
        output_high(RtcClkBit);
    }
    return(SData);
}

//------------------------------------------------------------------------------
//   Write 1 Byte to RTC register:                                             
//------------------------------------------------------------------------------
void RTCWrite(int addr, int data)
{
    output_low(RtcClkBit);      // Clock pin low
    output_low(RtcSioBit);      // Start with IO pin low
    delay_us(20);               // Time delay to allow time for Data clock setup
    output_high(RtcRstBit);     // Raise RTC Reset pin to enable interface
    addr = addr<<1;             // Rotate left
    addr |= RTC_Reg | RTC_WR;   // Make the address byte
    RtcWrByte(addr);            // Write address pointer byte out serial pipe
    RtcWrByte(data);            // Write data byte out serial pipe
    output_low(RtcRstBit);      // Lower RTC Reset pin to dis-able interface
}
//------------------------------------------------------------------------------
//  Read 1 byte from RTC:                                                     
//------------------------------------------------------------------------------
int RTCRead(int addr)
{
    int data;

    output_low(RtcClkBit);      // Clock pin low
    output_low(RtcSioBit);      // Start with IO pin low
    delay_us(20);               // Time delay to allow time for Data clock setup
    output_high(RtcRstBit);     // Raise RTC Reset pin to enable interface
    addr = addr<<1;             // Rotate left
    addr |= RTC_Reg | RTC_RD;   // Make the address byte
    RtcWrByte(addr);            // Write address pointer byte out serial pipe
    data = RtcRdByte();         // Read data from Serial pipe
    output_low(RtcRstBit);      // Lower RTC Reset pin to dis-able interface
    return(data);               // Return the result
}
//------------------------------------------------------------------------------

