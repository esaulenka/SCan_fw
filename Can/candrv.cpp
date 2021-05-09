#include "candrv.h"
#include "Debug.h"
#include "Pins.h"
#include "timer.h"
#include "stm32.h"
#include "stm32_rcc.h"
#include "canhacker.h"
#include "CanHackerBinary.h"

// switch off automatic filters distributions between channels
// CanHacker uses 13 filters for CAN1 and 15 - for CAN2
#define SPLIT_FILTERS_FOR_CAN_HACKER 13



uint32_t CanDrv::init (Can::Channel channel, uint32_t baudrate, bool silent)
{
	CAN_TypeDef * CANx = id2hw (channel);

	// enable clock, set pin mode
	if (channel == Can::CANch1)
	{
		PinCan1Rx::Mode(INPUTPULLED); PinCan1Rx::PullUp();
		PinCan1Tx::Mode(ALT_OUTPUT);
		PinCan1Stdby::Mode(OUTPUT_2MHZ); PinCan1Stdby::On();

		PeriphPwr::Enable (PeriphPwr::PwrCAN1);
	}
	else
	{
		PinCan2Rx::Mode(INPUTPULLED); PinCan2Rx::PullUp();
		PinCan2Tx::Mode(ALT_OUTPUT);
		PinCan2Stdby::Mode(OUTPUT_2MHZ); PinCan2Stdby::On();

		// CAN2 should include CAN1
		PeriphPwr::Enable (PeriphPwr::PwrCAN1);
		PeriphPwr::Enable (PeriphPwr::PwrCAN2);
	}

	//master reset
	CANx->MCR = CAN_MCR_RESET;

	// set initialization mode
	CANx->MCR = CAN_MCR_INRQ;

	// wait while mode changes
	Timer tmr;
	while (1)
	{
		if (CANx->MSR & CAN_MSR_INAK) break;
		if (tmr.checkTimeout (1000))
		{
			DBG ("CAN enter init error!\n");
			return 1;
		}
		//OS::sleep(1);
	}

	// set bit timings
	// enable silent mode
	CANx->BTR = baudrate | (silent ? CAN_BTR_SILM : 0);

	// enable interrupts
	CANx->IER = CAN_IER_FMPIE0;		// rx fifo pending

	// enable interrupts vectors
	if (channel == Can::CANch1)
		NVIC_EnableIRQ (CAN1_RX0_IRQn);
	else
		NVIC_EnableIRQ (CAN2_RX0_IRQn);


	// setup filters

	if (channel == Can::CANch1)
	{
		PeriphBit<CAN1_BASE + offsetof(CAN_TypeDef, FMR), 0>		CAN_FilterInit;

		CAN_FilterInit = 1;
		CAN1->FA1R = 0;					// disable filters

		CAN1->FM1R = 0x0FFFFFFF;		// all filters - in list mode
		CAN1->FS1R = 0x0FFFFFFF;		// all filters - 32-bit scale
		CAN1->FFA1R = 0;				// all filters - to FIFO 0

#if defined SPLIT_FILTERS_FOR_CAN_HACKER
		// set filters offset for channel 2
		CAN1->FMR = (SPLIT_FILTERS_FOR_CAN_HACKER << 8) | CAN_FMR_FINIT;
#endif

		CAN_FilterInit = 0;
	}


	// exit initialization mode
	// возвращаться из bus-off при отсутствии ошибок на шине
	// включить wakeup при внешней активности
	// порядок передачи - в порядке записи в mailbox'ы
	CANx->MCR = CAN_MCR_INRQ*0 |
				CAN_MCR_ABOM |
				CAN_MCR_AWUM |
				CAN_MCR_TXFP;

	// подождать, пока модуль поменяет режим
	tmr.restart ();
	while (1)
	{
		if (! (CANx->MSR & CAN_MSR_INAK)) break;
		if (tmr.checkTimeout (5000))
		{
			DBG ("CAN exit init error!\n");
			return 1;
		}
		//OS::sleep(1);
	}

	return 0;
}


uint32_t CanDrv::setFilter (Can::Channel channel, const Can::Filter *filters)
{
	PeriphBit<CAN1_BASE + offsetof(CAN_TypeDef, FMR), 0>		CAN_FilterInit;

	uint32_t res = 0;
	uint32_t filter_offset;

	if (channel == Can::CANch1)
		filter_offset = 0;
	else
		filter_offset = (CAN1->FMR & CAN_FMR_CAN2SB) >> 8;

	CAN_FilterInit = 1;

	uint32_t filter_en_msk = 0;		// filter enable reg
	uint32_t filter_list_msk = 0;	// filter mode reg: 1 - list / 0 - mask

	// fill filter registers
	for (int i = 0; ; i++, filter_offset++)
	{
		// bits 31..1, 63..33 - identeficator
		// bit 32 - list mode (not mask)
		// bit 0 - filter presence

		// list fully parsed
		if (! (filters[i].val & 0x01))
			break;

		// error! not enought space!
		if (filter_offset >= 28)
		{
			res = 1;
			break;
		}

		CAN1->sFilterRegister[filter_offset].FR1 = (filters[i].val >> 32) & 0xFFFFFFFE;
		CAN1->sFilterRegister[filter_offset].FR2 = (filters[i].val		) & 0xFFFFFFFE;

		filter_en_msk |= (1u << filter_offset);

		if (filters[i].val & (1ULL << 32))
			filter_list_msk |= (1u << filter_offset);
	}


	if (! res)
	{
		// enable filters
		CAN1->FA1R |= filter_en_msk;

		// set list/mask mode
		CAN1->FM1R = (CAN1->FM1R & ~filter_en_msk) | filter_list_msk;
	}

#if not defined SPLIT_FILTERS_FOR_CAN_HACKER
	// set filters offset for channel 2
	if (channel == Can::CANch1)
		CAN1->FMR = (filter_offset << 8) | CAN_FMR_FINIT;
#endif

	CAN_FilterInit = 0;
	return res;
}


