/*
 * SysClock.h
 *
 *  Created on: 08 апр. 2015 г.
 *      Author: esaulenko
 */

#pragma once

#define MHz(val)			((val) * 1000 * 1000)

#if BOARD == BOARD_CSAT
	// частота основного кварца
	#define CLOCK_HSE			MHz(12)
#else
	// частота основного кварца
	#define CLOCK_HSE			MHz(16)
#endif


// частота ядра
#define CLOCK_SYSCLK		MHz(72)

// частота шины AHB (USB)
#define CLOCK_AHB			(CLOCK_SYSCLK/2)	// должно быть не меньше 14.2 МГц, см. errata

// частота шины APB1 (USART2,3,4,5, SPI2,3, CAN, TIM2,3,4,5,6,7, DAC)
#define CLOCK_APB1			(CLOCK_AHB/1)


// частота шины APB2 (GPIO, USART1, TIM1, SPI1, ADC)
#define CLOCK_APB2			(CLOCK_AHB/4)



// на таймеры приходит частота в два раза больше
// частота шины APB1 (TIM2,3,4,5,6,7)
#define CLOCK_APB1_TIM		((CLOCK_APB1 == CLOCK_AHB) ? CLOCK_APB1 : (CLOCK_APB1 * 2))
// частота шины APB2 (TIM1)
#define CLOCK_APB2_TIM		((CLOCK_APB2 == CLOCK_AHB) ? CLOCK_APB2 : (CLOCK_APB2 * 2))




