#pragma once

#include "pin.h"

// USB
using PinUsbDP		= Pin<'A',12>;
using PinUsbDM		= Pin<'A',11>;
// connected to USART-TX blue wire
using PinUsbVbus	= Pin<'A',9>;
// real USB connection status
using PinUsbConnect	= Pin<'B',15>;


// SWD
using PinSWDIO		= Pin<'A',13>;
using PinSWDCLK		= Pin<'A',14>;
using PinSWO		= Pin<'B',3>;


// Misc
using PinButton		= Pin<'B',14, 'L'>;	// active low
using PinBuzzer		= Pin<'C',8>;		// TIM1_CH1N



// Inputs
using PinWirePink			= Pin<'C',0>;
using PinWireGreenYellow	= Pin<'C',1>;
using PinWireViolet			= Pin<'C',4>;
using PinWireBlackWhite		= Pin<'B',0>;
using PinWireGreenBlack		= Pin<'B',7>;
// using PinWireYellowBlack		= Pin<'?',??>;

// Outputs
using PinWireBlackYellow	= Pin<'C',3>;	// !!! поднимается до +4 вольт, меандра нет?!
using PinWireOrangeViolet	= Pin<'A',4>;
using PinWireBlackRed		= Pin<'A',5>;
using PinWireOrangeGray		= Pin<'A',6>;
using PinWireGrayBlack		= Pin<'A',7>;
using PinWireOrangeBlack	= Pin<'B',2>;

//using PinWireYellowRed	= Pin<'?',??>;
//using PinWireBlueRed		= Pin<'?',??>;

//using PinWireBlueBlack	= Pin<'?',??>;
//using PinWireYellow		= Pin<'?',??>;



// CAN-bus
using PinCan1Rx		= Pin<'B',8>;	// remapped!
using PinCan1Tx		= Pin<'B',9>;
using PinCan1Stdby	= Pin<'B',1>;
using PinCan2Rx		= Pin<'B',12>;
using PinCan2Tx		= Pin<'B',13>;
using PinCan2Stdby	= Pin<'C',14>;


// LIN
using PinLin3Tx		= Pin<'B',10>;	// ???
using PinLin3Rx		= Pin<'B',11>;	// ???
using PinLinBreak	= Pin<'B',6>;


// External flash
// SPI3, remapped
using PinFlashCS	= Pin<'C',9>;
using PinFlashSCK	= Pin<'C',10>;
using PinFlashMISO	= Pin<'C',11>;
using PinFlashMOSI	= Pin<'C',12>;



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
	if (on)
		gpio->BSRR = 1<<pin;
	else
		gpio->BRR = 1<<pin;
}

