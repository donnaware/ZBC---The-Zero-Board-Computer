//------------------------------------------------------------------------------
// Library for an SST25V DataFlash                             
//
// init_STFlash() - Initializes the pins that control the flash device. This must  
//                  be called before any other flash function is used.             
//
// void STFlash_startContinuousRead(p, i) - Initiate a continuous read starting 
//                                          with page p at index i
//
// BYTE STFlash_getByte() - Gets a byte of data from the flash device                     
//                          Use after calling STFlash_startContinuousRead()              
//
// void STFlash_getBytes(a, n) - Read n bytes and store in array a                              
//                               Use after calling STFlash_startContinuousRead()              
//
// void STFlash_stopContinuousRead() - Use to stop continuously reading data 
//                                     from the flash device
//
// void STFlash_readBuffer(b, i, a, n) - Read n bytes from buffer b at index i
//                                       and store in array a
//
// BYTE STFlash_readStatus() - Return the status of the flash device:  
//                             Rdy/Busy Comp 0101XX
//
// void STFlash_writeToBuffer(b, i, a, n) - Write n bytes from array a to 
//                                          buffer b at index i
//
// void STFlash_eraseBlock(b) - Erase all bytes in block b to 0xFF. A block is 256.    
// 
// void STFlash_waitUntilReady() - Waits until the flash device is ready to accept commands    
//                                                               
// The main program may define FLASH_SELECT, FLASH_CLOCK,   
// FLASH_DI, and FLASH_DO to override the defaults below.  
//                                      
//                       Pin Layout                         
//   ---------------------------------------------------    
//   |    __                                           | 
//   | 1: CS    FLASH_SELECT   | 8: VCC  +2.7V - +3.6V | 
//   |                         |    ____               |  
//   | 2: SO   FLASH_DO        | 7: HOLD  Hold         |   
//   |    ___                  |                       |    
//   | 3: WP    Write Protect  | 6: SCK   FLASH_CLOCK  |              
//   |                         |    __                 |              
//   | 4: Vss   Ground         | 5: SI     FLASH_DI    |              
//   ---------------------------------------------------              
//                                                                    
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//     * * * USER CONFIGURATION Section, set these per Hardware set up * * *
//------------------------------------------------------------------------------
// #define     FLASH_SIZE   2097152  // The size of the flash device in bytes

//------------------------------------------------------------------------------
// Purpose:       Initialize the pins that control the flash device.
//                This must be called before any other flash function is used.
// Inputs:        None
// Outputs:       None
// Dependencies:  None
//------------------------------------------------------------------------------
void Init_STFlash(void)
{
    Set_Tris_B(TRISB_Master);     // Flash Disabled, turn on output pins
    Set_Tris_C(TRISC_Master);     // Set up for PIC being Master 
    output_high(FLASH_SELECT);    // FLASH_SELECT high
    output_high(FLASH_CLOCK);     // Clock High
    
}

//------------------------------------------------------------------------------
// Purpose:       Disables Flash Functions to yeild to FPGA by tri-stating 
//                FLASH_SELECT Line
// Inputs:        None
// Outputs:       None
// Dependencies:  None
//------------------------------------------------------------------------------
void Disable_STFlash(void)
{
    Set_Tris_C(TRISC_Disable);     // Set up for PIC being Master 
    Set_Tris_B(TRISB_Disable);     // Flash Disabled, output pins Tristated
}

//------------------------------------------------------------------------------
// Purpose:       Send data Byte to the flash device
// Inputs:        1 byte of data
// Outputs:       None
// Dependencies:  None
//------------------------------------------------------------------------------
void STFlash_SendByte(int data)
{
    int i;
    for(i=0; i<8; ++i) {
        output_bit(FLASH_DI, shift_left(&data,1,0));    // Send a data bit
        output_low(FLASH_CLOCK);
        delay_cycles(10);                                // Same as a NOP
        output_high(FLASH_CLOCK);                       // Pulse the clock
   }
}

//------------------------------------------------------------------------------
// Purpose:       Receive data Byte from the flash device
// Inputs:        None
// Outputs:       1 byte of data
// Dependencies:  Must enter with Clock high (preceded by a send)
//------------------------------------------------------------------------------
int STFlash_GetByte(void)
{
    int i, flashData;
    for(i=0; i<8; ++i) {
        output_low(FLASH_CLOCK);
        shift_left(&flashData, 1, input(FLASH_DO));      
        output_high(FLASH_CLOCK);                       // Pulse the clock
    }
   return(flashData);
}

//------------------------------------------------------------------------------
// Purpose:       Wait until the flash device is ready to accept commands
// Inputs:        None
// Outputs:       None
// Dependencies:  STFlash_sendData()
//------------------------------------------------------------------------------
void STFlash_waitUntilReady(void)
{
/*
   output_low(FLASH_SELECT);               // Enable select line
   STFlash_sendByte(0x05);                 // Send status command
   while(input(FLASH_DO));                 // Wait until ready
   STFlash_GetByte();                      // Get byte 
   output_high(FLASH_SELECT);              // Disable select line
*/   
}

//------------------------------------------------------------------------------
// Purpose:       Return the Read status Register of the flash device
// Inputs:        None            ____
// Outputs:       The Read status
// Dependencies:  STFlash_sendData(), STFlash_getByte()
//------------------------------------------------------------------------------
int STFlash_readStatus()
{
   int status;
   output_low(FLASH_SELECT);            // Enable select line
   STFlash_SendByte(0x05);              // Send status command
   status = STFlash_GetByte();          // Get the status
   output_high(FLASH_SELECT);           // Disable select line
   return(status);                      // Return the status
}

