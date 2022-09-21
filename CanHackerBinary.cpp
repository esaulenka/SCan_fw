#include "CanHackerBinary.h"
#include "cdcacm.h"
#include "Can/candrv.h"
#include "Lin/LinBus.h"
#include "LedBlink.h"
#include "CHLic.h"
#include "Debug.h"
#include <cstring>
#include <array>


//static const uint8_t deviceType[] = "CH32";		// cmd 0x01
static const uint8_t deviceType[] = "CH-3.2";		// cmd 0x01
static const uint8_t softVersion[] = "0.1.21";		// cmd 0x02
static const uint8_t deviceFeatures[8] = { 0,0,0,0,0,0,0x08,0x05 };



#if PROTOCOL == PROTOCOL_BINARY
CanHackerBinary canHacker;
#endif





bool CanHackerBinary::processCmd()
{
	char rxBuf[32];
	auto rxLen = Usb::receive(rxBuf, sizeof(rxBuf));

	if (rxLen)
	{
		// check rx timeout
		if (cmd.lastRx.checkTimeout(50))
			cmd.flush();
		cmd.lastRx.restart();

//		DBG("Rx :");
//		for (auto i = 0u; i < rxLen; i++)
//			DBG(" %02X", rxBuf[i]);
//		DBG("\n");
	}

	for (auto i = 0u; i < rxLen; i++)
	{
		cmd.push(rxBuf[i]);
		if (cmd.complete())
		{
			LedBlink::pulseRx();

			parse();
			cmd.flush();
		}
	}

	return rxLen != 0;	// some data proccessed
}


bool CanHackerBinary::Cmd::complete() const
{
	if (idx < 4) return false;
	if (Command() == 0x40) 	// send/receive cmd
		return (DataLen2() + 6u == idx);
	else
		return (DataLen1() + 4u == idx);
}


void CanHackerBinary::parse()
{
#if defined DEBUG
	DBG("Cmd:");
	for (auto i = 0u; i < cmd.idx; i++)
		DBG(" %02X", cmd[i]);
	DBG("\n");
#endif

	switch (cmd.Command())
	{
	case 0xa5:		// startup handshake
		// << a5 00 a5 00
		// >> 5a 00 5a 00
		for (auto & sett : canSettings)
			sett = CanSettings();	// set initial values
		linSettings = LinSettings();
		send(0x5a, 0x5a, nullptr, 0);
		break;
	case 0x01:		// get hardware code
		// << 01 01 00 00
		// >> 01 01 00 04 43 48 33 32
		send(0x01, 0, deviceType, sizeof(deviceType)-1);
		break;
	case 0x02:	  	// get firmware version
		// << 02 02 00 00
		// >> 02 02 00 06 30 2e 31 2e 31 31
		send(0x02, 0, softVersion, sizeof(softVersion)-1);
		break;
	case 0x03:		// get serial number
	{	// << 03 04 00 00
		// >> 03 04 00 08 00 00 00 00 00 00 08 05
		send(0x03, 0, deviceFeatures, sizeof(deviceFeatures));
		break;
	}
	case 0x04:		// set device mode: 0 - 2CAN+Lin, 1 - 2CAN, 2 - LIN
		setMode();
		break;
	case 0x05:		// get hardware index (??)
	{	// << 05 01 00 00
		// >> 05 01 00 01 11
		uint8_t tmp = 0x11;
		send(0x05, 0, &tmp, sizeof(tmp));
		break;
	}
	case 0x08:		// open device
		// << 08 06 00 00
		// >> 88 06 00 00
		send(0x88);
		break;
	case 0x09:		// close device
		// << 09 12 00 00
		// >> 89 12 00 00
		send(0x89);
		break;
	//case 0x0A:	// get device load statistics (??)
	case 0x0F:		// check license
		checkLicense();
		break;
	case 0x11:		// configure channel
		// << 11 07 20 01 07
		// >> 91 07 00 00
		canSetup();
		break;
	case 0x18:		// open channel
		// << 18 08 20 00
		// >> 98 08 00 00
		canOpen();
		break;
	case 0x19:		// close channel
		// << 19 0f 20 00
		// >> 99 0f 00 00
		canClose();
		break;
	//case 0x1F:	// reset channel
	case 0x21:		// set filter
		canFilter(true);
		break;
	case 0x22:		// clear filter
		canFilter(false);
		break;
	//case 0x23:	// get filter
	case 0x31:		// gate enable
		// << 31 0e 42 00	24 = gate 1->2, 42 = gate 2->1
		// >> b1 0e 00 00
		canGate(true);
		break;
	case 0x32:		// gate disable
		canGate(false);
		break;
	//case 0x33:	// gate set filter
	//case 0x34:	// gate remove filter
	//case 0x35:	// gate remove filters
	case 0x48:		// LIN slave: set data
		// << 48 08 20 0d 00 00 00 80 08 00 00 00 00 00 00 00 00
		// >> c8 08 00 00
		// << 48 14 60 08 00 00 00 11 03 aa bb cc - pkt: id=11, data=aa bb cc
		// TODO LIN slave
		send(cmd.Command() | 0x80);
		break;
	case 0x49:		// LIN slave: enable response
		// << 49 09 20 00 - disable
		// >> c9 09 00 00
		// << 49 09 21 00 - enable
		// TODO LIN slave
		send(cmd.Command() | 0x80);
		break;
	case 0x40:		// transmit packet
		canSend();
		break;
	case 0x0a:		// session end?
		// << 0a 11 00 00
		// >> ff 11 00 00
		send(0xff);
		break;
	default:
		DBG("Unknown command %02X!\n", cmd.Command());
		break;
	}
}

