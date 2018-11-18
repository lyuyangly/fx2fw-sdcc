#-----------------------------------------------------------------------------
# Makefile for usb_jtag FX2 Firmware
#-----------------------------------------------------------------------------

LIBDIR=fx2
LIB=libfx2.lib

ifeq (${HARDWARE},)
  HARDWARE=hw_basic
  #HARDWARE=hw_xpcu_i
  #HARDWARE=hw_xpcu_x
endif

all: usbjtag.hex

CC=sdcc
CFLAGS+=-mmcs51 --no-xinit-opt -I${LIBDIR} -D${HARDWARE}

CFLAGS+=--opt-code-size

AS=sdas8051
ASFLAGS+=-plosgff

LDFLAGS=--code-loc 0x0000 --code-size 0x1800
LDFLAGS+=--xram-loc 0x1800 --xram-size 0x0800
LDFLAGS+=-Wl '-b USBDESCSEG = 0xE100'
LDFLAGS+=-L ${LIBDIR}

%.rel : %.a51
	$(AS) $(ASFLAGS) $<

%.rel : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

dscr.rel: dscr.a51
eeprom.rel: eeprom.c eeprom.h
usbjtag.rel: usbjtag.c hardware.h eeprom.h
${HARDWARE}.rel: ${HARDWARE}.c hardware.h

${LIBDIR}/${LIB}:
	make -C ${LIBDIR}

usbjtag.hex: vectors.rel usbjtag.rel dscr.rel eeprom.rel ${HARDWARE}.rel startup.rel ${LIBDIR}/${LIB}
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $+
	@packihx $@ > .tmp.hex
	@rm $@
	@mv .tmp.hex $@
	@ls -al $@

%.iic : %.hex
	wine hex2bix -ir -v 0x09fb -p 0x6001 -f 0xC2 -m 0xF000 -c 0x1 -o $@ $<

%.bix: %.hex
	objcopy -I ihex -O binary $< $@

.PHONY: boot usb
boot: usbjtag.hex
	/sbin/fxload -t fx2lp -I usbjtag.hex -v -D `lsusb -d 04b4:8613 | cut -d: -f1 | awk '{ print "/dev/bus/usb/" $$2 "/" $$4 }'`

usb:
	@gcc usb.c -o usb_test -lusb-1.0
	@./usb_test

.PHONY: clean
clean:
	make -C ${LIBDIR} clean
	rm -f *.lst *.asm *.lib *.sym *.rel *.mem *.map *.rst *.lnk *.hex *.ihx *.iic *.lk usb_test

