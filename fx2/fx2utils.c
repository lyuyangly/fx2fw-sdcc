#include "fx2utils.h"
#include "fx2regs.h"
#include "delay.h"

void fx2_stall_ep0 (void)
{
    EP0CS |= bmEPSTALL;
}

void fx2_reset_data_toggle (unsigned char ep)
{
    TOGCTL = ((ep & 0x80) >> 3 | (ep & 0x0f));
    TOGCTL |= bmRESETTOGGLE;
}

void fx2_renumerate (void)
{
    USBCS |= bmDISCON | bmRENUM;

    mdelay (250);

    USBIRQ = 0xff;		// clear any pending USB irqs...
    EPIRQ =  0xff;		// they're from before the renumeration

    EXIF &= ~bmEXIF_USBINT;
    USBCS &= ~bmDISCON;		// reconnect USB
}