bool CanHackerBinary::setMode()
{
	// << 04 03 01 00
	// >> 84 03 00 00
	switch (cmd.Channel())
	{
	case 0:		// 2CAN + LIN
		cmd.maskCh1 = 0x20;
		cmd.maskCh2 = 0x40;
		cmd.maskLin = 0x60;
		break;
	case 1:		// 2CAN
		cmd.maskCh1 = 0x20;
		cmd.maskCh2 = 0x40;
		cmd.maskLin = 0x00;
		break;
	case 2:		// LIN
		cmd.maskCh1 = 0x00;
		cmd.maskCh2 = 0x00;
		cmd.maskLin = 0x20;
		break;
	default:
		return false;
	}
	send(cmd.Command() | 0x80);		// send ack
	return true;
}

bool CanHackerBinary::canSetup()
{
	static const uint32_t canBaudrate[] = {
		CanDrv::Baudrate10,		// 0
		CanDrv::Baudrate20,		// 1
		CanDrv::Baudrate33_3,	// 2
		CanDrv::Baudrate50,		// 3
		CanDrv::Baudrate62_5,	// 4
		CanDrv::Baudrate83_3,	// 5
		CanDrv::Baudrate95_2,	// 6
		CanDrv::Baudrate100,	// 7
		CanDrv::Baudrate125,	// 8
		CanDrv::Baudrate250,	// 9
		CanDrv::Baudrate400,	// 10
		CanDrv::Baudrate500,	// 11
		CanDrv::Baudrate800,	// 12
		CanDrv::Baudrate1000,	// 13
	};
	static const uint16_t linBaudrate[] = {
		2400,		// 0
		9600,		// 1
		10400,		// 2
		14400,		// 3
		15600,		// 4
		19200,		// 5
		20000,		// 6
		38400,		// 7
	};

	const bool ch1 = cmd.ChCan1();
	const bool ch2 = cmd.ChCan2();
	const bool lin = cmd.ChLin();
	if (!ch1 && !ch2 && !lin) return false;

	if (cmd.DataLen1() != 1) return false;

	switch (cmd.Channel() & 0x0F)
	{
	case 0x0:		// set speed index
		if (ch1 || ch2)
		{
			if (cmd.Data1(0) >= std::size(canBaudrate)) return false;
			uint32_t baudrate = canBaudrate[cmd.Data1(0)];
			if (ch1) canSettings[0].baudrate = baudrate;
			if (ch2) canSettings[1].baudrate = baudrate;
		}
		if (lin)
		{
			if (cmd.Data1(0) >= std::size(linBaudrate)) return false;
			linSettings.baudrate = linBaudrate[cmd.Data1(0)];
		}
		break;
	//case 0x1:		// set FD speed index
	//case 0x2:		// set manual speed
	//case 0x3:		// set manual FD speed
	//case 0x5:		// terminator relay
	//case 0x6:		// pullup relay
	case 0x7:		// LIN CRC mode
		// 01 - crc, 02 - ecrc
		linSettings.extCrc = cmd.Data1(0) == 0x02;
		break;
	case 0x8:		// set syncronization jump width
		// SJW = cmd.Data(0) + 1;
		break;
	case 0x9:		// CAN listen mode
		if (ch1) canSettings[0].silent = cmd.Data1(0);
		if (ch2) canSettings[1].silent = cmd.Data1(0);
		break;
	//case 0xA:		// Frame format
	default: return false;
	}

	send(cmd.Command() | 0x80);		// send ack
	return true;
}

