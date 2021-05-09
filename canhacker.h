#pragma once
#include <cstdint>
#include "Can/Can.h"
#include "Buffer.h"


class CanHacker
{
public:
	CanHacker() {}

	// вызывается из main'а
	bool processCmd();
	bool processPackets();


	// вызывается из прерывания CAN
	void packetReceived(Can::Channel channel, const Can::Pkt & packet);

	bool gateEnabled(Can::Channel sourceCh, uint32_t pktId) const
	{
		return canSettings[sourceCh].gate &&
			   canSettings[sourceCh].gateFilter != pktId;
	}

private:

	void parse();

	int parseDecimal(const char * str, uint32_t len);
	int parseHex(const char * str, uint32_t len);

	void makeHex(char * buf, uint32_t value, uint32_t bufLen);
	void makeHex2(char * buf, uint32_t value);	// make byte
	uint8_t makeHex(uint32_t value);	// make half-byte


	void testPin();	// debug function

	bool canSpeed(Can::Channel channel, char baudrate);
	bool canOpen(Can::Channel channel, bool silent);
	bool canClose(Can::Channel channel);
	bool canGate(Can::Channel channel, bool enable);
	bool canGateBlock(uint8_t channel);
	bool canSend(Can::Channel channel, bool id29bit);
	bool canSetFilter(bool mask);

	struct {
		bool open = false;
		bool silent = false;
		bool gate = false;
		uint32_t gateFilter = -1;
		uint32_t baudrate = 0;
		struct {
			uint32_t id;
			uint32_t mask;
		} filters[15] = {};
	} canSettings[2];


	struct TCanPktExt : Can::Pkt {
		TCanPktExt() {}
		TCanPktExt(const Can::Pkt& pkt, uint32_t timestamp=0):
			Can::Pkt(pkt), timestamp(timestamp) {}

		uint32_t timestamp;
	};

	CircularBuffer<TCanPktExt, 32> canPkt[2];

	static const Can::Filter canFilterEverything[];



	struct {
		char data[32];
		uint32_t idx = 0;

		void push(char b) {
			if (b < '0' || b > 'z') return;
			data[idx++] = b;
			idx %= sizeof(data);
		}
		bool complete(char b) const {
			if (idx < 1) return false;
			return (b == '\r') || (b == '\n');
		}
	} cmd;

};

#if PROTOCOL == PROTOCOL_LAWICEL
	extern CanHacker canHacker;
#endif
