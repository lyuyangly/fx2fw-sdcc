#include "timer.h"
#include "fx2regs.h"
#include "isr.h"

/*
 * Arrange to have isr_tick_handler called at 100 Hz.
 *
 * The cpu clock is running at 48e6.  The input to the timer
 * is 48e6 / 12 = 4e6.
 *
 * We arrange to have the timer overflow every 40000 clocks == 100 Hz
 */

#define	RELOAD_VALUE	((unsigned short) -40000)

void hook_timer_tick (unsigned short isr_tick_handler)
{
    ET2 = 0; // disable timer 2 interrupts
    hook_sv (SV_TIMER_2, isr_tick_handler);

    RCAP2H = RELOAD_VALUE >> 8;	// setup the auto reload value
    RCAP2L = RELOAD_VALUE & 0xFF;

    T2CON = 0x04;		// interrupt on overflow; reload; run
    ET2 = 1;			// enable timer 2 interrupts
}
