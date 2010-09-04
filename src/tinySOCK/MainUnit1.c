/* ----- TinySOCK tester -----------------------------------------------------
 *
 * This is the main for the test program. Just call TinySOCK, to change which
 * demo gets compiled, look at the TinySOCK.C file and change the conditional
 * compilation defines.
 *
 * ------------------------------------------------------------------------- */
#include <stdio.h>
#pragma hdrstop

void TinySOCK(void); /* TinySOCK Test Function Prototype                     */

/* ----- Main tcp tester --------------------------------------------------- */
#pragma argsused
int main(int argc, char* argv[])
{
    TinySOCK();
	return 0;
}
/* ---- end of Main.c ------------------------------------------------------ */

