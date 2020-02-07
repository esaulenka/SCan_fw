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
#include "usb_private.h"
#include "usb_dwc_common.h"
#include "otg_fs.h"



void dwc_set_address(usbd_device *usbd_dev, uint8_t addr)
{
	(void)usbd_dev;
	USB_DEVICE->DCFG = (USB_DEVICE->DCFG & ~USB_OTG_DCFG_DAD) | (addr << 4);
}

void dwc_ep_setup(usbd_device *usbd_dev, uint8_t addr, uint8_t type, uint16_t max_size,
			void (*callback) (usbd_device *usbd_dev, uint8_t ep))
{
	// Configure endpoint address and type. Allocate FIFO memory for
	// endpoint. Install callback function.
	const uint8_t dir = addr & 0x80;
	addr &= 0x7f;

	if (addr == 0)  // For the default control endpoint
	{
		// Configure IN part.
		if (max_size >= 64)
			USB_INEP(0)->DIEPCTL = OTG_DIEPCTL0_MPSIZ_64;
		else if (max_size >= 32)
			USB_INEP(0)->DIEPCTL = OTG_DIEPCTL0_MPSIZ_32;
		else if (max_size >= 16)
			USB_INEP(0)->DIEPCTL = OTG_DIEPCTL0_MPSIZ_16;
		else
			USB_INEP(0)->DIEPCTL = OTG_DIEPCTL0_MPSIZ_8;

		USB_INEP(0)->DIEPTSIZ = (max_size & OTG_DIEPSIZ0_XFRSIZ_MASK);
		USB_INEP(0)->DIEPCTL |= USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_SNAK;

		// Configure OUT part.
		usbd_dev->doeptsiz[0] = OTG_DIEPSIZ0_STUPCNT_1 |
			OTG_DIEPSIZ0_PKTCNT |
			(max_size & OTG_DIEPSIZ0_XFRSIZ_MASK);
		USB_OUTEP(0)->DOEPTSIZ = usbd_dev->doeptsiz[0];
		USB_OUTEP(0)->DOEPCTL |= USB_OTG_DOEPCTL_EPENA | USB_OTG_DIEPCTL_SNAK;

		USB_OTG_FS->DIEPTXF0_HNPTXFSIZ = ((max_size / 4) << 16) | usbd_dev->driver->rx_fifo_size;
		usbd_dev->fifo_mem_top += max_size / 4;
		usbd_dev->fifo_mem_top_ep0 = usbd_dev->fifo_mem_top;

		return;
	}

	if (dir)
	{
		// attention to DIEPTXF addressing!
		USB_OTG_FS->DIEPTXF[addr - 1] = ((max_size / 4) << 16) | usbd_dev->fifo_mem_top;
		usbd_dev->fifo_mem_top += max_size / 4;

		USB_INEP(addr)->DIEPTSIZ = (max_size & OTG_DIEPSIZ0_XFRSIZ_MASK);
		USB_INEP(addr)->DIEPCTL |=
			USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_SNAK | (type << 18) |
			USB_OTG_DIEPCTL_USBAEP | USB_OTG_DIEPCTL_SD0PID_SEVNFRM |
			(addr << 22) | max_size;

		if (callback)
			usbd_dev->user_callback_ctr[addr][USB_TRANSACTION_IN] = callback;
	}

	if (!dir)
	{
		usbd_dev->doeptsiz[addr] = OTG_DIEPSIZ0_PKTCNT | (max_size & OTG_DIEPSIZ0_XFRSIZ_MASK);
		USB_OUTEP(addr)->DOEPTSIZ = usbd_dev->doeptsiz[addr];
		USB_OUTEP(addr)->DOEPCTL |=
			USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK | (type << 18) |
			USB_OTG_DOEPCTL_USBAEP | USB_OTG_DOEPCTL_SD0PID_SEVNFRM | max_size;

		if (callback)
			usbd_dev->user_callback_ctr[addr][USB_TRANSACTION_OUT] = callback;
	}
}

