#include <stdio.h>
#include <dos.h>
union REGS in,out;

void main(void)
{
	int row, col, but;

	printf("DOS Mouse Driver Test\n");

	printf("Reset Mouse\n");
	in.x.ax = 0x0000;
	int86(0x33,&in,&out);
	printf("%04x\n", out.x.ax);
	if(out.x.ax != 0xFFFF) {
      	printf("mouse hardware/driver not installed\n");
		return;		
	}

	printf("Mouse Type\n");
	in.x.ax = 0x0024;
	int86(0x33,&in,&out);
	printf("%04x, %04x\r", out.x.bx, out.x.cx);


	printf("Show Mouse Cursor\n");
	in.x.ax = 0x0001;
	int86(0x33,&in,&out);

	printf("Step 3: Display test\n");

	do {
		in.x.ax = 0x0003;
	  	int86(0x33,&in,&out);
		row = out.x.dx;
		col = out.x.bx;
		but = out.x.bx;
		printf(r = %3d,  c = %3d,  b = %3d\r", row, col, but);
	
/*
	  in.x.ax = 0x0B;
	  int86(0x33,&in,&out);
	  printf("mickeys: %04x %04x\r",out.x.cx, out.x.dx);

*/

	} while(!kbhit());

	printf("Step 4: hide Mouse Cursor\n");
	in.x.ax = 0x0002;
	int86(0x33,&in,&out);

}