// --------------------------------------------------------------------
// --------------------------------------------------------------------
// Module:      hpdmc.v
// Description: Wishbone based High Performance Dynamic Ram Controller
// --------------------------------------------------------------------
// --------------------------------------------------------------------
module hpdmc #(
	parameter csr_addr = 1'b0,
	/*
	 * The depth of the SDRAM array, in bytes.
	 * Capacity (in bytes) is 2^sdram_depth.
	 */
	parameter sdram_depth = 23,
	
	/*
	 * The number of column address bits of the SDRAM.
	 */
	parameter sdram_columndepth = 8
) (
	input sys_clk,
	input sys_rst,
	
	/* Control interface */
	input [2:0] csr_a,
	input csr_we,
	input [15:0] csr_di,
	output [15:0] csr_do,
	
	/* Simple FML 8x16 interface to the memory contents */
	input [sdram_depth-1:0] fml_adr,
	input fml_stb,
	input fml_we,
	output fml_ack,
	input [1:0] fml_sel,
	input [15:0] fml_di,
	output [15:0] fml_do,
	
	/* SDRAM interface.
	 * The SDRAM clock should be driven synchronously to the system clock.
	 * It is not generated inside this core so you can take advantage of
	 * architecture-dependent clocking resources to generate a clean
	 * differential clock.
	 */
	output reg sdram_cke,
	output reg sdram_cs_n,
	output reg sdram_we_n,
	output reg sdram_cas_n,
	output reg sdram_ras_n,
	output reg [11:0] sdram_adr,
	output reg [1:0] sdram_ba,
	
	output [1:0] sdram_dqm,
	inout [15:0] sdram_dq
);

/* Register all control signals, leaving the possibility to use IOB registers */
wire sdram_cke_r;
wire sdram_cs_n_r;
wire sdram_we_n_r;
wire sdram_cas_n_r;
wire sdram_ras_n_r;
wire [11:0] sdram_adr_r;
wire [1:0] sdram_ba_r;

always @(posedge sys_clk) begin
	sdram_cke <= sdram_cke_r;
	sdram_cs_n <= sdram_cs_n_r;
	sdram_we_n <= sdram_we_n_r;
	sdram_cas_n <= sdram_cas_n_r;
	sdram_ras_n <= sdram_ras_n_r;
	sdram_ba <= sdram_ba_r;
	sdram_adr <= sdram_adr_r;
end

/* Mux the control signals according to the "bypass" selection.
 * CKE always comes from the control interface.
 */
wire bypass;

wire sdram_cs_n_bypass;
wire sdram_we_n_bypass;
wire sdram_cas_n_bypass;
wire sdram_ras_n_bypass;
wire [11:0] sdram_adr_bypass;
wire [1:0] sdram_ba_bypass;

wire sdram_cs_n_mgmt;
wire sdram_we_n_mgmt;
wire sdram_cas_n_mgmt;
wire sdram_ras_n_mgmt;
wire [11:0] sdram_adr_mgmt;
wire [1:0] sdram_ba_mgmt;

assign sdram_cs_n_r = bypass ? sdram_cs_n_bypass : sdram_cs_n_mgmt;
assign sdram_we_n_r = bypass ? sdram_we_n_bypass : sdram_we_n_mgmt;
assign sdram_cas_n_r = bypass ? sdram_cas_n_bypass : sdram_cas_n_mgmt;
assign sdram_ras_n_r = bypass ? sdram_ras_n_bypass : sdram_ras_n_mgmt;
assign sdram_adr_r = bypass ? sdram_adr_bypass : sdram_adr_mgmt;
assign sdram_ba_r = bypass ? sdram_ba_bypass : sdram_ba_mgmt;

/* Control interface */
wire sdram_rst;

wire [2:0] tim_rp;
wire [2:0] tim_rcd;
wire tim_cas;
wire [10:0] tim_refi;
wire [3:0] tim_rfc;
wire [1:0] tim_wr;

