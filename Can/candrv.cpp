#include "candrv.h"
#include "Debug.h"
#include "Pins.h"
#include "timer.h"
#include "stm32.h"
#include "stm32_rcc.h"
#include "canhacker.h"


// отключить авторазделение фильтров на первый и второй каналы;
// для CAN1 используются 13 фильтров, для CAN2 - 15
#define SPLIT_FILTERS_FOR_CAN_HACKER 13



uint32_t CanDrv::init (Can::Channel aChannel, uint32_t aBaudrate, bool silent)
{
	CAN_TypeDef * CANx = id2hw (aChannel);

	// enable clock, set pin mode
	// при включении CAN2 надо включать и CAN1
	if (aChannel == Can::CANch1)
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

		PeriphPwr::Enable (PeriphPwr::PwrCAN1);
		PeriphPwr::Enable (PeriphPwr::PwrCAN2);
	}

	//master reset
	CANx->MCR = CAN_MCR_RESET;

	// set initialization mode
	CANx->MCR = CAN_MCR_INRQ;

	// подождать, пока модуль поменяет режим
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
	CANx->BTR = aBaudrate | (silent ? CAN_BTR_SILM : 0);

	// enable interrupts
	CANx->IER = CAN_IER_FMPIE0;		// rx fifo pending

	// enable interrupts vectors
	if (aChannel == Can::CANch1)
		NVIC_EnableIRQ (CAN1_RX0_IRQn);
	else
		NVIC_EnableIRQ (CAN2_RX0_IRQn);


	// выставляем настройки фильтров

	if (aChannel == Can::CANch1)
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


uint32_t CanDrv::setFilter (Can::Channel aChannel, const uint64_t * apFilters)
{
	PeriphBit<CAN1_BASE + offsetof(CAN_TypeDef, FMR), 0>		CAN_FilterInit;

	uint32_t res = 0;
	uint32_t filter_offset;

	if (aChannel == Can::CANch1)
		filter_offset = 0;
	else
		filter_offset = (CAN1->FMR & CAN_FMR_CAN2SB) >> 8;

	CAN_FilterInit = 1;

	uint32_t filter_en_msk = 0;		// биты включения фильтров
	uint32_t filter_list_msk = 0;	// биты режима фильтров: 1 - список / 0 - маска

	// заполняем структуру с фильтрами
	for (int i = 0; ; i++)
	{
		// биты 31..1, 63..33 - идентификатор
		// бит 32 - признак списка ID (не маски)
		// бит 0 - признак наличия самого фильтра

		const uint32_t filterNo = filter_offset + i;

		// список закончился
		if (! (apFilters[i] & 0x01))
			break;

		// ошибка! закончилось место!!
		if (filterNo >= 28)
		{
			res = 1;
			break;
		}

		CAN1->sFilterRegister[filterNo].FR1 = (apFilters[i] >> 32  ) & 0xFFFFFFFE;
		CAN1->sFilterRegister[filterNo].FR2 = (apFilters[i] 		) & 0xFFFFFFFE;

		filter_en_msk |= (1u << filterNo);

		if (apFilters[i] & (1ULL << 32))
			filter_list_msk |= (1u << filterNo);
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
	if (aChannel == Can::CANch1)
		CAN1->FMR = (filter_offset << 8) | CAN_FMR_FINIT;
#endif

	CAN_FilterInit = 0;
	return res;
}


// отключает шину.
// ВАЖНО! при отключении CAN1 остановится и CAN2!!
void CanDrv::deinit (Can::Channel aChannel)
{
	if (aChannel == Can::CANch1)
	{
		PeriphPwr::Disable (PeriphPwr::PwrCAN1);
		PinCan1Stdby::Off ();

		PinCan1Rx::Mode(ANALOGINPUT);
		PinCan1Tx::Mode(ANALOGINPUT);
	}
	else
	{
		PeriphPwr::Disable (PeriphPwr::PwrCAN1);
		PinCan2Stdby::Off ();

		PinCan2Rx::Mode(ANALOGINPUT);
		PinCan2Tx::Mode(ANALOGINPUT);
	}

}

/*
// перевести трансиверы в спящий режим. Позже драйвер может перевести их в нужное состояние.
void CanDrv::setTransceiverStandby()
{
	CAN1_En::Mode(OUTPUT);
	CAN2_En::Mode(OUTPUT);

	CAN1_En::Off();
	CAN2_En::Off();
}
*/

uint32_t CanDrv::send (Can::Channel aChannel, const Can::Pkt &pkt)
{
	CAN_TypeDef * CANx = id2hw (aChannel);

	// проверяем, есть ли свободные mailbox'ы
	if (! (CANx->TSR & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2)))
		return 1;

	// получаем номер mailbox'а
	const uint32_t mailbox_num = (CANx->TSR >> 24) & 0x03;
	CAN_TxMailBox_TypeDef * tx = & CANx->sTxMailBox[mailbox_num];

	// заполняем
	tx->TDLR =  __UNALIGNED_UINT32_READ(&pkt.data[0]);
	tx->TDHR =  __UNALIGNED_UINT32_READ(&pkt.data[4]);

	tx->TDTR = pkt.data_len;

	// заполняем id, выставляем флаг на передачу
	uint32_t id = pkt.id;
	if (id >= 0x800)		// exteneded id
		id = (id << 3) | CAN_TI0R_IDE;
	else					// standard id
		id = (id << 21);
	tx->TIR = id | CAN_TI0R_TXRQ;

	return 0;
}



