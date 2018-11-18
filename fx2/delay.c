#include "fx2regs.h"

/*-----------------------------------------------------------------------------
 * Delay routines
 *-----------------------------------------------------------------------------
 */

/*
 * Delay approximately 1 microsecond (including overhead in udelay).
 */
static void udelay1 (void) __naked
{
    __asm      ; lcall that got us here took 4 bus cycles
    ret        ; 4 bus cycles
    __endasm;
}

/*
 * delay for approximately usecs microseconds
 */
void udelay (unsigned char usecs)
{
    do {
        udelay1 ();
    } while (--usecs != 0);
}

/*
 * Delay approximately 1 millisecond.
 * We're running at 48 MHz, so we need 48,000 clock cycles.
 *
 * Note however, that each bus cycle takes 4 clock cycles (not obvious,
 * but explains the factor of 4 problem below).
 */
static void mdelay1 (void) __naked
{
    __asm
    mov    dptr,#(-1200 & 0xffff)
002$:
    inc    dptr        ; 3 bus cycles
    mov    a, dpl      ; 2 bus cycles
    orl    a, dph      ; 2 bus cycles
    jnz    002$        ; 3 bus cycles

    ret
    __endasm;
}

void mdelay (unsigned int msecs)
{
    do {
        mdelay1 ();
    } while (--msecs != 0);
}