hpdmc_ctlif #(
	.csr_addr(csr_addr)
) ctlif (
	.sys_clk(sys_clk),
	.sys_rst(sys_rst),
	
	.csr_a(csr_a),
	.csr_we(csr_we),
	.csr_di(csr_di),
	.csr_do(csr_do),
	
	.bypass(bypass),
	.sdram_rst(sdram_rst),
	
	.sdram_cke(sdram_cke_r),
	.sdram_cs_n(sdram_cs_n_bypass),
	.sdram_we_n(sdram_we_n_bypass),
	.sdram_cas_n(sdram_cas_n_bypass),
	.sdram_ras_n(sdram_ras_n_bypass),
	.sdram_adr(sdram_adr_bypass),
	.sdram_ba(sdram_ba_bypass),
	
	.tim_rp(tim_rp),
	.tim_rcd(tim_rcd),
	.tim_cas(tim_cas),
	.tim_refi(tim_refi),
	.tim_rfc(tim_rfc),
	.tim_wr(tim_wr)
);

/* SDRAM management unit */
wire mgmt_stb;
wire mgmt_we;
wire [sdram_depth-1-1:0] mgmt_address;
wire mgmt_ack;

wire read;
wire write;
wire [3:0] concerned_bank;
wire read_safe;
wire write_safe;
wire [3:0] precharge_safe;

hpdmc_mgmt #(
	.sdram_depth(sdram_depth),
	.sdram_columndepth(sdram_columndepth)
) mgmt (
	.sys_clk(sys_clk),
	.sdram_rst(sdram_rst),
	
	.tim_rp(tim_rp),
	.tim_rcd(tim_rcd),
	.tim_refi(tim_refi),
	.tim_rfc(tim_rfc),
	
	.stb(mgmt_stb),
	.we(mgmt_we),
	.address(mgmt_address),
	.ack(mgmt_ack),
	
	.read(read),
	.write(write),
	.concerned_bank(concerned_bank),
	.read_safe(read_safe),
	.write_safe(write_safe),
	.precharge_safe(precharge_safe),
	
	.sdram_cs_n(sdram_cs_n_mgmt),
	.sdram_we_n(sdram_we_n_mgmt),
	.sdram_cas_n(sdram_cas_n_mgmt),
	.sdram_ras_n(sdram_ras_n_mgmt),
	.sdram_adr(sdram_adr_mgmt),
	.sdram_ba(sdram_ba_mgmt)
);

/* Bus interface */
wire data_ack;

hpdmc_busif #(
	.sdram_depth(sdram_depth)
) busif (
	.sys_clk(sys_clk),
	.sdram_rst(sdram_rst),
	
	.fml_adr(fml_adr),
	.fml_stb(fml_stb),
	.fml_we(fml_we),
	.fml_ack(fml_ack),
	
	.mgmt_stb(mgmt_stb),
	.mgmt_we(mgmt_we),
	.mgmt_address(mgmt_address),
	.mgmt_ack(mgmt_ack),
	
	.data_ack(data_ack)
);

/* Data path controller */
wire direction;
wire direction_r;

hpdmc_datactl datactl(
	.sys_clk(sys_clk),
	.sdram_rst(sdram_rst),
	
	.read(read),
	.write(write),
	.concerned_bank(concerned_bank),
	.read_safe(read_safe),
	.write_safe(write_safe),
	.precharge_safe(precharge_safe),
	
	.ack(data_ack),
	.direction(direction),
	.direction_r(direction_r),
	
	.tim_cas(tim_cas),
	.tim_wr(tim_wr)
);

/* Data path */
hpdmc_sdrio sdrio(
	.sys_clk(sys_clk),
	
	.direction(direction),
	.direction_r(direction_r),
	/* Bit meaning is the opposite between
	 * the FML selection signal and SDRAM DQM pins.
	 */
	.mo(~fml_sel),
	.dat_o(fml_di),
	.dat_i(fml_do),
	
	.sdram_dqm(sdram_dqm),
	.sdram_dq(sdram_dq)
);

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Module:      hpdmc.v
// Description: Wishbone based High Performance Dynamic Ram Controller
// --------------------------------------------------------------------
module hpdmc_sdrio(
	input             sys_clk,
	input             direction,
	input             direction_r,
	input      [ 1:0] mo,
	input      [15:0] dat_o,
	output reg [15:0] dat_i,
	output     [ 1:0] sdram_dqm,
	inout      [15:0] sdram_dq
);

wire [15:0] sdram_data_out;
assign sdram_dq = direction ? sdram_data_out : 16'hzzzz;

/* In this case, without DDR primitives and delays, this block
 * is extremely simple  */
assign sdram_dqm      = mo;
assign sdram_data_out = dat_o;

