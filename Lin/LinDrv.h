#pragma once

#include "stm32.h"


struct LinProperties {
	USART_TypeDef * usart;
	IRQn irq;
	uint32_t periphClock;

	void (*enableClock)(bool enable);
	void (*initPins)();
	void (*enableDriver)(bool enable);

};

class LinBus;



class LinDrv
{
	const LinProperties & properties;
	LinBus & parent;

public:
	LinDrv(const LinProperties & prop, LinBus & linBus) :
		properties(prop),
		parent(linBus)
	{}


	struct LinInit {
		uint32_t baudrate;
		uint8_t stopBits;	// 1, 15, 2
		char parity;		// 'N', 'E', 'O'
		bool linMode;
	};

	void init(const LinInit & params);
	void deinit();

	void sleep(bool sleep);

	void startSend();

	void sendBreak();

	void irqHandler();

private:
	bool driverDisabled = false;

	struct {
		uint32_t reg = 0;
		int8_t count = 0;
		void push(uint8_t d)
		{
			reg = (reg << 8) | d;
			if (count < 3) count++;
		}
		int32_t pop()
		{
			if (count > 0) count--;
			else return -1;		// EOF
			uint32_t shift = count * 8;
			return ((reg >> shift) & 0xff);
		}
	} echo;

};

