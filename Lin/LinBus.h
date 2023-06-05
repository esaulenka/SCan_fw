#pragma once

#include <cstdint>
#include "LinPkt.h"
#include "timer.h"


#if LIN_BUS_SUPPORTED

class LinDrv;

class LinBus
{
	LinDrv &driver;

public:
	LinBus(LinDrv &linDrv):
		driver(linDrv)
	{}

	bool init(uint32_t baudrate, uint32_t interbyteDelay);
	bool deinit();

	bool send(const Lin::Pkt &pkt);

	void checkTimeout();

private:
	friend class LinDrv;
	void rxData(uint8_t data);
	void rxBreak();
	void rxError();
	int getTxData();

	struct LinBuf {
		LinBuf() = default;
		LinBuf(const Lin::Pkt & pkt);
		enum {
			Idle,
			Break,
			Sync,
			Data,	// with ID
		} state = Idle;
		uint8_t dataIdx = 0;
		uint8_t dataLength = 0;
		uint8_t data[1+sizeof(Lin::Pkt::data)] = {};
		Timer rxTimer;
	} rxBuf, txBuf;

	uint32_t rxTimeout = 0;
	void rxComplete();
};

#else	// LIN_BUS_SUPPORTED

class LinBus
{
public:
	LinBus() {}

	bool init(uint32_t, uint32_t)	{ return true; }
	bool deinit()					{ return true; }

	bool send(const Lin::Pkt &)		{ return true; }

	void checkTimeout()				{}
};
#endif

extern LinBus linBus1, linBus2;

