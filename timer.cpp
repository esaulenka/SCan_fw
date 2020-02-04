#include "timer.h"
#include "stm32.h"
#include "SysClock.h"

volatile Timer::T_Timer	Timer::_counter;

extern "C"
void SysTick_Handler()
{
	Timer::_counter ++;
}


void Timer::init()
{
	NVIC_SetPriority(SysTick_IRQn, 0xFE);	// lowest priority

	SysTick->LOAD = CLOCK_AHB/1000 - 1;

	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |	// use processor clock
					SysTick_CTRL_TICKINT_Msk |		// enable interrupt
					SysTick_CTRL_ENABLE_Msk;		// enable counter

}
