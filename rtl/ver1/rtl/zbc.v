// --------------------------------------------------------------------
// --------------------------------------------------------------------
// Module:      ZBC.v
// Description: Wishbone based SoC
// --------------------------------------------------------------------
// --------------------------------------------------------------------
module zbc (
    input			clk_50_,			// Main Clock oscilator input (50Mhz)
    input			reset_,				// Reset input

    output [1:0]	GPIO_Out_,			// GPIO outputs
    input  [1:0]	GPIO_In_,			// GPIO Inputs
    input  [3:0]	GamePort_In_,		// Game Port input pins
	input  [1:0]	MCU_In_,			// General purpose Inputs from MCU

	output			speaker_L,			// Speaker output, Left channel	
	output			speaker_R,			// Speaker output, Right channel	

    output [11:0]	sdram_addr_,		// SDRAM Address Pins
    inout  [15:0]	sdram_data_,		// SDRAM Data
    output [ 1:0]	sdram_ba_,			// SDRAM Bank Address
    output			sdram_ras_n_,		// SDRAM Row Address Strobe
    output			sdram_cas_n_,		// SDRAM Column Address Strobe
    output			sdram_ce_,			// SDRAM Clock Enable
    output			sdram_clk_,			// SDRAM Clock
    output			sdram_we_n_,		// SDRAM Write Enable
    output			sdram_cs_n_,		// SDRAM Chip Select

    output [ 3:0]	vga_lcd_r_,			// VGA RED signals
    output [ 3:0]	vga_lcd_g_,			// VGA Green signals
    output [ 3:0]	vga_lcd_b_,			// VGA Blue signals
    output			vga_lcd_hsync_,		// VGA Horizontal Sync pulse
    output			vga_lcd_vsync_,		// VGA Vertical Sync pulse

    input			uart_rxd_,			// UART Receive signal
    output			uart_txd_,			// UART Transmit signal

	input 			Ethernet_rx_,		// Ethernet 10BASE-T Input
	output 			Ethernet_tx_, 		// Ethernet 10BASE-T output

    inout			ps2_kclk_,			// PS2 keyboard Clock
    inout			ps2_kdat_,			// PS2 Keyboard Data
    inout			ps2_mclk_,			// PS2 Mouse Clock
    inout			ps2_mdat_,			// PS2 Mouse Data
    
    output			sd_sclk_,			// SD card clock signal
    input			sd_miso_,			// SD card clock signal
    output			sd_mosi_,			// SD card Master In, Slave Out (zet is master)
    output			sd_ss_,				// SD card Select line 

    output			spi_sclk_,			// SPI pad signal for flash and MCU
    input			spi_miso_,			// SPI Master In, Slave Out (zet is master)
    output			spi_mosi_,			// SPI Master out, Slave In
    output			spi_sels_,			// SPI Select line for Flash
    output			spi_mcus_			// SPI Select line for and MCU
  );

  // --------------------------------------------------------------------
  // Main Wishbone Wires
  // --------------------------------------------------------------------
  wire [15:0] sw_dat_o;			// WB Switch Data Output lines
  wire [15:0] dat_o;			// Wishbone Master Data out
  wire [15:0] dat_i;			// Wishbone Master Data in
  wire [19:1] adr;				// Wishbone Master 
  wire [ 1:0] sel;				// Wishbone Master 
  wire        we;				// Wishbone Master 
  wire        tga;				// Wishbone Master 
  wire        stb;				// Wishbone Master 
  wire        cyc;				// Wishbone Master 
  wire        ack;				// Wishbone Master 
  wire 		  def_cyc_i;		// WB default cyc 
  wire 		  def_stb_i;		// WB default ack

  // --------------------------------------------------------------------
  // Phased Locked Loop instantiation
  // --------------------------------------------------------------------
  wire		clk12;			//  12.5 Mhz Clock
  wire		clk20;			//  20.0 Mhz Clock
  wire		clk25;			//  25.0 Mhz Clock
  wire		clk40;			//  40.0 Mhz Clock
  wire		clk100;			// 100.0 Mhz Clock
  wire      lock;			// PLL is Locked line
  pll   pll1(
	.inclk0	( clk_50_   ),	//  50.0 Mhz Clock
	.c0		( clk100    ),	// 100.0 Mhz Clock
	.c1		( clk25     ),	//  25.0 Mhz Clock
	.c2 	( clk12     ),	//  12.5 Mhz Clock
	.c3 	( clk40     ),	//  40.0 Mhz Clock
	.c4 	( clk20     ),	//  20.0 Mhz Clock
	.locked ( lock		)	// Locked output
	);  
  
  wire        clk		= clk12;	// Wishbone Master Clock
  wire        vga_clk	= clk25;	// VGA Clock
  wire		  sdram_clk	= clk100; 	// SDRAM Clock
  
  // --------------------------------------------------------------------
  // Reset debounce control
  // --------------------------------------------------------------------
  wire        rst_lck;				// Reset Lock out
  assign      rst_lck = reset_ & lock;
  reg  [16:0] rst_debounce;		// Reset debouse counter
   `ifndef SIMULATION	
     initial rst_debounce <= 17'h1FFFF;	// Debounce it (counter holds reset for 10.49ms), and generate power-on reset.
     reg rst;
     initial rst <= 1'b1;
     always @(posedge clk) begin
        if(~rst_lck) rst_debounce <= 17'h1FFFF;   /* reset is active low */
        else if(rst_debounce != 17'd0) rst_debounce <= rst_debounce - 17'd1;
        rst <= rst_debounce != 17'd0;
     end
  `else
     wire rst;
     assign rst = !rst_lck;
  `endif

  // --------------------------------------------------------------------
  // Wishbone Compatible BIOS ROM core 
  // --------------------------------------------------------------------
  wire [15:0] rom_dat_o;	  		// BIOS ROM Data output 
  wire [15:0] rom_dat_i;	  		// BIOS ROM Data input 
  wire        rom_tga_i;	  		// BIOS ROM Address Tag
  wire [19:1] rom_adr_i;	  		// BIOS ROM Address
  wire [ 1:0] rom_sel_i;	  		// BIOS ROM byte select (upper or lower)
  wire        rom_we_i;	  			// BIOS ROM Write Enable
  wire        rom_cyc_i;	  		// BIOS ROM Cycle
  wire        rom_stb_i;	  		// BIOS ROM Strobe
  wire        rom_ack_o;	  		// BIOS ROM Acknowledge 
  BIOSROM bios(						// BIOS Shadow Boot ROM 
    .wb_clk_i (clk),				// On FPGA ROM mapped to 0xFFF00
    .wb_rst_i (rst),
    .wb_dat_i (rom_dat_i),
    .wb_dat_o (rom_dat_o),
    .wb_adr_i (rom_adr_i),
    .wb_we_i  (rom_we_i),
    .wb_tga_i (rom_tga_i),
    .wb_stb_i (rom_stb_i),
    .wb_cyc_i (rom_cyc_i),
    .wb_sel_i (rom_sel_i),
    .wb_ack_o (rom_ack_o)
  );

  // --------------------------------------------------------------------
  // Wishbone based Fast Memory Link Bridge
  // --------------------------------------------------------------------
  wire [19:1] fmlbrg_adr_s;			// SDRAM FML Bridge
  wire [15:0] fmlbrg_dat_w_s;		// SDRAM FML Bridge
  wire [15:0] fmlbrg_dat_r_s;		// SDRAM FML Bridge
  wire [ 1:0] fmlbrg_sel_s;			// SDRAM FML Bridge
  wire        fmlbrg_cyc_s;			// SDRAM FML Bridge
  wire        fmlbrg_stb_s;			// SDRAM FML Bridge
  wire        fmlbrg_tga_s;			// SDRAM FML Bridge
  wire        fmlbrg_we_s;			// SDRAM FML Bridge
  wire        fmlbrg_ack_s;			// SDRAM FML Bridge  
  wb_abrgr wb_fmlbrg (				// FML to WB bridge
    .sys_rst (rst),
    .wbs_clk_i (clk),			    // Wishbone slave interface
    .wbs_adr_i (fmlbrg_adr_s),
    .wbs_dat_i (fmlbrg_dat_w_s),
    .wbs_dat_o (fmlbrg_dat_r_s),
    .wbs_sel_i (fmlbrg_sel_s),
    .wbs_tga_i (fmlbrg_tga_s),
    .wbs_stb_i (fmlbrg_stb_s),
    .wbs_cyc_i (fmlbrg_cyc_s),
    .wbs_we_i  (fmlbrg_we_s),
    .wbs_ack_o (fmlbrg_ack_s),
    .wbm_clk_i (sdram_clk),			// Wishbone master interface
    .wbm_adr_o (fmlbrg_adr),
    .wbm_dat_o (fmlbrg_dat_w),
    .wbm_dat_i (fmlbrg_dat_r),
    .wbm_sel_o (fmlbrg_sel),
    .wbm_tga_o (fmlbrg_tga),
    .wbm_stb_o (fmlbrg_stb),
    .wbm_cyc_o (fmlbrg_cyc),
    .wbm_we_o  (fmlbrg_we),
    .wbm_ack_i (fmlbrg_ack)
  );

  // --------------------------------------------------------------------
  // Wishbone based Fast Memory Link Bridge
  // --------------------------------------------------------------------
  wire [19:1] fmlbrg_adr;			// SDRAM FML Bridge
  wire [15:0] fmlbrg_dat_w;
  wire [15:0] fmlbrg_dat_r;
  wire [ 1:0] fmlbrg_sel;
  wire        fmlbrg_cyc;
  wire        fmlbrg_stb;
  wire        fmlbrg_tga;
  wire        fmlbrg_we;
  wire        fmlbrg_ack; 
  fmlbrg #(.fml_depth(23), .cache_depth (10))   // 1 Kbyte cache
    fmlbrg (
    .sys_clk  (sdram_clk),
    .sys_rst  (rst),
    .wb_adr_i ({3'b000,fmlbrg_adr}),	// Wishbone slave interface
    .wb_dat_i (fmlbrg_dat_w),
    .wb_dat_o (fmlbrg_dat_r),
    .wb_sel_i (fmlbrg_sel),
    .wb_cyc_i (fmlbrg_cyc),
    .wb_stb_i (fmlbrg_stb),
    .wb_tga_i (fmlbrg_tga),
    .wb_we_i  (fmlbrg_we),
    .wb_ack_o (fmlbrg_ack),
    .fml_adr  (fml_adr),					// FML master interface
    .fml_stb  (fml_stb),
    .fml_we   (fml_we),
    .fml_ack  (fml_ack),
    .fml_sel  (fml_sel),
    .fml_do   (fml_do),
    .fml_di   (fml_di)
  );

  // --------------------------------------------------------------------
  // Configuration and Staus Register(CSR)Bus
  // --------------------------------------------------------------------
  wire [19:1] csrbrg_adr_s;			// CSR Bridge for SDRAM
  wire [15:0] csrbrg_dat_w_s;
  wire [15:0] csrbrg_dat_r_s;
  wire [ 1:0] csrbrg_sel_s;
  wire        csrbrg_cyc_s;
  wire        csrbrg_stb_s;
  wire        csrbrg_tga_s;
  wire        csrbrg_we_s;
  wire        csrbrg_ack_s;
  wb_abrgr wb_csrbrg (					// CSR Bridge
    .sys_rst  (rst),
    .wbs_clk_i (clk),					// Wishbone slave interface
    .wbs_adr_i (csrbrg_adr_s),
    .wbs_dat_i (csrbrg_dat_w_s),
    .wbs_dat_o (csrbrg_dat_r_s),
    .wbs_stb_i (csrbrg_stb_s),
    .wbs_cyc_i (csrbrg_cyc_s),
    .wbs_we_i  (csrbrg_we_s),
    .wbs_ack_o (csrbrg_ack_s),
    .wbm_clk_i (sdram_clk),				// Wishbone master interface
    .wbm_adr_o (csrbrg_adr),
    .wbm_dat_o (csrbrg_dat_w),
    .wbm_dat_i (csrbrg_dat_r),
    .wbm_stb_o (csrbrg_stb),
    .wbm_cyc_o (csrbrg_cyc),
    .wbm_we_o  (csrbrg_we),
    .wbm_ack_i (csrbrg_ack)
  );

  // --------------------------------------------------------------------
  // Configuration and Staus Register(CSR)Bus
  // --------------------------------------------------------------------
  wire [19:1] csrbrg_adr;
  wire [15:0] csrbrg_dat_w;
  wire [15:0] csrbrg_dat_r;
  wire        csrbrg_cyc;
  wire        csrbrg_stb;
  wire        csrbrg_we;
  wire        csrbrg_ack;
  wire [ 2:0] csr_a;
  wire        csr_we;
  wire [15:0] csr_dw;
  wire [15:0] csr_dr_hpdmc;   
  csrbrg csrbrg (						// CSR interface
    .sys_clk  (sdram_clk),
    .sys_rst  (rst),
    .wb_adr_i (csrbrg_adr[3:1]),		// Wishbone slave interface
    .wb_dat_i (csrbrg_dat_w),
    .wb_dat_o (csrbrg_dat_r),
    .wb_cyc_i (csrbrg_cyc),
    .wb_stb_i (csrbrg_stb),
    .wb_we_i  (csrbrg_we),
    .wb_ack_o (csrbrg_ack),
    .csr_a    (csr_a),					// CSR master interface
    .csr_we   (csr_we),
    .csr_do   (csr_dw),
    .csr_di   (csr_dr_hpdmc)
  );

  // --------------------------------------------------------------------
  // High Power Dynamic Ram Memory Controller
  // --------------------------------------------------------------------
  assign 	  sdram_clk_ = sdram_clk;
  wire [ 1:0] sdram_dqm_;		// SDRAM dqm pins (tied low in hardware)
  wire [22:0] fml_adr;
  wire        fml_stb;
  wire        fml_we;
  wire        fml_ack;
  wire [ 1:0] fml_sel;
  wire [15:0] fml_di;
  wire [15:0] fml_do;   
  hpdmc #(.csr_addr(1'b0),.sdram_depth(23),.sdram_columndepth(8)) hpdmc (
    .sys_clk 	 (sdram_clk),
    .sys_rst 	 (rst),
    .csr_a  	 (csr_a),				// CSR slave interface
    .csr_we 	 (csr_we),
    .csr_di 	 (csr_dw),
    .csr_do 	 (csr_dr_hpdmc),
    .fml_adr 	 (fml_adr),			// FML slave interface
    .fml_stb 	 (fml_stb),
    .fml_we  	 (fml_we),
    .fml_ack 	 (fml_ack),
    .fml_sel 	 (fml_sel),
    .fml_di  	 (fml_do),
    .fml_do  	 (fml_di),
    .sdram_cke   (sdram_ce_),		// SDRAM pad signals
    .sdram_cs_n  (sdram_cs_n_),
    .sdram_we_n  (sdram_we_n_),
    .sdram_cas_n (sdram_cas_n_),
    .sdram_ras_n (sdram_ras_n_),
    .sdram_dqm   (sdram_dqm_),
    .sdram_adr   (sdram_addr_),
    .sdram_ba    (sdram_ba_),
    .sdram_dq    (sdram_data_)
  );

  // --------------------------------------------------------------------
  // VGA Bridge
  // --------------------------------------------------------------------
  wire [15:0] vga_dat_o_s;			// These lines are for the async bridge
  wire [15:0] vga_dat_i_s;			// to cross clock domain of synchronized signals
  wire        vga_tga_i_s;			// VGA Async Bridge
  wire [19:1] vga_adr_i_s;			// VGA Async Bridge
  wire [ 1:0] vga_sel_i_s;			// VGA Async Bridge
  wire        vga_we_i_s;			// VGA Async Bridge
  wire        vga_cyc_i_s;			// VGA Async Bridge
  wire        vga_stb_i_s;			// VGA Async Bridge
  wire        vga_ack_o_s;			// VGA Async Bridge
  wb_abrg vga_brg (					
    .sys_rst   (rst),
    .wbs_clk_i (clk),				// Wishbone slave interface
    .wbs_adr_i (vga_adr_i_s),
    .wbs_dat_i (vga_dat_i_s),
    .wbs_dat_o (vga_dat_o_s),
    .wbs_sel_i (vga_sel_i_s),
    .wbs_tga_i (vga_tga_i_s),
    .wbs_stb_i (vga_stb_i_s),
    .wbs_cyc_i (vga_cyc_i_s),
    .wbs_we_i  (vga_we_i_s),
    .wbs_ack_o (vga_ack_o_s),
    .wbm_clk_i (vga_clk),		 	// Wishbone master interface
    .wbm_adr_o (vga_adr_i),
    .wbm_dat_o (vga_dat_i),
    .wbm_dat_i (vga_dat_o),
    .wbm_sel_o (vga_sel_i),
    .wbm_tga_o (vga_tga_i),
    .wbm_stb_o (vga_stb_i),
    .wbm_cyc_o (vga_cyc_i),
    .wbm_we_o  (vga_we_i),
    .wbm_ack_i (vga_ack_o)
  );

  // --------------------------------------------------------------------
  // VGA Text Interface
  // --------------------------------------------------------------------
  wire [15:0] vga_dat_o;			// VGA controller Data output 
  wire [15:0] vga_dat_i;			// VGA controller Data input 
  wire        vga_tga_i;			// VGA controller Address Tag
  wire [19:1] vga_adr_i;			// VGA controller Address
  wire [ 1:0] vga_sel_i;			// VGA controller byte select (upper or lower)
  wire        vga_we_i;				// VGA controller Write Enable
  wire        vga_cyc_i;			// VGA controller Cycle
  wire        vga_stb_i;			// VGA controller Strobe
  wire        vga_ack_o;			// VGA controller Acknowledge
  assign 	  vga_lcd_r_[1:0] = vga_lcd_r_[3:2];	// Tied like this for Text only
  assign 	  vga_lcd_g_[1:0] = vga_lcd_g_[3:2];	// Tied like this for Text only
  assign 	  vga_lcd_b_[1:0] = vga_lcd_b_[3:2];	// Tied like this for Text only  
  vdu vga (							
    .wb_rst_i (rst),				// Wishbone slave interface
    .wb_clk_i (vga_clk),   			// 25MHz VGA clock
    .wb_dat_i (vga_dat_i),
    .wb_dat_o (vga_dat_o),
    .wb_adr_i (vga_adr_i),    
    .wb_we_i  (vga_we_i),
    .wb_tga_i (vga_tga_i),
    .wb_sel_i (vga_sel_i),
    .wb_stb_i (vga_stb_i),
    .wb_cyc_i (vga_cyc_i),
    .wb_ack_o (vga_ack_o),
    .vga_red_o   (vga_lcd_r_[3:2]),		// VGA pad signals
    .vga_green_o (vga_lcd_g_[3:2]),
    .vga_blue_o  (vga_lcd_b_[3:2]),
    .horiz_sync  (vga_lcd_hsync_),
    .vert_sync   (vga_lcd_vsync_)
  );

  // --------------------------------------------------------------------
  // 10BaseT Interface
  // --------------------------------------------------------------------
  wire [15:0] wb_net_dat_o;         // 10BaseT Network controller
  wire [15:0] wb_net_dat_i;         // 10BaseT  Network controller
  wire        wb_net_tga_i;         // 10BaseT  Network controller
  wire [ 2:1] wb_net_adr_i;         // 10BaseT  Network controller
  wire [ 1:0] wb_net_sel_i;         // 10BaseT  Network controller
  wire        wb_net_we_i;          // 10BaseT  Network controller
  wire        wb_net_cyc_i;         // 10BaseT  Network controller
  wire        wb_net_stb_i;         // 10BaseT  Network controller
  wire        wb_net_ack_o;         // 10BaseT  Network controller
  WB_Ethernet Ethernet(				
    .wb_clk_i(clk),					// Main Clock 
    .wb_rst_i(rst),  				// Reset Line
    .wb_dat_i(wb_net_dat_i),        // Command to send to Ethernet
    .wb_dat_o(wb_net_dat_o),        // Received data
    .wb_cyc_i(wb_net_cyc_i),        // Cycle
    .wb_stb_i(wb_net_stb_i),        // Strobe
    .wb_adr_i(wb_net_adr_i),        // Address lines
    .wb_sel_i(wb_net_sel_i),        // Select lines
    .wb_we_i(wb_net_we_i),          // Write enable
    .wb_ack_o(wb_net_ack_o),        // Normal bus termination
    .wb_tgc_o(intv[7]),             // Ethernet Interrupt request
	
	.clk20(clk20),					// Clock for transmitting
	.clk40(clk40),					// Clock for Receiving
    .Ethernet_Rx(Ethernet_rx_),     // Etherent 10BASE-T receive 
    .Ethernet_Tx(Ethernet_tx_)      // Ethernet 10BASE-T transmit
  );
  
  // --------------------------------------------------------------------
  // Sound Module Instantiation
  // --------------------------------------------------------------------
  wire [19:1] wb_sb_adr_i;			// Sound Blaster Address 15:1 
  wire [15:0] wb_sb_dat_i;        	// Sound Blaster 
  wire [15:0] wb_sb_dat_o;        	// Sound Blaster 
  wire [ 1:0] wb_sb_sel_i;        	// Sound Blaster 
  wire        wb_sb_cyc_i;     		// Sound Blaster 
  wire        wb_sb_stb_i;     		// Sound Blaster 
  wire        wb_sb_we_i;      		// Sound Blaster 
  wire        wb_sb_ack_o;     		// Sound Blaster 
  wire        wb_sb_tga_i;			// Sound Blaster 
  sound snd1(					
    .wb_clk_i (clk),				// Main Clock 
    .wb_rst_i (rst),  				// Reset Line
    .wb_dat_i (wb_sb_dat_i),        // Command to send 
    .wb_dat_o (wb_sb_dat_o),        // Received data
    .wb_cyc_i (wb_sb_cyc_i),        // Cycle
    .wb_stb_i (wb_sb_stb_i),        // Strobe
    .wb_adr_i (wb_sb_adr_i[3:1]),	// Address lines
    .wb_sel_i (wb_sb_sel_i),        // Select lines
    .wb_we_i  (wb_sb_we_i),         // Write enable
    .wb_ack_o (wb_sb_ack_o),        // Normal bus termination

    .dac_clk(clk_50_),				// DAC Clock
    .audio_L(speaker_L),			// Audio Output Left  Channel
    .audio_R(speaker_R)				// Audio Output Right Channel
  );

  // --------------------------------------------------------------------
  // RS232 COM1 Port
  // --------------------------------------------------------------------
  wire [15:0] uart_dat_o;			// uart controller
  wire [15:0] uart_dat_i;			// uart controller
  wire        uart_tga_i;			// uart controller
  wire [19:1] uart_adr_i;			// uart controller
  wire [ 1:0] uart_sel_i;			// uart controller
  wire        uart_we_i;			// uart controller
  wire        uart_cyc_i;			// uart controller
  wire        uart_stb_i;			// uart controller
  wire        uart_ack_o;			// uart controller
  WB_Serial uart1(					
    .wb_clk_i (clk),				// Main Clock 
    .wb_rst_i (rst),  				// Reset Line
    .wb_adr_i (uart_adr_i[2:1]),	// Address lines
    .wb_sel_i (uart_sel_i),			// Select lines
    .wb_dat_i (uart_dat_i),			// Command to send 
    .wb_dat_o (uart_dat_o),
    .wb_we_i  (uart_we_i),          // Write enable
    .wb_stb_i (uart_stb_i),
    .wb_cyc_i (uart_cyc_i),
    .wb_ack_o (uart_ack_o),
    .wb_tgc_o (intv[4]),            // Interrupt request

    .rs232_tx (uart_txd_),			// UART signals
    .rs232_rx (uart_rxd_)			// serial input/output
  );

  // --------------------------------------------------------------------
  // SPI On Board FLASH 
  // --------------------------------------------------------------------
  wire [15:0] wb_spi_dat_o;			// Flash RAM SPI interface
  wire [15:0] wb_spi_dat_i;     	// Flash RAM SPI interface
  wire [ 1:0] wb_spi_sel_i;     	// Flash RAM SPI interface
  wire [19:1] wb_spi_adr_i;     	// Flash RAM SPI interface
  wire        wb_spi_tga_i;     	// Flash RAM SPI interface
  wire        wb_spi_we_i;     		// Flash RAM SPI interface
  wire        wb_spi_cyc_i;     	// Flash RAM SPI interface
  wire        wb_spi_stb_i;     	// Flash RAM SPI interface
  wire        wb_spi_ack_o;       	// Flash RAM SPI interface
  WB_SPI_Flash SPI_Flash(			
    .wb_clk_i(clk),					// Main Clock 
    .wb_rst_i(rst),  				// Reset Line
    .wb_dat_i(wb_spi_dat_i),	    // Command to send
    .wb_dat_o(wb_spi_dat_o[7:0]),   // Received data
    .wb_cyc_i(wb_spi_cyc_i),        // Cycle
    .wb_stb_i(wb_spi_stb_i),        // Strobe
    .wb_sel_i(wb_spi_sel_i),        // Select lines
    .wb_we_i(wb_spi_we_i),          // Write enable
    .wb_ack_o(wb_spi_ack_o),        // Normal bus termination

    .sclk(spi_sclk_),				// SPI pad signal for flash and MCU
    .miso(spi_miso_),				// SPI Master In, Slave Out (zet is master)
    .mosi(spi_mosi_),				// SPI Master out, Slave In
    .sels(spi_sels_),				// SPI Select line for Flash
    .mcus(spi_mcus_)				// SPI Select line for and MCU
  );

  // --------------------------------------------------------------------
  // PS2 Keyboard + Mouse Interface
  // --------------------------------------------------------------------
  wire [15:0] keyb_dat_o;			// Keyboard controller
  wire [15:0] keyb_dat_i;			// Keyboard controller
  wire        keyb_tga_i;			// Keyboard controller
  wire [19:1] keyb_adr_i;			// Keyboard controller
  wire [ 1:0] keyb_sel_i;			// Keyboard controller
  wire        keyb_we_i;			// Keyboard controller
  wire        keyb_cyc_i;			// Keyboard controller
  wire        keyb_stb_i;			// Keyboard controller
  wire        keyb_ack_o;			// Keyboard controller
  WB_PS2 PS2(						
    .wb_clk_i(clk),					// Main Clock 
    .wb_rst_i(rst),  				// Reset Line
    .wb_adr_i(keyb_adr_i[2:1]),		// Address lines
    .wb_sel_i(keyb_sel_i),			// Select lines
    .wb_dat_i(keyb_dat_i),			// Command to send to Ethernet
    .wb_dat_o(keyb_dat_o),
    .wb_we_i(keyb_we_i),            // Write enable
    .wb_stb_i(keyb_stb_i),
    .wb_cyc_i(keyb_cyc_i),
    .wb_ack_o(keyb_ack_o),
    .wb_tgk_o(intv[1]),             // Keyboard Interrupt request
    .wb_tgm_o(intv[3]),             // Mouse Interrupt request

    .PS2_KBD_CLK(ps2_kclk_),.PS2_KBD_DAT(ps2_kdat_),
	.PS2_MSE_CLK(ps2_mclk_),.PS2_MSE_DAT(ps2_mdat_)
  );  
  
  // --------------------------------------------------------------------
  // Super Simple Timer controller
  // --------------------------------------------------------------------
  timer #(.res(33), .phase (12507))  timer0 (
    .wb_clk_i (clk),
    .wb_rst_i (rst),
    .wb_tgc_o (intv[0])
  );

  // --------------------------------------------------------------------
  // Super Simple Priority Interupt controller
  //
  // Interupt   Assignment          Typical assignment 
  // --------   -----------------   -------------------------------------
  //   int[0] - System Timer        timer (55ms intervals, 18.21590 per second)
  //   int[1] - Keyboard            keyboard service 
  //   int[2] - Reserved            slave 8259 or EGA/VGA vertical retrace
  //   int[3] - Mouse               COM2 service
  //   int[4] - RS232 UART          COM1 service 
  //   int[5] -                     fixed disk or data request from LPT2
  //   int[6] -                     floppy disk service 
  //   int[7] - 10BaseT Interface   data request from LPT1 
  // --------------------------------------------------------------------
  wire [ 7:0] intv;				// Interupt Vectors
  wire [ 2:0] iid;				// Interupt ID register
  wire        intr;				// CPU Interupt output line
  wire        inta;				// CPU Interupt Input line
  simple_pic pic0 (					
    .clk  (clk),
    .rst  (rst),
    .intv (intv),
    .inta (inta),
    .intr (intr),
    .iid  (iid)
  );

  // --------------------------------------------------------------------
  // SD CARD to WB Async Bridge
  // --------------------------------------------------------------------
  wire [15:0] sd_dat_o;				// SD Card controller
  wire [15:0] sd_dat_i;				// SD Card controller
  wire [ 1:0] sd_sel_i;				// SD Card controller
  wire        sd_we_i;				// SD Card controller
  wire        sd_cyc_i;				// SD Card controller
  wire        sd_stb_i;				// SD Card controller
  wire        sd_ack_o;				// SD Card controller
  wb_abrgr sd_brg (					
    .sys_rst   (rst),
    .wbs_clk_i (clk),				// Wishbone slave interface
    .wbs_dat_i (sd_dat_i_s),
    .wbs_dat_o (sd_dat_o_s),
    .wbs_sel_i (sd_sel_i_s),
    .wbs_stb_i (sd_stb_i_s),
    .wbs_cyc_i (sd_cyc_i_s),
    .wbs_we_i  (sd_we_i_s),
    .wbs_ack_o (sd_ack_o_s),
    .wbm_clk_i (sdram_clk),			// Wishbone master interface
    .wbm_dat_o (sd_dat_i),
    .wbm_dat_i (sd_dat_o),
    .wbm_sel_o (sd_sel_i),
    .wbm_stb_o (sd_stb_i),
    .wbm_cyc_o (sd_cyc_i),
    .wbm_we_o  (sd_we_i),
    .wbm_ack_i (sd_ack_o)
  );

  // --------------------------------------------------------------------
  // SD CARD SPI Interface
  // --------------------------------------------------------------------
  wire [19:1] sd_adr_i_s;			// SD Card Controller Async bridge
  wire [15:0] sd_dat_o_s;			// SD Card Controller Async bridge
  wire [15:0] sd_dat_i_s;			// SD Card Controller Async bridge
  wire        sd_tga_i_s;			// SD Card Controller Async bridge
  wire [ 1:0] sd_sel_i_s;			// SD Card Controller Async bridge
  wire        sd_we_i_s;			// SD Card Controller Async bridge
  wire        sd_cyc_i_s;			// SD Card Controller Async bridge
  wire        sd_stb_i_s;			// SD Card Controller Async bridge
  wire        sd_ack_o_s;			// SD Card Controller Async bridge
  sdspi sdspi (						
    .sclk     (sd_sclk_),			// Serial pad signal
    .miso     (sd_miso_),
    .mosi     (sd_mosi_),
    .ss       (sd_ss_),
    .wb_clk_i (sdram_clk),			// Wishbone slave interface
    .wb_rst_i (rst),
    .wb_dat_i (sd_dat_i),
    .wb_dat_o (sd_dat_o),
    .wb_we_i  (sd_we_i),
    .wb_sel_i (sd_sel_i),
    .wb_stb_i (sd_stb_i),
    .wb_cyc_i (sd_cyc_i),
    .wb_ack_o (sd_ack_o)
  );

  // --------------------------------------------------------------------
  // GPIO Module
  // --------------------------------------------------------------------
  wire [15:0] gpio_dat_o;			// GPIO controller
  wire [15:0] gpio_dat_i;			// GPIO controller
  wire        gpio_tga_i;			// GPIO controller
  wire [19:1] gpio_adr_i;			// GPIO controller
  wire [ 1:0] gpio_sel_i;			// GPIO controller
  wire        gpio_we_i;			// GPIO controller
  wire        gpio_cyc_i;			// GPIO controller
  wire        gpio_stb_i;			// GPIO controller
  wire        gpio_ack_o;			// GPIO controller  
  wire  [7:0] GPIO_Output; 
  assign      GPIO_Out_ = GPIO_Output[1:0];
  gpio gpio1 (
    .wb_clk_i (clk),			// Wishbone slave interface
    .wb_rst_i (rst),
    .wb_adr_i (gpio_adr_i),
    .wb_dat_o (gpio_dat_o),
    .wb_dat_i (gpio_dat_i),
    .wb_sel_i (gpio_sel_i),
    .wb_we_i  (gpio_we_i),
    .wb_stb_i (gpio_stb_i),
    .wb_cyc_i (gpio_cyc_i),
    .wb_ack_o (gpio_ack_o),
    
    .leds_ 	  (GPIO_Output),					// GPIO outputs
    .sw_   	  ({GamePort_In_,GPIO_In_,MCU_In_})	// GPIO inputs
  );

  // --------------------------------------------------------------------
  // 80186 compatible CPU Instantiation
  // --------------------------------------------------------------------
  wire [15:0] ip;				// CPU Instrcution Pointer
  wire [15:0] cs;				// CPU Control State
  wire [ 2:0] state;			// CPU State
  cpu proc (
    .ip         (ip),
    .cs         (cs),
    .state      (state),
    .dbg_block  (1'b0),
    .wb_clk_i 	(clk),			// Wishbone master interface
    .wb_rst_i 	(rst),
    .wb_dat_i 	(dat_i),
    .wb_dat_o 	(dat_o),
    .wb_adr_o 	(adr),
    .wb_we_o  	(we),
    .wb_tga_o 	(tga),
    .wb_sel_o 	(sel),
    .wb_stb_o 	(stb),
    .wb_cyc_o 	(cyc),
    .wb_ack_i 	(ack),
    .wb_tgc_i 	(intr),
    .wb_tgc_o 	(inta)
  );

  // --------------------------------------------------------------------
  // CPU Interrupt assignment
  // --------------------------------------------------------------------
  assign dat_i = inta ? { 13'b0000_0000_0000_1, iid } : sw_dat_o;


  // --------------------------------------------------------------------
  // --------------------------------------------------------------------
  // Wishbone switch:
  //
  // Memory and IO assignments:
  //
  // Type    Range                Description
  // ------  -------------------  --------------------------------------
  // Memory  0x0_0000 - 0xF_FFFF  Base RAM, SDRAM range 1MB
  // Memory  0xF_FF00 - 0xF_FFFF  Bios BOOT ROM Memory 
  // Memory  0xB_8000 - 0xB_FFFF  VGA Memory (text only)
  //
  // IO        0xf300 -   0xf3ff  SDRAM Control Registers
  // IO         0x3c0 -    0x3df  VGA IO Registers
  // IO         0x3f8 -    0x3ff  RS232 IO Registers
  // IO          0x60 -     0x64  Keyboard / Mouse IO Registers
  // IO         0x100 -    0x101  SD Card IO Registers
  // IO        0xf100 -   0xf103  GPIO Registers
  // IO        0xf200 -   0xf20f  CSR Bridge Registers, SDRAM Config
  // IO        0x0238 -   0x023f  SPI Flash Registers
  // IO        0x0210 -   0x021F  Sound Blaster Registers
  // IO        0x0360 -   0x0368  10BaseT IO Registers
  //
  // --------------------------------------------------------------------
  // --------------------------------------------------------------------
  wb_switch #(
    .s0_addr_1 (20'b0_1111_1111_1111_0000_000), // Bios BOOT mem 0xF_FF00 - 0xF_FFFF
    .s0_mask_1 (20'b1_1111_1111_1111_0000_000), // Bios BOOT ROM Memory 

    .s1_addr_1 (20'b0_1011_1000_0000_0000_000), // mem 0xB_8000 - 0xB_FFFF
    .s1_mask_1 (20'b1_1111_1000_0000_0000_000), // VGA Memory. text only

    .s1_addr_2 (20'b1_0000_0000_0011_1100_000), // io 0x3c0 - 0x3df
    .s1_mask_2 (20'b1_0000_1111_1111_1110_000), // VGA IO
    
    .s2_addr_1 (20'b1_0000_0000_0011_1111_100), // io 0x3f8 - 0x3ff
    .s2_mask_1 (20'b1_0000_1111_1111_1111_100), // RS232 IO
    
    .s3_addr_1 (20'b1_0000_0000_0000_0110_000), // io 0x60, 0x64
    .s3_mask_1 (20'b1_0000_1111_1111_1111_101), // Keyboard / Mouse IO
    
    .s4_addr_1 (20'b1_0000_0000_0001_0000_000), // io 0x100 - 0x101
    .s4_mask_1 (20'b1_0000_1111_1111_1111_111), // SD Card IO
    
    .s5_addr_1 (20'b1_0000_1111_0001_0000_000), // io 0xf100 - 0xf103
    .s5_mask_1 (20'b1_0000_1111_1111_1111_110), // GPIO
    
    .s6_addr_1 (20'b1_0000_1111_0010_0000_000), // io 0xf200 - 0xf20f
    .s6_mask_1 (20'b1_0000_1111_1111_1111_000), // CSR Bridge

    .s7_addr_1 (20'b1_0000_0000_0010_0011_100), // io 0x0238 - 0x023f
    .s7_mask_1 (20'b1_0000_1111_1111_1111_100), // SPI Flash

    .s8_addr_1 (20'b1_0000_0000_0011_0110_000), // io 0x0360 - 0x0367
    .s8_mask_1 (20'b1_0000_1111_1111_1111_100), // 10BaseT IO

    .s9_addr_1 (20'b1_0000_0000_0010_0001_000), // io 0x0210 - 0x021F
    .s9_mask_1 (20'b1_0000_1111_1111_1111_000), // Sound Blaster

    .sA_addr_1 (20'b1_0000_1111_0011_0000_000), // io 0xf300 - 0xf3ff
    .sA_mask_1 (20'b1_0000_1111_1111_0000_000), // SDRAM Control
    
    .sA_addr_2 (20'b0_0000_0000_0000_0000_000), // mem 0x0_0000 - 0xF_FFFF
    .sA_mask_2 (20'b1_0000_0000_0000_0000_000)  // Base RAM
    ) 
    wbs (
    .m_dat_i (dat_o),				// Master interface
    .m_dat_o (sw_dat_o),
    .m_adr_i ({tga,adr}),
    .m_sel_i (sel),
    .m_we_i  (we),
    .m_cyc_i (cyc),
    .m_stb_i (stb),
    .m_ack_o (ack),

    .s0_dat_i (rom_dat_o),			 // Slave 0 interface - BIOS ROM
    .s0_dat_o (rom_dat_i),
    .s0_adr_o ({rom_tga_i,rom_adr_i}),
    .s0_sel_o (rom_sel_i),
    .s0_we_o  (rom_we_i),
    .s0_cyc_o (rom_cyc_i),
    .s0_stb_o (rom_stb_i),
    .s0_ack_i (rom_ack_o),

    .s1_dat_i (vga_dat_o_s),		// Slave 1 interface - vga
    .s1_dat_o (vga_dat_i_s),
    .s1_adr_o ({vga_tga_i_s,vga_adr_i_s}),
    .s1_sel_o (vga_sel_i_s),
    .s1_we_o  (vga_we_i_s),
    .s1_cyc_o (vga_cyc_i_s),
    .s1_stb_o (vga_stb_i_s),
    .s1_ack_i (vga_ack_o_s),

    .s2_dat_i (uart_dat_o),			// Slave 2 interface - uart
    .s2_dat_o (uart_dat_i),
    .s2_adr_o ({uart_tga_i,uart_adr_i}),
    .s2_sel_o (uart_sel_i),
    .s2_we_o  (uart_we_i),
    .s2_cyc_o (uart_cyc_i),
    .s2_stb_o (uart_stb_i),
    .s2_ack_i (uart_ack_o),

    .s3_dat_i (keyb_dat_o),			// Slave 3 interface - keyb
    .s3_dat_o (keyb_dat_i),
    .s3_adr_o ({keyb_tga_i,keyb_adr_i}),
    .s3_sel_o (keyb_sel_i),
    .s3_we_o  (keyb_we_i),
    .s3_cyc_o (keyb_cyc_i),
    .s3_stb_o (keyb_stb_i),
    .s3_ack_i (keyb_ack_o),

    .s4_dat_i (sd_dat_o_s),			// Slave 4 interface - sd
    .s4_dat_o (sd_dat_i_s),
    .s4_adr_o ({sd_tga_i_s,sd_adr_i_s}),
    .s4_sel_o (sd_sel_i_s),
    .s4_we_o  (sd_we_i_s),
    .s4_cyc_o (sd_cyc_i_s),
    .s4_stb_o (sd_stb_i_s),
    .s4_ack_i (sd_ack_o_s),

    .s5_dat_i (gpio_dat_o),			// Slave 5 interface - gpio
    .s5_dat_o (gpio_dat_i),
    .s5_adr_o ({gpio_tga_i,gpio_adr_i}),
    .s5_sel_o (gpio_sel_i),
    .s5_we_o  (gpio_we_i),
    .s5_cyc_o (gpio_cyc_i),
    .s5_stb_o (gpio_stb_i),
    .s5_ack_i (gpio_ack_o),

    .s6_dat_i (csrbrg_dat_r_s),			// Slave 6 interface - csr bridge
    .s6_dat_o (csrbrg_dat_w_s),
    .s6_adr_o ({csrbrg_tga_s,csrbrg_adr_s}),
    .s6_sel_o (csrbrg_sel_s),
    .s6_we_o  (csrbrg_we_s),
    .s6_cyc_o (csrbrg_cyc_s),
    .s6_stb_o (csrbrg_stb_s),
    .s6_ack_i (csrbrg_ack_s),

    .s7_dat_i (wb_spi_dat_o),		    // Slave 7 interface - SPI Flash
    .s7_dat_o (wb_spi_dat_i),
    .s7_adr_o ({wb_spi_tga_i,wb_spi_adr_i}),
    .s7_sel_o (wb_spi_sel_i),
    .s7_we_o  (wb_spi_we_i),
    .s7_cyc_o (wb_spi_cyc_i),
    .s7_stb_o (wb_spi_stb_i),
    .s7_ack_i (wb_spi_ack_o),

    .s8_dat_i (wb_net_dat_o),			 // Slave 7 interface - 10BaseT Ethernet Interface
    .s8_dat_o (wb_net_dat_i),
    .s8_adr_o ({wb_net_tga_i,wb_net_adr_i}),
    .s8_sel_o (wb_net_sel_i),
    .s8_we_o  (wb_net_we_i),
    .s8_cyc_o (wb_net_cyc_i),
    .s8_stb_o (wb_net_stb_i),
    .s8_ack_i (wb_net_ack_o),

    .s9_dat_i (wb_sb_dat_o),			 // Slave 8 interface - Sound Blaster
    .s9_dat_o (wb_sb_dat_i),
    .s9_adr_o ({wb_sb_tga_i,wb_sb_adr_i}),
    .s9_sel_o (wb_sb_sel_i),
    .s9_we_o  (wb_sb_we_i),
    .s9_cyc_o (wb_sb_cyc_i),
    .s9_stb_o (wb_sb_stb_i),
    .s9_ack_i (wb_sb_ack_o),

    .sA_dat_i (fmlbrg_dat_r_s),		// Slave A interface - sdram
    .sA_dat_o (fmlbrg_dat_w_s),
    .sA_adr_o ({fmlbrg_tga_s,fmlbrg_adr_s}),
    .sA_sel_o (fmlbrg_sel_s),
    .sA_we_o  (fmlbrg_we_s),
    .sA_cyc_o (fmlbrg_cyc_s),
    .sA_stb_o (fmlbrg_stb_s),
    .sA_ack_i (fmlbrg_ack_s),

    .sB_dat_i (16'hffff),			// Slave B interface - default
    .sB_dat_o (),
    .sB_adr_o (),
    .sB_sel_o (),
    .sB_we_o  (),
    .sB_cyc_o (def_cyc_i),
    .sB_stb_o (def_stb_i),
    .sB_ack_i (def_cyc_i & def_stb_i)
  );

// --------------------------------------------------------------------
endmodule
// --------------------------------------------------------------------
