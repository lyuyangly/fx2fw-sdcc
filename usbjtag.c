/*-----------------------------------------------------------------------------
 * Code that turns a Cypress FX2 USB Controller into an USB JTAG adapter
 *-----------------------------------------------------------------------------
 */

#include "isr.h"
#include "timer.h"
#include "delay.h"
#include "fx2regs.h"
#include "fx2utils.h"
#include "usb_common.h"
#include "usb_descriptors.h"
#include "usb_requests.h"
#include "eeprom.h"
#include "hardware.h"

//-----------------------------------------------------------------------------
// Define USE_MOD256_OUTBUFFER:
// Saves about 256 bytes in code size, improves speed a little.
// A further optimization could be not to use an extra output buffer at
// all, but to write directly into EP1INBUF. Not implemented yet. When
// downloading large amounts of data _to_ the target, there is no output
// and thus the output buffer isn't used at all and doesn't slow down things.

#define USE_MOD256_OUTBUFFER 1

//-----------------------------------------------------------------------------
typedef bit BOOL;
#define FALSE 0
#define TRUE  1
static BOOL Running;
static BOOL WriteOnly;

static BYTE ClockBytes;
static WORD Pending;

#ifdef USE_MOD256_OUTBUFFER
static BYTE FirstDataInOutBuffer;
static BYTE FirstFreeInOutBuffer;
#else
static WORD FirstDataInOutBuffer;
static WORD FirstFreeInOutBuffer;
#endif

#ifdef USE_MOD256_OUTBUFFER
/* Size of output buffer must be exactly 256 */
#define OUTBUFFER_LEN 0x100
/* Output buffer must begin at some address with lower 8 bits all zero */
xdata at 0xE000 BYTE OutBuffer[OUTBUFFER_LEN];
#else
#define OUTBUFFER_LEN 0x200
static xdata BYTE OutBuffer[OUTBUFFER_LEN];
#endif

//-----------------------------------------------------------------------------
void usb_jtag_init(void)
{
	WORD tmp;

	Running = FALSE;
	ClockBytes = 0;
	Pending = 0;
	WriteOnly = TRUE;
	FirstDataInOutBuffer = 0;
	FirstFreeInOutBuffer = 0;

	ProgIO_Init();

	// Make Timer2 reload at 100 Hz to trigger Keepalive packets
	tmp = 65536 - ( 48000000 / 12 / 100 );
	RCAP2H = tmp >> 8;
	RCAP2L = tmp & 0xFF;
	CKCON = 0; // Default Clock
	T2CON = 0x04; // Auto-reload mode using internal clock, no baud clock.

	// Enable Autopointer
	EXTACC = 1; // Enable
	APTR1FZ = 1; // Don't freeze
	APTR2FZ = 1; // Don't freeze

	// define endpoint configuration

	REVCTL = 3; SYNCDELAY; // Allow FW access to FIFO buffer
	FIFORESET = 0x80; SYNCDELAY; // From now on, NAK all, reset all FIFOS
	FIFORESET  = 0x02; SYNCDELAY; // Reset FIFO 2
	FIFORESET  = 0x04; SYNCDELAY; // Reset FIFO 4
	FIFORESET  = 0x06; SYNCDELAY; // Reset FIFO 6
	FIFORESET  = 0x08; SYNCDELAY; // Reset FIFO 8
	FIFORESET  = 0x00; SYNCDELAY; // Restore normal behaviour

	EP1OUTCFG  = 0xA0; SYNCDELAY; // Endpoint 1 Type Bulk
	EP1INCFG   = 0xA0; SYNCDELAY; // Endpoint 1 Type Bulk

	EP2FIFOCFG = 0x00; SYNCDELAY; // Endpoint 2
	EP2CFG     = 0xA2; SYNCDELAY; // Endpoint 2 Valid, Out, Type Bulk, Double buffered

	EP4FIFOCFG = 0x00; SYNCDELAY; // Endpoint 4 not used
	EP4CFG     = 0xA0; SYNCDELAY; // Endpoint 4 not used

	REVCTL = 0; SYNCDELAY; // Reset FW access to FIFO buffer, enable auto-arming when AUTOOUT is switched to 1

	EP6CFG     = 0xA2; SYNCDELAY; // Out endpoint, Bulk, Double buffering
	EP6FIFOCFG = 0x00; SYNCDELAY; // Firmware has to see a rising edge on auto bit to enable auto arming
	EP6FIFOCFG = bmAUTOOUT | bmWORDWIDE; SYNCDELAY; // Endpoint 6 used for user communicationn, auto commitment, 16 bits data bus

	EP8CFG     = 0xE0; SYNCDELAY; // In endpoint, Bulk
	EP8FIFOCFG = 0x00; SYNCDELAY; // Firmware has to see a rising edge on auto bit to enable auto arming
	EP8FIFOCFG = bmAUTOIN  | bmWORDWIDE; SYNCDELAY; // Endpoint 8 used for user communication, auto commitment, 16 bits data bus

	EP8AUTOINLENH = 0x00; SYNCDELAY; // Size in bytes of the IN data automatically commited (64 bytes here, but changed dynamically depending on the connection)
	EP8AUTOINLENL = 0x40; SYNCDELAY; // Can use signal PKTEND if you want to commit a shorter packet

	// Out endpoints do not come up armed
	// Since the defaults are double buffered we must write dummy byte counts twice
	EP2BCL = 0x80; SYNCDELAY; // Arm EP2OUT by writing byte count w/skip
	EP4BCL = 0x80; SYNCDELAY;
	EP2BCL = 0x80; SYNCDELAY; // Arm EP4OUT by writing byte count w/skip
	EP4BCL = 0x80; SYNCDELAY;

	// JTAG from FX2 enabled by default
	IOC |= (1 << 7);

	// Put the system in high speed by default (REM: USB-Blaster is in full speed)
	// This can be changed by vendor commands
	CT1 &= ~0x02;
	// Put the FIFO in sync mode
	IFCONFIG &= ~bmASYNC;
}

