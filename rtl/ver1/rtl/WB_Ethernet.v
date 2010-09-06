//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// Module:      WB_Ethernet.v
//
// Description: Wishbone Ethernet core.  This module implements a simple 
// interface to a 10BaseT PHY. There are separate transmit and receive buffers
// that map into the IO space. To send a packet, simply construct the packet
// in the transmit buffer and then toggle the sendpacket bit of the control
// register. If a packet is received, it is placed in the receive buffer and 
// a flag is set in the status register. If the enable interrupt flag is 
// set in the control register, an interrupt will be triggered when either
// a packet arrives or when one is finished transmitting. The IO address space
// used is 8 bytes. Here is an example IO map. All registers are 8 bit and r/w
// 
// I/O Address  Description
// -----------  -----------------------------------------
// Base + 0x00  Transmit Status  Register
// Base + 0x01  Transmit Control and MSB Address Register
// Base + 0x02  Transmit Buffer  LSB Address Register 
// Base + 0x03  Transmit Buffer  Data Register 
// Base + 0x04  Receive  Status  Register  
// Base + 0x05  Receive  Control and MSB Address Register 
// Base + 0x06  Receive  Buffer  LSB Address Register 
// Base + 0x07  Receive  Buffer  Data Register 
//
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
module WB_Ethernet(
    input              wb_clk_i,        // Clock Input
    input              wb_rst_i,        // Reset Input
    input       [15:0] wb_dat_i,        // Command to send to mouse
    output      [15:0] wb_dat_o,        // Received data
    input              wb_cyc_i,        // Cycle
    input              wb_stb_i,        // Strobe
    input       [ 1:0] wb_adr_i,        // Wishbone address lines
    input       [ 1:0] wb_sel_i,        // Wishbone Select lines
    input              wb_we_i,         // Write enable
    output reg         wb_ack_o,        // Normal bus termination
    output             wb_tgc_o,        // Interrupt request
   
    input              clk20,			// 20Mhz Clock for transmitting
    input              clk50,			// 40Mhz Clock For receiving
    input              Ethernet_Rx,     // Etherent Receive line
    output             Ethernet_Tx      // Ethernet 10BASE-T outputs
);

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// This section is a simple WB interface
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
reg   [7:0]  dat_o;
assign       wb_dat_o    =  wb_sel_i[0] ? {8'h00, dat_o} : {dat_o, 8'h00}; // 8 to 16 bit WB
wire   [7:0] dat_i       =  wb_sel_i[0] ? wb_dat_i[7:0]  : wb_dat_i[15:8]; // 8 to 16 bit WB
wire   [2:0] wb_net_addr = {wb_adr_i,   wb_sel_i[1]};  	// Interface Address
wire         wb_ack_i    =  wb_stb_i &  wb_cyc_i;      	// Immediate ack
wire         wr_command  =  wb_ack_i &  wb_we_i;       	// Wishbone write access, Singal to send
wire         rd_command  =  wb_ack_i & ~wb_we_i;       	// Wishbone write access, Singal to send
assign       wb_tgc_o    =  intr;                      	// Received interupts ocurred
wire         intr  		=   1'b0;        				// Interupt Line

always @(posedge wb_clk_i or posedge wb_rst_i) begin    // Synchrounous
    if(wb_rst_i) wb_ack_o <= 1'b0;
    else         wb_ack_o <= wb_ack_i; // one clock delay on acknowledge output
end

