#include "LinDrv.h"
#include "LinBus.h"
#include "Pins.h"
#include "stm32_rcc.h"
#include "SysClock.h"

#if LIN_BUS_SUPPORTED

static const LinProperties lin1prop = {
	.usart = USART2,
	.irq = USART2_IRQn,
	.periphClock = CLOCK_APB1,
	.enableClock = [](bool enable) {
		PeriphPwr::Enable(PeriphPwr::PwrUSART2, enable);
	},
	.initPins = []() {
		PinLin1Rx::Mode(INPUT);
		PinLin1Tx::Mode(ALT_OUTPUT_2MHZ);
		PinLin1Slp::On();
		PinLin1Slp::Mode(OUTPUT_2MHZ);
	},
	.enableDriver = [](bool enable) {
		PinLin1Slp::On(enable);
	},
};

static const LinProperties lin2prop = {
	.usart = USART3,
	.irq = USART3_IRQn,
	.periphClock = CLOCK_APB1,
	.enableClock = [](bool enable) {
		PeriphPwr::Enable(PeriphPwr::PwrUSART3, enable);
	},
	.initPins = []() {
		PinLin2Rx::Mode(INPUT);
		PinLin2Tx::Mode(ALT_OUTPUT_2MHZ);
		PinLin2Slp::On();
		PinLin2Slp::Mode(OUTPUT_2MHZ);
	},
	.enableDriver = [](bool enable) {
		PinLin2Slp::On(enable);
	},
};


LinDrv lin1drv(lin1prop, linBus1);
LinDrv lin2drv(lin2prop, linBus2);


extern "C" {
void USART2_IRQHandler ()
{	lin1drv.irqHandler();	}

void USART3_IRQHandler ()
{	lin2drv.irqHandler();	}

}


void LinDrv::init(const LinInit & params)
{
	properties.enableClock(true);
	properties.initPins();

	uint32_t parity = 0;				// no parity, 8-bits
	if (params.parity == 'E')			// even parity
		parity = USART_CR1_M | USART_CR1_PCE;
	else if (params.parity == 'O')		// odd parity
		parity = USART_CR1_M | USART_CR1_PCE | USART_CR1_PS;

	properties.usart->CR1 = 0
			| parity
			| USART_CR1_RE      // receive enable
			| USART_CR1_TE      // transmit enable
			| USART_CR1_RXNEIE  // RX not empty interrupt enable
//			| USART_CR1_TCIE	// transmit completed interrupt enable
			;

	// lin mode enable, lin break interrupt enable, lin break - 11 bits
	uint32_t linMode = params.linMode ?
				(USART_CR2_LINEN | USART_CR2_LBDIE | USART_CR2_LBDL) : 0;

	uint32_t stopBits = 0;				// 1 stop bit
	if (params.stopBits == 2)			// 2 stop bits
		stopBits = USART_CR2_STOP_0 * 2;
	else if (params.stopBits == 15)		// 1.5 stop bit
		stopBits = USART_CR2_STOP_0 * 3;

	properties.usart->CR2 = 0
			| stopBits
			| linMode
			;

	properties.usart->CR3 = 0; // no flow control

	// set baudrate
	properties.usart->BRR = (properties.periphClock + params.baudrate/2) / params.baudrate;

	properties.usart->CR1 |= USART_CR1_UE;		// enable uart

	NVIC_EnableIRQ(properties.irq);
}

void LinDrv::deinit()
{
	NVIC_DisableIRQ(properties.irq);
	properties.usart->CR1 = 0;		// disable uart
	properties.enableClock(false);
}

void LinDrv::sleep(bool sleep)
{
	if (driverDisabled == sleep)
		return;
	//DBG("LinDrv::sleep(%d)\n", sleep);
	driverDisabled = sleep;
	// turn off external driver
	properties.enableDriver(! sleep);
}

void LinDrv::startSend()
{
	// enable tx empty interrupt
	properties.usart->CR1 |= USART_CR1_TXEIE;
}

void LinDrv::sendBreak()
{
	sleep(false);
	properties.usart->CR1 |= USART_CR1_SBK;
}

void LinDrv::irqHandler()
{
	auto regs = properties.usart;
	const uint32_t status = regs->SR;
	const uint32_t enabled = regs->CR1;

	if (driverDisabled)
		sleep(false);

	// receive not empty
	if (status & USART_SR_RXNE)
	{
		uint8_t data = regs->DR;
		//DBG("%p irqRx d=%02X echo=%X/%d\n", this, data, echo.reg, echo.count);
		//if (echo.pop() != data)
		{
			parent.rxData(data);
		}
	}

	// transmitter empty
	if (status & enabled & USART_SR_TXE)
	{
		int d = parent.getTxData();
		//DBG("%p irqTx d=%02X echo=%X/%d\n", this, d, echo.reg, echo.count);

		if (d >= 0)
		{
			//echo.push(d);
			regs->DR = d;
		}
		else
		{
			// disable tx empty intrrupt
			regs->CR1 &= ~USART_CR1_TXEIE;
		}
	}

	// error (overrun, parity, frame, noise)
	if (status & (USART_SR_ORE | USART_SR_PE | USART_SR_FE | USART_SR_NE))
	{
		// clear flag
		(void)regs->DR;
		parent.rxError();
	}

	// LIN break
	if (status & USART_SR_LBD)
	{
		// clear flag
		regs->SR = ~USART_SR_LBD;
		parent.rxBreak();
	}
}

#endif
