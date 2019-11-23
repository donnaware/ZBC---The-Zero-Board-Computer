//----------------------------------------------------------------------------
// Module:      simple_pic.v
// Description: Super Simple Priority Interrupt Controller  
// 
// This module controls the hardware interrupts. A table is set up in lower
// RAM that contains  4 bytes per vector and continues up to 0xFF. The first
// 8 slots are assigned here The program counter is loaded with the address 4 
// times the INT. E.g. IRQ7 pointer is 4 * 0x0f = 0x3C.
//
// Set the interupt vectors in the lower 256 bytes of memory
// INT 0x00 - Reset - SYSTEM RESET; 
// INT 0x04 - NMI   - Non-maskable interrupt 
// INT 0x08 - IRQ0  - SYSTEM TIMER 
// INT 0x09 - IRQ1  - KEYBOARD DATA READY
// INT 0x0A - IRQ2  - LPT2/EGA,VGA/IRQ9
// INT 0x0B - IRQ3  - SERIAL COMMUNICATIONS (COM2)
// INT 0x0C - IRQ4  - SERIAL COMMUNICATIONS (COM1)
// INT 0x0D - IRQ5  - FIXED DISK/LPT2/reserved
// INT 0x0E - IRQ6  - DISKETTE CONTROLLER
// INT 0x0F - IRQ7  - PARALLEL PRINTER, (we are are going to steal this one)
//
//----------------------------------------------------------------------------
module simple_pic (
    input             clk,
    input             rst,
    input       [7:0] intv,
    input             inta,
    output            intr,
    output reg  [2:0] iid
  );

  reg       inta_r;
  reg [7:0] irr;
  reg [7:0] int_r;

  assign intr = irr[7] | irr[6] | irr[5] | irr[4] | irr[3] | irr[2] | irr[1] | irr[0];

  always @(posedge clk) inta_r <= inta;  

  // the first 2 are handled differently, depends on ISR always being present
  //  always @(posedge clk) irr[0] <= rst ? 1'b0 : (intv[0] | irr[0] & !(iid == 3'b000 && inta_r && !inta));

  always @(posedge clk) int_r[0] <= rst ? 1'b0 : intv[0];		// int0 
  always @(posedge clk) int_r[1] <= rst ? 1'b0 : intv[1];		// int1
  always @(posedge clk) int_r[2] <= rst ? 1'b0 : intv[2];		// int2
  always @(posedge clk) int_r[3] <= rst ? 1'b0 : intv[3];		// int3
  always @(posedge clk) int_r[4] <= rst ? 1'b0 : intv[4];		// int4
  always @(posedge clk) int_r[5] <= rst ? 1'b0 : intv[5];		// int5
  always @(posedge clk) int_r[6] <= rst ? 1'b0 : intv[6];		// int6
  always @(posedge clk) int_r[7] <= rst ? 1'b0 : intv[7];		// int7
  
  always @(posedge clk) irr[0] <= rst ? 1'b0 : ((intv[0] && !int_r[0]) | irr[0] & !(iid == 3'd0 && inta_r && !inta));
  always @(posedge clk) irr[1] <= rst ? 1'b0 : ((intv[1] && !int_r[1]) | irr[1] & !(iid == 3'd1 && inta_r && !inta));
  always @(posedge clk) irr[2] <= rst ? 1'b0 : ((intv[2] && !int_r[2]) | irr[2] & !(iid == 3'd2 && inta_r && !inta));
  always @(posedge clk) irr[3] <= rst ? 1'b0 : ((intv[3] && !int_r[3]) | irr[3] & !(iid == 3'd3 && inta_r && !inta));
  always @(posedge clk) irr[4] <= rst ? 1'b0 : ((intv[4] && !int_r[4]) | irr[4] & !(iid == 3'd4 && inta_r && !inta));
  always @(posedge clk) irr[5] <= rst ? 1'b0 : ((intv[5] && !int_r[5]) | irr[5] & !(iid == 3'd5 && inta_r && !inta));
  always @(posedge clk) irr[6] <= rst ? 1'b0 : ((intv[6] && !int_r[6]) | irr[6] & !(iid == 3'd6 && inta_r && !inta));
  always @(posedge clk) irr[7] <= rst ? 1'b0 : ((intv[7] && !int_r[7]) | irr[7] & !(iid == 3'd7 && inta_r && !inta));

  always @(posedge clk)                      // Set the interrupt ID
    iid <= rst ? 3'b0 : (inta ? iid :
                        (irr[0] ? 3'b000 :
                        (irr[1] ? 3'b001 :
                        (irr[2] ? 3'b010 :
                        (irr[3] ? 3'b011 :
                        (irr[4] ? 3'b100 : 
                        (irr[5] ? 3'b101 : 
                        (irr[6] ? 3'b110 : 
                        (irr[7] ? 3'b111 : 
                                  3'b000 )))))))));

//----------------------------------------------------------------------------
endmodule
//----------------------------------------------------------------------------
