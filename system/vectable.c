//******************************************************************************
//*
//*      STM32F10X_CL vector table
//*
//*      Version 1.2
//*
//*      Copyright (c) 2016-2020, emb-lib Project Team
//*
//*      This file is part of the arm-none-eabi-startup project.
//*      Visit https://github.com/emb-lib/arm-none-eabi-startup for new versions.
//*
//*      Permission is hereby granted, free of charge, to any person
//*      obtaining  a copy of this software and associated documentation
//*      files (the "Software"), to deal in the Software without restriction,
//*      including without limitation the rights to use, copy, modify, merge,
//*      publish, distribute, sublicense, and/or sell copies of the Software,
//*      and to permit persons to whom the Software is furnished to do so,
//*      subject to the following conditions:
//*
//*      The above copyright notice and this permission notice shall be included
//*      in all copies or substantial portions of the Software.
//*
//*      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//*      EXPRESS  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//*      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//*      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//*      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//*      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
//*      THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//*
//------------------------------------------------------------------------------

#include "exhandler.h"

//------------------------------------------------------------------------------
__attribute__ ((used))
__attribute__ ((section(".isr_vector")))
const __vector_table_t __vector_table =
{
    __top_of_stack,
    
    {
    Reset_Handler,

    //--------------------------------------------------------------------------
    //
    // Cortex-M core exceptions 
    // 
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0,                          // Reserved
    0,                          // Reserved
    0,                          // Reserved
    0,                          // Reserved
    SVC_Handler,
    DebugMon_Handler,
    0,                          // Reserved
    PendSV_Handler,             // The OS context switch interrupt
    SysTick_Handler,            // The OS timer

    //--------------------------------------------------------------------------
    //
    // Peripheral interrupts
    // 
    WWDG_IRQHandler,             // Window Watchdog
    PVD_IRQHandler,              // PVD through EXTI Line detect
    TAMPER_IRQHandler,           // Tamper
    RTC_IRQHandler,              // RTC
    FLASH_IRQHandler,            // Flash
    RCC_IRQHandler,              // RCC
    EXTI0_IRQHandler,            // EXTI Line 0
    EXTI1_IRQHandler,            // EXTI Line 1
    EXTI2_IRQHandler,            // EXTI Line 2
    EXTI3_IRQHandler,            // EXTI Line 3
    EXTI4_IRQHandler,            // EXTI Line 4
    DMA1_Channel1_IRQHandler,    // DMA1 Channel 1
    DMA1_Channel2_IRQHandler,    // DMA1 Channel 2
    DMA1_Channel3_IRQHandler,    // DMA1 Channel 3
    DMA1_Channel4_IRQHandler,    // DMA1 Channel 4
    DMA1_Channel5_IRQHandler,    // DMA1 Channel 5
    DMA1_Channel6_IRQHandler,    // DMA1 Channel 6
    DMA1_Channel7_IRQHandler,    // DMA1 Channel 7
    ADC1_2_IRQHandler,           // ADC1 and ADC2
    CAN1_TX_IRQHandler,          // CAN1 TX
    CAN1_RX0_IRQHandler,         // CAN1 RX0
    CAN1_RX1_IRQHandler,         // CAN1 RX1
    CAN1_SCE_IRQHandler,         // CAN1 SCE
    EXTI9_5_IRQHandler,          // EXTI Line 9..5
    TIM1_BRK_IRQHandler,         // TIM1 Break
    TIM1_UP_IRQHandler,          // TIM1 Update
    TIM1_TRG_COM_IRQHandler,     // TIM1 Trigger and Commutation
    TIM1_CC_IRQHandler,          // TIM1 Capture Compare
    TIM2_IRQHandler,             // TIM2
    TIM3_IRQHandler,             // TIM3
    TIM4_IRQHandler,             // TIM4
    I2C1_EV_IRQHandler,          // I2C1 Event
    I2C1_ER_IRQHandler,          // I2C1 Error
    I2C2_EV_IRQHandler,          // I2C2 Event
    I2C2_ER_IRQHandler,          // I2C1 Error
    SPI1_IRQHandler,             // SPI1
    SPI2_IRQHandler,             // SPI2
    USART1_IRQHandler,           // USART1
    USART2_IRQHandler,           // USART2
    USART3_IRQHandler,           // USART3
    EXTI15_10_IRQHandler,        // EXTI Line 15..10
    RTCAlarm_IRQHandler,         // RTC alarm through EXTI line
    OTG_FS_WKUP_IRQHandler,      // USB OTG FS Wakeup through EXTI line
    0,                           // Reserved
    0,                           // Reserved
    0,                           // Reserved
    0,                           // Reserved
    0,                           // Reserved
    0,                           // Reserved
    0,                           // Reserved
    TIM5_IRQHandler,             // TIM5
    SPI3_IRQHandler,             // SPI3
    UART4_IRQHandler,            // UART4
    UART5_IRQHandler,            // UART5
    TIM6_IRQHandler,             // TIM6
    TIM7_IRQHandler,             // TIM7
    DMA2_Channel1_IRQHandler,    // DMA2 Channel1
    DMA2_Channel2_IRQHandler,    // DMA2 Channel2
    DMA2_Channel3_IRQHandler,    // DMA2 Channel3
    DMA2_Channel4_IRQHandler,    // DMA2 Channel4
    DMA2_Channel5_IRQHandler,    // DMA2 Channel5
    ETH_IRQHandler,              // Ethernet
    ETH_WKUP_IRQHandler,         // Ethernet Wakeup through EXTI line
    CAN2_TX_IRQHandler,          // CAN2 TX
    CAN2_RX0_IRQHandler,         // CAN2 RX0
    CAN2_RX1_IRQHandler,         // CAN2 RX1
    CAN2_SCE_IRQHandler,         // CAN2 SCE
    OTG_FS_IRQHandler            // USB OTG FS
    }
};
//------------------------------------------------------------------------------
__attribute__ ((noreturn))
static void default_handler() { for(;;) { } }
#ifndef NDEBUG
static void hf_handler()
{
    volatile int i = 0;         //  debug variable: set non-zero value to 
    while(!i) { }               //  return from handler - this figures out 
                                //  an address where HW fault raises
}
#endif // NDEBUG
//------------------------------------------------------------------------------
//
//   Default exception handlers
//
#ifdef NDEBUG