Can::Pkt CanDrv::rcvIrq(Can::Channel aChannel)
{
	Can::Pkt pkt;

	CAN_TypeDef * CANx = id2hw (aChannel);
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

	// осводождаем аппаратное FIFO
	CANx->RF0R = CAN_RF0R_RFOM0;

	return pkt;
}



bool CanDrv::isTxEmpty (Can::Channel aChannel)
{
	CAN_TypeDef * CANx = id2hw (aChannel);

	if ((CANx->TSR & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2)) ==
					 (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2))
		return true;
	return false;
}


void CanDrv::sleep (Can::Channel aChannel, bool aSleep)
{
	PeriphBit< CAN1_BASE + offsetof(CAN_TypeDef, MCR), CAN_MCR_SLEEP >		CAN1_MCR_Sleep;
	PeriphBit< CAN2_BASE + offsetof(CAN_TypeDef, MCR), CAN_MCR_SLEEP >		CAN2_MCR_Sleep;

	//DBG ("Can %d sleep %s\n", aChannel, aSleep ? "ON" : "OFF");

	if (aChannel == Can::CANch1)
	{
		PinCan1Stdby::On (!aSleep);
		CAN1_MCR_Sleep = aSleep;
	}
	else
	{
		PinCan1Stdby::On (!aSleep);
		CAN2_MCR_Sleep = aSleep;
	}
}


/*
CCan::TCanState CanDrv::getState (TCanChannel aChannel)
{
	CAN_TypeDef * CANx = Id2Hw (aChannel);

	// модуль в спящем режиме
	if (CANx->MCR & CAN_MCR_SLEEP)		// снимается не сразу после Sleep(false), а через несколько миллисекунд!
		return CCan::CanSleep;

	// модуль не спит, но драйверы выключены
	uint32_t drv_on = (aChannel == CANch1) ? CAN1_En::Signalled() : CAN2_En::Signalled();
	if (! drv_on)
		return CCan::CanRxOnly;

	// всё полностью включено
	return CCan::CanActive;
}
*/




extern "C" void CAN1_RX0_IRQHandler ()
{
	auto pkt = CanDrv::rcvIrq (Can::CANch1);
	canHacker.packetReceived(Can::CANch1, pkt);

	if (canHacker.gateEnabled(Can::CANch1, pkt.id))
		CanDrv::send(Can::CANch2, pkt);
}

extern "C" void CAN2_RX0_IRQHandler ()
{
	auto pkt = CanDrv::rcvIrq (Can::CANch2);
	canHacker.packetReceived(Can::CANch2, pkt);

	if (canHacker.gateEnabled(Can::CANch2, pkt.id))
		CanDrv::send(Can::CANch1, pkt);
}



