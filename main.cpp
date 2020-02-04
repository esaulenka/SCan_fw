
#include "Pins.h"
#include "Debug.h"
#include "timer.h"
#include "cdcacm.h"
#include <cstdio>



extern "C" int main();
int main()
{
	DBG(0, "\n\nCore started!\n");

	Timer::init();
	Usb::init();

	Timer sendTmr;
	bool connectPrev = false;


	while (1)
	{

		bool connect = Usb::checkConnect();
		if (connect != connectPrev)
		{
			connectPrev = connect;
			if (connect)
				SEGGER_RTT_printf(0, "connected\n");
			else
				SEGGER_RTT_printf(0, "disconnect\n");
		}


		if (sendTmr.checkTimeout(1000))
		{
			sendTmr.restart();
			char buf[32];
			int len = sprintf(buf, "%d\n", (int)Timer::counter());
			Usb::send(buf, len);
		}
//		auto checkPort = [](uint16_t &oldVal, uint16_t newVal, const char* label) {
//			if (oldVal != newVal) {
//				DBG(0, "%s: %X -> %X\n", label, oldVal, newVal);
//				oldVal = newVal;
//			}
//		};
//		static uint16_t port[4] = {};
//		const uint16_t maskA = (1<<13) | (1<<14) | (1<<11) | (1<<12);	// USB, SWD
//		checkPort(port[0], GPIOA->IDR & (~maskA), "portA");
//		checkPort(port[1], GPIOB->IDR, "portB");
//		checkPort(port[2], GPIOC->IDR, "portC");
//		checkPort(port[3], GPIOD->IDR, "portD");

	}
}

