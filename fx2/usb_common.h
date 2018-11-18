#ifndef USB_COMMON_H
#define USB_COMMON_H

#define	bRequestType	SETUPDAT[0]
#define	bRequest	    SETUPDAT[1]
#define	wValueL		    SETUPDAT[2]
#define	wValueH		    SETUPDAT[3]
#define	wIndexL		    SETUPDAT[4]
#define	wIndexH		    SETUPDAT[5]
#define	wLengthL	    SETUPDAT[6]
#define	wLengthH	    SETUPDAT[7]

#define MSB(x)	(((unsigned short) x) >> 8)
#define LSB(x)	(((unsigned short) x) & 0xff)

extern volatile __bit _usb_got_SUDAV;

// Provided by user application to report device status.
// returns non-zero if it handled the command.
unsigned char app_get_status (void);
// Provided by user application to handle VENDOR commands.
// returns non-zero if it handled the command.
unsigned char app_vendor_cmd (void);

void usb_install_handlers (void);
void usb_handle_setup_packet (void);

#define usb_setup_packet_avail()	_usb_got_SUDAV

#endif