always @(posedge sys_clk) dat_i <= sdram_dq;

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Module:      hpdmc.v
// Description: Wishbone based High Performance Dynamic Ram Controller
// --------------------------------------------------------------------
module hpdmc_banktimer(
	input sys_clk,
	input sdram_rst,
	input tim_cas,
	input [1:0] tim_wr,
	input read,
	input write,
	output reg precharge_safe
);

reg [3:0] counter;
always @(posedge sys_clk) begin
	if(sdram_rst) begin
		counter <= 4'd0;
		precharge_safe <= 1'b1;
	end else begin
		if(read) begin
			/* see p.26 of datasheet :
			 * "A Read burst may be followed by, or truncated with, a Precharge command
			 * to the same bank. The Precharge command should be issued x cycles after
			 * the Read command, where x equals the number of desired data element
			 * pairs"
			 */
			counter <= 4'd8;
			precharge_safe <= 1'b0;
		end else if(write) begin
			counter <= {2'b10, tim_wr};
			precharge_safe <= 1'b0;
		end else begin
			if(counter == 4'b1) precharge_safe <= 1'b1;
			if(~precharge_safe) counter <= counter - 4'b1;
		end
	end
end

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Module:      hpdmc.v
// Description: Wishbone based High Performance Dynamic Ram Controller
// --------------------------------------------------------------------
module hpdmc_busif #(
	parameter sdram_depth = 23
) (
	input sys_clk,
	input sdram_rst,
	input [sdram_depth-1:0] fml_adr,
	input fml_stb,
	input fml_we,
	output fml_ack,
	output mgmt_stb,
	output mgmt_we,
	output [sdram_depth-1-1:0] mgmt_address, /* in 16-bit words */
	input mgmt_ack,
	input data_ack
);
reg mgmt_stb_en;
assign mgmt_stb = fml_stb & mgmt_stb_en;
assign mgmt_we = fml_we;
assign mgmt_address = fml_adr[sdram_depth-1:1];
assign fml_ack = data_ack;

always @(posedge sys_clk) begin
	if(sdram_rst)
		mgmt_stb_en = 1'b1;
	else begin
		if(mgmt_ack)
			mgmt_stb_en = 1'b0;
		if(data_ack)
			mgmt_stb_en = 1'b1;
	end
end

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Module:      hpdmc.v
// Description: Wishbone based High Performance Dynamic Ram Controller
// --------------------------------------------------------------------
module hpdmc_ctlif #(
	parameter csr_addr = 1'b0
) (
	input sys_clk,
	input sys_rst,
	
	input      [ 2:0] csr_a,
	input             csr_we,
	input      [15:0] csr_di,
	output reg [15:0] csr_do,
	
	output reg bypass,
	output reg sdram_rst,
	
	output reg        sdram_cke,
	output reg        sdram_cs_n,
	output reg        sdram_we_n,
	output reg        sdram_cas_n,
	output reg        sdram_ras_n,
	output reg [11:0] sdram_adr,
	output     [ 1:0] sdram_ba,
	
	/* Clocks we must wait following a PRECHARGE command (usually tRP). */
	output reg [2:0] tim_rp,
	/* Clocks we must wait following an ACTIVATE command (usually tRCD). */
	output reg [2:0] tim_rcd,
	/* CAS latency, 0 = 2 */
	output reg tim_cas,
	/* Auto-refresh period (usually tREFI). */
	output reg [10:0] tim_refi,
	/* Clocks we must wait following an AUTO REFRESH command (usually tRFC). */
	output reg [3:0] tim_rfc,
	/* Clocks we must wait following the last word written to the SDRAM (usually tWR). */
	output reg [1:0] tim_wr
);

wire csr_selected = csr_a[2] == csr_addr;

// We assume sdram_ba will be always zero, so we can truncate the bus to 16 bits
assign sdram_ba = 2'b00;