void dwc_endpoints_reset(usbd_device *usbd_dev)
{
	// The core resets the endpoints automatically on reset.
	usbd_dev->fifo_mem_top = usbd_dev->fifo_mem_top_ep0;

	// Disable any currently active endpoints
	for (int i = 1; i < 4; i++)
	{
		if (USB_OUTEP(i)->DOEPCTL & USB_OTG_DOEPCTL_EPENA)
			USB_OUTEP(i)->DOEPCTL |= USB_OTG_DOEPCTL_EPDIS;

		if (USB_INEP(i)->DIEPCTL & USB_OTG_DIEPCTL_EPENA)
			USB_INEP(i)->DIEPCTL |= USB_OTG_DIEPCTL_EPDIS;
	}

	// Flush all tx/rx fifos
	USB_OTG_FS->GRSTCTL = USB_OTG_GRSTCTL_TXFFLSH | USB_OTG_GRSTCTL_TXFNUM_4
			      | USB_OTG_GRSTCTL_RXFFLSH;
}

void dwc_ep_stall_set(usbd_device *usbd_dev, uint8_t addr, uint8_t stall)
{
	(void)usbd_dev;
	if (addr == 0)
	{
		if (stall)
			USB_INEP(addr)->DIEPCTL |= USB_OTG_DIEPCTL_STALL;
		else
			USB_INEP(addr)->DIEPCTL &= ~USB_OTG_DIEPCTL_STALL;
	}

	if (addr & 0x80)
	{
		addr &= 0x7F;
		if (stall) {
			USB_INEP(addr)->DIEPCTL |= USB_OTG_DIEPCTL_STALL;
		} else {
			USB_INEP(addr)->DIEPCTL &= ~USB_OTG_DIEPCTL_STALL;
			USB_INEP(addr)->DIEPCTL |= USB_OTG_DIEPCTL_SD0PID_SEVNFRM;
		}
	}
	else
	{
		if (stall) {
			USB_OUTEP(addr)->DOEPCTL |= USB_OTG_DOEPCTL_STALL;
		} else {
			USB_OUTEP(addr)->DOEPCTL &= ~USB_OTG_DOEPCTL_STALL;
			USB_OUTEP(addr)->DOEPCTL |= USB_OTG_DOEPCTL_SD0PID_SEVNFRM;
		}
	}
}

uint8_t dwc_ep_stall_get(usbd_device *usbd_dev, uint8_t addr)
{
	(void)usbd_dev;
	// Return non-zero if STALL set.
	if (addr & 0x80)
		return (USB_INEP(addr & 0x7f)->DIEPCTL & USB_OTG_DIEPCTL_STALL) ? 1 : 0;
	else
		return (USB_OUTEP(addr)->DOEPCTL & USB_OTG_DOEPCTL_STALL) ? 1 : 0;
}

void dwc_ep_nak_set(usbd_device *usbd_dev, uint8_t addr, uint8_t nak)
{
	// It does not make sense to force NAK on IN endpoints.
	if (addr & 0x80)
		return;

	usbd_dev->force_nak[addr] = nak;
	USB_OUTEP(addr)->DOEPCTL |= nak ?
				USB_OTG_DOEPCTL_SNAK : USB_OTG_DOEPCTL_CNAK;
}

uint16_t dwc_ep_write_packet(usbd_device *usbd_dev, uint8_t addr, const void *buf, uint16_t len)
{
	(void)usbd_dev;

	addr &= 0x7F;

	// Return if endpoint is already enabled.
	if (USB_INEP(addr)->DIEPTSIZ & USB_OTG_DIEPTSIZ_PKTCNT)
		return 0;

	// Enable endpoint for transmission.
	USB_INEP(addr)->DIEPTSIZ = OTG_DIEPSIZ0_PKTCNT | len;
	USB_INEP(addr)->DIEPCTL |= USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK;

	// Copy buffer to endpoint FIFO, note - memcpy does not work.
	const uint32_t *buf32 = buf;
	for (int i = len; i > 0; i -= 4)
		USB_DFIFO(addr) = *buf32++;

	return len;
}

