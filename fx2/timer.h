#ifndef TIMER_H
#define TIMER_H

/*
 * Arrange to have isr_tick_handler called at 100 Hz
 */
void hook_timer_tick (unsigned short isr_tick_handler);

#define clear_timer_irq()  				\
	TF2 = 0 	/* clear overflow flag */


#endif
