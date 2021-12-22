#pragma once
#include <cstdint>
#include "Can/Can.h"
#include "Buffer.h"
#include "timer.h"


class CanHackerBinary
{
public:
	CanHackerBinary() {}

	// вызывается из main'а
	bool processCmd();
	bool processPackets();


	// вызывается из прерывания CAN
	void packetReceived(Can::Channel channel, const Can::Pkt & packet);

	bool gateEnabled(Can::Channel sourceCh, uint32_t pktId) const
	{
		(void)pktId;
		return canSettings[sourceCh].gate;
			//&& canSettings[sourceCh].gateFilter != pktId;
	}

private:

	void parse();

	void send(uint8_t command, uint8_t channel, const void * buf, std::size_t bufLen);

	void send(uint8_t command)
	{	send(command, 0, nullptr, 0);	}

	bool setMode();				// set mode (CAN / LIN / CAN+LIN)
	bool canSetup();			// setup channel
	bool canOpen();				// open channel
	bool canFilter(bool en);	// set filter
	bool canGate(bool en);		// set gate
	bool canSend();				// transmit packet
	bool canClose();			// close channel

	bool checkLicense();

	void applyFilters(Can::Channel ch);

	struct CanSettings {
		bool open = false;
		bool silent = false;
		bool gate = false;
		//uint32_t gateFilter = -1;
		uint32_t baudrate = 0;
		struct {
			bool extid;
			uint32_t id;
			uint32_t mask;
		} filters[14] = {};
	} canSettings[2];

	struct TCanPktExt : Can::Pkt {
		TCanPktExt() {}
		TCanPktExt(const Can::Pkt& pkt, uint32_t timestamp=0):
			Can::Pkt(pkt), timestamp(timestamp) {}

		uint32_t timestamp;
	};

	CircularBuffer<TCanPktExt, 32> canPkt[2];

	static const Can::Filter canFilterEverything[];


	struct Cmd {
		uint8_t data[32];
		uint32_t idx = 0;
		Timer lastRx;

		uint8_t maskCh1 = 0, maskCh2 = 0, maskLin = 0;

		uint8_t Command()	 const { return data[0]; }
		uint8_t Counter()	 const { return data[1]; }
		uint8_t Channel()	 const { return data[2]; }
		uint8_t DataLen1()	 const { return data[3]; }
		uint8_t Data1(int i) const { return data[4 + i]; }
		uint8_t DataLen2()	 const { return data[5]; }		// send/receive
		uint8_t Data2(int i) const { return data[6 + i]; }	// send/receive

		bool ChCan1() const { return maskCh1 && (data[2] & maskCh1) == maskCh1; }
		bool ChCan2() const { return maskCh2 && (data[2] & maskCh2) == maskCh2; }
		bool ChLin()  const { return maskLin && (data[2] & maskLin) == maskLin; }

		const uint8_t & operator[](std::size_t i) const
		{	return data[i]; }

		void push(uint8_t b) {
			data[idx++] = b;
			idx %= sizeof(data);
		}
		bool complete() const;
		void flush()
		{	idx = 0;	}
	} cmd;

	uint8_t txCounter = 0;

};

#if PROTOCOL == PROTOCOL_BINARY
extern CanHackerBinary canHacker;
#endif
