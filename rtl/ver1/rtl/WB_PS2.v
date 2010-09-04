// --------------------------------------------------------------------
// --------------------------------------------------------------------
// Module:      WB_PS2.v
// Description: Wishbone Compatible PS2 core.
// --------------------------------------------------------------------
// --------------------------------------------------------------------
module WB_PS2(
	input              wb_clk_i,         // Clock Input
	input              wb_rst_i,         // Reset Input
	input       [15:0] wb_dat_i,         // Command to send to mouse
	output      [15:0] wb_dat_o,         // Received data
	input              wb_cyc_i,         // Cycle
	input              wb_stb_i,         // Strobe
	input       [ 2:1] wb_adr_i,         // Wishbone address lines
	input       [ 1:0] wb_sel_i,         // Wishbone Select lines
	input              wb_we_i,          // Write enable
	output             wb_ack_o,         // Normal bus termination
	output             wb_tgk_o,         // Interrupt request
	output             wb_tgm_o,         // Interrupt request
   
	inout              PS2_KBD_CLK,      // PS2 Keyboard Clock, Bidirectional
	inout              PS2_KBD_DAT,      // PS2 Keyboard Data, Bidirectional
	inout              PS2_MSE_CLK,      // PS2 Mouse Clock, Bidirectional
	inout              PS2_MSE_DAT       // PS2 Mouse Data, Bidirectional
);

// --------------------------------------------------------------------
// --------------------------------------------------------------------
// This section is a simple WB interface
// --------------------------------------------------------------------
// --------------------------------------------------------------------
wire   [7:0] dat_i       =  wb_sel_i[0] ? wb_dat_i[7:0]  : wb_dat_i[15:8]; // 8 to 16 bit WB
assign       wb_dat_o    =  wb_sel_i[0] ? {8'h00, dat_o} : {dat_o, 8'h00}; // 8 to 16 bit WB
wire   [2:0] wb_ps2_addr = {wb_adr_i,   wb_sel_i[1]};	// Computer UART Address
wire         wb_ack_i    =  wb_stb_i &  wb_cyc_i;		// Immediate ack
assign       wb_ack_o    =  wb_ack_i;
wire         write_i     =  wb_ack_i &  wb_we_i;		// WISHBONE write access, Singal to send
wire         read_i      =  wb_ack_i & ~wb_we_i;		// WISHBONE write access, Singal to send
assign       wb_tgm_o    =  MSE_INT & PS_INT2;			// Mouse Receive interupts ocurred
assign       wb_tgk_o    =  KBD_INT & PS_INT;			// Keyboard Receive interupts ocurred

// --------------------------------------------------------------------
// --------------------------------------------------------------------
// This section is a simple front end for the PS2 Mouse, it is NOT 100% 
// 8042 compliant but is hopefully close enough to work for most apps.
// There are two variants in common use, the AT style and the PS2 style 
// Interface, this is an implementation of the PS2 style which seems to be
// The most common. Reference: http://www.computer-engineering.org/ps2keyboard/
//
//  |7|6|5|4|3|2|1|0|  PS2 Status Register
//   | | | | | | | `-- PS_IBF  - Input  Buffer Full-- 0: Empty, No unread input at port, 1: Data available for host to read
//   | | | | | | `---- PS_OBF  - Output Buffer Full-- 0: Output Buffer Empty, 1: data in buffer being processed
//   | | | | | `------ PS_SYS  - System flag-- POST read this to determine if power-on reset, 0: in reset, 1: BAT code received - System has already beed initialized
//   | | | | `-------- PS_A2   - Address line A2-- Used internally by kbd controller, 0: A2 = 0 - Port 0x60 was last written to, 1: A2 = 1 - Port 0x64 was last written to 
//   | | | `---------- PS_INH  - Inhibit flag-- Indicates if kbd communication is inhibited; 0: Keyboard Clock = 0 - Keyboard is inhibited , 1: Keyboard Clock = 1 - Keyboard is not inhibited 
//   | | `------------ PS_MOBF - Mouse Output Buffer Full; 0: Output buffer empty, 1: Output buffer full
//   | `-------------- PS_TO   - General Timout-- Indicates timeout during command write or response. (Same as TxTO + RxTO.) 
//   `---------------- RX_PERR - Parity Error-- Indicates communication error with keyboard (possibly noisy/loose connection) 
//
//  |7|6|5|4|3|2|1|0|  PS2 Control Byte
//   | | | | | | | `-- PS_INT  - Input Buffer Full Interrupt-- When set, IRQ 1 is generated when data is available in the input buffer. 
//   | | | | | | `---- PS_INT2 - Mouse Input Buffer Full Interrupt - When set, IRQ 12 is generated when mouse data is available. 
//   | | | | | `------ PS_SYSF - System Flag-- Used to manually set/clear SYS flag in Status register. 
//   | | | | `--------         - No assignment
//   | | | `---------- PS_EN   - Disable keyboard-- Disables/enables keyboard interface
//   | | `------------ PS_EN2  - Disable Mouse-- Disables/enables mouse interface
//   | `-------------- PS_XLAT - Translate Scan Codes - Enables/disables translation to set 1 scan codes
//   `----------------         - No assignment
//
// --------------------------------------------------------------------


// --------------------------------------------------------------------
// Status Register and Wires 
// --------------------------------------------------------------------
wire		PS_IBF  = IBF;					// 0: Empty, No unread input at port,  1: Full, New input can be read from port 0x60 
wire		PS_OBF  = KBD_Txdone;			// 0: Ok to write to port 0x60  1: Full, Don't write to port 0x60
wire		PS_SYS  = 1'b1;					// 1: Always 1 cuz this is fpga so will always be initialized
wire		PS_A2   = 1'b0;					// 0: A2 = 0 - Port 0x60 was last written to, 1: A2 = 1 - Port 0x64 was last written to 
wire		PS_INH  = 1'b1;					// 0: Keyboard Clock = 0 - Keyboard is inhibited , 1: Keyboard Clock = 1 - Keyboard is not inhibited 
wire		PS_MOBF = MSE_DONE;				// 0: Buffer empty - Okay to write to auxillary device's output buffer, 1: Output buffer full - Don't write to port auxillary device's output buffer 
wire		PS_TO   = MSE_TOER;				// 0: No Error - Keyboard received and responded to last command, 1: Timeout Error - See TxTO and RxTO for more information. 
wire		RX_PERR = MSE_OVER;				// 0: No Error - Odd parity received and proper command response recieved, 1: Parity Error - Even parity received or 0xFE received as command response. 

wire [7:0]  PS_STAT = {RX_PERR, PS_TO, PS_MOBF, PS_INH, PS_A2, PS_SYS, PS_OBF, PS_IBF};		// Status Register

// --------------------------------------------------------------------
// Control Register and Wires 
// --------------------------------------------------------------------
reg  [7:0]  PS_CNTL;				// Control Register
wire        PS_INT  = PS_CNTL[0];	// 0: IBF Interrupt Disabled, 1: IBF Interrupt Enabled - Keyboard driver at software int 0x09 handles input.
wire        PS_INT2 = PS_CNTL[1];	// 0: Auxillary IBF Interrupt Disabled, 1: Auxillary IBF Interrupt Enabled 
wire        PS_SYSF = PS_CNTL[2];	// 0: Power-on value - Tells POST to perform power-on tests/initialization. 1: BAT code received - Tells POST to perform "warm boot" tests/initiailization. 
wire        PS_EN   = PS_CNTL[4];	// 0: Enable - Keyboard interface enabled. 1: Disable - All keyboard communication is disabled. 
wire        PS_EN2  = PS_CNTL[5];	// 0: Enable - Auxillary PS/2 device interface enabled 1: Disable - Auxillary PS/2 device interface disabled 
wire        PS_XLAT = PS_CNTL[6];	// 0: Translation disabled - Data appears at input buffer exactly as read from keyboard 1: Translation enabled - Scan codes translated to set 1 before put in input buffer 

