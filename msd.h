#ifndef MSD_H
#define MSD_H

#include <libusb-1.0/libusb.h>

#define be_to_int32(buf) (((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|(buf)[3])

int msd_init(libusb_device_handle **handle);
int msd_inquiry(libusb_device_handle *handle, uint8_t *buffer);
int msd_read_capacity(libusb_device_handle *handle, uint8_t *buffer);
int msd_write(libusb_device_handle *handle, uint8_t *buffer, uint8_t lba, unsigned int dataLength);
int msd_read(libusb_device_handle *handle, uint8_t *buffer, uint8_t lba, unsigned int dataLength);
void msd_close_dev(libusb_device_handle *handle);

#endif
