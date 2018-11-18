/*-----------------------------------------------------------------------------
 * I2C read/write functions for FX2
 *-----------------------------------------------------------------------------
 */

#include "i2c.h"
#include "fx2regs.h"
#include <string.h>

// returns non-zero if successful, else 0
unsigned char i2c_read (unsigned char addr, xdata unsigned char *buf, unsigned char len)
{
	volatile unsigned char	junk;

	if (len == 0)			// reading zero bytes always works
		return 1;
	
	while (I2CS & bmSTOP);
	
	I2CS = bmSTART;
	I2DAT = (addr << 1) | 1;	// write address and direction (1's the read bit)
	
	while ((I2CS & bmDONE) == 0);
	
	if ((I2CS & bmBERR) || (I2CS & bmACK) == 0)	// no device answered...
		goto fail;
	
	if (len == 1)
		I2CS |= bmLASTRD;
	
	junk = I2DAT;			// trigger the first read cycle
	
	while (--len != 0){
		while ((I2CS & bmDONE) == 0);
	
		if (I2CS & bmBERR)
			goto fail;
	
		if (len == 1)
			I2CS |= bmLASTRD;
	
		*buf++ = I2DAT;		// get data, trigger another read
	}
	
	// wait for final byte
	while ((I2CS & bmDONE) == 0);
	
	if (I2CS & bmBERR)
		goto fail;
	
	I2CS |= bmSTOP;
	*buf = I2DAT;

	return 1;

fail:
	I2CS |= bmSTOP;
	return 0;
}

// returns non-zero if successful, else 0
unsigned char i2c_write (unsigned char addr, xdata const unsigned char *buf, unsigned char len)
{
	while (I2CS & bmSTOP);

	I2CS = bmSTART;
	I2DAT = (addr << 1) | 0;	// write address and direction (0's the write bit)

	while ((I2CS & bmDONE) == 0);

	if ((I2CS & bmBERR) || (I2CS & bmACK) == 0)	// no device answered...
		goto fail;

	while (len > 0){
		I2DAT = *buf++;
		len--;

		while ((I2CS & bmDONE) == 0);

		if ((I2CS & bmBERR) || (I2CS & bmACK) == 0)	// no device answered...
			goto fail;
	}

	I2CS |= bmSTOP;
	return 1;

fail:
	I2CS |= bmSTOP;
	return 0;
}