`define     default_cntl  8'b0100_0111

// --------------------------------------------------------------------
// Behaviour for Command Register 
// The PS2 has this funky command byte structure:
//
// - If you write 0x60 to 0x64 port and they next byte you write to port 0x60
// is stored as the command byte (nice and confusing).
//
// - If you write 0x20 to port 0x64, the next byte you read from port
// 0x60 is the command byte read back. 
//
// - If you read from 0x64, you get the status
//
// - if you read 0x60, that is either mouse or keyboard data, depending
// on the last command sent
//
// - if you write data to 0x60, either mouse or keyboard data is transmitted
// to either the mouse or keyboard depending on the last command.
//
// Right now, we do not allow sending data to the keyboard, that maybe
// will change later on.
// 
// --------------------------------------------------------------------
// Controller Commands:
//           ,------------------------ - Currently Supported Command 
//           |
// 0x20      X Read control lines      - Next byte read from 0x60 is control line info
// 0x60      X Write to control lines  - Next byte writen to 0x60 is control line info
// 0x90-0x9F _ Write to output port    - Writes command's lower nibble to lower nibble of output port 
// 0xA1      _ Get version number      - Returns firmware version number. 
// 0xA4      _ Get password            - Returns 0xFA if password exists; otherwise, 0xF1. 
// 0xA5      _ Set password            - Set the new password by sending a null-terminated string of scan codes as this command's parameter. 
// 0xA6      _ Check password          - Compares keyboard input with current password. 
// 0xA7      _ Disable mouse interface - PS/2 mode only.  Similar to "Disable keyboard interface" (0xAD) command. 
// 0xA8      _ Enable mouse interface  - PS/2 mode only.  Similar to "Enable keyboard interface" (0xAE) command. 
// 0xA9      X Mouse interface test    - Returns 0x00 if okay, 0x01 if Clock line stuck low, 0x02 if clock line stuck high, 0x03 if data line stuck low, and 0x04 if data line stuck high. 
// 0xAA      X Controller self-test    - Returns 0x55 if okay. 
// 0xAB      _ Keyboard interface test - Returns 0x00 if okay, 0x01 if Clock line stuck low, 0x02 if clock line stuck high, 0x03 if data line stuck low, and 0x04 if data line stuck high. 
// 0xAD      _ Disable keybrd interface- Sets bit 4 of command byte and disables all communication with keyboard. 
// 0xAE      _ Enable keybrd interface - Clears bit 4 of command byte and re-enables communication with keyboard. 
// 0xAF      _ Get version              
// 0xC0      _ Read input port         - Returns values on input port (see Input Port definition.) 
// 0xC1      _ Copy input port LSn     - PS/2 mode only. Copy input port's low nibble to Status register (see Input Port definition) 
// 0xC2      _ Copy input port MSn     - PS/2 mode only. Copy input port's high nibble to Status register (see Input Port definition.) 
// 0xD0      _ Read output port        - Returns values on output port (see Output Port definition.)  
// 0xD1      _ Write output port       - Write parameter to output port (see Output Port definition.) 
// 0xD2      _ Write keyboard buffer   - Parameter written to input buffer as if received from keyboard. 
// 0xD3      _ Write mouse buffer      - Parameter written to input buffer as if received from mouse. 
// 0xD4      X Write mouse Device      - Sends parameter to the auxillary PS/2 device. 
// 0xE0      _ Read test port          - Returns values on test port (see Test Port definition.) 
// 0xF0-0xFF _ Pulse output port       - Pulses command's lower nibble onto lower nibble of output port (see Output Port definition.) 
// --------------------------------------------------------------------
`define PS2_CMD_A9		8'hA9		// Mouse Interface test
`define PS2_CMD_AA		8'hAA		// Controller self test
`define PS2_CMD_D4		8'hD4		// Write to mouse
`define PS2_CNT_RD		8'h20		// Read command byte
`define PS2_CNT_WR		8'h60		// Write control byte

`define PS2_DAT_REG		3'b000		// 0x60 - RW Transmit / Receive register
`define PS2_CMD_REG		3'b100		// 0x64 - RW - Status / command register

