#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define REG(x) (0x0210+(x))
#define PI        (2.0 * asin(1.0))
#define SCALE      256
#define SampleSize 256
void main(void)
{
	int i,j, points, intv;
	unsigned char sinLUT[SampleSize];
	char str[16];

	points = SampleSize;
	for(i=0; i<points ; i++) {
		sinLUT[i]= (unsigned char)(128 + (sin(2. * PI * (float)i/(float)points) * (SCALE/2 - 1)));
	}

	printf("Enter interval: ");
	intv = atoi(gets(str));
	if(intv ==0) intv =0x0F00;

	outportb(REG(2), (intv >>   8));
	outportb(REG(3), (intv & 0xFF));
	printf("Interval = 0x%02x%02x\n",inportb(REG(2)),inportb(REG(3)));

	for(j=0; j < 256; j++) {
		for(i=0; i < points; i++) {
			outportb(REG(0), sinLUT[i]);
			outportb(REG(1), sinLUT[i]);
			do {
				if(inportb(REG(7))) break;
			} while(1);
		}
	 }

	printf("Hit any key to continue...\n");
	getch();
}
