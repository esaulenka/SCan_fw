#pragma once

#include "pin.h"
#include "stm32.h"

// SWD
using PinSWDIO		= Pin<'A',13>;
using PinSWDCLK		= Pin<'A',14>;
using PinSWO		= Pin<'B',3>;


#if BOARD == BOARD_SIGMA			// Sigma board

// USB
using PinUsbDP		= Pin<'A',12>;
using PinUsbDM		= Pin<'A',11>;
// connected to USART-TX blue wire
using PinUsbVbus	= Pin<'A',9>;
// real USB connection status
using PinUsbConnect	= Pin<'B',15>;


// Misc
using PinButton		= Pin<'B',14, 'L'>;	// active low
using PinBuzzer		= Pin<'C',8>;		// TIM3_CH3 remapped



// Inputs
using PinWirePink			= Pin<'C',0>;
using PinWireGreenYellow	= Pin<'C',1>;
using PinWireViolet			= Pin<'C',4>;
using PinWireBlackWhite		= Pin<'B',0>;
using PinWireGreenBlack		= Pin<'B',7>;
//using PinWireYellowBlack		= Pin<'?',??>;


// Outputs
using PinWireYellowRed		= Pin<'C',15>;
using PinWireBlueRed		= Pin<'C',13>;
using PinWireBlackYellow	= Pin<'C',3>;
using PinWireBlueBlack		= Pin<'A',0>;
using PinWireYellow			= Pin<'A',1>;
using PinWireOrangeViolet	= Pin<'A',4>;
using PinWireBlackRed		= Pin<'A',5>;
using PinWireOrangeGray		= Pin<'A',6>;
using PinWireGrayBlack		= Pin<'A',7>;
using PinWireOrangeBlack	= Pin<'B',2>;


// SigmaBus - USART1
using PinWireBlue			= Pin<'A',10>;	// RX
using PinWireGreen			= Pin<'A',9>;	// TX !!! same as VBus


// CAN-bus; TJA1048
using PinCan1Rx		= Pin<'B',8>;	// remapped!
using PinCan1Tx		= Pin<'B',9>;
using PinCan1Stdby	= Pin<'B',1>;
using PinCan2Rx		= Pin<'B',12>;
using PinCan2Tx		= Pin<'B',13>;
using PinCan2Stdby	= Pin<'C',14>;


// LIN; TJA1022
using PinLin1Tx		= Pin<'A',2>;	// USART2
using PinLin1Rx		= Pin<'A',3>;
using PinLin1Slp	= Pin<'A',8>;
using PinLin2Tx		= Pin<'B',10>;	// USART3
using PinLin2Rx		= Pin<'B',11>;
using PinLin2Slp	= Pin<'D',2>;
using PinLinBreak	= Pin<'B',6>;	// high to break link


// External flash
// SPI3, remapped
using PinFlashCS	= Pin<'C',9>;
using PinFlashSCK	= Pin<'C',10>;
using PinFlashMISO	= Pin<'C',11>;
using PinFlashMOSI	= Pin<'C',12>;


// unknown:
// PC2, PC5, PC6, PC7, PA15, PB4, PB5

inline void initRemap()
{
	// use only SWD (not JTAG)
	// remap CAN1, TIM3 (buzzer), SPI3
	AFIO->MAPR =
			AFIO_MAPR_SWJ_CFG_0 * 2 |
			AFIO_MAPR_CAN_REMAP_0*2 |
			AFIO_MAPR_TIM3_REMAP_0*3 |
			AFIO_MAPR_SPI3_REMAP;
}


#elif BOARD == BOARD_2CAN

// USB
using PinUsbDP		= Pin<'A',12>;
using PinUsbDM		= Pin<'A',11>;
// connected to USART-TX blue wire
using PinUsbVbus	= Pin<'A',9>;
using PinUsbConnect = DummyPinOn;

// CAN-bus; TJA1048
using PinCan1Rx		= Pin<'B',8>;	// remapped
using PinCan1Tx		= Pin<'B',9>;
using PinCan1Stdby	= Pin<'B',7>;
using PinCan2Rx		= Pin<'B',5>;	// remapped
using PinCan2Tx		= Pin<'B',6>;
using PinCan2Stdby	= Pin<'B',4>;


inline void initRemap()
{
	// use only SWD (not JTAG)
	// remap CAN1, CAN2
	AFIO->MAPR =
			AFIO_MAPR_SWJ_CFG_0 * 2 |
			AFIO_MAPR_CAN_REMAP_0*2 |
			AFIO_MAPR_CAN2_REMAP;
}

#endif	// BOARD == xxx


inline void setMode(char port, int pin, int mode)
{
	auto gpio = (GPIOxTypeDef*)(pGPIOA_BASE + (pGPIOB_BASE-pGPIOA_BASE) * (port-'A'));
	auto &control = (pin < 8) ? gpio->CRL : gpio->CRH;
	int offset = (pin & 7) * 4;
	control = (control & ~(0xF << offset)) | (mode << offset);
}
inline void setOut(char port, int pin, int on)
{
	auto gpio = (GPIOxTypeDef*)(pGPIOA_BASE + (pGPIOB_BASE-pGPIOA_BASE) * (port-'A'));
	pin &= 0xF;
	if (on)	gpio->BSRR = 1<<pin;
	else	gpio->BRR = 1<<pin;
}