wire        DAT_SEL  = (wb_ps2_addr == `PS2_DAT_REG);
wire        DAT_wr   = DAT_SEL && write_i;
wire        DAT_rd   = DAT_SEL && read_i;

wire        CMD_SEL  = (wb_ps2_addr == `PS2_CMD_REG);
wire        CMD_wr   = CMD_SEL && write_i;
wire        CMD_rdc  = CMD_wr  && (dat_i == `PS2_CNT_RD);	// Request to read control info
wire        CMD_wrc  = CMD_wr  && (dat_i == `PS2_CNT_WR);	// Request to write control info
wire		CMD_mwr  = CMD_wr  && (dat_i == `PS2_CMD_D4);	// Signal to transmit data to mouse
wire		CMD_tst  = CMD_wr  && (dat_i == `PS2_CMD_AA);	// User requested self test
wire		CMD_mit  = CMD_wr  && (dat_i == `PS2_CMD_A9);	// User mouse interface test

// --------------------------------------------------------------------
// Command Behavior
// --------------------------------------------------------------------
wire [7:0]  dat_o    = DAT_SEL    ? r_dat_o   : PS_STAT;	// Select register
wire [7:0]	r_dat_o  = cnt_r_flag ? PS_CNTL   : t_dat_o;	// return control or data 
wire [7:0]	t_dat_o  = cmd_r_test ? ps_tst_o  : i_dat_o;	// return control or data 
wire [7:0]	i_dat_o  = cmd_r_mint ? ps_mit_o  : p_dat_o;	// return control or data 
wire [7:0]	p_dat_o  = MSE_INT    ? MSE_dat_o : KBD_dat_o;	// defer to mouse
wire [7:0]	ps_tst_o = 8'h55;								// Controller self test
wire [7:0]	ps_mit_o = 8'h00;								// Controller self test

wire 		cmd_msnd = cmd_w_msnd && DAT_wr;	// OK to write to mouse

wire		IBF = MSE_INT || KBD_INT || cnt_r_flag || cmd_r_test || cmd_r_mint;

// --------------------------------------------------------------------
// Behavior of Control Register
// --------------------------------------------------------------------
reg	cnt_r_flag;							// Read Control lines flag
reg	cnt_w_flag;							// Write to Control lines flag
reg	cmd_w_msnd;							// Signal to send to mouse flag
reg cmd_r_test;							// Signal to send test flag
reg cmd_r_mint;							// Signal to send test flag
always @(posedge wb_clk_i) begin		// Synchrounous
    if(wb_rst_i) begin
		PS_CNTL     <= `default_cntl; 	// Set initial default value
		cnt_r_flag	<= 1'b0;				// Reset the flag
		cnt_w_flag	<= 1'b0;				// Reset the flag
		cmd_w_msnd  <= 1'b0;				// Reset the flag
		cmd_r_test  <= 1'b0;				// Reset the flag
		cmd_r_mint  <= 1'b0;				// Reset the flag
    end									
    else
		if(CMD_rdc) begin
			cnt_r_flag <= 1'b1;		// signal next read from 0x60 is control info
		end
    else
		if(CMD_wrc) begin
			cnt_w_flag <= 1'b1;				// signal next write to  0x60 is control info
			cmd_w_msnd <= 1'b0;				// Reset the flag
		end
    else
		if(CMD_mwr) begin
			cmd_w_msnd <= 1'b1;		// signal next write to  0x60 is mouse info
		end
    else
		if(CMD_tst) begin
			cmd_r_test <= 1'b1;		// signal next read from 0x60 is test info
		end
    else
		if(CMD_mit) begin
			cmd_r_mint <= 1'b1;		// signal next read from 0x60 is test info
		end

    else
		if(DAT_rd) begin
			if(cnt_r_flag) cnt_r_flag <= 1'b0;		// Reset the flag
			if(cmd_r_test) cmd_r_test <= 1'b0;		// Reset the flag
			if(cmd_r_mint) cmd_r_mint <= 1'b0;		// Reset the flag
		end
    else
		if(DAT_wr) begin
			if(cnt_w_flag) begin
				PS_CNTL		<= dat_i;				// User requested to write control info
				cnt_w_flag	<= 1'b0;				// Reset the flag
			end
		end
	
	if(cmd_w_msnd && MSE_DONE) cmd_w_msnd <= 1'b0;		// Reset the flag
	
	
end  // Synchrounous always

// --------------------------------------------------------------------
// --------------------------------------------------------------------
// Mouse Transceiver Section
// --------------------------------------------------------------------
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Mouse Receive Interrupt behavior
// --------------------------------------------------------------------
reg 	MSE_INT;						// Mouse Receive interrupt signal
wire	PS_READ = DAT_rd && !(cnt_r_flag || cmd_r_test || cmd_r_mint);

always @(posedge wb_clk_i or posedge wb_rst_i) begin  // Synchrounous
    if(wb_rst_i) MSE_INT <= 1'b0;                     // Default value
    else begin
        if(MSE_RDY) MSE_INT <= 1'b1;      // Latch interrupt
        if(PS_READ) MSE_INT <= 1'b0;      // Clear interrupt
    end
end  // Synchrounous always

// --------------------------------------------------------------------
// Instantiate the PS2 UART for MOUSE
// --------------------------------------------------------------------
wire [7:0]  MSE_dat_o;				// Receive Register
wire [7:0]  MSE_dat_i = dat_i;		// Transmit register
wire        MSE_RDY;				// Signal data received
wire        MSE_DONE;				// Signal command finished sending
wire        MSE_TOER;           	// Indicates a Transmit error occured
wire        MSE_OVER;           	// Indicates buffer over run error
wire        MSE_SEND = cmd_msnd;	// Signal to transmit data

PS2_NO_FIFO ps2u1(
	.clk(wb_clk_i),
	.reset(wb_rst_i),
	.PS2_CLK(PS2_MSE_CLK),
	.PS2_DAT(PS2_MSE_DAT),
	
    .writedata(MSE_dat_i),				// data to send
    .write(MSE_SEND),					// signal to send it
    .command_was_sent(MSE_DONE),		// Done sending

    .readdata(MSE_dat_o),				// data read
    .irq(MSE_RDY),						// signal data has arrived and is ready to be read

    .error_sending_command(MSE_TOER),	// Time out error
    .buffer_overrun_error(MSE_OVER)		// Buffer over run error
);

// --------------------------------------------------------------------
// --------------------------------------------------------------------
// Keyboard Receiver Section
// --------------------------------------------------------------------
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Instantiate the PS2 UART for KEYBOARD
// --------------------------------------------------------------------
wire       KBD_INT;
wire [7:0] KBD_dat_o;
wire	   KBD_Txdone;
wire	   KBD_Rxdone;

PS2_Keyboard #(
    .TIMER_60USEC_VALUE_PP (750),
    .TIMER_60USEC_BITS_PP  (10),
    .TIMER_5USEC_VALUE_PP  (60),
    .TIMER_5USEC_BITS_PP   (6)
 ) kbd(
			.clk(wb_clk_i), 
			.reset(wb_rst_i),
			.rx_shifting_done(KBD_Rxdone),		// done receivign
			.tx_shifting_done(KBD_Txdone),		// done transmiting
            .scancode(KBD_dat_o),               // scancode
            .rx_output_strobe(KBD_INT), 		// Signals a key presseed
            .ps2_clk_(PS2_KBD_CLK),             // PS2 PAD signals
            .ps2_data_(PS2_KBD_DAT)
);

// --------------------------------------------------------------------
// End of WB PS2 Module
// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// PS2 Wishbone 8042 compatible keyboard controller
// --------------------------------------------------------------------
module WB_PS2_Keyboard (
    input            wb_clk_i,   // Wishbone slave interface
    input            wb_rst_i,
    input     [ 2:1] wb_adr_i,
    output    [15:0] wb_dat_o,   // scancode
    input            wb_stb_i,
    input            wb_cyc_i,
    output           wb_ack_o,
    output reg       wb_tgc_o,   // intr

    inout            ps2_clk_,   // PS2 PAD signals
    inout            ps2_data_
);

assign wb_ack_o = wb_stb_i & wb_cyc_i;
assign wb_dat_o = wb_adr_i[2] ? 16'h10 : { 8'h0, dat_o };

// --------------------------------------------------------------------
// Behaviour
// --------------------------------------------------------------------
always @(posedge wb_clk_i) wb_tgc_o <= wb_rst_i ? 1'b0 : rx_output_strobe;

wire       rx_output_strobe;
wire [7:0] dat_o;

PS2_Keyboard #(
    .TIMER_60USEC_VALUE_PP (750),
    .TIMER_60USEC_BITS_PP  (10),
    .TIMER_5USEC_VALUE_PP  (60),
    .TIMER_5USEC_BITS_PP   (6)
 ) kbd(.clk(wb_clk_i), .reset(wb_rst_i),
             .scancode(dat_o),                    // scancode
             .rx_output_strobe(rx_output_strobe), // Signals a key presseed
             .ps2_clk_(ps2_clk_),                 // PS2 PAD signals
             .ps2_data_(ps2_data_)
);

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// PS2 8042 partially compatible keyboard controller
// (can not receive commands from host)
// --------------------------------------------------------------------
module PS2_Keyboard (
    output           released,
    output           rx_shifting_done,
    output           tx_shifting_done, 
    input            reset,               // Main reset line
    input            clk,                 // Main Clock
    output     [7:0] scancode,            // scancode
    output           rx_output_strobe,    // Signals a key presseed
    inout            ps2_clk_,            // PS2 PAD signals
    inout            ps2_data_
);