//----------------------------------------------------------------------------
// Purpose:       Enable Page Program write
// Inputs:        None.
// Outputs:       None.
// Dependencies:  STFlash_sendData(), STFlash_waitUntilReady()
//----------------------------------------------------------------------------
void STFlash_WriteEnable(void)
{
   output_low(FLASH_SELECT);            // Enable select line
   STFlash_sendByte(0x06);              // Send opcode
   output_high(FLASH_SELECT);           // Disable select line
}

//----------------------------------------------------------------------------
// Purpose:       Disable Page Program write
// Inputs:        None.
// Outputs:       None.
// Dependencies:  STFlash_sendData(), STFlash_waitUntilReady()
//----------------------------------------------------------------------------
void STFlash_WriteDisable(void)
{
   output_low(FLASH_SELECT);            // Enable select line
   STFlash_sendByte(0x04);              // Send opcode
   output_high(FLASH_SELECT);           // Disable select line
}

//------------------------------------------------------------------------------
// Purpose:       Write a byte to the status register of the flash device
// Inputs:        None
// Outputs:       None
// Dependencies:  STFlash_sendData(), STFlash_getByte()
//------------------------------------------------------------------------------
void STFlash_writeStatus(int value)
{
    STFlash_WriteEnable();

    output_low(FLASH_SELECT);           // Enable select line
    STFlash_sendByte(0x01);             // Send status command
    STFlash_sendByte(value);            // Send status value
    output_high(FLASH_SELECT);          // Disable select line

    STFlash_WriteDisable();
}

//------------------------------------------------------------------------------
// Purpose:       Get a byte of data from the flash device. This function is
//                meant to be used after STFlash_startContinuousRead() has
//                been called to initiate a continuous read. This function is
//                also used by STFlash_readPage() and STFlash_readBuffer().
// Inputs:        1) A pointer to an array to fill
//                2) The number of bytes of data to read
// Outputs:       None
// Dependencies:  None
//------------------------------------------------------------------------------
void STFlash_getBytes(int *data, int16 size)
{
    int16 i;
    signed int  j;
   
    for(i=0; i<size; ++i) {
        for(j=0; j<8; ++j) {
            output_low(FLASH_CLOCK);
            shift_left(data+i, 1, input(FLASH_DO));
            output_high(FLASH_CLOCK);
        }
    }
}

//------------------------------------------------------------------------------
// STFlash_ReadBlock()
//
// Purpose:       Reads a block of data (usually 256 bytes) from the ST
//                Flash and into a buffer pointed to by int Buffer;
//
// Inputs:        1) Address of block to read from
//                2) A pointer to an array to fill
//                3) The number of bytes of data to read
// Outputs:       None
//------------------------------------------------------------------------------
void STFlash_ReadBlock(int32 Address, int* buffer, int16 size)
{
    output_low(FLASH_SELECT);                 // Enable select line
    STFlash_sendByte(0x03);                   // Send opcode
    STFlash_sendByte(Make8(Address, 2));    // Send address 
    STFlash_sendByte(Make8(Address, 1));    // Send address
    STFlash_sendByte(Make8(Address, 0));    // Send address
    STFlash_getBytes(buffer, size);
    output_high(FLASH_SELECT);                // Disable select line
}

//------------------------------------------------------------------------------
// Purpose:       Send some bytes of data to the flash device
// Inputs:        1) A pointer to an array of data to send
//                2) The number of bytes to send
// Outputs:       None
// Dependencies:  None
//------------------------------------------------------------------------------
void STFlash_Write1Byte(int32 Address, int data)
{
    output_low(FLASH_SELECT);               // Enable select line
    STFlash_sendByte(0x02);                 // Send Opcode
    STFlash_sendByte(Make8(Address, 2));    // Send Address 
    STFlash_sendByte(Make8(Address, 1));    // Send Address
    STFlash_sendByte(Make8(Address, 0));    // Send Address
    STFlash_SendByte(data);                 // Send Data
    output_high(FLASH_SELECT);              // Disable select line
}

//------------------------------------------------------------------------------
// STFlash_WriteBlock()
//
// Purpose:       Writes a block of data (usually 256 bytes) to the ST
//                Flash from a buffer pointed to by int Buffer;
//
// Inputs:        1) Address of block to read from
//                2) A pointer to an array to fill
//                3) The number of bytes of data to read
// Outputs:       None
//------------------------------------------------------------------------------
void STFlash_WriteBlock(int32 Address, int buffer[], int16 size)
{
    int16 i;
    for(i = 0; i < size; i++) {
        STFlash_WriteEnable();
        STFlash_Write1Byte(Address+i, buffer[i]);
        delay_us(10);
        while(STFlash_readStatus() & 0x01);
    }
    STFlash_WriteDisable();
}

//------------------------------------------------------------------------------
// STFlash_EraseBlock()
//
// Purpose:       Erase a block of data (usually 256 bytes)
//
// Inputs:        1) Address of block to erase
// Outputs:       None
//------------------------------------------------------------------------------
void STFlash_EraseBlock(int32 Address)
{
    STFlash_WriteEnable();

    output_low(FLASH_SELECT);                // Enable select line
    STFlash_sendByte(0xD8);                  // Send opcode
    STFlash_sendByte(Make8(Address, 2));     // Send address 
    STFlash_sendByte(Make8(Address, 1));     // Send address
    STFlash_sendByte(Make8(Address, 0));     // Send address
    output_high(FLASH_SELECT);                // Disable select line

    STFlash_WriteDisable();
}

//----------------------------------------------------------------------------
//  End .h
//----------------------------------------------------------------------------