// turn off bus
// NOTE! Swithing off CAN1 also rely on CAN2!!
void CanDrv::deinit (Can::Channel channel)
{
	if (channel == Can::CANch1)
	{
		PeriphPwr::Disable (PeriphPwr::PwrCAN1);
		PinCan1Stdby::Off ();

		PinCan1Rx::Mode(ANALOGINPUT);
		PinCan1Tx::Mode(ANALOGINPUT);
	}
	else
	{
		PeriphPwr::Disable (PeriphPwr::PwrCAN2);
		PinCan2Stdby::Off ();

		PinCan2Rx::Mode(ANALOGINPUT);
		PinCan2Tx::Mode(ANALOGINPUT);
	}

}


uint32_t CanDrv::send (Can::Channel channel, const Can::Pkt &pkt)
{
	CAN_TypeDef * CANx = id2hw (channel);

	// check free mailboxes
	if (! (CANx->TSR & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2)))
		return 1;

	// get mailbox number
	const uint32_t mailbox_num = (CANx->TSR >> 24) & 0x03;
	CAN_TxMailBox_TypeDef * tx = & CANx->sTxMailBox[mailbox_num];

	// fill data
	tx->TDLR =  __UNALIGNED_UINT32_READ(&pkt.data[0]);
	tx->TDHR =  __UNALIGNED_UINT32_READ(&pkt.data[4]);

	tx->TDTR = pkt.data_len;

	// fill id, turn on transmit
	uint32_t id = pkt.id;
	if (id >= 0x800)		// exteneded id
		id = (id << 3) | CAN_TI0R_IDE;
	else					// standard id
		id = (id << 21);
	tx->TIR = id | CAN_TI0R_TXRQ;

	return 0;
}



Can::Pkt CanDrv::rcvIrq(Can::Channel channel)
{
	Can::Pkt pkt;

	CAN_TypeDef * CANx = id2hw (channel);
	CAN_FIFOMailBox_TypeDef * rx = & CANx->sFIFOMailBox[0];

	if (! (CANx->RF0R & CAN_RF0R_FMP0))
	{
		DBG ("CAN: error, no data received\n");
		return pkt;
	}

	pkt.id = rx->RIR;
	if (pkt.id & CAN_RI0R_IDE)	// extended ID
		pkt.id = pkt.id >> 3;
	else						// standard ID
		pkt.id = pkt.id >> 21;
	__UNALIGNED_UINT32_WRITE(&pkt.data[0], rx->RDLR);
	__UNALIGNED_UINT32_WRITE(&pkt.data[4], rx->RDHR);
	pkt.data_len = rx->RDTR & 0x0F;

	// free hw FIFO
	CANx->RF0R = CAN_RF0R_RFOM0;

	return pkt;
}



bool CanDrv::isTxEmpty (Can::Channel channel)
{
	CAN_TypeDef * CANx = id2hw (channel);

	if ((CANx->TSR & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2)) ==
					 (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2))
		return true;
	return false;
}


void CanDrv::sleep (Can::Channel channel, bool sleep)
{
	PeriphBit< CAN1_BASE + offsetof(CAN_TypeDef, MCR), CAN_MCR_SLEEP >		CAN1_MCR_Sleep;
	PeriphBit< CAN2_BASE + offsetof(CAN_TypeDef, MCR), CAN_MCR_SLEEP >		CAN2_MCR_Sleep;

	//DBG ("Can %d sleep %s\n", channel, sleep ? "ON" : "OFF");

	if (channel == Can::CANch1)
	{
		PinCan1Stdby::On (sleep);
		CAN1_MCR_Sleep = sleep;
	}
	else
	{
		PinCan2Stdby::On (sleep);
		CAN2_MCR_Sleep = sleep;
	}
}





extern "C" {

void CAN1_RX0_IRQHandler ()
{
	auto pkt = CanDrv::rcvIrq (Can::CANch1);
	canHacker.packetReceived(Can::CANch1, pkt);

	if (canHacker.gateEnabled(Can::CANch1, pkt.id))
		CanDrv::send(Can::CANch2, pkt);
}

void CAN2_RX0_IRQHandler ()
{
	auto pkt = CanDrv::rcvIrq (Can::CANch2);
	canHacker.packetReceived(Can::CANch2, pkt);

	if (canHacker.gateEnabled(Can::CANch2, pkt.id))
		CanDrv::send(Can::CANch1, pkt);
}

};	// extern "C"