`timescale 1ns/100ps
`define TOTAL_BITS   11
`define RELEASE_CODE 16'hF0
`define LEFT_SHIFT   16'h12
`define RIGHT_SHIFT  16'h59

// --------------------------------------------------------------------
// Parameter declarations, the timer value can be up to (2^bits) inclusive.
// --------------------------------------------------------------------
parameter TIMER_60USEC_VALUE_PP = 1920; // Number of sys_clks for 60usec.
parameter TIMER_60USEC_BITS_PP  = 11;   // Number of bits needed for timer
parameter TIMER_5USEC_VALUE_PP  = 186;  // Number of sys_clks for debounce
parameter TIMER_5USEC_BITS_PP   = 8;    // Number of bits needed for timer
parameter TRAP_SHIFT_KEYS_PP    = 0;    // Default: No shift key trap.

// --------------------------------------------------------------------
// State encodings, provided as parametersf or flexibility to the one
// instantiating the module. In general, the default values need not be changed.
//
// State "m1_rx_clk_l" has been chosen on purpose.  Since the inputs ynchronizing
// flip-flops initially contain zero, it takes one clkfor them to update to reflect
// the actual (idle = high) status oft he I/O lines from the keyboard.  Therefore,
// choosing 0 for m1_rx_clk_la llows the state machine to transition to m1_rx_clk_h
// when the true values of the input signals become present at the outputs of the
// synchronizing flip-flops.  This initial transition is harmless, and it
// eliminates the need for a "reset" pulse before the interface can operate.
// --------------------------------------------------------------------
parameter m1_rx_clk_h = 1;
parameter m1_rx_clk_l = 0;
parameter m1_rx_falling_edge_marker = 13;
parameter m1_rx_rising_edge_marker = 14;
parameter m1_tx_force_clk_l = 3;
parameter m1_tx_first_wait_clk_h = 10;
parameter m1_tx_first_wait_clk_l = 11;
parameter m1_tx_reset_timer = 12;
parameter m1_tx_wait_clk_h = 2;
parameter m1_tx_clk_h = 4;
parameter m1_tx_clk_l = 5;
parameter m1_tx_wait_keyboard_ack = 6;
parameter m1_tx_done_recovery = 7;
parameter m1_tx_error_no_keyboard_ack = 8;
parameter m1_tx_rising_edge_marker = 9;

// --------------------------------------------------------------------
// Nets and registers
// --------------------------------------------------------------------
assign       scancode = dat_o;
wire         rx_output_event;
//assign		 tx_shifting_done;
wire         timer_60usec_done;
wire         timer_5usec_done;
wire   [6:0] xt_code;
reg    [7:0] dat_o;
reg    [3:0] bit_count;
reg    [3:0] m1_state;
reg    [3:0] m1_next_state;
reg          ps2_clk_hi_z;          // Without keyboard, high Z equals 1 due to pullups.
reg          ps2_data_hi_z;         // Without keyboard, high Z equals 1 due to pullups.
reg          ps2_clk_s;             // Synchronous version of this input
reg          ps2_data_s;            // Synchronous version of this input
reg          enable_timer_60usec;
reg          enable_timer_5usec;
reg          hold_released;       // Holds prior value, cleared at rx_output_strobe

reg [TIMER_60USEC_BITS_PP-1:0]  timer_60usec_count;
reg [TIMER_5USEC_BITS_PP-1 :0]  timer_5usec_count;
reg [`TOTAL_BITS-1:0]           q;

// --------------------------------------------------------------------
// Module instantiation
// --------------------------------------------------------------------
translate_8042 tr0(.at_code(q[7:1]), .xt_code (xt_code));

// --------------------------------------------------------------------
// Continuous assignments
// This signal is high for one clock at the end of the timer count.
// --------------------------------------------------------------------
assign rx_shifting_done = (bit_count == `TOTAL_BITS);
assign tx_shifting_done = (bit_count == `TOTAL_BITS-1);
assign rx_output_event  = (rx_shifting_done  && ~released );
assign rx_output_strobe = (rx_shifting_done  && ~released
                          && ( (TRAP_SHIFT_KEYS_PP == 0) || ( (q[8:1] != `RIGHT_SHIFT)
                                    &&(q[8:1] != `LEFT_SHIFT) ) ) );

assign ps2_clk_  = ps2_clk_hi_z  ? 1'bZ : 1'b0;
assign ps2_data_ = ps2_data_hi_z ? 1'bZ : 1'b0;
assign timer_60usec_done = (timer_60usec_count == (TIMER_60USEC_VALUE_PP - 1));
assign timer_5usec_done = (timer_5usec_count == TIMER_5USEC_VALUE_PP - 1);

// --------------------------------------------------------------------
// Create the signals which indicate special scan codes received.
// These are the "unlatched versions."
// assign extended = (q[8:1] == `EXTEND_CODE) && rx_shifting_done;
// --------------------------------------------------------------------
assign released = (q[8:1] == `RELEASE_CODE) && rx_shifting_done;

// --------------------------------------------------------------------
// This is the shift register
// --------------------------------------------------------------------
always @(posedge clk)
    if(reset) q <= 0;
    else 
    if((m1_state == m1_rx_falling_edge_marker) ||(m1_state == m1_tx_rising_edge_marker)) q <= {ps2_data_s,q[`TOTAL_BITS-1:1]};

// This is the 60usec timer counter
always @(posedge clk)
    if(~enable_timer_60usec) timer_60usec_count <= 0;
    else if (~timer_60usec_done) timer_60usec_count <= timer_60usec_count + 10'd1;

// This is the 5usec timer counter
always @(posedge clk)
    if (~enable_timer_5usec) timer_5usec_count <= 0;
    else if (~timer_5usec_done) timer_5usec_count <= timer_5usec_count + 6'd1;

// --------------------------------------------------------------------
// Input "synchronizing" logic -- synchronizes the inputs to the state
// machine clock, thus avoiding errors related to spurious state machine transitions.
//
// Since the initial state of registers is zero, and the idle state
// of the ps2_clk and ps2_data lines is "1" (due to pullups), the
// "sense" of the ps2_clk_s signal is inverted from the true signal.
// This allows the state machine to "come up" in the correct
always @(posedge clk) begin
    ps2_clk_s <= ps2_clk_;
    ps2_data_s <= ps2_data_;
end

// State transition logic
always @(m1_state
           or q
           or tx_shifting_done
           or ps2_clk_s
           or ps2_data_s
           or timer_60usec_done
           or timer_5usec_done
          )
