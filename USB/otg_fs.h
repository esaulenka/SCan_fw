/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
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

/*
 * This file covers definitions for DesignWare USB OTG HS peripherals.
 */

#ifndef LIBOPENCM3_USB_DWC_OTG_FS_H
#define LIBOPENCM3_USB_DWC_OTG_FS_H

#include "otg_common.h"
#include "stm32f1xx.h"



typedef struct
{
  __IO uint32_t PCGCR;                /*!< Power and clock gating control             Address offset: E00h*/
  uint32_t ReservedE04[127];          /*!< Reserved                                   E04h .. FFFh*/
} USB_OTG_PowerClockGatingTypeDef;


//#define USB_OTG_FS_DEV          ((USB_OTG_DeviceTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_DEVICE_BASE))
#define USB_DEVICE			((USB_OTG_DeviceTypeDef *)		(USB_OTG_FS_PERIPH_BASE + USB_OTG_DEVICE_BASE))
#define USB_INEP(i)			((USB_OTG_INEndpointTypeDef *)	(USB_OTG_FS_PERIPH_BASE + USB_OTG_IN_ENDPOINT_BASE +  (i)*USB_OTG_EP_REG_SIZE))
#define USB_OUTEP(i)		((USB_OTG_OUTEndpointTypeDef *)	(USB_OTG_FS_PERIPH_BASE + USB_OTG_OUT_ENDPOINT_BASE + (i)*USB_OTG_EP_REG_SIZE))
#define USB_DFIFO(i)		*(__IO uint32_t *)				(USB_OTG_FS_PERIPH_BASE + USB_OTG_FIFO_BASE + (i) * USB_OTG_FIFO_SIZE)
#define USB_OTG_PCGCCTL		((USB_OTG_PowerClockGatingTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_PCGCCTL_BASE))


#endif