always @(posedge sys_clk) begin
	if(sys_rst) begin
		csr_do <= 32'd0;
	
		bypass <= 1'b1;
		sdram_rst <= 1'b1;
		
		sdram_cke <= 1'b0;
		sdram_adr <= 13'd0;
		
		tim_rp <= 3'd2;
		tim_rcd <= 3'd2;
		tim_cas <= 1'b0;
		tim_refi <= 11'd740;
		tim_rfc <= 4'd8;
		tim_wr <= 2'd2;
	end else begin
		sdram_cs_n <= 1'b1;
		sdram_we_n <= 1'b1;
		sdram_cas_n <= 1'b1;
		sdram_ras_n <= 1'b1;
		
		csr_do <= 32'd0;
		if(csr_selected) begin
			if(csr_we) begin
				case(csr_a[1:0])
					2'b00: begin
						bypass <= csr_di[0];
						sdram_rst <= csr_di[1];
						sdram_cke <= csr_di[2];
					end
					2'b01: begin
						sdram_cs_n <= ~csr_di[0];
						sdram_we_n <= ~csr_di[1];
						sdram_cas_n <= ~csr_di[2];
						sdram_ras_n <= ~csr_di[3];
						sdram_adr <= csr_di[15:4];
					end
					2'b10: begin
						tim_rp <= csr_di[2:0];
						tim_rcd <= csr_di[5:3];
						tim_cas <= csr_di[6];
						tim_rfc <= csr_di[10:7];
						tim_wr <= csr_di[12:11];
					end
          2'b11: begin
						tim_refi <= csr_di[10:0];
          end
				endcase
			end
			case(csr_a[1:0])
				2'b00: csr_do <= {sdram_cke, sdram_rst, bypass};
				2'b01: csr_do <= {sdram_adr, 4'h0};
				2'b10: csr_do <= {tim_wr, tim_rfc, tim_cas, tim_rcd, tim_rp};
        2'b11: csr_do <= {5'd0, tim_refi};
			endcase
		end
	end
end

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Module:      hpdmc.v
// Description: Wishbone based High Performance Dynamic Ram Controller
// --------------------------------------------------------------------
module hpdmc_datactl(
	input sys_clk,
	input sdram_rst,
	
	input read,
	input write,
	input [3:0] concerned_bank,
	output reg read_safe,
	output reg write_safe,
	output [3:0] precharge_safe,
	
	output reg ack,
	output reg direction,
	output direction_r,
	
	input tim_cas,
	input [1:0] tim_wr
);

/*
 * read_safe: whether it is safe to register a Read command
 * into the SDRAM at the next cycle.
 */