begin : m1_state_logic

    // Output signals default to this value,
    //  unless changed in a state condition.
    ps2_clk_hi_z  <= 1;
    ps2_data_hi_z <= 1;
    enable_timer_60usec <= 0;
    enable_timer_5usec  <= 0;

    case (m1_state)

      m1_rx_clk_h :
      begin
        enable_timer_60usec <= 1;
        if (~ps2_clk_s)
          m1_next_state <= m1_rx_falling_edge_marker;
        else m1_next_state <= m1_rx_clk_h;
      end

      m1_rx_falling_edge_marker :
      begin
        enable_timer_60usec <= 0;
        m1_next_state <= m1_rx_clk_l;
      end

      m1_rx_rising_edge_marker :
      begin
        enable_timer_60usec <= 0;
        m1_next_state <= m1_rx_clk_h;
      end

      m1_rx_clk_l :
      begin
        enable_timer_60usec <= 1;
        if (ps2_clk_s)
          m1_next_state <= m1_rx_rising_edge_marker;
        else m1_next_state <= m1_rx_clk_l;
      end

      m1_tx_reset_timer :
      begin
        enable_timer_60usec <= 0;
        m1_next_state <= m1_tx_force_clk_l;
      end

      m1_tx_force_clk_l :
      begin
        enable_timer_60usec <= 1;
        ps2_clk_hi_z <= 0;  // Force the ps2_clk line low.
        if (timer_60usec_done)
          m1_next_state <= m1_tx_first_wait_clk_h;
        else m1_next_state <= m1_tx_force_clk_l;
      end

      m1_tx_first_wait_clk_h :
      begin
        enable_timer_5usec <= 1;
        ps2_data_hi_z <= 0;        // Start bit.
        if (~ps2_clk_s && timer_5usec_done)
          m1_next_state <= m1_tx_clk_l;
        else
          m1_next_state <= m1_tx_first_wait_clk_h;
      end

      // This state must be included because the device might possibly
      // delay for up to 10 milliseconds before beginning its clock pulses.
      // During that waiting time, we cannot drive the data (q[0]) because it
      // is possibly 1, which would cause the keyboard to abort its receive
      // and the expected clocks would then never be generated.
      m1_tx_first_wait_clk_l :
      begin
        ps2_data_hi_z <= 0;
        if (~ps2_clk_s) m1_next_state <= m1_tx_clk_l;
        else m1_next_state <= m1_tx_first_wait_clk_l;
      end

      m1_tx_wait_clk_h :
      begin
        enable_timer_5usec <= 1;
        ps2_data_hi_z <= q[0];
        if (ps2_clk_s && timer_5usec_done)
          m1_next_state <= m1_tx_rising_edge_marker;
        else
          m1_next_state <= m1_tx_wait_clk_h;
      end

      m1_tx_rising_edge_marker :
      begin
        ps2_data_hi_z <= q[0];
        m1_next_state <= m1_tx_clk_h;
      end

      m1_tx_clk_h :
      begin
        ps2_data_hi_z <= q[0];
        if (tx_shifting_done) m1_next_state <= m1_tx_wait_keyboard_ack;
        else if (~ps2_clk_s) m1_next_state <= m1_tx_clk_l;
        else m1_next_state <= m1_tx_clk_h;
      end

      m1_tx_clk_l :
      begin
        ps2_data_hi_z <= q[0];
        if (ps2_clk_s) m1_next_state <= m1_tx_wait_clk_h;
        else m1_next_state <= m1_tx_clk_l;
      end

      m1_tx_wait_keyboard_ack :
      begin
        if (~ps2_clk_s && ps2_data_s)
          m1_next_state <= m1_tx_error_no_keyboard_ack;
        else if (~ps2_clk_s && ~ps2_data_s)
          m1_next_state <= m1_tx_done_recovery;
        else m1_next_state <= m1_tx_wait_keyboard_ack;
      end

      m1_tx_done_recovery :
      begin
        if (ps2_clk_s && ps2_data_s) m1_next_state <= m1_rx_clk_h;
        else m1_next_state <= m1_tx_done_recovery;
      end

      m1_tx_error_no_keyboard_ack :
      begin
        if (ps2_clk_s && ps2_data_s) m1_next_state <= m1_rx_clk_h;
        else m1_next_state <= m1_tx_error_no_keyboard_ack;
      end

      default : m1_next_state <= m1_rx_clk_h;
    endcase
  end

  // State register
  always @(posedge clk)
  begin : m1_state_register
    if(reset) m1_state <= m1_rx_clk_h;
    else m1_state <= m1_next_state;
  end

  // dat_o - scancode
  always @(posedge clk)
    if(reset) dat_o <= 8'b0;
    else dat_o <= (rx_output_strobe && q[8:1]) ? (q[8] ? q[8:1] : {hold_released,xt_code}) : dat_o;

  // This is the bit counter
  always @(posedge clk)
    begin
      if(reset || rx_shifting_done  || (m1_state == m1_tx_wait_keyboard_ack) // After tx is done.
         ) bit_count <= 0;  // normal reset
      else if (timer_60usec_done && (m1_state == m1_rx_clk_h) && (ps2_clk_s)
              ) bit_count <= 0;  // rx watchdog timer reset
      else if ( (m1_state == m1_rx_falling_edge_marker) // increment for rx
              ||(m1_state == m1_tx_rising_edge_marker)  // increment for tx
              )
        bit_count <= bit_count + 4'd1;
  end

  // Store the special scan code status bits
  // Not the final output, but an intermediate storage place,
  // until the entire set of output data can be assembled.
  always @(posedge clk)
    if(reset || rx_output_event) hold_released <= 0;
    else if (rx_shifting_done && released) hold_released <= 1;

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Internal Module Definition
// --------------------------------------------------------------------
module translate_8042 (
    input      [6:0] at_code,
    output     [6:0] xt_code
);

// Registers, nets and parameters
reg [7:0] rom[0:2**7-1];

assign xt_code = rom[at_code][6:0];

// Behaviour
initial $readmemh("xt_codes.dat", rom);

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------


// --------------------------------------------------------------------
// Module:      PS2_FIFO
// Description: PS2 Mouse with FIFO buffer
// --------------------------------------------------------------------
module PS2_FIFO(
    input              clk,
    input              reset,

    input        [7:0] writedata,    // data to send
    input              write,        // signal to send it 

    output  reg  [7:0] readdata,     // data read
    input              read,         // request to read from FIFO
    output             irq,          // signal data has arrived

    output             command_was_sent,
    output             error_sending_command,
    output             buffer_overrun_error,

    inout              PS2_CLK,      // PS2 Mouse Clock Line
    inout              PS2_DAT       // PS2 Mouse Data Line
);

// --------------------------------------------------------------------
// Internal wires and registers Declarations
// --------------------------------------------------------------------
wire     [7:0]    data_from_the_PS2_port;
wire              data_from_the_PS2_port_en;
wire              data_fifo_is_empty;
wire              data_fifo_is_full;
wire              write_to_buffer       =  data_from_the_PS2_port_en & ~data_fifo_is_full;
assign            irq                   = ~data_fifo_is_empty;
assign            buffer_overrun_error  =  data_fifo_is_full;

// --------------------------------------------------------------------
// Internal Modules
// --------------------------------------------------------------------
PS2_Mouse PS2_Serial_Port(
    .clk                            (clk),
    .reset                          (reset),
    
    .the_command                    (writedata),
    .send_command                   (write),

    .received_data                  (data_from_the_PS2_port),
    .received_data_en               (data_from_the_PS2_port_en),

    .command_was_sent               (command_was_sent),
    .error_communication_timed_out  (error_sending_command),

    .PS2_CLK                        (PS2_CLK),
    .PS2_DAT                        (PS2_DAT)
);

// --------------------------------------------------------------------
// FIFO Data queue
// --------------------------------------------------------------------
scfifo Incoming_Data_FIFO (
    .clock           (clk),
    .sclr            (reset),
    .rdreq           (read & ~data_fifo_is_empty),
    .wrreq           (write_to_buffer),
    .data            (data_from_the_PS2_port),
    .q               (readdata),
    .empty           (data_fifo_is_empty),
    .full            (data_fifo_is_full)

                         // synopsys translate_off
    ,                    // un-used lines
    .usedw          (),
    .almost_empty   (),
    .almost_full    (),
    .aclr           ()   // synopsys translate_on
);
defparam
    Incoming_Data_FIFO.add_ram_output_register   = "ON",
    Incoming_Data_FIFO.intended_device_family    = "Cyclone II",
    Incoming_Data_FIFO.lpm_numwords              = 256,
    Incoming_Data_FIFO.lpm_showahead             = "ON",
    Incoming_Data_FIFO.lpm_type                  = "scfifo",
    Incoming_Data_FIFO.lpm_width                 = 8,
    Incoming_Data_FIFO.lpm_widthu                = 8,
    Incoming_Data_FIFO.overflow_checking         = "OFF",
    Incoming_Data_FIFO.underflow_checking        = "OFF",
    Incoming_Data_FIFO.use_eab                   = "ON";

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Module:      PS2_FIFO
// Description: PS2 Mouse with FIFO buffer
// --------------------------------------------------------------------
module PS2_NO_FIFO(
    input                clk,
    input                reset,

    input       [7:0]    writedata,   // data to send
    input                write,       // signal to send it

    output      [7:0]    readdata,    // data read
    input                read,        // request to read from FIFO
    output               irq,         // signal data has arrived

    output               command_was_sent,
    output               error_sending_command,
    output               buffer_overrun_error,

    inout                PS2_CLK,
    inout                PS2_DAT
);

// --------------------------------------------------------------------
// Internal wires and registers Declarations
// --------------------------------------------------------------------
assign     buffer_overrun_error = error_sending_command;

// --------------------------------------------------------------------
// Internal Modules
// --------------------------------------------------------------------
PS2_Mouse PS2_Serial_Port(
    .clk                            (clk),
    .reset                          (reset),
    
    .the_command                    (writedata),
    .send_command                   (write),

    .received_data                  (readdata),
    .received_data_en               (irq),

    .command_was_sent               (command_was_sent),
    .error_communication_timed_out  (error_sending_command),

    .PS2_CLK                        (PS2_CLK),
     .PS2_DAT                       (PS2_DAT)
);

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------


