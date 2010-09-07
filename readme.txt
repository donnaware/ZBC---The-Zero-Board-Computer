ZBC Readme and Description:
---------------------------------------------------------------------
ZBC is a really fun little PC-XT SoC (System on a Chip) that  is 
implemented on a Single Board circuit board. The board is called ZBC 
for Zero Board Computer instead of Single Board Computer just to be 
funny (in computers we always start at Zero not one right ???).  
All joking aside, ZBC is intended to be fun and educational and able
to run DOS and perhaps general enough to run some other tiny OS 
(perhaps tiny Linux) or an older version of Windows (maybe Windows
3 or whatever). The design and code is based heavily on the Terasic
(www.terasic.com) boards and the FPGA code is based heavily on the
ZET computer (http://zet.aluzina.org) which runs on the Terasic DE1. 

For more details, read the article "ZBC RevD.pdf" posted under the 
subdirectory "design".



Directory tree:
---------------------------------------------------------------------
design			main design files
pcb			pcb schematic and artwork
rtl			verilog code
rtl\ver1		version 1 of ZBC
rtl\ver1\rtl		version 1 verilog source
rtl\ver1\syn		version 1 quartus 10 project file
src			source code files
src\controller		Borland BCB6 source code for the PC configurator
src\mcu			CCSC PIC code for the USB interface
src\mouse		TurboC test code for the mouse
src\sound		TurboC test code for the sound module
src\tinySOCK		Borland 4.52 source for a 10BASET driver
zbcbios			OpenWatcom source for the bios



Verilog module list:


File		Module Description
---------------------------------------------------------------------
zbc.v		ZBC Top Level Module
cpu.v		8086 CPU
wb_abrg.v   	Wishbone Async Bridge
wb_switch.v	Wishbone Async Bridge with registered outputs
pll.v		Phase Locked Loop
BIOSROM.v	BIOS ROM 
WB_SPI_Flash.v	Wishbone SPI FLASH RAM IO
fmlbrg.v	Fast Memory Link Bridge
hpdmc.v 	High Performance Dynamic Memory Controller
csrbrg.v	SDRAM Control Bridge
sdspi.v		SD CARD SPI Interface
vdu.v		Text mode only VGA
simple_pic.v	Simple Priority Interrupt Controller
timer.v 	Simple Timer
gpio.v		General Purpose IO
Sound.v		Sound Module
WB_PS2.v	PS2 Keyboard and Mouse Controller
WB_Serial.v	Wishbone RS232 Serial Controller
WB_Ethernet.v	Wishbone 10BaseT Ethernet Controller
seq_rom.dat	8086 Sequencer ROM
micro_rom.dat 	8086 Micro code ROM
char_rom.dat	DOS Character Set ROM
biosrom.hex	BIOS ROM
xt_codes.dat    PC Keyboard Scan Code ROM Look up table
