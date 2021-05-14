#pragma once

#include "stm32_tim.h"
#include "Pins.h"
#include "SysClock.h"

class LedBlink {
using Tmr = STM32::TIM::Timer<STM32::TIM::TIM_3>;

public:

static void init()
{
#if BOARD == BOARD_CSAT
	PinLedTx::Mode(ALT_OUTPUT);		// ch1
	PinLedRx::Mode(ALT_OUTPUT);		// ch2
	static_assert (PinLedTx::port_no == 0); static_assert(PinLedTx::pin == 6);
	static_assert (PinLedRx::port_no == 0); static_assert(PinLedRx::pin == 7);

	Tmr::EnableClocks();

	// Timer clock = 1000 Hz
	const uint32_t prescaler = CLOCK_APB1_TIM / 1000;
	static_assert(prescaler < 0xffff);

	Tmr::TIMx->PSC = prescaler - 1;
	Tmr::TIMx->ARR = 0xFFFF;

	Tmr::Ch1::CCMRx = (Tmr::Ch1::CCMRx & ~Tmr::Ch1::CCMR_MASK)
			| Tmr::Ch1::CCMR_CCS_OUTPUT
			| Tmr::Ch1::CCMR_OCM_FORCE_OFF;

	Tmr::Ch2::CCMRx = (Tmr::Ch2::CCMRx & ~Tmr::Ch2::CCMR_MASK)
			| Tmr::Ch2::CCMR_CCS_OUTPUT
			| Tmr::Ch2::CCMR_OCM_FORCE_OFF;

	// enable, with inverse polarity
	Tmr::TIMx->CCER = 0
			| Tmr::Ch1::CCER_CCxP | Tmr::Ch1::CCER_CCxE
			| Tmr::Ch2::CCER_CCxP | Tmr::Ch2::CCER_CCxE;

	Tmr::TIMx->EGR = TIM_EGR_UG;	// force update
	Tmr::Enable();
#endif
}

static void pulseTx() {
	runChannel<Tmr::Ch1>(10);
}
static void pulseRx() {
	runChannel<Tmr::Ch2>(10);
}


private:

template <class Channel>
static void runChannel(uint16_t timeout)
{
	(void)timeout;
#if BOARD == BOARD_CSAT
	// force active
	Channel::CCMRx = (Channel::CCMRx & ~Channel::CCMR_OCM)
			| Channel::CCMR_OCM_FORCE_ON;
	// set inactive on compare
	Channel::CCRx = Tmr::Count() + timeout;
	Channel::CCMRx = (Channel::CCMRx & ~Channel::CCMR_OCM)
			| Channel::CCMR_OCM_RESET;
#endif
}


};
