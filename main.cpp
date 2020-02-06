
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

	// remap CAN1, TIM3 (buzzer), SPI3
	AFIO->MAPR =
			AFIO_MAPR_CAN_REMAP_0*2 |
			AFIO_MAPR_TIM3_REMAP_0*3 |
			AFIO_MAPR_SPI3_REMAP;

	Timer::init();
	Usb::init();

	// LIN tests
//	PinLin1Slp::Mode(OUTPUT_2MHZ);	PinLin1Slp::On();
//	PinLin2Slp::Mode(OUTPUT_2MHZ);	PinLin2Slp::On();
//	PinLinBreak::Mode(OUTPUT_2MHZ);	PinLinBreak::On();

	//bool connectPrev = false;

	while (1)
	{

		/*bool connect =*/ Usb::checkConnect();
		/*if (connect != connectPrev)
		{
			connectPrev = connect;
			if (connect)	DBG("connected\n");
			else			DBG("disconnect\n");
		}*/

		canHacker.processPackets();
		canHacker.processCmd();

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

