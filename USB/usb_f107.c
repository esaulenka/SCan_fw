/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2011 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include "usbd.h"
#include "otg_fs.h"
#include "usb_private.h"
#include "usb_dwc_common.h"

// Receive FIFO size in 32-bit words.
#define RX_FIFO_SIZE 128

static usbd_device *stm32f107_usbd_init(void);

static struct _usbd_device usbd_dev;

const struct _usbd_driver stm32f107_usb_driver = {
	.init = stm32f107_usbd_init,
	.set_address = dwc_set_address,
	.ep_setup = dwc_ep_setup,
	.ep_reset = dwc_endpoints_reset,
	.ep_stall_set = dwc_ep_stall_set,
	.ep_stall_get = dwc_ep_stall_get,
	.ep_nak_set = dwc_ep_nak_set,
	.ep_write_packet = dwc_ep_write_packet,
	.ep_read_packet = dwc_ep_read_packet,
	.poll = dwc_poll,
	.disconnect = dwc_disconnect,
	.rx_fifo_size = RX_FIFO_SIZE,
};

//* Initialize the USB device controller hardware of the STM32.
static usbd_device *stm32f107_usbd_init(void)
{
	RCC->AHBENR |= RCC_AHBENR_OTGFSEN;	// enable clock
	USB_OTG_FS->GUSBCFG |= USB_OTG_GUSBCFG_PHYSEL;


	// Wait for AHB idle.
	while (!(USB_OTG_FS->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL)) {}
	// Do core soft reset.
	USB_OTG_FS->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;
	while (USB_OTG_FS->GRSTCTL & USB_OTG_GRSTCTL_CSRST) {}

//	if (USB_OTG_FS->CID >= OTG_CID_HAS_VBDEN) {
//		// Enable VBUS detection in device mode and power up the PHY.
//		USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_VBDEN | USB_OTG_GCCFG_PWRDWN;
//	} else {
		// Enable VBUS sensing in device mode and power up the PHY.
		USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_VBUSBSEN | USB_OTG_GCCFG_PWRDWN;
//	}
	// Explicitly enable DP pullup (not all cores do this by default)
	USB_DEVICE->DCTL &= ~USB_OTG_DCTL_SDIS;

	// Force peripheral only mode.
	const uint32_t turnaroundTime = 9;	// see datasheet
	//USB_OTG_FS->GUSBCFG |= USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_TRDT_MASK;
	USB_OTG_FS->GUSBCFG |= USB_OTG_GUSBCFG_FDMOD | (turnaroundTime << 10) | USB_OTG_GUSBCFG_PHYSEL;


	// clear mode mismatch interrupt
	USB_OTG_FS->GINTSTS = USB_OTG_GINTSTS_MMIS;

	// Full speed device.
	USB_DEVICE->DCFG |= USB_OTG_DCFG_DSPD;


	// Restart the PHY clock.
	USB_OTG_PCGCCTL->PCGCR = 0;

	USB_OTG_FS->GRXFSIZ = stm32f107_usb_driver.rx_fifo_size;
	usbd_dev.fifo_mem_top = stm32f107_usb_driver.rx_fifo_size;

	// Unmask interrupts for TX and RX.
	USB_OTG_FS->GAHBCFG |= USB_OTG_GAHBCFG_GINT;
	USB_OTG_FS->GINTMSK =	USB_OTG_GINTMSK_ENUMDNEM |		// enumeration done
							USB_OTG_GINTMSK_RXFLVLM |		// receive not empty
							USB_OTG_GINTMSK_IEPINT |		// IN endpoints interrupt
							USB_OTG_GINTMSK_USBSUSPM |		// suspend
							USB_OTG_GINTMSK_WUIM;			// resume/wakeup
	USB_DEVICE->DAINTMSK = 0xF; 							// unmask IN endpoints
	USB_DEVICE->DIEPMSK = USB_OTG_DIEPMSK_XFRCM;			// transfer completed

	return &usbd_dev;
}
