#pragma once



#include "usbd.h"


extern const struct usb_device_descriptor dev;
extern const struct usb_dfu_descriptor dfu_function;
extern const struct usb_interface ifaces[];
extern const struct usb_config_descriptor config;
extern const char * const usb_strings[];