bool CanHackerBinary::canOpen()
{
	const bool ch1 = cmd.ChCan1();
	const bool ch2 = cmd.ChCan2();
	const bool lin = cmd.ChLin();
	if (!ch1 && !ch2 && !lin) return false;

	auto open = [this](Can::Channel ch, CanSettings & sett) {
		CanDrv::init(ch, sett.baudrate, sett.silent);
		CanDrv::setFilter(ch, canFilterEverything);
		sett.open = true;
	};

	if (ch1) open(Can::CANch1, canSettings[0]);
	if (ch2) open(Can::CANch2, canSettings[1]);
	if (lin)
	{
		linBus1.init(linSettings.baudrate);
		linSettings.open = true;
	}

	send(cmd.Command() | 0x80);		// send ack
	return true;
}

bool CanHackerBinary::canFilter(bool enable)
{
	const bool ch1 = cmd.ChCan1();
	const bool ch2 = cmd.ChCan2();
	if (!ch1 && !ch2) return false;

	// << 21 0d 20 09  00  00 00 01 23  00 00 07 ff // CH1, idx=0, id=123, mask=7ff
	// >> a1 0d 00 00
	// << 22 13 20 01 0a							// CH1, idx=10, disable filter

	if (cmd.DataLen1() !=
			(enable ? 9 : 1)) return false;

	const bool extId = cmd.Channel() & 0x01;
	const uint8_t filterNo = cmd.Data1(0);

	uint32_t id = 0, mask = 0;
	if (enable)
	{
		id =	(cmd.Data1(1) << 24) | (cmd.Data1(2) << 16) |
				(cmd.Data1(3) << 8)  | (cmd.Data1(4) << 0);
		mask =	(cmd.Data1(5) << 24) | (cmd.Data1(6) << 16) |
				(cmd.Data1(7) << 8)  | (cmd.Data1(8) << 0);
	}
	if (filterNo >= std::size(canSettings[0].filters)) return false;

	auto setFilter = [&](CanSettings & sett) {
		sett.filters[filterNo].extid = extId;
		sett.filters[filterNo].id = id;
		sett.filters[filterNo].mask = mask;
	};

	auto applyFilters = [](CanSettings & sett, Can::Channel ch)
	{
		Can::Filter filtArr[16]; int outIdx = 0;

		for (const auto & protFlt : sett.filters)
		{
			if (protFlt.mask)
			{
				if (! protFlt.extid)
					filtArr[outIdx++] = Can::Filter::Mask11(protFlt.id, protFlt.mask);
				else
					filtArr[outIdx++] = Can::Filter::Mask29(protFlt.id, protFlt.mask);
			}
		}
		filtArr[outIdx] = Can::Filter::End();

		if (outIdx)
			CanDrv::setFilter(ch, filtArr);
		else
			CanDrv::setFilter(ch, canFilterEverything);
	};

	if (ch1)
	{
		setFilter(canSettings[0]);
		applyFilters(canSettings[0], Can::CANch1);
	}
	if (ch2)
	{
		setFilter(canSettings[1]);
		applyFilters(canSettings[1], Can::CANch2);
	}

	send(cmd.Command() | 0x80);		// send ack
	return true;
}


