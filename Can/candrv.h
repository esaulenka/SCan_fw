#pragma once

#include "stm32.h"
#include "can.h"


inline constexpr uint32_t can_btr(uint32_t prescaler, uint32_t seg1, uint32_t seg2, uint32_t sync_jump)
{
	uint32_t SJW = (sync_jump - 1) & 0x03;	// bits 24..25
	uint32_t TSEG2 = (seg2 - 1) & 0x07;		// bits 20..22
	uint32_t TSEG1 = (seg1 - 1) & 0x0F;		// bits 16..19
	uint32_t BRP = (prescaler - 1) & 0x3FF;	// bits 0..9
	return (SJW << 24) | (TSEG2 << 20) | (TSEG1 << 16) | BRP;
}


class CanDrv
{

public:
	// значения регистра BTR
/*	enum TCanBaudrate {		// частота APB1 = 9 MHz
		Baudrate500	= can_btr(1,  11, 6, 4),
		Baudrate250	= can_btr(2,  11, 6, 4),
		Baudrate125	= can_btr(4,  11, 6, 4),
		Baudrate100	= can_btr(5,  11, 6, 4),
		Baudrate83	= can_btr(6,  11, 6, 4),	// 83.333
		Baudrate50	= can_btr(10, 11, 6, 4),
		Baudrate33	= can_btr(15, 11, 6, 4),
		Baudrate20	= can_btr(18, 16, 8, 4),
		Baudrate15	= can_btr(24, 16, 8, 4),
		Baudrate10	= can_btr(36, 16, 8, 4),
	};*/
	enum TCanBaudrate {		// частота APB1 = 36 MHz
		Baudrate1000	= can_btr(  2, 11, 6, 4),	// 18 tq
		Baudrate800		= can_btr(  3,  9, 5, 3),	// 15 tq
		Baudrate500		= can_btr(  4, 11, 6, 4),	// 18 tq
		Baudrate400		= can_btr(  5, 11, 6, 4),	// 18 tq
		Baudrate250		= can_btr(  8, 11, 6, 4),	// 18 tq
		Baudrate125		= can_btr( 16, 11, 6, 4),	// 18 tq
		Baudrate100		= can_btr( 20, 11, 6, 4),	// 18 tq
		Baudrate95_2	= can_btr( 21, 11, 6, 4),	// 18 tq 95.238
		Baudrate83_3	= can_btr( 24, 11, 6, 4),	// 18 tq 83.333
		Baudrate62_5	= can_btr( 32, 11, 6, 4),	// 18 tq 62.500
		Baudrate50		= can_btr( 40, 11, 6, 4),	// 18 tq
		Baudrate33_3	= can_btr( 60, 11, 6, 4),	// 18 tq 33.333
		Baudrate20		= can_btr(100, 11, 6, 4),	// 18 tq
		Baudrate15		= can_btr(160,  9, 5, 3),	// 15 tq
		Baudrate10		= can_btr(200, 11, 6, 4),	// 18 tq
	};
	static uint32_t init (Can::Channel channel, uint32_t baudrate, bool silent);
	static uint32_t setFilter (Can::Channel channel, const Can::Filter * filters);

	static void deinit (Can::Channel channel);

	static uint32_t send (Can::Channel channel, const Can::Pkt &pkt);

	static Can::Pkt rcvIrq (Can::Channel channel);

	static bool isTxEmpty (Can::Channel channel);


	static void sleep (Can::Channel channel, bool sleep);



private:
	static inline CAN_TypeDef * id2hw (Can::Channel id)
	{	return (id == Can::CANch1) ? CAN1 : CAN2;		}

	static inline Can::Channel hw2id (CAN_TypeDef * regs)
	{	return (regs == CAN1) ? Can::CANch1 : Can::CANch2;	}

};