//-------------------------------------------------------------------------------------------------
// Below are the bit assigments for the Status and Control Registers.
// The lower 4 bits of the Control Register are the MSB bits of the 
// Buffer. 
//
//  |7|6|5|4|3|2|1|0|  Transmit Status Register, writting anything to this register clears bit7
//   | | | | | | | `-- 0       - Not used, reads zero
//   | | | | | | `---- 0       - Not used, reads zero
//   | | | | | `------ 0       - Not used, reads zero 
//   | | | | `-------- 0       - Not used, reads zero 
//   | | | `---------- 0       - Not used, reads zero 
//   | | `------------ 0       - Not used, reads zero 
//   | `-------------- FTTX    - Indicates an Ethernet frame is being sent, but is not done yet
//   `---------------- FSENT   - Indicates a complete Ethernet frame was sent, reset to 0 if new frame loaded
//
//  |7|6|5|4|3|2|1|0|  Transmit Control Register
//   | | | | | | | `-- A08     - Address Bit  8 of buffer 
//   | | | | | | `---- A09     - Address Bit  9 of buffer 
//   | | | | | `------ A10     - Address Bit 10 of buffer 
//   | | | | `-------- 0       - Not used, reads zero
//   | | | `---------- 0       - Not used, reads zero
//   | | `------------ 0       - Not used, reads zero
//   | `-------------- FTINT   - Allow Interrupt to be generated on completion of frame transmit
//   `---------------- FTSND   - Send contents of transmit buffer now
//
//
//  |7|6|5|4|3|2|1|0|  Receive Status Register, writting anything to this register clears bit7, FRCVD
//   | | | | | | | `-- 0       - Not used, reads zero
//   | | | | | | `---- 0       - Not used, reads zero
//   | | | | | `------ 0       - Not used, reads zero 
//   | | | | `-------- 0       - Not used, reads zero 
//   | | | `---------- 0       - Not used, reads zero 
//   | | `------------ 0       - Not used, reads zero 
//   | `-------------- FRRX    - Indicates a complete Ethernet frameis being received, but no in yet
//   `---------------- FRCVD   - Indicates a complete Ethernet frame was, 0 if no frame waiting 
//
//  |7|6|5|4|3|2|1|0|  Receive Control Register
//   | | | | | | | `-- A08     - Address Bit  8 of buffer 
//   | | | | | | `---- A09     - Address Bit  9 of buffer 
//   | | | | | `------ A10     - Address Bit 10 of buffer 
//   | | | | `-------- 0       - Not used, reads zero
//   | | | `---------- 0       - Not used, reads zero
//   | | `------------ 0       - Not used, reads zero
//   | `-------------- FRINT   - Allow Interrupt to be generated on receipt of new frame
//   `---------------- FRRCV   - Allow a frame to be receive into buffer, set to 0 to hold while processing
//
//-------------------------------------------------------------------------------------------------
  
//-------------------------------------------------------------------------------------------------
`define REG_TX_STAT     3'h0      // R/W  - Transmit Status
`define REG_TX_CNTL     3'h1      // R/W  - Transmit Control + MSB Address
`define REG_TX_ADDR     3'h2      // Transmit Buffer LSB Address Register 
`define REG_TX_DATA     3'h3      // Transmit Buffer Data Register 
`define REG_RX_STAT     3'h4      // Receive  Status  Register  
`define REG_RX_CNTL     3'h5      // Receive  Control and MSB Address Register 
`define REG_RX_ADDR     3'h6      // Receive  Buffer LSB Address Register 
`define REG_RX_DATA     3'h7      // Receive  Buffer Data Register 