uint16_t dwc_ep_read_packet(usbd_device *usbd_dev, uint8_t addr, void *buf, uint16_t len)
{
	int i;
	uint32_t *buf32 = buf;

	// We do not need to know the endpoint address since there is only one
	// receive FIFO for all endpoints.
	(void) addr;
	len = MIN(len, usbd_dev->rxbcnt);

	for (i = len; i >= 4; i -= 4)
	{
		*buf32++ = USB_DFIFO(0);
		usbd_dev->rxbcnt -= 4;
	}

	if (i)
	{
		uint32_t extra = USB_DFIFO(0);
		// we read 4 bytes from the fifo, so update rxbcnt
		if (usbd_dev->rxbcnt < 4)
			// Be careful not to underflow (rxbcnt is unsigned)
			usbd_dev->rxbcnt = 0;
		else
			usbd_dev->rxbcnt -= 4;
		memcpy(buf32, &extra, i);
	}

	return len;
}

static void dwc_flush_txfifo(usbd_device *usbd_dev, int ep)
{
	(void)usbd_dev;

	// set IN endpoint NAK
	USB_INEP(ep)->DIEPCTL |= USB_OTG_DIEPCTL_SNAK;
	// wait for core to respond
	while (!(USB_INEP(ep)->DIEPINT & USB_OTG_DIEPINT_INEPNE)) {
		// idle
	}
	// get fifo for this endpoint
	const uint32_t fifo = (USB_INEP(ep)->DIEPCTL & OTG_DIEPCTL0_TXFNUM_MASK) >> 22;
	// wait for core to idle

	while (!(USB_OTG_FS->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL)) {
		// idle
	}
	// flush tx fifo
	USB_OTG_FS->GRSTCTL = (fifo << 6) | USB_OTG_GRSTCTL_TXFFLSH;
	// reset packet counter
	USB_INEP(ep)->DIEPTSIZ = 0;
	while ((USB_OTG_FS->GRSTCTL & USB_OTG_GRSTCTL_TXFFLSH)) {
		// idle
	}
}



