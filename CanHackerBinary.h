#pragma once
#include <cstdint>
#include "cmsis_compiler.h"
#include "Can/Can.h"
#include "Lin/LinPkt.h"
#include "Buffer.h"
#include "timer.h"


class CanHackerBinary
{
public:
	CanHackerBinary() {}

	// called from main()
	bool processCmd();
	bool processPackets();


	// IRQ can packet received
	void packetReceived(Can::Channel ch, const Can::Pkt & packet) {
		if (canSettings[ch].open)
			canPkt[ch].Put(TCanPktExt(packet, Timer::counter()));
	}
	// IRQ lin packet received
	void packetReceived(const Lin::Pkt & packet) {
		if (linSettings.open)
			linPkt.Put(TLinPktExt(packet, Timer::counter()));
	}

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

	struct LinSettings {
		bool open = false;
		uint32_t baudrate = 0;
		bool extCrc = false;
	} linSettings;

	struct TCanPktExt : Can::Pkt {
		TCanPktExt() {}
		TCanPktExt(const Can::Pkt& pkt, uint32_t timestamp=0):
			Can::Pkt(pkt), timestamp(timestamp) {}

		uint32_t timestamp;
	};
	struct TLinPktExt : Lin::Pkt {
		TLinPktExt() {}
		TLinPktExt(const Lin::Pkt& pkt, uint32_t timestamp=0):
			Lin::Pkt(pkt), timestamp(timestamp) {}

		uint32_t timestamp;
	};

	CircularBuffer<TCanPktExt, 32> canPkt[2];
	CircularBuffer<TLinPktExt, 16> linPkt;

	static constexpr Can::Filter canFilterEverything[] = {
			Can::Filter::Mask11 (0,0),
			Can::Filter::Mask29 (0,0),
			Can::Filter::End()	// end of filters mask
	};


	struct Cmd {
		uint8_t data[64];
		uint32_t idx = 0;
		Timer lastRx;

		uint8_t maskCh1 = 0, maskCh2 = 0, maskLin = 0;

		uint8_t Command()		const { return data[0]; }
		uint8_t Counter()		const { return data[1]; }
		uint8_t Channel()		const { return data[2]; }
		uint8_t DataLen1()		const { return data[3]; }
		uint8_t Data1(int i)	const { return data[4 + i]; }
		uint32_t Data1dw(int i)	const { return __UNALIGNED_UINT32(data + 4 + i*4); }
		uint8_t DataLen2()		const { return data[4]; }		// send/receive
		uint8_t Data2(int i)	const { return data[6 + i]; }	// send/receive
		uint32_t Data2dw(int i)	const { return __UNALIGNED_UINT32(data + 6 + i*4); }	// send/receive

		bool ChCan1() const { return (data[2] & 0xF0) == maskCh1; }
		bool ChCan2() const { return (data[2] & 0xF0) == maskCh2; }
		bool ChLin()  const { return (data[2] & 0xF0) == maskLin; }

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
