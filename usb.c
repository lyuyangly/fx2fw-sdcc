#include <unistd.h>  
#include <stdio.h>  
#include <libusb-1.0/libusb.h>  
  
#define VID 0x09fb
#define PID 0x6001 

int main(int argc, char *argv[]) {  
	unsigned int len = 64;
	unsigned char buf[64];

    libusb_device_handle *dev;
    libusb_context *ctx = NULL;

	printf("libusb test: get firmware version from fx2 based usb-blaster.\n");

    libusb_init(&ctx);

    dev = libusb_open_device_with_vid_pid(ctx, VID, PID);
	if(dev == NULL) return 0;

    libusb_claim_interface(dev, 0);

	libusb_control_transfer(dev, LIBUSB_ENDPOINT_IN | (0x2<<5), 0x94, 0x0, 0x0, buf, len, 1000);

	printf("libusb test: usb-blaster firmware version: %s\n", buf);

    libusb_close(dev);
    libusb_exit(ctx);
  
    return 0;  
}

