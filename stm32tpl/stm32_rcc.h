#pragma once
#include "stm32.h"


class PeriphPwr
{
public:

	enum TPowerEn
	{
	// AHB peripheral
	PwrDMA1		= 0x000 | mask2bit(RCC_AHBENR_DMA1EN),			/*!< DMA1 clock enable */
	PwrSRAM		= 0x000 | mask2bit(RCC_AHBENR_SRAMEN),			/*!< SRAM interface clock enable */
	PwrFLITF	= 0x000 | mask2bit(RCC_AHBENR_FLITFEN),			/*!< FLITF clock enable */
	PwrCRC		= 0x000 | mask2bit(RCC_AHBENR_CRCEN),			/*!< CRC clock enable */
	PwrDMA2		= 0x000 | mask2bit(RCC_AHBENR_DMA2EN),			/*!< DMA2 clock enable */
	PwrOTGFS	= 0x000 | mask2bit(RCC_AHBENR_OTGFSEN),			/*!< USB OTG FS clock enable */

	//APB1 peripheral
	PwrTIM2		= 0x100 | mask2bit(RCC_APB1ENR_TIM2EN),			/*!< Timer 2 clock enabled*/
	PwrTIM3		= 0x100 | mask2bit(RCC_APB1ENR_TIM3EN),			/*!< Timer 3 clock enable */
	PwrWWDG		= 0x100 | mask2bit(RCC_APB1ENR_WWDGEN),			/*!< Window Watchdog clock enable */
	PwrUSART2	= 0x100 | mask2bit(RCC_APB1ENR_USART2EN),		/*!< USART 2 clock enable */
	PwrI2C1		= 0x100 | mask2bit(RCC_APB1ENR_I2C1EN),			/*!< I2C 1 clock enable */
	PwrCAN1		= 0x100 | mask2bit(RCC_APB1ENR_CAN1EN),			/*!< CAN1 clock enable */
	PwrBKP		= 0x100 | mask2bit(RCC_APB1ENR_BKPEN),			/*!< Backup interface clock enable */
	PwrPWR		= 0x100 | mask2bit(RCC_APB1ENR_PWREN),			/*!< Power interface clock enable */
	PwrTIM4		= 0x100 | mask2bit(RCC_APB1ENR_TIM4EN),			/*!< Timer 4 clock enable */
	PwrSPI2		= 0x100 | mask2bit(RCC_APB1ENR_SPI2EN),			/*!< SPI 2 clock enable */
	PwrUSART3	= 0x100 | mask2bit(RCC_APB1ENR_USART3EN),		/*!< USART 3 clock enable */
	PwrI2C2		= 0x100 | mask2bit(RCC_APB1ENR_I2C2EN),			/*!< I2C 2 clock enable */
	PwrTIM5		= 0x100 | mask2bit(RCC_APB1ENR_TIM5EN),			/*!< Timer 5 clock enable */
	PwrTIM6		= 0x100 | mask2bit(RCC_APB1ENR_TIM6EN),			/*!< Timer 6 clock enable */
	PwrTIM7		= 0x100 | mask2bit(RCC_APB1ENR_TIM7EN),			/*!< Timer 7 clock enable */
	PwrSPI3		= 0x100 | mask2bit(RCC_APB1ENR_SPI3EN),			/*!< SPI 3 clock enable */
	PwrUART4	= 0x100 | mask2bit(RCC_APB1ENR_UART4EN),		/*!< UART 4 clock enable */
	PwrUART5	= 0x100 | mask2bit(RCC_APB1ENR_UART5EN),		/*!< UART 5 clock enable */
	PwrCAN2		= 0x100 | mask2bit(RCC_APB1ENR_CAN2EN),			/*!< CAN2 clock enable */
	PwrDAC		= 0x100 | mask2bit(RCC_APB1ENR_DACEN),			/*!< DAC interface clock enable */

	// APB2 peripheral
	PwrAFIO		= 0x200 | mask2bit(RCC_APB2ENR_AFIOEN),			/*!< Alternate Function I/O clock enable */
	PwrIOPA		= 0x200 | mask2bit(RCC_APB2ENR_IOPAEN),			/*!< I/O port A clock enable */
	PwrIOPB		= 0x200 | mask2bit(RCC_APB2ENR_IOPBEN),			/*!< I/O port B clock enable */
	PwrIOPC		= 0x200 | mask2bit(RCC_APB2ENR_IOPCEN),			/*!< I/O port C clock enable */
	PwrIOPD		= 0x200 | mask2bit(RCC_APB2ENR_IOPDEN),			/*!< I/O port D clock enable */
	PwrADC1		= 0x200 | mask2bit(RCC_APB2ENR_ADC1EN),			/*!< ADC 1 interface clock enable */
	PwrADC2		= 0x200 | mask2bit(RCC_APB2ENR_ADC2EN),			/*!< ADC 2 interface clock enable */
	PwrTIM1		= 0x200 | mask2bit(RCC_APB2ENR_TIM1EN),			/*!< TIM1 Timer clock enable */
	PwrSPI1		= 0x200 | mask2bit(RCC_APB2ENR_SPI1EN),			/*!< SPI 1 clock enable */
	PwrUSART1	= 0x200 | mask2bit(RCC_APB2ENR_USART1EN),		/*!< USART1 clock enable */
	PwrIOPE		= 0x200 | mask2bit(RCC_APB2ENR_IOPEEN),			/*!< I/O port E clock enable */

	};

	static void Enable (TPowerEn periph, bool en = true)
	{
		uint32_t addr = 0;
		switch (periph >> 8)
		{
		case 0: addr = (uint32_t) &RCC->AHBENR; break;
		case 1: addr = (uint32_t) &RCC->APB1ENR; break;
		case 2: addr = (uint32_t) &RCC->APB2ENR; break;
		}
		uint32_t bb_addr = pPERIPH_BB_BASE + (addr - pPERIPH_BASE) * 32;
		uint8_t bit = periph & 0x0FF;

		((volatile uint32_t*)bb_addr)[bit] = en;
	}

	static void Disable (TPowerEn periph)
	{
		Enable (periph, false);
	}

};