void OutputByte(BYTE d)
{
#ifdef USE_MOD256_OUTBUFFER
	OutBuffer[FirstFreeInOutBuffer] = d;
	FirstFreeInOutBuffer = ( FirstFreeInOutBuffer + 1 ) & 0xFF;
#else
	OutBuffer[FirstFreeInOutBuffer++] = d;
	if(FirstFreeInOutBuffer >= OUTBUFFER_LEN) FirstFreeInOutBuffer = 0;
#endif
	Pending++;
}

//-----------------------------------------------------------------------------
// usb_jtag_activity does most of the work. It now happens to behave just like
// the combination of FT245BM and Altera-programmed EPM7064 CPLD in Altera's
// USB-Blaster. The CPLD knows two major modes: Bit banging mode and Byte
// shift mode. It starts in Bit banging mode. While bytes are received
// from the host on EP2OUT, each byte B of them is processed as follows:
//
// Please note: nCE, nCS, LED pins and DATAOUT actually aren't supported here.
// Support for these would be required for AS/PS mode and isn't too complicated,
// but I haven't had the time yet.
//
// Bit banging mode:
//
//   1. Remember bit 6 (0x40) in B as the "Read bit".
//
//   2. If bit 7 (0x40) is set, switch to Byte shift mode for the coming
//      X bytes ( X := B & 0x3F ), and don't do anything else now.
//
//    3. Otherwise, set the JTAG signals as follows:
//        TCK/DCLK high if bit 0 was set (0x01), otherwise low
//        TMS/nCONFIG high if bit 1 was set (0x02), otherwise low
//        nCE high if bit 2 was set (0x04), otherwise low
//        nCS high if bit 3 was set (0x08), otherwise low
//        TDI/ASDI/DATA0 high if bit 4 was set (0x10), otherwise low
//        Output Enable/LED active if bit 5 was set (0x20), otherwise low
//
//    4. If "Read bit" (0x40) was set, record the state of TDO(CONF_DONE) and
//        DATAOUT(nSTATUS) pins and put it as a byte ((DATAOUT<<1)|TDO) in the
//        output FIFO _to_ the host (the code here reads TDO only and assumes
//        DATAOUT=1)
//
// Byte shift mode:
//
//   1. Load shift register with byte from host
//
//   2. Do 8 times (i.e. for each bit of the byte; implemented in shift.a51)
//      2a) if nCS=1, set carry bit from TDO, else set carry bit from DATAOUT
//      2b) Rotate shift register through carry bit
//      2c) TDI := Carry bit
//      2d) Raise TCK, then lower TCK.
//
//   3. If "Read bit" was set when switching into byte shift mode,
//      record the shift register content and put it into the FIFO
//      _to_ the host.
//
// Some more (minor) things to consider to emulate the FT245BM:
//
//   a) The FT245BM seems to transmit just packets of no more than 64 bytes
//      (which perfectly matches the USB spec). Each packet starts with
//      two non-data bytes (I use 0x31,0x60 here). A USB sniffer on Windows
//      might show a number of packets to you as if it was a large transfer
//      because of the way that Windows understands it: it _is_ a large
//      transfer until terminated with an USB packet smaller than 64 byte.
//
//   b) The Windows driver expects to get some data packets (with at least
//      the two leading bytes 0x31,0x60) immediately after "resetting" the
//      FT chip and then in regular intervals. Otherwise a blue screen may
//      appear... In the code below, I make sure that every 10ms there is
//      some packet.
//
//   c) Vendor specific commands to configure the FT245 are mostly ignored
//      in my code. Only those for reading the EEPROM are processed. See
//      DR_GetStatus and DR_VendorCmd below for my implementation.
//
//   All other TD_ and DR_ functions remain as provided with CY3681.
//
//-----------------------------------------------------------------------------

