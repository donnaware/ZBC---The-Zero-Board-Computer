// --------------------------------------------------------------------
// --------------------------------------------------------------------
// Module:      gpio.v
// Description: Wishbone based general purpose IO
// --------------------------------------------------------------------
// --------------------------------------------------------------------
module gpio (
    input         wb_clk_i,	    // Wishbone slave interface
    input         wb_rst_i,
    input         wb_adr_i,
    output [15:0] wb_dat_o,
    input  [15:0] wb_dat_i,
    input  [ 1:0] wb_sel_i,
    input         wb_we_i,
    input         wb_stb_i,
    input         wb_cyc_i,
    output        wb_ack_o,

    output reg [7:0] leds_,    // GPIO inputs/outputs
    input      [7:0] sw_
  );

  wire   op;

  assign op       = wb_cyc_i & wb_stb_i;
  assign wb_ack_o = op;
  assign wb_dat_o = wb_adr_i ? { 8'h00, leds_ } : { 8'h00, sw_ };

  always @(posedge wb_clk_i)
    leds_ <= wb_rst_i ? 8'h0 : ((op & wb_we_i & wb_adr_i) ? wb_dat_i[7:0] : leds_);

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------