//-------------------------------------------------------------------------------------------------
// Register behavior
//-------------------------------------------------------------------------------------------------
reg			 FSENT;   	// Indicates a complete Ethernet frame was sent, reset to 0 if new frame loaded
reg			 FRCVD;  	// Indicates a complete Ethernet frame was receved, reset to 0 if new frame loaded
reg          FEOF;		// end of frame signal
wire		 FTTX        =  SendingPacket;      // Indicates an Ethernet frame is being sent, but is not done yet
wire		 FRRX        = ~end_of_frame;  		// Indicates an Ethernet frame is being received, but is not done yet
wire 		 FTSND 		 =  tx_control[7];		// Trigger to send a frame 
wire 		 FRRCV 		 =  rx_control[7];		// Allow a frame to be received
wire [7:0]   tx_status   = {FSENT, FTTX, 6'h00};
wire [7:0]   rx_status	 = {FEOF,  FRRX, FRCVD, DR_risingedge, 4'b0000};
reg  [7:0]   tx_control;
reg  [7:0]   rx_control;
reg  [7:0]   tx_address;
reg  [7:0]   rx_address;
wire [7:0]   tx_buffer	= dat_i;
wire [7:0]   rx_buffer	= dat_i;

always @(posedge wb_clk_i or posedge wb_rst_i) begin // Synchrounous Logic
  if(wb_rst_i) begin
	dat_o   	 <= 8'h00;                         	// Default value
  end 
  else begin
     if(rd_command) begin                           // If a read was requested
        case(wb_net_addr)                           // Determine which register was read
            `REG_TX_STAT: dat_o <= tx_status;       // Read Tx status register
            `REG_TX_CNTL: dat_o <= tx_control;      // Read back the control register
            `REG_TX_ADDR: dat_o <= tx_address;      // Read back the address register
            `REG_TX_DATA: dat_o <= tx_rd_buf;       // Read the Tx Data register
            `REG_RX_STAT: dat_o <= rx_status;       // Read Rx status register
            `REG_RX_CNTL: dat_o <= {rx_control[7:3],rxaddr[10:8]};      // Read back the control register
            `REG_RX_ADDR: dat_o <= rxaddr[7:0];      // Read back the address register
            `REG_RX_DATA: dat_o <= rx_rd_buf;       // Read the Tx Data register
            default:      dat_o <= 8'h00;           // Default
        endcase                                     // End of case
     end                                            // End if read
   end                                              // End if reset
end                                                 // End Synchrounous always

always @(posedge wb_clk_i or posedge wb_rst_i) begin  // Synchrounous Logic
  if(wb_rst_i) begin
    tx_control <=  8'h00;                   		// not on
    rx_control <=  8'h00;                   		// not on
	FSENT		 <=  1'b0;                   		// not on
	FRCVD		 <=  1'b0;                   		// not on
	wren_tx 	 <=  1'b0;							// Not writting to buffer
	wren_rx 	 <=  1'b0;							// Not writting to buffer
	StartSending <= 1'b0;							// Not Send a byte
  end 
  else begin
	if(wr_command) begin                           // If a write was requested
        case(wb_net_addr)                           // Determine which register was writen to
            `REG_TX_STAT: FSENT		 <=  1'b0;      // Tx status register, clear interrupt flag
            `REG_TX_CNTL: tx_control <= dat_i;      // Read back the control register
            `REG_TX_ADDR: tx_address <= dat_i;      // Read back the address register
            `REG_TX_DATA: wren_tx    <=  1'b1;      // Write to the Tx Buffer
            `REG_RX_STAT: FRCVD		 <=  1'b0;      // Rx status register, clear  flag
            `REG_RX_CNTL: rx_control <= dat_i;      // Read back the control register
            `REG_RX_ADDR: rx_address <= dat_i;      // Read back the address register
            `REG_RX_DATA: wren_rx    <=  1'b1;      // Write to the Rx Buffer
            default: ;                              // Default value                        
        endcase                                     // End of case
    end                                             // End of Write Command if
	else begin
		wren_tx <= 1'b0;      						// Done Writting to the Tx Buffer
		wren_rx <= 1'b0;      						// Done Writting to the Rx Buffer
	end
	
    if(FTSND) begin 
		FSENT <= 1'b0;							// Reset the Frame Sent flag
		if(FTTX) tx_control[7] <= 1'b0;
		StartSending  <= 1'b1;						// Send a byte
	end
	else StartSending <= 1'b0;
	if(!FTTX && !FSENT) FSENT <= 1'b1;				// Set the Frame Sent flag

	if(FEOF) begin
		FRCVD <= 1'b1;							// Set the Frame receive flag
	end
		
  end                                               // End of Reset if
end                                                     

//-------------------------------------------------------------------------------------------------
// Transmit Section
//-------------------------------------------------------------------------------------------------
wire [10:0] txrwaddr = {tx_control[2:0],tx_address};
reg  [10:0] txaddr;
wire [ 7:0] tx_rd_buf;
reg			wren_tx;
ram2 txram(.clock_a(wb_clk_i),.address_a(txrwaddr),.data_a(tx_buffer),.q_a(tx_rd_buf),.wren_a(wren_tx&wb_stb_i),
		   .clock_b(clk20),   .address_b(txaddr),                     .q_b(TxData),   .wren_b(1'b0));

wire [10:0] wrtotal = txrwaddr;
reg 		StartSending;
reg 		SendingPacket;
always @(posedge clk20) if(StartSending) SendingPacket <= 1'b1; else if(NextByte && txaddr == wrtotal) SendingPacket <= 1'b0;
always @(posedge clk20) if(NextByte) txaddr <= SendingPacket ? txaddr +  11'd1 : 11'd0;

wire		NextByte;
wire [7:0]  TxData;
TENBASET_TxD U1(.clk20(clk20), .Ethernet_Tx(Ethernet_Tx), .TxData(TxData), .SendingPacket(SendingPacket), .NextByte(NextByte)); 

	
//-------------------------------------------------------------------------------------------------
// Receive Section
//-------------------------------------------------------------------------------------------------
wire [10:0] rxwraddr = {rx_control[2:0],rx_address};
wire [ 7:0] rx_rd_buf;
reg			wren_rx;
ram2 rxram(.clock_a(wb_clk_i),.address_a(rxwraddr), .data_a(rx_buffer),.q_a(rx_rd_buf),.wren_a(wren_rx&wb_stb_i),
		   .clock_b(clk50),   .address_b(rxaddr),   .data_b(RxData),                   .wren_b(wren_rb));

//-----------------------------------------------------------------------------
// Find Rising edge of end_of_frame line using a 3-bits shift register
//-----------------------------------------------------------------------------
reg [2:0] DRedge;  
always @(posedge clk50) DRedge <= {DRedge[1:0], end_of_frame};
wire DR_risingedge  = (DRedge[2:1] == 2'b01);      // now we can detect DR rising edge
		   
wire        new_byte;
wire		end_of_frame;
wire [ 7:0] RxData;
reg			wren_rb;

TENBASET_RxD rx1(.clk48(clk50),.manchester_data_in(Ethernet_Rx),.RxData(RxData),.new_byte_available(new_byte), .end_of_frame(end_of_frame));

//wire        rcv_ready = (!FRCVD && FRRCV);
reg  [10:0] rxaddr;
reg         isreceiving;

always @(posedge clk50) begin
	if(wb_rst_i) begin
		rxaddr 		<= 11'h000;           	// Default value
		FEOF		<= 1'b0;				// No new frame
		isreceiving	<= 1'b0;				// Not Receiving
		wren_rb 	<= 1'b0;				// Not writting to buffer
	end 
	else begin

		if(DR_risingedge) FEOF <= 1'b1;
		
		if(FRRCV)  begin 
			FEOF   <= 1'b0;
			rxaddr <= 11'h000;				// start transmitting new frame
		end
		
		if(!FEOF && !isreceiving) isreceiving <= 1'b1;
		
		if(isreceiving) begin
			if(end_of_frame) begin				// New byte of data came in 
				isreceiving <= 1'b0;			// Done receiving
			end 
			else begin
				if(new_byte) begin
					rxaddr     <= rxaddr + 11'd1;	// receive next byte in frame
					wren_rb    <= 1'b1;
				end
				else begin
					wren_rb 	<= 1'b0;				// Not writting to buffer
				end
			end
		end
	end
end

//-------------------------------------------------------------------------------------------------
endmodule
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// Design Name : TenBaseT
// Function    : Test the 10BaseT function
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
module TENBASET_TxD(
	input 		clk20,		 	// 20MHz clock
    input [7:0] TxData,         // Byteof data to transmit
	input 		SendingPacket,
	output		NextByte,
	output reg	Ethernet_Tx 	// Ethernet 10BASE-T outputs
);

assign 		NextByte = (ShiftCount==15);
reg [7:0] 	ShiftData; 
reg [3:0] 	ShiftCount;
always @(posedge clk20) ShiftCount <= SendingPacket ? ShiftCount + 4'd1 : 4'd15;
always @(posedge clk20) if(ShiftCount[0]) ShiftData <= NextByte ? TxData : {1'b0, ShiftData[7:1]};

// generate an NLP every 13ms (the spec calls for anything between 8ms and 24ms).
reg [17:0] 	LinkPulseCount; 
reg 		LinkPulse; 
always @(posedge clk20) LinkPulseCount <= SendingPacket ? 18'd0 : LinkPulseCount + 18'd1;
always @(posedge clk20) LinkPulse <= &LinkPulseCount[17:1];

// TP_IDL, shift-register and manchester encoder
reg 		qo;
reg 		qoe; 
reg 		SendingPacketData; 
reg [2:0] 	idlecount; 
always @(posedge clk20) SendingPacketData <= SendingPacket;
always @(posedge clk20) if(SendingPacketData) idlecount <= 3'd0; else if(~&idlecount) idlecount <= idlecount + 3'd1;
always @(posedge clk20) qo  <= SendingPacketData ? ~ShiftData[0]^ShiftCount[0] : 1'b1;
always @(posedge clk20) qoe <= SendingPacketData | LinkPulse | (idlecount<6);
always @(posedge clk20) Ethernet_Tx <= (qoe ?  qo : 1'b0);

//-------------------------------------------------------------------------------------------------
endmodule
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// Design Name : Ethernet_RxD
// File Name   : Ethernet_RxD.v
// Function    : Receives a UDP Packet 
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
module TENBASET_RxD(
    input            clk48,                 // 40 Mhz Clockey
    input            manchester_data_in,    // Ethernet data input
    output     [7:0] RxData,                // Data byte output
    output           new_byte_available,    //
    output 		     end_of_frame  			// indicate an Ethernet frame was captured
);
reg [2:0] in_data;
always @(posedge clk48) in_data <= {in_data[1:0], manchester_data_in};

reg [1:0] cnt;
always @(posedge clk48) if(|cnt || (in_data[2] ^ in_data[1])) cnt<=cnt+1;

assign 	  RxData = data;
reg [7:0] data;
reg new_bit_avail;
always @(posedge clk48) new_bit_avail <= (cnt==3);
always @(posedge clk48) if(cnt==3) data<={in_data[1],data[7:1]};
//-------------------------------------------------------------------------------------------------
reg [4:0] sync1;
always @(posedge clk48)
	if(end_of_Ethernet_frame) sync1<=0; 
	else 
	if(new_bit_avail) begin
		if(!(data==8'h55 || data==8'hAA))  sync1 <= 0;	 // not preamble?
		else
		if(~&sync1) // if all bits of this "sync1" counter are one, we decide that enough of the preamble
                    // has been received, so stop counting and wait for "sync2" to detect the SFD
			sync1 <= sync1 + 1; // otherwise keep counting
end

reg [9:0] sync2;
always @(posedge clk48)
	if(end_of_Ethernet_frame) sync2 <= 0;
	else 
	if(new_bit_avail) begin
		if(|sync2) // if the SFD has already been detected (Ethernet data is coming in)
			sync2 <= sync2 + 1; // then count the bits coming in
		else
			if(&sync1 && data==8'hD5) // otherwise, let's wait for the SFD (0xD5)
				sync2 <= sync2 + 1;
end

assign new_byte_available = new_bit_avail && (sync2[2:0]==3'h0) && (sync2[9:3]!=0);  

//-------------------------------------------------------------------------------------------------
// if no clock transistion is detected for some time, that's the end of the Ethernet frame
//-------------------------------------------------------------------------------------------------
reg [2:0] transition_timeout;
always @(posedge clk48) 
	if(in_data[2]^in_data[1]) transition_timeout <= 0; 
	else if(~&cnt) 			  transition_timeout <= transition_timeout + 1;
reg 	  end_of_Ethernet_frame;
always @(posedge clk48) end_of_Ethernet_frame <= &transition_timeout;

reg [14:0] frame_timeout;
always @(posedge clk48) 
	if(new_byte_available) 	frame_timeout <= 0; 
	else 			  		frame_timeout <= frame_timeout + 1;
	
reg frame_end;
always @(posedge clk48) begin
	if(&frame_timeout)     frame_end <= 1'b1;
	if(new_byte_available) frame_end <= 1'b0;	
end

assign end_of_frame = frame_end;

//-------------------------------------------------------------------------------------------------
endmodule
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// Design Name : Ethernet_RxD
// File Name   : Ethernet_RxD.v
// Function    : Receives an Ethernet Packet 
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
module TENBASET_RxD_1(
    input            clk50,                 // 40 Mhz Clockey
    input            manchester_data_in,    // Ethernet data input
    output     [7:0] RxData,                // Data byte output
    output           new_byte_available,    //
    output reg       end_of_Ethernet_frame  // indicate an Ethernet frame was captured
);
//-------------------------------------------------------------------------------------------------
assign 		  RxData = data;
//-------------------------------------------------------------------------------------------------
reg [2:0]     in_data;
reg [7:0]     data;
reg [1:0]     cnt;
reg           new_bit_avail;
always @(posedge clk50) in_data <= {in_data[1:0], manchester_data_in};
always @(posedge clk50) if(|cnt || (in_data[2] ^ in_data[1])) cnt <= cnt + 2'd1;
always @(posedge clk50) new_bit_avail <= (cnt == 3);
always @(posedge clk50) if(cnt == 3) data <= {in_data[1],data[7:1]};

//-------------------------------------------------------------------------------------------------
reg [4:0] sync1;
always @(posedge clk50) begin 
    if(end_of_Ethernet_frame) sync1 <= 0; 
    else begin
        if(new_bit_avail) begin
              if(!(data==8'h55 || data==8'hAA)) sync1 <= 0;    // not preamble?
            else                // if all bits of this "sync1" counter are one, we decide that enough of the preamble
                if(~&sync1)     // has been received, so stop counting and wait for "sync2" to detect the SFD
                    sync1 <= sync1 + 5'd1; // otherwise keep counting
        end
    end
end

//-------------------------------------------------------------------------------------------------
reg [9:0] sync2;
always @(posedge clk50) begin
    if(end_of_Ethernet_frame) sync2 <= 0;
    else begin
        if(new_bit_avail) begin                    // if the SFD has already been detected (Ethernet data is coming in)
            if(|sync2) sync2 <= sync2 + 10'd1;     // then count the bits coming in
              else if(&sync1 && data == 8'hD5) sync2 <= sync2 + 10'd1; // otherwise, let's wait for the SFD (0xD5)
                
        end
    end
end
assign new_byte_available = new_bit_avail && (sync2[2:0] == 3'h0) && (sync2[9:3] != 0);  

//-------------------------------------------------------------------------------------------------
// if no clock transistion is detected for some time, that's the end of the Ethernet frame
//-------------------------------------------------------------------------------------------------
reg [2:0] transition_timeout;
always @(posedge clk50) begin 
    if(in_data[2]^in_data[1]) transition_timeout <= 0; 
    else if(~&cnt)            transition_timeout <= transition_timeout + 3'd1; // if cnt =0, increment
end
    
always @(posedge clk50) end_of_Ethernet_frame <= &transition_timeout; // if timeout is all 1's

//-------------------------------------------------------------------------------------------------
endmodule
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// End of WB Ethernet Modules
//-------------------------------------------------------------------------------------------------

