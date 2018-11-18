#-----------------------------------------------------------------------------
# Makefile for FX2 Library
#-----------------------------------------------------------------------------

CC=sdcc
CFLAGS+=-mmcs51 --no-xinit-opt -I.
CPPFLAGS+=

%.rel : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

libfx2.lib: delay.rel fx2utils.rel i2c.rel isr.rel timer.rel usb_common.rel
	@rm -f $@
	@touch $@
	@for obj in $^ ; do basename $$obj .rel >> $@ ; done

clean:
	@rm -f *.lst *.asm *.lib *.sym *.rel *.lib