#pragma weak NMI_Handler        = default_handler
#pragma weak HardFault_Handler  = default_handler
#pragma weak MemManage_Handler  = default_handler
#pragma weak BusFault_Handler   = default_handler
#pragma weak UsageFault_Handler = default_handler
#pragma weak SVC_Handler        = default_handler
#pragma weak DebugMon_Handler   = default_handler
#pragma weak PendSV_Handler     = default_handler
#pragma weak SysTick_Handler    = default_handler

#else // NDEBUG

WEAK void NMI_Handler        ()  { default_handler(); }
WEAK void HardFault_Handler  ()  { hf_handler();      }
WEAK void MemManage_Handler  ()  { default_handler(); }
WEAK void BusFault_Handler   ()  { default_handler(); }
WEAK void UsageFault_Handler ()  { default_handler(); }
WEAK void SVC_Handler        ()  { default_handler(); }
WEAK void DebugMon_Handler   ()  { default_handler(); }
WEAK void PendSV_Handler     ()  { default_handler(); }
WEAK void SysTick_Handler    ()  { default_handler(); }

#endif // NDEBUG

//------------------------------------------------------------------------------
//
//   Default interrupt handlers
//
#ifdef NDEBUG

#pragma weak WWDG_IRQHandler          = default_handler
#pragma weak PVD_IRQHandler           = default_handler
#pragma weak TAMPER_IRQHandler        = default_handler
#pragma weak RTC_IRQHandler           = default_handler
#pragma weak FLASH_IRQHandler         = default_handler
#pragma weak RCC_IRQHandler           = default_handler
#pragma weak EXTI0_IRQHandler         = default_handler
#pragma weak EXTI1_IRQHandler         = default_handler
#pragma weak EXTI2_IRQHandler         = default_handler
#pragma weak EXTI3_IRQHandler         = default_handler
#pragma weak EXTI4_IRQHandler         = default_handler
#pragma weak DMA1_Channel1_IRQHandler = default_handler
#pragma weak DMA1_Channel2_IRQHandler = default_handler
#pragma weak DMA1_Channel3_IRQHandler = default_handler
#pragma weak DMA1_Channel4_IRQHandler = default_handler
#pragma weak DMA1_Channel5_IRQHandler = default_handler
#pragma weak DMA1_Channel6_IRQHandler = default_handler
#pragma weak DMA1_Channel7_IRQHandler = default_handler
#pragma weak ADC1_2_IRQHandler        = default_handler
#pragma weak CAN1_TX_IRQHandler       = default_handler
#pragma weak CAN1_RX0_IRQHandler      = default_handler
#pragma weak CAN1_RX1_IRQHandler      = default_handler
#pragma weak CAN1_SCE_IRQHandler      = default_handler
#pragma weak EXTI9_5_IRQHandler       = default_handler
#pragma weak TIM1_BRK_IRQHandler      = default_handler
#pragma weak TIM1_UP_IRQHandler       = default_handler
#pragma weak TIM1_TRG_COM_IRQHandler  = default_handler
#pragma weak TIM1_CC_IRQHandler       = default_handler
#pragma weak TIM2_IRQHandler          = default_handler
#pragma weak TIM3_IRQHandler          = default_handler
#pragma weak TIM4_IRQHandler          = default_handler
#pragma weak I2C1_EV_IRQHandler       = default_handler
#pragma weak I2C1_ER_IRQHandler       = default_handler
#pragma weak I2C2_EV_IRQHandler       = default_handler
#pragma weak I2C2_ER_IRQHandler       = default_handler
#pragma weak SPI1_IRQHandler          = default_handler
#pragma weak SPI2_IRQHandler          = default_handler
#pragma weak USART1_IRQHandler        = default_handler
#pragma weak USART2_IRQHandler        = default_handler
#pragma weak USART3_IRQHandler        = default_handler
#pragma weak EXTI15_10_IRQHandler     = default_handler
#pragma weak RTCAlarm_IRQHandler      = default_handler
#pragma weak OTG_FS_WKUP_IRQHandler   = default_handler
#pragma weak TIM5_IRQHandler          = default_handler
#pragma weak SPI3_IRQHandler          = default_handler
#pragma weak UART4_IRQHandler         = default_handler
#pragma weak UART5_IRQHandler         = default_handler
#pragma weak TIM6_IRQHandler          = default_handler
#pragma weak TIM7_IRQHandler          = default_handler
#pragma weak DMA2_Channel1_IRQHandler = default_handler
#pragma weak DMA2_Channel2_IRQHandler = default_handler
#pragma weak DMA2_Channel3_IRQHandler = default_handler
#pragma weak DMA2_Channel4_IRQHandler = default_handler
#pragma weak DMA2_Channel5_IRQHandler = default_handler
#pragma weak ETH_IRQHandler           = default_handler
#pragma weak ETH_WKUP_IRQHandler      = default_handler
#pragma weak CAN2_TX_IRQHandler       = default_handler
#pragma weak CAN2_RX0_IRQHandler      = default_handler
#pragma weak CAN2_RX1_IRQHandler      = default_handler
#pragma weak CAN2_SCE_IRQHandler      = default_handler
#pragma weak OTG_FS_IRQHandler        = default_handler

