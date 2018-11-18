#ifndef USB_DESC_H
#define USB_DESC_H

extern __xdata const char high_speed_device_descr[];
extern __xdata const char high_speed_devqual_descr[];
extern __xdata const char high_speed_config_descr[];

extern __xdata const char full_speed_device_descr[];
extern __xdata const char full_speed_devqual_descr[];
extern __xdata const char full_speed_config_descr[];

extern __xdata unsigned char nstring_descriptors;
extern __xdata char * __xdata string_descriptors[];

/*
 * We patch these locations with info read from the usrp config eeprom
 */
extern __xdata char usb_desc_hw_rev_binary_patch_location_0[];
extern __xdata char usb_desc_hw_rev_binary_patch_location_1[];
extern __xdata char usb_desc_hw_rev_ascii_patch_location_0[];
extern __xdata char usb_desc_serial_number_ascii[];

#endif