reg [2:0] read_safe_counter;
always @(posedge sys_clk) begin
	if(sdram_rst) begin
		read_safe_counter <= 3'd0;
		read_safe <= 1'b1;
	end else begin
		if(read) begin
			read_safe_counter <= 3'd7;
			read_safe <= 1'b0;
		end else if(write) begin
			/* after a write, read is unsafe for 9-CL cycles, therefore we load :
			 * 7 at CAS Latency 2 (tim_cas = 0)
			 * 6 at CAS Latency 3 (tim_cas = 1)
			 */
			read_safe_counter <= {2'b11, ~tim_cas};
			read_safe <= 1'b0;
		end else begin
			if(read_safe_counter == 3'd1)
				read_safe <= 1'b1;
			if(~read_safe)
				read_safe_counter <= read_safe_counter - 3'd1;
		end
	end
end

/*
 * write_safe: whether it is safe to register a Write command
 * into the SDRAM at the next cycle.
 */

reg [3:0] write_safe_counter;
always @(posedge sys_clk) begin
	if(sdram_rst) begin
		write_safe_counter <= 4'd0;
		write_safe <= 1'b1;
	end else begin
		if(read) begin
			write_safe_counter <= {3'b100, ~tim_cas};
			write_safe <= 1'b0;
		end else if(write) begin
			write_safe_counter <= 4'd7;
			write_safe <= 1'b0;
		end else begin
			if(write_safe_counter == 4'd1)
				write_safe <= 1'b1;
			if(~write_safe)
				write_safe_counter <= write_safe_counter - 4'd1;
		end
	end
end

/* Generate ack signal.
 * After write is asserted, it should pulse after 2 cycles.
 * After read is asserted, it should pulse after CL+2 cycles, that is
 * 4 cycles when tim_cas = 0
 * 5 cycles when tim_cas = 1
 */

reg ack_read2;
reg ack_read1;
reg ack_read0;

always @(posedge sys_clk) begin
	if(sdram_rst) begin
		ack_read2 <= 1'b0;
		ack_read1 <= 1'b0;
		ack_read0 <= 1'b0;
	end else begin
		if(tim_cas) begin
			ack_read2 <= read;
			ack_read1 <= ack_read2;
			ack_read0 <= ack_read1;
		end else begin
			ack_read1 <= read;
			ack_read0 <= ack_read1;
		end
	end
end

reg ack0;
always @(posedge sys_clk) begin
	if(sdram_rst) begin
		ack0 <= 1'b0;
		ack <= 1'b0;
	end else begin
		ack0 <= ack_read0;
		ack <= ack0|write;
	end
end

/* during a 8-word write, we drive the pins for 9 cycles
 * and 1 cycle in advance (first word is invalid)
 * so that we remove glitches on DQS without resorting
 * to asynchronous logic.
 */

/* direction must be glitch-free, as it directly drives the
 * tri-state enable for DQ and DQS.
 */
reg [3:0] counter_writedirection;
always @(posedge sys_clk) begin
	if(sdram_rst) begin
		counter_writedirection <= 4'd0;
		direction <= 1'b0;
	end else begin
		if(write) begin
			counter_writedirection <= 4'b1001;
			direction <= 1'b1;
		end else begin
			if(counter_writedirection == 4'b0001)
				direction <= 1'b0;
			if(direction)
				counter_writedirection <= counter_writedirection - 4'd1;
		end
	end
end

assign direction_r = write|(|counter_writedirection);

/* Counters that prevent a busy bank from being precharged */
hpdmc_banktimer banktimer0(
	.sys_clk(sys_clk),
	.sdram_rst(sdram_rst),
	
	.tim_cas(tim_cas),
	.tim_wr(tim_wr),
	
	.read(read & concerned_bank[0]),
	.write(write & concerned_bank[0]),
	.precharge_safe(precharge_safe[0])
);
hpdmc_banktimer banktimer1(
	.sys_clk(sys_clk),
	.sdram_rst(sdram_rst),
	
	.tim_cas(tim_cas),
	.tim_wr(tim_wr),
	
	.read(read & concerned_bank[1]),
	.write(write & concerned_bank[1]),
	.precharge_safe(precharge_safe[1])
);
hpdmc_banktimer banktimer2(
	.sys_clk(sys_clk),
	.sdram_rst(sdram_rst),
	
	.tim_cas(tim_cas),
	.tim_wr(tim_wr),
	
	.read(read & concerned_bank[2]),
	.write(write & concerned_bank[2]),
	.precharge_safe(precharge_safe[2])
);
hpdmc_banktimer banktimer3(
	.sys_clk(sys_clk),
	.sdram_rst(sdram_rst),
	
	.tim_cas(tim_cas),
	.tim_wr(tim_wr),
	
	.read(read & concerned_bank[3]),
	.write(write & concerned_bank[3]),
	.precharge_safe(precharge_safe[3])
);

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Module:      hpdmc.v
// Description: Wishbone based High Performance Dynamic Ram Controller
// --------------------------------------------------------------------

module hpdmc_mgmt #(
	parameter sdram_depth = 26,
	parameter sdram_columndepth = 9
) (
	input sys_clk,
	input sdram_rst,
	
	input [2:0] tim_rp,
	input [2:0] tim_rcd,
	input [10:0] tim_refi,
	input [3:0] tim_rfc,
	
	input stb,
	input we,
	input [sdram_depth-1-1:0] address, /* in 16-bit words */
	output reg ack,
	
	output reg read,
	output reg write,
	output [3:0] concerned_bank,
	input read_safe,
	input write_safe,
	input [3:0] precharge_safe,
	
	output sdram_cs_n,
	output sdram_we_n,
	output sdram_cas_n,
	output sdram_ras_n,
	output [11:0] sdram_adr,
	output [1:0] sdram_ba
);

/*
 * Address Mapping :
 * |    ROW ADDRESS   |    BANK NUMBER    |  COL ADDRESS  | for 16-bit words
 * |depth-1 coldepth+2|coldepth+1 coldepth|coldepth-1    0|
 * (depth for 16-bit words, which is sdram_depth-1)
 */

parameter rowdepth = sdram_depth-1-1-(sdram_columndepth+2)+1;

wire [sdram_depth-1-1:0] address16 = address;

wire [sdram_columndepth-1:0] col_address = address16[sdram_columndepth-1:0];
wire [1:0] bank_address = address16[sdram_columndepth+1:sdram_columndepth];
wire [rowdepth-1:0] row_address = address16[sdram_depth-1-1:sdram_columndepth+2];

reg [3:0] bank_address_onehot;
always @(*) begin
	case(bank_address)
		2'b00: bank_address_onehot <= 4'b0001;
		2'b01: bank_address_onehot <= 4'b0010;
		2'b10: bank_address_onehot <= 4'b0100;
		2'b11: bank_address_onehot <= 4'b1000;
	endcase
end

/* Track open rows */
reg [3:0] has_openrow;
reg [rowdepth-1:0] openrows[0:3];
reg [3:0] track_close;
reg [3:0] track_open;

always @(posedge sys_clk) begin
	if(sdram_rst) begin
		has_openrow = 4'h0;
	end else begin
		has_openrow = (has_openrow | track_open) & ~track_close;
		
		if(track_open[0]) openrows[0] <= row_address;
		if(track_open[1]) openrows[1] <= row_address;
		if(track_open[2]) openrows[2] <= row_address;
 		if(track_open[3]) openrows[3] <= row_address;
	end
end

/* Bank precharge safety */
assign concerned_bank = bank_address_onehot;
wire current_precharge_safe =
	 (precharge_safe[0] | ~bank_address_onehot[0])
	&(precharge_safe[1] | ~bank_address_onehot[1])
	&(precharge_safe[2] | ~bank_address_onehot[2])
	&(precharge_safe[3] | ~bank_address_onehot[3]);


/* Check for page hits */
wire bank_open = has_openrow[bank_address];
wire page_hit = bank_open & (openrows[bank_address] == row_address);

/* Address drivers */
reg sdram_adr_loadrow;
reg sdram_adr_loadcol;
reg sdram_adr_loadA10;
assign sdram_adr =
	 ({13{sdram_adr_loadrow}}	& row_address)
	|({13{sdram_adr_loadcol}}	& col_address)
	|({13{sdram_adr_loadA10}}	& 13'd1024);

assign sdram_ba = bank_address;

/* Command drivers */
reg sdram_cs;
reg sdram_we;
reg sdram_cas;
reg sdram_ras;
assign sdram_cs_n = ~sdram_cs;
assign sdram_we_n = ~sdram_we;
assign sdram_cas_n = ~sdram_cas;
assign sdram_ras_n = ~sdram_ras;

/* Timing counters */

/* The number of clocks we must wait following a PRECHARGE command (usually tRP). */
reg [2:0] precharge_counter;
reg reload_precharge_counter;
wire precharge_done = (precharge_counter == 3'd0);
always @(posedge sys_clk) begin
	if(reload_precharge_counter)
		precharge_counter <= tim_rp;
	else if(~precharge_done)
		precharge_counter <= precharge_counter - 3'd1;
end

/* The number of clocks we must wait following an ACTIVATE command (usually tRCD). */
reg [2:0] activate_counter;
reg reload_activate_counter;
wire activate_done = (activate_counter == 3'd0);
always @(posedge sys_clk) begin
	if(reload_activate_counter)
		activate_counter <= tim_rcd;
	else if(~activate_done)
		activate_counter <= activate_counter - 3'd1;
end

/* The number of clocks we have left before we must refresh one row in the SDRAM array (usually tREFI). */
reg [10:0] refresh_counter;
reg reload_refresh_counter;
wire must_refresh = refresh_counter == 11'd0;
always @(posedge sys_clk) begin
	if(sdram_rst)
		refresh_counter <= 11'd0;
	else begin
		if(reload_refresh_counter)
			refresh_counter <= tim_refi;
		else if(~must_refresh)
			refresh_counter <= refresh_counter - 11'd1;
	end
end

/* The number of clocks we must wait following an AUTO REFRESH command (usually tRFC). */
reg [3:0] autorefresh_counter;
reg reload_autorefresh_counter;
wire autorefresh_done = (autorefresh_counter == 4'd0);
always @(posedge sys_clk) begin
	if(reload_autorefresh_counter)
		autorefresh_counter <= tim_rfc;
	else if(~autorefresh_done)
		autorefresh_counter <= autorefresh_counter - 4'd1;
end

/* FSM that pushes commands into the SDRAM */

reg [3:0] state;
reg [3:0] next_state;

parameter IDLE			= 4'd0;
parameter ACTIVATE		= 4'd1;
parameter READ			= 4'd2;
parameter WRITE			= 4'd3;
parameter PRECHARGEALL		= 4'd4;
parameter AUTOREFRESH		= 4'd5;
parameter AUTOREFRESH_WAIT	= 4'd6;

always @(posedge sys_clk) begin
	if(sdram_rst)
		state <= IDLE;
	else begin
		//$display("state: %d -> %d", state, next_state);
		state <= next_state;
	end
end

always @(*) begin
	next_state = state;
	
	reload_precharge_counter = 1'b0;
	reload_activate_counter = 1'b0;
	reload_refresh_counter = 1'b0;
	reload_autorefresh_counter = 1'b0;
	
	sdram_cs = 1'b0;
	sdram_we = 1'b0;
	sdram_cas = 1'b0;
	sdram_ras = 1'b0;
	
	sdram_adr_loadrow = 1'b0;
	sdram_adr_loadcol = 1'b0;
	sdram_adr_loadA10 = 1'b0;
	
	track_close = 4'b0000;
	track_open = 4'b0000;
	
	read = 1'b0;
	write = 1'b0;
	
	ack = 1'b0;
	
	case(state)
		IDLE: begin
			if(must_refresh)
				next_state = PRECHARGEALL;
			else begin
				if(stb) begin
					if(page_hit) begin
						if(we) begin
							if(write_safe) begin
								/* Write */
								sdram_cs = 1'b1;
								sdram_ras = 1'b0;
								sdram_cas = 1'b1;
								sdram_we = 1'b1;
								sdram_adr_loadcol = 1'b1;
								
								write = 1'b1;
								ack = 1'b1;
							end
						end else begin
							if(read_safe) begin
								/* Read */
								sdram_cs = 1'b1;
								sdram_ras = 1'b0;
								sdram_cas = 1'b1;
								sdram_we = 1'b0;
								sdram_adr_loadcol = 1'b1;
								
								read = 1'b1;
								ack = 1'b1;
							end
						end
					end else begin
						if(bank_open) begin
							if(current_precharge_safe) begin
								/* Precharge Bank */
								sdram_cs = 1'b1;
								sdram_ras = 1'b1;
								sdram_cas = 1'b0;
								sdram_we = 1'b1;
								
								track_close = bank_address_onehot;
								reload_precharge_counter = 1'b1;
								next_state = ACTIVATE;
							end
						end else begin
							/* Activate */
							sdram_cs = 1'b1;
							sdram_ras = 1'b1;
							sdram_cas = 1'b0;
							sdram_we = 1'b0;
							sdram_adr_loadrow = 1'b1;
				
							track_open = bank_address_onehot;
							reload_activate_counter = 1'b1;
							if(we)
								next_state = WRITE;
							else
								next_state = READ;
						end
					end
				end
			end
		end
		
		ACTIVATE: begin
			if(precharge_done) begin
				sdram_cs = 1'b1;
				sdram_ras = 1'b1;
				sdram_cas = 1'b0;
				sdram_we = 1'b0;
				sdram_adr_loadrow = 1'b1;
				
				track_open = bank_address_onehot;
				reload_activate_counter = 1'b1;
				if(we)
					next_state = WRITE;
				else
					next_state = READ;
			end
		end
		READ: begin
			if(activate_done) begin
				if(read_safe) begin
					sdram_cs = 1'b1;
					sdram_ras = 1'b0;
					sdram_cas = 1'b1;
					sdram_we = 1'b0;
					sdram_adr_loadcol = 1'b1;
					
					read = 1'b1;
					ack = 1'b1;
					next_state = IDLE;
				end
			end
		end
		WRITE: begin
			if(activate_done) begin
				if(write_safe) begin
					sdram_cs = 1'b1;
					sdram_ras = 1'b0;
					sdram_cas = 1'b1;
					sdram_we = 1'b1;
					sdram_adr_loadcol = 1'b1;
					
					write = 1'b1;
					ack = 1'b1;
					next_state = IDLE;
				end
			end
		end
		
		PRECHARGEALL: begin
			if(precharge_safe == 4'b1111) begin
				sdram_cs = 1'b1;
				sdram_ras = 1'b1;
				sdram_cas = 1'b0;
				sdram_we = 1'b1;
				sdram_adr_loadA10 = 1'b1;
	
				reload_precharge_counter = 1'b1;
				
				track_close = 4'b1111;
				
				next_state = AUTOREFRESH;
			end
		end
		AUTOREFRESH: begin
			if(precharge_done) begin
				sdram_cs = 1'b1;
				sdram_ras = 1'b1;
				sdram_cas = 1'b1;
				sdram_we = 1'b0;
				reload_refresh_counter = 1'b1;
				reload_autorefresh_counter = 1'b1;
				next_state = AUTOREFRESH_WAIT;
			end
		end
		AUTOREFRESH_WAIT: begin
			if(autorefresh_done)
				next_state = IDLE;
		end
		
	endcase
end

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------