bool CanHackerBinary::canGate(bool en)
{
	const bool ch1 = cmd.ChCan1();
	const bool ch2 = cmd.ChCan2();
	if (!ch1 && !ch2) return false;

	if (ch1) canSettings[0].gate = en;
	if (ch2) canSettings[1].gate = en;

	send(cmd.Command() | 0x80);		// send ack
	return true;
}

bool CanHackerBinary::canClose()
{
	const bool ch1 = cmd.ChCan1();
	const bool ch2 = cmd.ChCan2();
	const bool lin = cmd.ChLin();
	if (!ch1 && !ch2 && !lin) return false;

	if (ch1 || ch2)
	{
		bool & ch1open = canSettings[0].open;
		bool & ch2open = canSettings[1].open;

		if (ch1) ch1open = false;
		if (ch2) ch2open = false;

		if (!ch2open)
			CanDrv::deinit(Can::CANch2);
		if (!ch1open && !ch2open)
			CanDrv::deinit(Can::CANch1);
	}
	if (lin)
	{
		linBus1.deinit();
		linSettings.open = false;
	}

	send(cmd.Command() | 0x80);		// send ack
	return true;
}

bool CanHackerBinary::canSend()
{
	// CAN ch=2 id=123 len=8 data=11...88
	// 40 0f 40 00 00 0d 00 00 01 23 08 11 22 33 44 55 66 77 88
	// LIN id=C0 len=3 data=C4 EE 48
	// 40 0a 20 00 00 08 00 00 00 c0 03 c4 ee 48
	const bool ch1 = cmd.ChCan1();
	const bool ch2 = cmd.ChCan2();
	const bool lin = cmd.ChLin();

	if (ch1 || ch2)
	{
		//const bool rtr = cmd.Channel() & 0x04;
		//const bool extId = cmd.Channel() & 0x08;

		Can::Pkt pkt;
		pkt.id =	(cmd.Data2(0) << 24) |
					(cmd.Data2(1) << 16) |
					(cmd.Data2(2) << 8) |
					(cmd.Data2(3) << 0);

		pkt.data_len = cmd.Data2(4);
		if (pkt.data_len > std::size(pkt.data) ||
			(4 + 1 + pkt.data_len) != cmd.DataLen2())
			return false;
		for (int i = 0; i < pkt.data_len; i++)
			pkt.data[i] = cmd.Data2(5 + i);

		if (ch1 && canSettings[0].open)
			CanDrv::send(Can::CANch1, pkt);
		if (ch2 && canSettings[1].open)
			CanDrv::send(Can::CANch2, pkt);

		return true;
	}

	if (lin)
	{
		Lin::Pkt pkt;
		pkt.id = cmd.Data2(3);

		pkt.data_len = cmd.Data2(4);
		if (pkt.data_len > std::size(pkt.data) ||
			(4 + 1 + pkt.data_len) != cmd.DataLen2())
			return false;

		for (int i = 0; i < pkt.data_len; i++)
			pkt.data[i] = cmd.Data2(5 + i);

		// add checksum to data field
		pkt.addChecksum(linSettings.extCrc);

		if (linSettings.open)
			linBus1.send(pkt);

		return true;
	}
	return false;
}