void usb_jtag_activity(void)
{
	if(!Running) return;

	if(!(EP1INCS & bmEPBUSY)) {
		if(Pending > 0) {
			BYTE o, n;

			AUTOPTRH2 = MSB( EP1INBUF );
			AUTOPTRL2 = LSB( EP1INBUF );

			XAUTODAT2 = 0x31;
			XAUTODAT2 = 0x60;

			if(Pending > 0x3E) { n = 0x3E; Pending -= n; }
			else { n = Pending; Pending = 0; };

			o = n;

#ifdef USE_MOD256_OUTBUFFER
			APTR1H = MSB( OutBuffer );
			APTR1L = FirstDataInOutBuffer;
			while(n--) {
				XAUTODAT2 = XAUTODAT1;
				APTR1H = MSB( OutBuffer ); // Stay within 256-Byte-Buffer
			}
			FirstDataInOutBuffer = APTR1L;
#else
			APTR1H = MSB( &(OutBuffer[FirstDataInOutBuffer]) );
			APTR1L = LSB( &(OutBuffer[FirstDataInOutBuffer]) );
			while(n--) {
				XAUTODAT2 = XAUTODAT1;
				if(++FirstDataInOutBuffer >= OUTBUFFER_LEN) {
					FirstDataInOutBuffer = 0;
					APTR1H = MSB( OutBuffer );
					APTR1L = LSB( OutBuffer );
				}
			}
#endif
			SYNCDELAY;
			EP1INBC = 2 + o;
			TF2 = 1; // Make sure there will be a short transfer soon
		} else if(TF2) {
			EP1INBUF[0] = 0x31;
			EP1INBUF[1] = 0x60;
			SYNCDELAY;
			EP1INBC = 2;
			TF2 = 0;
		}
	}

	if(!(EP2468STAT & bmEP2EMPTY) && (Pending < OUTBUFFER_LEN-0x3F)) {
		WORD i, n = EP2BCL|EP2BCH<<8;

		APTR1H = MSB( EP2FIFOBUF );
		APTR1L = LSB( EP2FIFOBUF );

		for(i = 0; i < n;) {
			if(ClockBytes > 0) {
				WORD m;

				m = n-i;
				if(ClockBytes < m) m = ClockBytes;
				ClockBytes -= m;
				i += m;

				/* Shift out 8 bits from d */
				if(WriteOnly) /* Shift out 8 bits from d */
					while(m--) ProgIO_ShiftOut(XAUTODAT1);
				else /* Shift in 8 bits at the other end  */
					while(m--) OutputByte(ProgIO_ShiftInOut(XAUTODAT1));
			} else {
				BYTE d = XAUTODAT1;
				WriteOnly = (d & bmBIT6) ? FALSE : TRUE;
				if(d & bmBIT7) {
					/* Prepare byte transfer, do nothing else yet */
					ClockBytes = d & 0x3F;
				} else {
					if(WriteOnly)
						ProgIO_Set_State(d);
					else
						OutputByte(ProgIO_Set_Get_State(d));
				}
				i++;
			}
		}

		SYNCDELAY;
		EP2BCL = 0x80; // Re-arm endpoint 2
	}
}

