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
using PinBuzzer		= Pin<'C',8>;		// TIM1_CH1N ???



// Inputs
using PinWirePink			= Pin<'C',0>;
using PinWireGreenYellow	= Pin<'C',1>;
using PinWireViolet			= Pin<'C',4>;
using PinWireBlackWhite		= Pin<'B',0>;
using PinWireGreenBlack		= Pin<'B',7>;


// CAN-bus
using PinCan1Rx		= Pin<'B',8>;	// remapped!
using PinCan1Tx		= Pin<'B',9>;
using PinCan1Stdby	= Pin<'B',1>;	// ???
using PinCan2Rx		= Pin<'B',12>;
using PinCan2Tx		= Pin<'B',13>;
using PinCan2Stdby	= Pin<'C',14>;	// ???


// LIN
using PinLin3Tx		= Pin<'B',10>;	// ???
using PinLin3Rx		= Pin<'B',11>;	// ???

