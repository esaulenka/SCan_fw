
#include "usbd.h"
#include "dfu.h"
#include "dfu_descriptors.h"
#include "stm32_flash.h"
#include "pin.h"
#include "Debug.h"
#include <cstring>

using STM32::Flash;


// USB pins
using USB_DM = Pin<'A', 11>;
using USB_DP = Pin<'A', 12>;
using USB_VBus = Pin<'A', 9>;

// Button
using PinButton = Pin<'B', 14, 'L'>;	// pushbutton inverted


#define APP_ADDRESS	0x08004000

// Commands sent with wBlockNum == 0 as per ST implementation.
#define CMD_SETADDR	0x21
#define CMD_ERASE	0x41

// We need a special large control buffer for this device:
static uint8_t usbd_control_buffer[1024];


static enum dfu_state usbdfu_state = STATE_DFU_IDLE;

static struct {
	uint8_t buf[sizeof(usbd_control_buffer)];
	uint16_t len;
	uint16_t blocknum;
	uint32_t addr;
} prog;


static uint8_t usbdfu_getstatus(usbd_device *usbd_dev, uint32_t *bwPollTimeout)
{
	(void)usbd_dev;

	switch (usbdfu_state)
	{
	case STATE_DFU_DNLOAD_SYNC:
		usbdfu_state = STATE_DFU_DNBUSY;
		*bwPollTimeout = 100;
		return DFU_STATUS_OK;
	case STATE_DFU_MANIFEST_SYNC:
		// Device will reset when read is complete.
		usbdfu_state = STATE_DFU_MANIFEST;
		return DFU_STATUS_OK;
	default:
		return DFU_STATUS_OK;
	}
}

static void usbdfu_getstatus_complete(usbd_device *usbd_dev, struct usb_setup_data *req)
{
	(void)req;
	(void)usbd_dev;

	switch (usbdfu_state)
	{
	case STATE_DFU_DNBUSY:
		Flash::Unlock();
		if (prog.blocknum == 0)
		{
			switch (prog.buf[0])
			{
			case CMD_ERASE:
				{
					uint32_t *dat = (uint32_t *)(prog.buf + 1);
					Flash::ErasePage(*dat);
					prog.addr = *dat;
					break;
				}
			case CMD_SETADDR:
				{
					uint32_t *dat = (uint32_t *)(prog.buf + 1);
					prog.addr = *dat;
					break;
				}
			}
		}
		else
		{
			const uint32_t baseaddr = prog.addr + ((prog.blocknum - 2) * dfu_function.wTransferSize);
			for (uint32_t i = 0; i < prog.len; i += 2)
			{
				uint16_t *dat = (uint16_t *)(prog.buf + i);
				Flash::WriteHalfword(baseaddr + i, *dat);
			}
		}
		Flash::Lock();
		// Jump straight to dfuDNLOAD-IDLE, skipping dfuDNLOAD-SYNC.
		usbdfu_state = STATE_DFU_DNLOAD_IDLE;
		return;

	case STATE_DFU_MANIFEST:
		// USB device must detach, we just reset...
		NVIC_SystemReset();
		return; // Will never return.

	default: return;
	}
}

static enum usbd_request_return_codes usbdfu_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
		uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	if ((req->bmRequestType & 0x7F) != DFU_FUNCTIONAL)
		return USBD_REQ_NOTSUPP; // Only accept class request.

	switch (req->bRequest)
	{
	case DFU_DNLOAD:
		if ((len == nullptr) || (*len == 0))
		{
			usbdfu_state = STATE_DFU_MANIFEST_SYNC;
		}
		else
		{
			// Copy download data for use on GET_STATUS.
			prog.blocknum = req->wValue;
			prog.len = *len;
			memcpy(prog.buf, *buf, *len);
			usbdfu_state = STATE_DFU_DNLOAD_SYNC;
		}
		return USBD_REQ_HANDLED;

	case DFU_CLRSTATUS:
		// Clear error and return to dfuIDLE.
		if (usbdfu_state == STATE_DFU_ERROR)
			usbdfu_state = STATE_DFU_IDLE;
		return USBD_REQ_HANDLED;

	case DFU_ABORT:
		// Abort returns to dfuIDLE state.
		usbdfu_state = STATE_DFU_IDLE;
		return USBD_REQ_HANDLED;

	case DFU_UPLOAD:
		// Upload not supported for now.
		return USBD_REQ_NOTSUPP;

	case DFU_GETSTATUS: {
		uint32_t bwPollTimeout = 0; // 24-bit integer in DFU class spec
		(*buf)[0] = usbdfu_getstatus(usbd_dev, &bwPollTimeout);
		(*buf)[1] = bwPollTimeout & 0xFF;
		(*buf)[2] = (bwPollTimeout >> 8) & 0xFF;
		(*buf)[3] = (bwPollTimeout >> 16) & 0xFF;
		(*buf)[4] = usbdfu_state;
		(*buf)[5] = 0; // iString not used here
		*len = 6;
		*complete = usbdfu_getstatus_complete;
		return USBD_REQ_HANDLED;
		}

	case DFU_GETSTATE:
		// Return state with no state transision.
		*buf[0] = usbdfu_state;
		*len = 1;
		return USBD_REQ_HANDLED;
	}

	return USBD_REQ_NOTSUPP;
}

static void usbdfu_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;

	usbd_register_control_callback(
				usbd_dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				usbdfu_control_request);
}

extern "C" int main();
int main()
{
	DBG("\n\nBoot started!\n");

	// try to run app, if button not pressed
	if (!PinButton::Signalled())
	{
		// Boot the application if it's valid.
		const uint32_t stackPtr = *(volatile uint32_t *)APP_ADDRESS;
		if ((stackPtr & 0xFFFE0000) == 0x20000000)
		{
			// Set vector table base address.
			SCB->VTOR = APP_ADDRESS;
			// Initialise master stack pointer.
			__set_MSP(*(volatile uint32_t *)APP_ADDRESS);
			// Jump to application.
			(*(void (**)())(APP_ADDRESS + 4))();
		}
	}

	// Init USB pins
	USB_DM::Mode(ALT_OUTPUT);
	USB_DP::Mode(ALT_OUTPUT);
	// force '1' on VBus
	USB_VBus::Mode(OUTPUT); USB_VBus::Set();

	usbd_device *usbd_dev = usbd_init(&stm32f107_usb_driver, &dev, &config, usb_strings, 4, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_dev, usbdfu_set_config);

	while (1)
	{
		usbd_poll(usbd_dev);
	}
}

