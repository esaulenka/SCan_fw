#include "stm32f1xx.h"
#include "SysClock.h"
#include "exhandler.h"

#define HSE_STARTUP_TIMEOUT   1024 // Time out for HSE start up. На демоплате получается порядка 100-150 итераций, сделаем c запасом.


static inline void init_clocks ()
{

	// Reset CFGR2 - PLL2, PLL3 on STM32F1xx_CL
	RCC->CFGR2 = 0;

	/* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/

	// Enable HSE
	RCC->CR |= RCC_CR_HSEON;

	// Wait till HSE is ready and if Time out is reached exit
	uint32_t HSEStatus = 0;
	for (int32_t i = HSE_STARTUP_TIMEOUT; i > 0; i--)
	{
		HSEStatus = RCC->CR & RCC_CR_HSERDY;
		if (HSEStatus) break;
	}

	if (!HSEStatus)
	{
		// FIXME!!!
		// If HSE fails to start-up, the application will have wrong clock configuration.

		return;
	}

	// Configure PLLs ------------------------------------------------------
#if CLOCK_HSE == MHz(25)		// Demoboard

	// PLL2 configuration: PLL2CLK = (HSE / 5) * 8 = 40 MHz
	// PREDIV1 configuration: PREDIV1CLK = PLL2 / 5 = 8 MHz
	RCC->CFGR2 = 	RCC_CFGR2_PREDIV2_DIV5 |		// PLL2Clk divisor = 5
					RCC_CFGR2_PLL2MUL8 |			// PLL2Clk multiplier = 8
					RCC_CFGR2_PREDIV1SRC_PLL2 |		// PLL1 prediv source = PLL2
					RCC_CFGR2_PREDIV1_DIV5;			// PLL1 divisor = 5

	// Enable PLL2
	RCC->CR |= RCC_CR_PLL2ON;
	// Wait till PLL2 is ready
	while (! (RCC->CR & RCC_CR_PLL2RDY))
		;

	// PLL1 configuration: PLLCLK = PREDIV1 * 9 = 72 MHz
	RCC->CFGR |= 	RCC_CFGR_PLLSRC |			// PLL1 source - PREDIV1
					RCC_CFGR_PLLMULL9;			// PLL1 multiplier = 9

#elif CLOCK_HSE == MHz(16)		// SCan board

	// PLL2 off
	// PREDIV1 configuration: PREDIV1CLK = HSE / 2 = 8 MHz
	RCC->CFGR2 =	RCC_CFGR2_PREDIV1SRC_HSE |		// PLL1 prediv source = HSE
					RCC_CFGR2_PREDIV1_DIV2;			// PLL1 divisor = 2

	// PLL1 configuration: PLLCLK = PREDIV1 * 9 = 72 MHz
	RCC->CFGR |= 	RCC_CFGR_PLLSRC |			// PLL1 source - PREDIV1
					RCC_CFGR_PLLMULL9;			// PLL1 multiplier = 9

#elif CLOCK_HSE == MHz(12)		// CSAT board

	// PLL2 off
	// PREDIV1 configuration: PREDIV1CLK = HSE / 1 = 12 MHz
	RCC->CFGR2 =	RCC_CFGR2_PREDIV1SRC_HSE |		// PLL1 prediv source = HSE
					RCC_CFGR2_PREDIV1_DIV1;			// PLL1 divisor = 1

	// PLL1 configuration: PLLCLK = PREDIV1 * 6 = 72 MHz
	RCC->CFGR |= 	RCC_CFGR_PLLSRC |			// PLL1 source - PREDIV1
					RCC_CFGR_PLLMULL6;			// PLL1 multiplier = 6

#else // CLOCK_HSE == MHz(xx)
	#error Unknown PLL configuration!
#endif

	// Enable PLL1
	RCC->CR |= RCC_CR_PLLON;
	// Wait till PLL is ready
	while (! (RCC->CR & RCC_CR_PLLRDY))
		;

	// Select PLL as system clock source
	RCC->CFGR |= RCC_CFGR_SW_PLL;
	// Wait till PLL is used as system clock source
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
		;


	// Configure USB clock ------------------------------------------------------
	// USBclk = PLL1clk * 2 (fixed) / 3 = 48 MHz
	// divisor=3 -> OTGFSPRE bit at RCC->CFGR = 0, nothing to do
}




static inline void init_prescalers (void)
{
	#if (CLOCK_AHB  != 36000000) || \
		(CLOCK_APB1 != 36000000) || \
		(CLOCK_APB2 != 9000000)
		#error "Incorrect prescaler!"
	#endif

	RCC->CFGR =		RCC_CFGR_HPRE_DIV2 |		// HCLK = SYSCLK/2
					RCC_CFGR_PPRE2_DIV4 |		// PCLK2 = HCLK/4,	APB2 will be 9 MHz
					RCC_CFGR_PPRE1_DIV1;		// PCLK1 = HCLK/1,	APB1 will be 36 MHz
												// ADC clock = default, PCLK2/2 = 4.5 MHz

}



extern "C" int __low_level_init();
extern "C" __vector_table_t __vector_table;

int __low_level_init()
{
#if BOARD == BOARD_CSAT
	// workaround: set vector table offset
	SCB->VTOR = (uint32_t)&__vector_table;
#endif

	// RCC system reset(for debug purpose)
	// Set HSION bit
	RCC->CR |= RCC_CR_HSION;
	// Reset SW[1:0], HPRE[3:0], PPRE1[2:0], PPRE2[2:0], ADCPRE[1:0] and MCO[2:0] bits
	RCC->CFGR &= (uint32_t)0xF0FF0000;
	// Reset HSEON, CSSON and PLLON bits
	RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLLON);
	// Reset HSEBYP bit
	RCC->CR &= ~RCC_CR_HSEBYP;
	// Reset PLLSRC, PLLXTPRE, PLLMUL[3:0] and USBPRE bits
	RCC->CFGR &= (uint32_t)0xFF80FFFF;

	// Reset PLL2ON and PLL3ON bits
	RCC->CR &= (uint32_t)0xEBFFFFFF;

	// Disable all interrupts and clear pending bits
	RCC->CIR = 0x00FF0000;

	// Reset CFGR2 register
	RCC->CFGR2 = 0x00000000;

	// Flash: prefetch enabled, 2 wait state
	FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_1;

	// peripheral prescalers
	init_prescalers ();

	init_clocks();

	// enable IOPx peripheral clocks
	RCC->APB2ENR |=
			RCC_APB2ENR_IOPAEN |
			RCC_APB2ENR_IOPBEN |
			RCC_APB2ENR_IOPCEN |
			RCC_APB2ENR_IOPDEN |
#ifdef RCC_APB2ENR_IOPEEN
			RCC_APB2ENR_IOPEEN |
#endif
			RCC_APB2ENR_AFIOEN;

//	NVIC_SetPriorityGrouping(7);	// no preemption, 4 bit of subprio
	NVIC_SetPriorityGrouping(6);	// 1 bit preemption, 3 bit of subprio
//	NVIC_SetPriorityGrouping(5);	// 2 bit preemption, 2 bit of subprio
//	NVIC_SetPriorityGrouping(4);	// 3 bit preemption, 1 bit of subprio
//	NVIC_SetPriorityGrouping(3);	// 4 bit preemption, 0 bit of subprio

	return 1;
}

