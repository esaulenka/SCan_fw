
#include "Pins.h"
#include "Debug.h"
#include "timer.h"
#include "cdcacm.h"
#include "canhacker.h"
#include "stm32.h"
#include <cstdio>


extern "C" int main();
int main()
{
	DBG("\n\nCore started!\n");

	initRemap();

	Timer::init();
	Usb::init();

	// LIN tests
//	PinLin1Slp::Mode(OUTPUT_2MHZ);	PinLin1Slp::On();
//	PinLin2Slp::Mode(OUTPUT_2MHZ);	PinLin2Slp::On();
//	PinLinBreak::Mode(OUTPUT_2MHZ);	PinLinBreak::On();

	while (1)
	{

		Usb::checkConnect();

		if (! canHacker.processPackets() &&
			! canHacker.processCmd())
		{
			// go to sleep
			__WFI();
		}

		/*
		auto checkPort = [](uint16_t &oldVal, uint16_t newVal, const char* label) {
			if (oldVal != newVal) {
				DBG("%s: %X -> %X\n", label, oldVal, newVal);
				oldVal = newVal;
			}
		};
		static uint16_t port[4] = {};
		const uint16_t maskA = (1<<13) | (1<<14) | (1<<11) | (1<<12);	// USB, SWD
		checkPort(port[0], GPIOA->IDR & (~maskA), "portA");
		checkPort(port[1], GPIOB->IDR, "portB");
		checkPort(port[2], GPIOC->IDR, "portC");
		checkPort(port[3], GPIOD->IDR, "portD");*/

	}
}

