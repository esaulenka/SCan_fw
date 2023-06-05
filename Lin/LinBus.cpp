#include "LinBus.h"
#include "LinDrv.h"
#include "Debug.h"
#include "CanHackerBinary.h"
#include <cstring>
#include <array>

#if LIN_BUS_SUPPORTED

extern LinDrv lin1drv, lin2drv;

LinBus linBus1(lin1drv);
LinBus linBus2(lin2drv);

bool LinBus::init(uint32_t baudrate, uint32_t interByteDelay)
{
	LinDrv::LinInit params = {
		.baudrate = baudrate,
		.stopBits = 1,
		.parity = 'N',
		.linMode = true,
	};
	driver.init(params);

	driver.sleep(false);

	rxTimeout = (interByteDelay + 999) / 1000;	// in milliseconds

	return true;
}

bool LinBus::deinit()
{
	driver.sleep(true);
	driver.deinit();
	return true;
}


// receiver
void LinBus::rxData(uint8_t data)
{
	rxBuf.rxTimer.restart();
	if (rxBuf.state == LinBuf::Break)
	{
		if (data == 0x55)
		{
			rxBuf.state = LinBuf::Data;
		}
		else
		{
			DBG("Lin RX: sync not received\n");
			rxBuf = LinBuf();
		}
		return;
	}

	if (rxBuf.state != LinBuf::Data)
		DBG("Lin RX: state=%d received %02X\n", rxBuf.state, data);
	if (rxBuf.dataIdx < std::size(rxBuf.data))
		rxBuf.data[rxBuf.dataIdx++] = data;

	if (rxBuf.dataIdx == std::size(rxBuf.data))
		rxComplete();
}
void LinBus::rxBreak()
{
	// try to read previous packet
	rxComplete();

	rxBuf.rxTimer.restart();
	rxBuf = LinBuf();
	rxBuf.state = LinBuf::Break;
}
void LinBus::rxError()
{
	// workaround: usart reacts on received Break with Frame Error and Break flags
	rxComplete();

	rxBuf.rxTimer.restart();
	rxBuf = LinBuf();
}

void LinBus::rxComplete()
{
	DBG("Lin RX: complete!\n");

	if (rxBuf.state != LinBuf::Data ||
		rxBuf.dataIdx < 1 ||
		rxBuf.dataIdx > std::size(rxBuf.data))
	{
		//DBG("Lin RX invalid buffer state!\n");
		return;
	}

	Lin::Pkt pkt;
	pkt.data_len = rxBuf.dataIdx - 1;
	pkt.id = rxBuf.data[0];
	memcpy(pkt.data, rxBuf.data+1, pkt.data_len);

	canHacker.packetReceived(pkt);
	rxBuf = LinBuf();
}

void LinBus::checkTimeout()
{
	if (rxTimeout &&
		rxBuf.state != LinBuf::Idle &&
		rxBuf.rxTimer.checkTimeout(rxTimeout))
	{
		rxComplete();
	}
}


// transmitter
LinBus::LinBuf::LinBuf(const Lin::Pkt & pkt)
{
	state = LinBuf::Idle;
	dataIdx = 0;
	if (pkt.data_len > std::size(data)-1) return;
	data[0] = pkt.id;
	memcpy(data+1, pkt.data, pkt.data_len);
	dataLength = pkt.data_len + 1;
}

bool LinBus::send(const Lin::Pkt & pkt)
{
	txBuf = LinBuf(pkt);
	driver.sendBreak();
	txBuf.state = LinBuf::Sync;
	driver.startSend();
	return true;
}

int LinBus::getTxData()
{
	if (txBuf.state == LinBuf::Sync)
	{
		txBuf.state = LinBuf::Data;
		return 0x55;	// sync
	}
	if (txBuf.state == LinBuf::Data &&
		txBuf.dataIdx < txBuf.dataLength)
	{
		return txBuf.data[txBuf.dataIdx++];
	}
	return -1;	// EOF
}

#else

// dummy Lin drivers
LinBus linBus1;
LinBus linBus2;

#endif