//-----------------------------------------------------------------------------
// Handler for Vendor Requests
//-----------------------------------------------------------------------------
unsigned char app_vendor_cmd(void)
{
	// OUT requests. Pretend we handle them all
	if ((bRequestType & bmRT_DIR_MASK) == bmRT_DIR_OUT){
		if(bRequest == RQ_GET_STATUS){
			Running = 1;
		};
		return 1;
	}

	// IN requests.
	switch (bRequest){
		case 0x90: { // Read EEPROM
				// We need a block for addr
				BYTE addr = (wIndexL<<1) & 0x7F;
				EP0BUF[0] = eeprom[addr];
				EP0BUF[1] = eeprom[addr+1];
				EP0BCL = (wLengthL<2) ? wLengthL : 2;
				break;
			}
		case 0x91: // change USB speed
			if (wIndexL == 0){ // high speed
				CT1 &= ~0x02;
				EP0BUF[0] = 0x2;
				fx2_renumerate(); // renumerate
			} else { // full speed
				CT1 |= 0x02;
				EP0BUF[0] = 0x1;
				fx2_renumerate(); // renumerate
			}
			EP0BCH = 0; // Arm endpoint
			EP0BCL = 1; // # bytes to transfer
			break;
		case 0x92: // change JTAG enable
			if (wIndexL == 0){ // FX2 is master of JTAG
				IOC |= (1 << 7);
			} else { // external connector is master of JTAG
				IOC &= ~(1 << 7);
			}
			EP0BCH = 0; // Arm endpoint
			EP0BCL = 0; // bytes to transfer
			break;
		case 0x93: // change synchronous/asynchronous mode
			if(wIndexL == 0){           // sync
				IFCONFIG &= ~bmASYNC;
				EP0BUF[0] = 0;
			} else {
				IFCONFIG |= bmASYNC;    // async
				EP0BUF[0] = 1;
			}
			EP0BCH = 0; // Arm endpoint
			EP0BCL = 1;
			break;
		case 0x94: { // get Firmware version
				int i=0;
				char* ver="4.2.0";
				while(ver[i]!='\0'){
					EP0BUF[i]=ver[i];
					i++;
				}
				EP0BCH = 0; // Arm endpoint
				EP0BCL = i;
				break;
			}
		default: // Dummy data
			EP0BUF[0] = 0x36;
			EP0BUF[1] = 0x83;
			EP0BCH = 0;
			EP0BCL = (wLengthL<2) ? wLengthL : 2;
			break;
	}
	return 1;
}

//-----------------------------------------------------------------------------
static void main_loop(void)
{
	while(1) {
		if(usb_setup_packet_avail())
			usb_handle_setup_packet();
		usb_jtag_activity();
	}
}

//-----------------------------------------------------------------------------
void main(void)
{
	EA = 0; // disable all interrupts
	usb_jtag_init();
	eeprom_init();
	setup_autovectors ();
	usb_install_handlers ();
	EA = 1; // enable interrupts
	fx2_renumerate(); // simulates disconnect / reconnect
	main_loop();
}
