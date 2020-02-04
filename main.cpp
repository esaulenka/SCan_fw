
#include "Pins.h"
#include "Debug.h"
#include "timer.h"
#include "cdcacm.h"
#include "canhacker.h"
#include "stm32.h"
#include <cstdio>

CanHacker canHack;

extern "C" int main();
int main()
{
	DBG("\n\nCore started!\n");

	Timer::init();
	Usb::init();

//	Timer sendTmr;
	bool connectPrev = false;


	while (1)
	{

		bool connect = Usb::checkConnect();
		if (connect != connectPrev)
		{
			connectPrev = connect;
			if (connect)
				DBG("connected\n");
			else
				DBG("disconnect\n");
		}


//		if (sendTmr.checkTimeout(1000))
//		{
//			sendTmr.restart();
//			char buf[32];
//			int len = sprintf(buf, "%d\n", (int)Timer::counter());
//			Usb::send(buf, len);
//		}

		canHack.processCmd();

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
		checkPort(port[3], GPIOD->IDR, "portD");

	}
}