void dwc_poll(usbd_device *usbd_dev)
{
	// Read interrupt status register.
	const uint32_t intsts = USB_OTG_FS->GINTSTS;

#if defined DEBUG && 0
	static uint32_t intsts_prev = 0;
	if (intsts_prev != intsts)
	{
		const uint32_t newflag = (intsts ^ intsts_prev) & intsts;

		char* name = "";
		switch(newflag)
		{
		case 1U<<31: name="WKUPINT"; break;
		case 1U<<30: name="SRQINT"; break;
		case 1U<<29: name="DISCINT"; break;
		case 1U<<28: name="CIDSCHG"; break;
		//case 1U<<27: name=""; break;
		case 1U<<26: name="PTXFE"; break;
		case 1U<<25: name="HCINT"; break;
		case 1U<<24: name="HPRTINT"; break;
		//case 1U<<23: name=""; break;
		//case 1U<<22: name=""; break;
		case 1U<<21: name="IPXFR"; break;
		case 1U<<20: name="IISOXFR"; break;
		case 1U<<19: name="OEPINT"; break;
		case 1U<<18: name="IEPINT"; break;
		//case 1U<<17: name=""; break;
		//case 1U<<16: name=""; break;
		case 1U<<15: name="EOPF"; break;
		case 1U<<14: name="ISOODRP"; break;
		case 1U<<13: name="ENUMDNE"; break;
		case 1U<<12: name="USBRST"; break;
		case 1U<<11: name="USBSUSP"; break;
		case 1U<<10: name="ESUSP"; break;
		//case 1U<<9: name=""; break;
		//case 1U<<8: name=""; break;
		case 1U<<7: name="GONAKEFF"; break;
		case 1U<<6: name="GINAKEFF"; break;
		case 1U<<5: name="NPTXFE"; break;
		case 1U<<4: name="RXFLVL"; break;
		case 1U<<3: name="SOF"; break;
		case 1U<<2: name="OTGINT"; break;
		case 1U<<1: name="MMIS"; break;
		case 1U<<0: name="CMOD"; break;
		}

		DBG("USB: %08X %X %s\n", intsts, newflag, name);
		intsts_prev = intsts;
	}
#endif

	if (intsts & USB_OTG_GINTSTS_ENUMDNE)
	{
		// Handle USB RESET condition.
		USB_OTG_FS->GINTSTS = USB_OTG_GINTSTS_ENUMDNE;
		usbd_dev->fifo_mem_top = usbd_dev->driver->rx_fifo_size;
		_usbd_reset(usbd_dev);
		return;
	}

	// There is no global interrupt flag for transmit complete.
	// The XFRC bit must be checked in each OTG_DIEPINT(x).
	for (int ep = 0; ep < 4; ep++)	// Iterate over endpoints.
	{
		if (USB_INEP(ep)->DIEPINT & USB_OTG_DIEPINT_XFRC)
		{
			// Transfer complete.
			if (usbd_dev->user_callback_ctr[ep][USB_TRANSACTION_IN])
				usbd_dev->user_callback_ctr[ep][USB_TRANSACTION_IN](usbd_dev, ep);
			USB_INEP(ep)->DIEPINT = USB_OTG_DIEPINT_XFRC;
		}
	}

	// Note: RX and TX handled differently in this device.
	if (intsts & USB_OTG_GINTSTS_RXFLVL)
	{
		// Receive FIFO non-empty.
		const uint32_t rxstsp = USB_OTG_FS->GRXSTSP;
		const uint32_t pktsts = rxstsp & USB_OTG_GRXSTSP_PKTSTS;
		const uint8_t ep = rxstsp & USB_OTG_GRXSTSP_EPNUM;

//		DBG("RXFLVL: ep:%d pktsts:%d\n", ep, pktsts>>17);

		if (pktsts == OTG_GRXSTSP_PKTSTS_SETUP_COMP)
			usbd_dev->user_callback_ctr[ep][USB_TRANSACTION_SETUP] (usbd_dev, ep);

		if (pktsts == OTG_GRXSTSP_PKTSTS_OUT_COMP ||
			pktsts == OTG_GRXSTSP_PKTSTS_SETUP_COMP)
		{
			USB_OUTEP(ep)->DOEPTSIZ = usbd_dev->doeptsiz[ep];
			USB_OUTEP(ep)->DOEPCTL |= USB_OTG_DOEPCTL_EPENA |
				(usbd_dev->force_nak[ep] ?
				 USB_OTG_DOEPCTL_SNAK : USB_OTG_DOEPCTL_CNAK);
			return;
		}

		if ((pktsts != OTG_GRXSTSP_PKTSTS_OUT) &&
			(pktsts != OTG_GRXSTSP_PKTSTS_SETUP))
			return;

		const uint8_t type = (pktsts == OTG_GRXSTSP_PKTSTS_SETUP) ?
			USB_TRANSACTION_SETUP : USB_TRANSACTION_OUT;

		if (type == USB_TRANSACTION_SETUP
			&& (USB_INEP(ep)->DIEPTSIZ & OTG_DIEPSIZ0_PKTCNT))
		{
			// SETUP received but there is still something stuck
			// in the transmit fifo.  Flush it.

			dwc_flush_txfifo(usbd_dev, ep);
		}

		// Save packet size for dwc_ep_read_packet().
		usbd_dev->rxbcnt = (rxstsp & USB_OTG_GRXSTSP_BCNT) >> 4;

		if (type == USB_TRANSACTION_SETUP) {
			dwc_ep_read_packet(usbd_dev, ep, &usbd_dev->control_state.req, 8);
		} else if (usbd_dev->user_callback_ctr[ep][type]) {
			usbd_dev->user_callback_ctr[ep][type] (usbd_dev, ep);
		}

		// Discard unread packet data.
		for (int i = 0; i < usbd_dev->rxbcnt; i += 4) {
			// There is only one receive FIFO, so use OTG_FIFO(0)
			(void)USB_DFIFO(0);
		}

		usbd_dev->rxbcnt = 0;
	}

	if (intsts & USB_OTG_GINTSTS_USBSUSP)
	{
		if (usbd_dev->user_callback_suspend)
			usbd_dev->user_callback_suspend();
		USB_OTG_FS->GINTSTS = USB_OTG_GINTSTS_USBSUSP;
	}

	if (intsts & USB_OTG_GINTSTS_WKUINT)
	{
		if (usbd_dev->user_callback_resume)
			usbd_dev->user_callback_resume();
		USB_OTG_FS->GINTSTS = USB_OTG_GINTSTS_WKUINT;
	}

}

void dwc_disconnect(usbd_device *usbd_dev, bool disconnected)
{
	(void)usbd_dev;
	if (disconnected)
		USB_DEVICE->DCTL |= USB_OTG_DCTL_SDIS;
	else
		USB_DEVICE->DCTL &= ~USB_OTG_DCTL_SDIS;
}