// --------------------------------------------------------------------
// Module:      PS2 Mouse
// Description: PS2 Mouse Interface
// --------------------------------------------------------------------
module PS2_Mouse(
    input            clk,                // Clock Input
    input            reset,              // Reset Input
    inout            PS2_CLK,            // PS2 Clock, Bidirectional
    inout            PS2_DAT,            // PS2 Data, Bidirectional

    input    [7:0]   the_command,        // Command to send to mouse
    input            send_command,       // Singal to send
    output           command_was_sent,   // Signal command finished sending
    output           error_communication_timed_out,

    output   [7:0]   received_data,        // Received data
    output           received_data_en,     // If 1 - new data has been received
    output           start_receiving_data,
    output           wait_for_incoming_data
);

// --------------------------------------------------------------------
// Internal wires and registers Declarations                 
// --------------------------------------------------------------------
wire            ps2_clk_posedge;        // Internal Wires
wire            ps2_clk_negedge;

reg    [7:0]    idle_counter;            // Internal Registers
reg             ps2_clk_reg;
reg             ps2_data_reg;
reg             last_ps2_clk;

reg    [2:0]    ns_ps2_transceiver;        // State Machine Registers
reg    [2:0]    s_ps2_transceiver;

// --------------------------------------------------------------------
// Constant Declarations                           
// --------------------------------------------------------------------
localparam  PS2_STATE_0_IDLE            = 3'h0,        // states
            PS2_STATE_1_DATA_IN         = 3'h1,
            PS2_STATE_2_COMMAND_OUT     = 3'h2,
            PS2_STATE_3_END_TRANSFER    = 3'h3,
            PS2_STATE_4_END_DELAYED     = 3'h4;

