// --------------------------------------------------------------------
// --------------------------------------------------------------------
// Module:      csrbrg.v
// Description: Configuration and Staus Register(CSR)Bus
// --------------------------------------------------------------------
// --------------------------------------------------------------------
module csrbrg(
	input 				sys_clk,
	input 				sys_rst,
	
	input 		[ 3:1]	wb_adr_i,	// WB Slave
	input 		[15:0]	wb_dat_i,
	output reg	[15:0]	wb_dat_o,
	input 				wb_cyc_i,
	input 				wb_stb_i,
	input 				wb_we_i,
	output reg 			wb_ack_o,
	
	output reg 	[ 2:0] 	csr_a,		// CSR Master
	output reg 			csr_we,
	output reg 	[15:0]	csr_do,
	input 		[15:0]	csr_di
);

// Datapath: WB <- CSR 
always @(posedge sys_clk) begin
	wb_dat_o <= csr_di;
end

// Datapath: CSR -> WB 
reg next_csr_we;
always @(posedge sys_clk) begin
	csr_a <= wb_adr_i[3:1];
	csr_we <= next_csr_we;
	csr_do <= wb_dat_i;
end

// Controller 
reg [1:0] state;
reg [1:0] next_state;

parameter IDLE		= 2'd0;
parameter DELAYACK1	= 2'd1;
parameter DELAYACK2	= 2'd2;
parameter ACK		= 2'd3;

always @(posedge sys_clk) begin
	if(sys_rst) state <= IDLE;
	else		state <= next_state;
end

always @(*) begin
	next_state  = state;
	wb_ack_o    = 1'b0;
	next_csr_we = 1'b0;
	
	case(state)
		IDLE: begin
			if(wb_cyc_i & wb_stb_i) begin  		// We have a request 
				next_csr_we = wb_we_i;
				if(wb_we_i) next_state = ACK;
				else		next_state = DELAYACK1;
			end
		end
		DELAYACK1: next_state = DELAYACK2;
		DELAYACK2: next_state = ACK;
		ACK: begin
			wb_ack_o = 1'b1;
			next_state = IDLE;
		end
	endcase
end

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------
