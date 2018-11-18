#ifndef DELAY_H
#define DELAY_H

/*
 * delay for approximately usecs microseconds
 * Note limit of 255 usecs.
 */
void udelay (unsigned char usecs);

/*
 * delay for approximately msecs milliseconds
 */
void mdelay (unsigned short msecs);

/*
 * Magic delay required between access to certain xdata registers (TRM page 15-106).
 * For our configuration, 48 MHz FX2 / 48 MHz IFCLK, we need three cycles.  Each
 * NOP is a single cycle....
 *
 * From TRM page 15-105:
 *
 * Under certain conditions, some read and write access to the FX2 registers must
 * be separated by a "synchronization delay".  The delay is necessary only under the
 * following conditions:
 *
 *   - between a write to any register in the 0xE600 - 0xE6FF range and a write to one
 *     of the registers listed below.
 *
 *   - between a write to one of the registers listed below and a read from any register
 *     in the 0xE600 - 0xE6FF range.
 *
 *   Registers which require a synchronization delay:
 *
 *	FIFORESET			FIFOPINPOLAR
 *	INPKTEND			EPxBCH:L
 *	EPxFIFOPFH:L			EPxAUTOINLENH:L
 *	EPxFIFOCFG			EPxGPIFFLGSEL
 *	PINFLAGSAB			PINFLAGSCD
 *	EPxFIFOIE			EPxFIFOIRQ
 *	GPIFIE				GPIFIRQ
 *	UDMACRCH:L			GPIFADRH:L
 *	GPIFTRIG			EPxGPIFTRIG
 *	OUTPKTEND			REVCTL
 *	GPIFTCB3			GPIFTCB2
 *	GPIFTCB1			GPIFTCB0
 */

// Ensure that the peep hole optimizer isn't screwing us
#define	SYNCDELAY	__asm nop; nop; nop; __endasm

#endif