// --------------------------------------------------------------------
// Finite State Machine(s)                           
// --------------------------------------------------------------------
always @(posedge clk) begin
    if(reset == 1'b1) s_ps2_transceiver <= PS2_STATE_0_IDLE;
    else              s_ps2_transceiver <= ns_ps2_transceiver;
end

always @(*) begin
    ns_ps2_transceiver = PS2_STATE_0_IDLE;        // Defaults

    case (s_ps2_transceiver)
    PS2_STATE_0_IDLE:
        begin
            if((idle_counter == 8'hFF) && (send_command == 1'b1))
                ns_ps2_transceiver = PS2_STATE_2_COMMAND_OUT;
            else if ((ps2_data_reg == 1'b0) && (ps2_clk_posedge == 1'b1))
                ns_ps2_transceiver = PS2_STATE_1_DATA_IN;
            else ns_ps2_transceiver = PS2_STATE_0_IDLE;
        end
    PS2_STATE_1_DATA_IN:
        begin
//            if((received_data_en == 1'b1)  && (ps2_clk_posedge == 1'b1))
            if((received_data_en == 1'b1))   ns_ps2_transceiver = PS2_STATE_0_IDLE;
            else                             ns_ps2_transceiver = PS2_STATE_1_DATA_IN;
        end
    PS2_STATE_2_COMMAND_OUT:
        begin
            if((command_was_sent == 1'b1) || (error_communication_timed_out == 1'b1))
                ns_ps2_transceiver = PS2_STATE_3_END_TRANSFER;
            else ns_ps2_transceiver = PS2_STATE_2_COMMAND_OUT;
        end
    PS2_STATE_3_END_TRANSFER:
        begin
            if(send_command == 1'b0) ns_ps2_transceiver = PS2_STATE_0_IDLE;
            else if((ps2_data_reg == 1'b0) && (ps2_clk_posedge == 1'b1))
                ns_ps2_transceiver = PS2_STATE_4_END_DELAYED;
            else ns_ps2_transceiver = PS2_STATE_3_END_TRANSFER;
        end
    PS2_STATE_4_END_DELAYED:    
        begin
            if(received_data_en == 1'b1) begin
                if(send_command == 1'b0) ns_ps2_transceiver = PS2_STATE_0_IDLE;
                else                     ns_ps2_transceiver = PS2_STATE_3_END_TRANSFER;
            end
            else ns_ps2_transceiver = PS2_STATE_4_END_DELAYED;
        end    

    default:
            ns_ps2_transceiver = PS2_STATE_0_IDLE;
    endcase
end

// --------------------------------------------------------------------
// Sequential logic                              
// --------------------------------------------------------------------
always @(posedge clk) begin
    if(reset == 1'b1)     begin
        last_ps2_clk    <= 1'b1;
        ps2_clk_reg     <= 1'b1;
        ps2_data_reg    <= 1'b1;
    end
    else begin
        last_ps2_clk    <= ps2_clk_reg;
        ps2_clk_reg     <= PS2_CLK;
        ps2_data_reg    <= PS2_DAT;
    end
end

always @(posedge clk) begin
    if(reset == 1'b1) idle_counter <= 6'h00;
    else if((s_ps2_transceiver == PS2_STATE_0_IDLE) && (idle_counter != 8'hFF))
        idle_counter <= idle_counter + 6'h01;
    else if (s_ps2_transceiver != PS2_STATE_0_IDLE)
        idle_counter <= 6'h00;
end

// --------------------------------------------------------------------
// Combinational logic                            
// --------------------------------------------------------------------
assign ps2_clk_posedge = ((ps2_clk_reg == 1'b1) && (last_ps2_clk == 1'b0)) ? 1'b1 : 1'b0;
assign ps2_clk_negedge = ((ps2_clk_reg == 1'b0) && (last_ps2_clk == 1'b1)) ? 1'b1 : 1'b0;

assign start_receiving_data      = (s_ps2_transceiver == PS2_STATE_1_DATA_IN);
assign wait_for_incoming_data    = (s_ps2_transceiver == PS2_STATE_3_END_TRANSFER);

// --------------------------------------------------------------------
// Internal Modules                             
// --------------------------------------------------------------------
PS2_Mouse_Command_Out Mouse_Command_Out (
    .clk                            (clk),            // Inputs
    .reset                          (reset),
    .the_command                    (the_command),
    .send_command                   (send_command),
    .ps2_clk_posedge                (ps2_clk_posedge),
    .ps2_clk_negedge                (ps2_clk_negedge),
    .PS2_CLK                        (PS2_CLK),        // Bidirectionals
     .PS2_DAT                       (PS2_DAT),
    .command_was_sent               (command_was_sent),    // Outputs
    .error_communication_timed_out  (error_communication_timed_out)
);

PS2_Mouse_Data_In PS2_Data_In (
    .clk                            (clk),        // Inputs
    .reset                          (reset),
    .wait_for_incoming_data         (wait_for_incoming_data),
    .start_receiving_data           (start_receiving_data),
    .ps2_clk_posedge                (ps2_clk_posedge),
    .ps2_clk_negedge                (ps2_clk_negedge),
    .ps2_data                       (ps2_data_reg),
    .received_data                  (received_data),   // Outputs
    .received_data_en               (received_data_en)
);

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Module:       PS2_Mouse_Command_Out
// Description:  This module sends commands to the PS2 interface
// --------------------------------------------------------------------
module PS2_Mouse_Command_Out (
    clk,
    reset,
    the_command,
    send_command,
    ps2_clk_posedge,
    ps2_clk_negedge,
    PS2_CLK,
    PS2_DAT,
    command_was_sent,
    error_communication_timed_out
);

// --------------------------------------------------------------------
// Port Declarations 
// --------------------------------------------------------------------
input                clk;
input                reset;
input          [7:0] the_command;
input                send_command;
input                ps2_clk_posedge;
input                ps2_clk_negedge;
inout                PS2_CLK;
inout                PS2_DAT;
output    reg        command_was_sent;
output    reg        error_communication_timed_out;

// --------------------------------------------------------------------
// Parameter Declarations , 1/12.5mhz => 0.08us
// --------------------------------------------------------------------
parameter    CLOCK_CYCLES_FOR_101US      = 1262;      // Timing info for initiating  
parameter    NUMBER_OF_BITS_FOR_101US    = 13;        // Host-to-Device communication 
parameter    COUNTER_INCREMENT_FOR_101US = 13'h0001;  //  when using a 12.5MHz system clock
parameter    CLOCK_CYCLES_FOR_15MS       = 187500;    // Timing info for start of 
parameter    NUMBER_OF_BITS_FOR_15MS     = 20;        // transmission error when
parameter    COUNTER_INCREMENT_FOR_15MS  = 20'h00001; // using a 12.5MHz system clock
parameter    CLOCK_CYCLES_FOR_2MS        = 25000;     // Timing info for sending 
parameter    NUMBER_OF_BITS_FOR_2MS      = 17;        // data error when 
parameter    COUNTER_INCREMENT_FOR_2MS   = 17'h00001; // using a 12.5MHz system clock

// --------------------------------------------------------------------
// Constant Declarations 
// --------------------------------------------------------------------
parameter   PS2_STATE_0_IDLE                    = 3'h0,
            PS2_STATE_1_INITIATE_COMMUNICATION  = 3'h1,
            PS2_STATE_2_WAIT_FOR_CLOCK          = 3'h2,
            PS2_STATE_3_TRANSMIT_DATA           = 3'h3,
            PS2_STATE_4_TRANSMIT_STOP_BIT       = 3'h4,
            PS2_STATE_5_RECEIVE_ACK_BIT         = 3'h5,
            PS2_STATE_6_COMMAND_WAS_SENT        = 3'h6,
            PS2_STATE_7_TRANSMISSION_ERROR      = 3'h7;

// --------------------------------------------------------------------
// Internal wires and registers Declarations 
// --------------------------------------------------------------------
reg            [3:0]    cur_bit;            // Internal Registers
reg            [8:0]    ps2_command;

reg            [NUMBER_OF_BITS_FOR_101US:1]    command_initiate_counter;

reg            [NUMBER_OF_BITS_FOR_15MS:1]        waiting_counter;
reg            [NUMBER_OF_BITS_FOR_2MS:1]        transfer_counter;

reg            [2:0]    ns_ps2_transmitter;            // State Machine Registers
reg            [2:0]    s_ps2_transmitter;

// --------------------------------------------------------------------
// Finite State Machine(s)
// --------------------------------------------------------------------
always @(posedge clk) begin
    if(reset == 1'b1) s_ps2_transmitter <= PS2_STATE_0_IDLE;
    else              s_ps2_transmitter <= ns_ps2_transmitter;
end

always @(*) begin        // Defaults
    ns_ps2_transmitter = PS2_STATE_0_IDLE;

    case (s_ps2_transmitter)
    PS2_STATE_0_IDLE:
        begin
            if (send_command == 1'b1) ns_ps2_transmitter = PS2_STATE_1_INITIATE_COMMUNICATION;
            else                      ns_ps2_transmitter = PS2_STATE_0_IDLE;
        end
    PS2_STATE_1_INITIATE_COMMUNICATION:
        begin
            if (command_initiate_counter == CLOCK_CYCLES_FOR_101US)
                ns_ps2_transmitter = PS2_STATE_2_WAIT_FOR_CLOCK;
            else
                ns_ps2_transmitter = PS2_STATE_1_INITIATE_COMMUNICATION;
        end
    PS2_STATE_2_WAIT_FOR_CLOCK:
        begin
            if (ps2_clk_negedge == 1'b1)
                ns_ps2_transmitter = PS2_STATE_3_TRANSMIT_DATA;
            else if (waiting_counter == CLOCK_CYCLES_FOR_15MS)
                ns_ps2_transmitter = PS2_STATE_7_TRANSMISSION_ERROR;
            else
                ns_ps2_transmitter = PS2_STATE_2_WAIT_FOR_CLOCK;
        end
    PS2_STATE_3_TRANSMIT_DATA:
        begin
            if ((cur_bit == 4'd8) && (ps2_clk_negedge == 1'b1))
                ns_ps2_transmitter = PS2_STATE_4_TRANSMIT_STOP_BIT;
            else if (transfer_counter == CLOCK_CYCLES_FOR_2MS)
                ns_ps2_transmitter = PS2_STATE_7_TRANSMISSION_ERROR;
            else
                ns_ps2_transmitter = PS2_STATE_3_TRANSMIT_DATA;
        end
    PS2_STATE_4_TRANSMIT_STOP_BIT:
        begin
            if (ps2_clk_negedge == 1'b1)
                ns_ps2_transmitter = PS2_STATE_5_RECEIVE_ACK_BIT;
            else if (transfer_counter == CLOCK_CYCLES_FOR_2MS)
                ns_ps2_transmitter = PS2_STATE_7_TRANSMISSION_ERROR;
            else
                ns_ps2_transmitter = PS2_STATE_4_TRANSMIT_STOP_BIT;
        end
    PS2_STATE_5_RECEIVE_ACK_BIT:
        begin
            if (ps2_clk_posedge == 1'b1)
                ns_ps2_transmitter = PS2_STATE_6_COMMAND_WAS_SENT;
            else if (transfer_counter == CLOCK_CYCLES_FOR_2MS)
                ns_ps2_transmitter = PS2_STATE_7_TRANSMISSION_ERROR;
            else
                ns_ps2_transmitter = PS2_STATE_5_RECEIVE_ACK_BIT;
        end
    PS2_STATE_6_COMMAND_WAS_SENT:
        begin
            if (send_command == 1'b0)
                ns_ps2_transmitter = PS2_STATE_0_IDLE;
            else
                ns_ps2_transmitter = PS2_STATE_6_COMMAND_WAS_SENT;
        end
    PS2_STATE_7_TRANSMISSION_ERROR:
        begin
            if (send_command == 1'b0)
                ns_ps2_transmitter = PS2_STATE_0_IDLE;
            else
                ns_ps2_transmitter = PS2_STATE_7_TRANSMISSION_ERROR;
        end
    default:
        begin
            ns_ps2_transmitter = PS2_STATE_0_IDLE;
        end
    endcase
end

// --------------------------------------------------------------------
// Sequential logic
// --------------------------------------------------------------------
always @(posedge clk) begin
    if(reset == 1'b1)     ps2_command <= 9'h000;
    else if(s_ps2_transmitter == PS2_STATE_0_IDLE)
        ps2_command <= {(^the_command) ^ 1'b1, the_command};
end

always @(posedge clk) begin
    if(reset == 1'b1) command_initiate_counter <= {NUMBER_OF_BITS_FOR_101US{1'b0}};
    else if((s_ps2_transmitter == PS2_STATE_1_INITIATE_COMMUNICATION) &&
            (command_initiate_counter != CLOCK_CYCLES_FOR_101US))
        command_initiate_counter <= 
            command_initiate_counter + COUNTER_INCREMENT_FOR_101US;
    else if(s_ps2_transmitter != PS2_STATE_1_INITIATE_COMMUNICATION)
        command_initiate_counter <= {NUMBER_OF_BITS_FOR_101US{1'b0}};
end

always @(posedge clk) begin
    if(reset == 1'b1)     waiting_counter <= {NUMBER_OF_BITS_FOR_15MS{1'b0}};
    else if((s_ps2_transmitter == PS2_STATE_2_WAIT_FOR_CLOCK) &&
            (waiting_counter != CLOCK_CYCLES_FOR_15MS))
        waiting_counter <= waiting_counter + COUNTER_INCREMENT_FOR_15MS;
    else if(s_ps2_transmitter != PS2_STATE_2_WAIT_FOR_CLOCK)
        waiting_counter <= {NUMBER_OF_BITS_FOR_15MS{1'b0}};
end

always @(posedge clk) begin
    if(reset == 1'b1) transfer_counter <= {NUMBER_OF_BITS_FOR_2MS{1'b0}};
    else begin
        if((s_ps2_transmitter == PS2_STATE_3_TRANSMIT_DATA) ||
            (s_ps2_transmitter == PS2_STATE_4_TRANSMIT_STOP_BIT) ||
            (s_ps2_transmitter == PS2_STATE_5_RECEIVE_ACK_BIT))
        begin
            if(transfer_counter != CLOCK_CYCLES_FOR_2MS)
               transfer_counter <= transfer_counter + COUNTER_INCREMENT_FOR_2MS;
        end
        else transfer_counter <= {NUMBER_OF_BITS_FOR_2MS{1'b0}};
    end
end

always @(posedge clk) begin
    if(reset == 1'b1)  cur_bit <= 4'h0;
    else if((s_ps2_transmitter == PS2_STATE_3_TRANSMIT_DATA) &&
            (ps2_clk_negedge == 1'b1))
        cur_bit <= cur_bit + 4'h1;
    else if(s_ps2_transmitter != PS2_STATE_3_TRANSMIT_DATA)
        cur_bit <= 4'h0;
end

always @(posedge clk) begin
    if(reset == 1'b1)     command_was_sent <= 1'b0;
    else if(s_ps2_transmitter == PS2_STATE_6_COMMAND_WAS_SENT)
        command_was_sent <= 1'b1;
    else if(send_command == 1'b0)     command_was_sent <= 1'b0;
end

always @(posedge clk) begin
    if(reset == 1'b1)     error_communication_timed_out <= 1'b0;
    else if(s_ps2_transmitter == PS2_STATE_7_TRANSMISSION_ERROR)
        error_communication_timed_out <= 1'b1;
    else if(send_command == 1'b0)
        error_communication_timed_out <= 1'b0;
end

// --------------------------------------------------------------------
// Combinational logic
// --------------------------------------------------------------------
assign PS2_CLK    = (s_ps2_transmitter == PS2_STATE_1_INITIATE_COMMUNICATION) ? 1'b0 : 1'bz;

assign PS2_DAT    = (s_ps2_transmitter == PS2_STATE_3_TRANSMIT_DATA) ? ps2_command[cur_bit] :
                  (s_ps2_transmitter == PS2_STATE_2_WAIT_FOR_CLOCK) ? 1'b0 :
                  ((s_ps2_transmitter == PS2_STATE_1_INITIATE_COMMUNICATION) && 
                  (command_initiate_counter[NUMBER_OF_BITS_FOR_101US] == 1'b1)) ? 1'b0 : 1'bz;

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------


// --------------------------------------------------------------------
// Module:       PS2_Mouse_Data_In                                       
// Description:  This module accepts incoming data from PS2 interface.
// --------------------------------------------------------------------
module PS2_Mouse_Data_In (
    clk,
    reset,

    wait_for_incoming_data,
    start_receiving_data,

    ps2_clk_posedge,
    ps2_clk_negedge,
    ps2_data,

    received_data,
    received_data_en            // If 1 - new data has been received
);

// --------------------------------------------------------------------
// Port Declarations                             
// --------------------------------------------------------------------
input                clk;
input                reset;
input                wait_for_incoming_data;
input                start_receiving_data;
input                ps2_clk_posedge;
input                ps2_clk_negedge;
input                ps2_data;
output reg    [7:0]  received_data;
output reg           received_data_en;

// --------------------------------------------------------------------
// Constant Declarations                           
// --------------------------------------------------------------------
localparam    PS2_STATE_0_IDLE            = 3'h0,
            PS2_STATE_1_WAIT_FOR_DATA    = 3'h1,
            PS2_STATE_2_DATA_IN            = 3'h2,
            PS2_STATE_3_PARITY_IN        = 3'h3,
            PS2_STATE_4_STOP_IN            = 3'h4;

// --------------------------------------------------------------------
// Internal wires and registers Declarations                 
// --------------------------------------------------------------------
reg            [3:0]    data_count;
reg            [7:0]    data_shift_reg;

// State Machine Registers
reg            [2:0]    ns_ps2_receiver;
reg            [2:0]    s_ps2_receiver;

// --------------------------------------------------------------------
// Finite State Machine(s)                           
// --------------------------------------------------------------------
always @(posedge clk) begin
    if (reset == 1'b1) s_ps2_receiver <= PS2_STATE_0_IDLE;
    else               s_ps2_receiver <= ns_ps2_receiver;
end

always @(*) begin     // Defaults
    ns_ps2_receiver = PS2_STATE_0_IDLE;

    case (s_ps2_receiver)
    PS2_STATE_0_IDLE:
        begin
            if((wait_for_incoming_data == 1'b1) && (received_data_en == 1'b0))
                ns_ps2_receiver = PS2_STATE_1_WAIT_FOR_DATA;
            else if ((start_receiving_data == 1'b1) && (received_data_en == 1'b0))
                ns_ps2_receiver = PS2_STATE_2_DATA_IN;
            else ns_ps2_receiver = PS2_STATE_0_IDLE;
        end
    PS2_STATE_1_WAIT_FOR_DATA:
        begin
            if((ps2_data == 1'b0) && (ps2_clk_posedge == 1'b1)) 
                ns_ps2_receiver = PS2_STATE_2_DATA_IN;
            else if (wait_for_incoming_data == 1'b0)
                ns_ps2_receiver = PS2_STATE_0_IDLE;
            else
                ns_ps2_receiver = PS2_STATE_1_WAIT_FOR_DATA;
        end
    PS2_STATE_2_DATA_IN:
        begin
            if((data_count == 3'h7) && (ps2_clk_posedge == 1'b1))
                ns_ps2_receiver = PS2_STATE_3_PARITY_IN;
            else
                ns_ps2_receiver = PS2_STATE_2_DATA_IN;
        end
    PS2_STATE_3_PARITY_IN:
        begin
            if (ps2_clk_posedge == 1'b1)
                ns_ps2_receiver = PS2_STATE_4_STOP_IN;
            else
                ns_ps2_receiver = PS2_STATE_3_PARITY_IN;
        end
    PS2_STATE_4_STOP_IN:
        begin
            if (ps2_clk_posedge == 1'b1)
                ns_ps2_receiver = PS2_STATE_0_IDLE;
            else
                ns_ps2_receiver = PS2_STATE_4_STOP_IN;
        end
    default:
        begin
            ns_ps2_receiver = PS2_STATE_0_IDLE;
        end
    endcase
end

// --------------------------------------------------------------------
// Sequential logic                              
// --------------------------------------------------------------------
always @(posedge clk) begin
    if (reset == 1'b1)     data_count <= 3'h0;
    else if((s_ps2_receiver == PS2_STATE_2_DATA_IN) && (ps2_clk_posedge == 1'b1))
        data_count    <= data_count + 3'h1;
    else if(s_ps2_receiver != PS2_STATE_2_DATA_IN)
        data_count    <= 3'h0;
end

always @(posedge clk) begin
    if(reset == 1'b1)     data_shift_reg <= 8'h00;
    else if((s_ps2_receiver == PS2_STATE_2_DATA_IN) && (ps2_clk_posedge == 1'b1))
        data_shift_reg    <= {ps2_data, data_shift_reg[7:1]};
end

always @(posedge clk) begin
    if(reset == 1'b1) received_data <= 8'h00;
    else if(s_ps2_receiver == PS2_STATE_4_STOP_IN)
        received_data    <= data_shift_reg;
end

always @(posedge clk) begin
    if(reset == 1'b1) received_data_en <= 1'b0;
    else if((s_ps2_receiver == PS2_STATE_4_STOP_IN) && (ps2_clk_posedge == 1'b1))
        received_data_en    <= 1'b1;
    else
        received_data_en    <= 1'b0;
end


// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------