#else // NDEBUG

WEAK void WWDG_IRQHandler          ()  { default_handler(); }
WEAK void PVD_IRQHandler           ()  { default_handler(); }
WEAK void TAMPER_IRQHandler        ()  { default_handler(); }
WEAK void RTC_IRQHandler           ()  { default_handler(); }
WEAK void FLASH_IRQHandler         ()  { default_handler(); }
WEAK void RCC_IRQHandler           ()  { default_handler(); }
WEAK void EXTI0_IRQHandler         ()  { default_handler(); }
WEAK void EXTI1_IRQHandler         ()  { default_handler(); }
WEAK void EXTI2_IRQHandler         ()  { default_handler(); }
WEAK void EXTI3_IRQHandler         ()  { default_handler(); }
WEAK void EXTI4_IRQHandler         ()  { default_handler(); }
WEAK void DMA1_Channel1_IRQHandler ()  { default_handler(); }
WEAK void DMA1_Channel2_IRQHandler ()  { default_handler(); }
WEAK void DMA1_Channel3_IRQHandler ()  { default_handler(); }
WEAK void DMA1_Channel4_IRQHandler ()  { default_handler(); }
WEAK void DMA1_Channel5_IRQHandler ()  { default_handler(); }
WEAK void DMA1_Channel6_IRQHandler ()  { default_handler(); }
WEAK void DMA1_Channel7_IRQHandler ()  { default_handler(); }
WEAK void ADC1_2_IRQHandler        ()  { default_handler(); }
WEAK void CAN1_TX_IRQHandler       ()  { default_handler(); }
WEAK void CAN1_RX0_IRQHandler      ()  { default_handler(); }
WEAK void CAN1_RX1_IRQHandler      ()  { default_handler(); }
WEAK void CAN1_SCE_IRQHandler      ()  { default_handler(); }
WEAK void EXTI9_5_IRQHandler       ()  { default_handler(); }
WEAK void TIM1_BRK_IRQHandler      ()  { default_handler(); }
WEAK void TIM1_UP_IRQHandler       ()  { default_handler(); }
WEAK void TIM1_TRG_COM_IRQHandler  ()  { default_handler(); }
WEAK void TIM1_CC_IRQHandler       ()  { default_handler(); }
WEAK void TIM2_IRQHandler          ()  { default_handler(); }
WEAK void TIM3_IRQHandler          ()  { default_handler(); }
WEAK void TIM4_IRQHandler          ()  { default_handler(); }
WEAK void I2C1_EV_IRQHandler       ()  { default_handler(); }
WEAK void I2C1_ER_IRQHandler       ()  { default_handler(); }
WEAK void I2C2_EV_IRQHandler       ()  { default_handler(); }
WEAK void I2C2_ER_IRQHandler       ()  { default_handler(); }
WEAK void SPI1_IRQHandler          ()  { default_handler(); }
WEAK void SPI2_IRQHandler          ()  { default_handler(); }
WEAK void USART1_IRQHandler        ()  { default_handler(); }
WEAK void USART2_IRQHandler        ()  { default_handler(); }
WEAK void USART3_IRQHandler        ()  { default_handler(); }
WEAK void EXTI15_10_IRQHandler     ()  { default_handler(); }
WEAK void RTCAlarm_IRQHandler      ()  { default_handler(); }
WEAK void OTG_FS_WKUP_IRQHandler   ()  { default_handler(); }
WEAK void TIM5_IRQHandler          ()  { default_handler(); }
WEAK void SPI3_IRQHandler          ()  { default_handler(); }
WEAK void UART4_IRQHandler         ()  { default_handler(); }
WEAK void UART5_IRQHandler         ()  { default_handler(); }
WEAK void TIM6_IRQHandler          ()  { default_handler(); }
WEAK void TIM7_IRQHandler          ()  { default_handler(); }
WEAK void DMA2_Channel1_IRQHandler ()  { default_handler(); }
WEAK void DMA2_Channel2_IRQHandler ()  { default_handler(); }
WEAK void DMA2_Channel3_IRQHandler ()  { default_handler(); }
WEAK void DMA2_Channel4_IRQHandler ()  { default_handler(); }
WEAK void DMA2_Channel5_IRQHandler ()  { default_handler(); }
WEAK void ETH_IRQHandler           ()  { default_handler(); }
WEAK void ETH_WKUP_IRQHandler      ()  { default_handler(); }
WEAK void CAN2_TX_IRQHandler       ()  { default_handler(); }
WEAK void CAN2_RX0_IRQHandler      ()  { default_handler(); }
WEAK void CAN2_RX1_IRQHandler      ()  { default_handler(); }
WEAK void CAN2_SCE_IRQHandler      ()  { default_handler(); }
WEAK void OTG_FS_IRQHandler        ()  { default_handler(); }

#endif // NDEBUG
//------------------------------------------------------------------------------