bool CanHackerBinary::processPackets()
{
	// receive pkt = id=020 dlc=8 data=01 02 03 04 05 06 07 08
	// 40 01 20 00 00 15 01 b8 c5 0d 00 00 00 00 00 00 00 20 08 01 02 03 04 05 06 07 08

	// L_19200_ECRC_TX_C4_4_5515EDEF
	// 40 01 24 00 00 11 00 32 22 ea 00 00 00 f2 00 00 00 c4 04 55 15 ed ef

	auto convertAndSend = [this](uint8_t channelFlags, const auto &pkt, uint32_t linChksum=0)
	{
		uint8_t tx[64];

		txCounter++;
		tx[0] = 0x40;
		tx[1] = txCounter;
		tx[2] = channelFlags;
		tx[3] = tx[4] = 0;
		tx[5] = 4 + 4 + 4 + 1 + pkt.data_len;
		std::size_t txLen = 6 + tx[5];

		uint32_t timestamp = (pkt.timestamp % 60'000) * 1000;	// microseconds, max=59.9sec
		tx[6] = timestamp >> 24;
		tx[7] = timestamp >> 16;
		tx[8] = timestamp >> 8;
		tx[9] = timestamp >> 0;

		tx[10] = linChksum >> 24;
		tx[11] = linChksum >> 16;
		tx[12] = linChksum >> 8;
		tx[13] = linChksum >> 0;

		tx[14] = pkt.id >> 24;
		tx[15] = pkt.id >> 16;
		tx[16] = pkt.id >> 8;
		tx[17] = pkt.id >> 0;

		tx[18] = pkt.data_len;

		memcpy(tx + 19, pkt.data, pkt.data_len);

		LedBlink::pulseTx();
		Usb::send(tx, txLen);

#if defined DEBUG
		DBG("Pkt:");
		for (auto i = 0u; i < txLen; i++)
			DBG(" %02X", tx[i]);
		DBG("\n");
#endif
	};

	bool haveData = false;
	for (auto ch = 0; ch < 2; ch++)
		while (canPkt[ch].Avail())
		{
			haveData = true;
			auto pkt = canPkt[ch].Get();

			const bool id29bit = (pkt.id > 0x7FF);
			const bool rtr = false;

			const uint8_t channelFlags =
					((ch == 0) ? cmd.maskCh1 : cmd.maskCh2) |
					(id29bit ? 0x08 : 0x00) |
					(rtr ? 0x04 : 0x00);

			convertAndSend(channelFlags, pkt);
		}

	while (linPkt.Avail())
	{
		haveData = true;
		auto pkt = linPkt.Get();

		const uint8_t channelFlags =
				cmd.maskLin |
				0x04;	// Lin enhanced checksum ???

		uint8_t chksum = 0;
		if (pkt.data_len)
		{
			chksum = pkt.data[pkt.data_len-1];
			pkt.data_len--;
		}

		convertAndSend(channelFlags, pkt, chksum);
	}

	return haveData;
}


void CanHackerBinary::send(uint8_t command, uint8_t channel, const void *buf, std::size_t bufLen)
{
	uint8_t tx[64];
	std::size_t txLen;

	tx[0] = command;
	tx[1] = cmd.Counter();
	tx[2] = channel;
	if (command == 0x40)	// send/receive
	{
		tx[3] = tx[4] = 0;
		tx[5] = bufLen;
		memcpy(tx + 6, buf, bufLen);
		txLen = 6 + bufLen;
	}
	else
	{
		tx[3] = bufLen;
		memcpy(tx + 4, buf, bufLen);
		txLen = 4 + bufLen;
	}

#if defined DEBUG
	DBG("Ans:");
	for (auto i = 0u; i < txLen; i++)
		DBG(" %02X", tx[i]);
	DBG("\n");
#endif

	Usb::send(tx, txLen);
}

bool CanHackerBinary::checkLicense()
{
	// << 0f 0f 00 10 5c 69 d7 57 57 a9 2a 9e b0 53 89 f5 1d b4 3f 93
	// >> 0f 0f 00 08 85 df c0 b7 94 18 42 35
	if (cmd.DataLen1() != 16) return false;

	uint8_t sessionKey[16];
	for (int i = 0; i < 16; i++)
		sessionKey[i] = cmd.Data1(i);
	CHLicense::Decrypt(CHLicense::FixedKey, sessionKey, 16);

	uint8_t answer[8];
	memcpy(answer, deviceFeatures, sizeof(answer));
	CHLicense::Encrypt((uint32_t*)sessionKey, answer, sizeof(answer));

	send(0x0F, 0, answer, sizeof(answer));
	return true;
}
