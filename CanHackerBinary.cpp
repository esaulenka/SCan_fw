#include "CanHackerBinary.h"
#include "cdcacm.h"
#include "Can/candrv.h"
#include "Lin/LinBus.h"
#include "LedBlink.h"
#include "CHLic.h"
#include "Debug.h"
#include <cstring>
#include <array>


static const uint8_t deviceInfo[] = "CH-3.2";			// cmd 0x01
static const uint8_t deviceFirmware[] = "2.1.1.9";		// cmd 0x02
static const uint8_t deviceSerial[8] = { 0,0,0,0,0,0,0x08,0x05 };



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
	case 0x01:		// get device info
		// << 01 01 00 00
		// >> 01 01 00 04 43 48 33 32
		send(0x01, 0, deviceInfo, sizeof(deviceInfo)-1);
		break;
	case 0x02:	  	// get device firmware
		// << 02 02 00 00
		// >> 02 02 00 06 30 2e 31 2e 31 31
		send(0x02, 0, deviceFirmware, sizeof(deviceFirmware)-1);
		break;
	case 0x03:		// get device serial number
	{	// << 03 04 00 00
		// >> 03 04 00 08 00 00 00 00 00 00 08 05
		send(0x03, 0, deviceSerial, sizeof(deviceSerial));
		break;
	}
	case 0x04:		// set device mode: 0 - 2CAN+Lin, 1 - 2CAN, 2 - LIN
		setMode();
		break;
	case 0x05:		// get device hardware
	{	// << 05 01 00 00
		// >> 05 01 00 01 11
		uint8_t tmp = 0x11;		// CanHacker 3.3 on F407 (dual CAN + single LIN)
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
	case 0x0A:		// TODO device load statistics
		// << 0a 0d 01 00
		// >> 8a 0d 00 00
		// should send periodically (1 sec) packets
		// 0a 01 00 5c <23 uints>
		send(cmd.Command() | 0x80);
		break;
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
	//case 0x1F:	// TODO reset channel
	case 0x21:		// set filter
		canFilter(true);
		break;
	case 0x22:		// clear filter
		canFilter(false);
		break;
	//case 0x23:	// TODO get filter
	case 0x31:		// gate enable
		// old version: 24 = gate 1->2, 42 = gate 2->1
		// new version: 43 = gate 1->2, 25 = gate 2->1
		// << 31 0e 42 00
		// >> b1 0e 00 00
		canGate(true);
		break;
	case 0x32:		// gate disable
		canGate(false);
		break;
	//case 0x33:	// gate set filter
	// idx = 0, mode=11bit, id=123, mask=7ff
	// 33 1d 42 10  00 00 00 00  00 00 00 00  23 01 00 00  ff 07 00 00
	// idx = 0f, mode=29bit, id=12345678, mask=10abcdef
	// 33 1e 42 10  0f 00 00 00  01 00 00 00  78 56 34 12  ef cd ab 10
	//case 0x34:	// gate remove filter
	// 34 1f 42 04  0f 00 00 00		// gate 1->2, filter idx = 0f 
	//case 0x35:	// gate remove filters
	case 0x48:		// old LIN slave set data
	case 0x49:		// old LIN slave enable response
		send(cmd.Command() | 0x80);
		break;
	case 0x40:		// transmit packet
		canSend();
		break;
	case 0x4a:		// TODO LIN slave: set data
		// id = 11, len = 4, data = aa bb cc dd
		// << 4a 18 60 0e  00 00 00 00  11 00 00 00  04 00  aa bb cc dd
		// >> ca 18 00 00
		send(cmd.Command() | 0x80);
		break;
	case 0x4b:		// TODO LIN slave: enable response
		// << 4b 09 61 00 - enable
		// << 4b 09 60 00 - disable
		// >> c9 09 00 00
		send(cmd.Command() | 0x80);
		break;
	default:
		DBG("Unknown command %02X!\n", cmd.Command());
		break;

	//0x48: 	// TODO bus error
	// #define ERROR_CAN_STUFF 0x00000001U
	// #define ERROR_CAN_FORMAT 0x00000002U
	// #define ERROR_LIN_FRAME 0x00000002U
	// #define ERROR_CAN_ACK 0x00000004U
	// #define ERROR_CAN_CRC 0x00000008U
	// #define ERROR_CAN_TRANSMIT1 0x00000010U
	// #define ERROR_CAN_TRANSMIT0 0x00000020U
	// #define ERROR_BUS_OFF 0x00000040U
	// #define ERROR_PASSIVE 0x00000080U
	// #define ERROR_WARNING 0x00000100U
	// #define ERROR_OVERFLOW 0x00000200U
	// #define ERROR_DATA_STUFF 0x00010000U
	// #define ERROR_DATA_FORMAT 0x00020000U
	// #define ERROR_DATA_ACK 0x00040000U
	// #define ERROR_DATA_CRC 0x00080000U
	// #define ERROR_DATA_TRANSMIT1 0x00100000U
	// #define ERROR_DATA_TRANSMIT0 0x00200000U	
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

	auto parseBaudrate = [](const Cmd & cmd) -> uint32_t {
		// speed             | presc | seg1  | seg2  | sjw
		// 500:  11 08 22 08   04 00   0f 00   02 00   01 00
		// 250:  11 08 22 08   09 00   0d 00   02 00   01 00
		// 125:  11 08 22 08   12 00   0d 00   02 00   01 00
		// application uses the same bus speed 36 MHz
		uint16_t prescaler = cmd.Data1(0) | (cmd.Data1(1) << 8);
		uint8_t seg1 = cmd.Data1(2);
		uint8_t seg2 = cmd.Data1(4);
		uint8_t sjw = cmd.Data1(6);
		return can_btr(prescaler, seg1, seg2, sjw);
	};

	const bool ch1 = cmd.ChCan1();
	const bool ch2 = cmd.ChCan2();
	const bool lin = cmd.ChLin();
	if (!ch1 && !ch2 && !lin) return false;

	if (cmd.DataLen1() < 1) return false;

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
	case 0x2:		// set manual speed
		if (ch1) canSettings[0].baudrate = parseBaudrate(cmd);
		if (ch2) canSettings[1].baudrate = parseBaudrate(cmd);
		if (lin) {
			// 11 0f 62 08  00 4b 00 00  00 00 00 00 -> 0x4b00 = 19200
			if (cmd.DataLen1() != 8) return false;
			linSettings.baudrate = cmd.Data1dw(0);
		}
		break;
	//case 0x3:		// set manual FD speed
	//case 0x5:		// terminator relay
	//case 0x6:		// pullup relay
	case 0x7:		// LIN CRC mode
		// 01 - crc, 02 - ecrc
		linSettings.extCrc = cmd.Data1(0) == 0x02;
		break;
	case 0x8:		// LIN idle delay (max delay between bytes)
		static const uint16_t lin_interbyte_delay[] = {		// in micro seconds
			0, 100, 200, 250, 500, 750, 1000, 1500, 2000,
		};
		if (lin && cmd.Data1(0) < std::size(lin_interbyte_delay)) {
			linSettings.interbyteDelay = lin_interbyte_delay[cmd.Data1(0)];
		}
		break;
	case 0x9:		// CAN listen mode. 0 - normal, 1 - listen, 2 - loopback
		if (ch1) canSettings[0].silent = (cmd.Data1(0) == 1);
		if (ch2) canSettings[1].silent = (cmd.Data1(0) == 1);
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
		linBus1.init(linSettings.baudrate, linSettings.interbyteDelay);
		linSettings.open = true;
	}

	send(cmd.Command() | 0x80);		// send ack
	return true;
}

bool CanHackerBinary::canFilter(bool enable)
{
	const bool ch1 = cmd.ChCan1();
	const bool ch2 = cmd.ChCan2();
	const bool lin = cmd.ChLin();
	if (!ch1 && !ch2 && !lin) return false;

	// new version
	// filter idx = 0xd, mode=29bit, id = 12345678, mask = 10abcdef
	// << 21 17 20 10  0d 00 00 00  01 00 00 00  78 56 34 12  ef cd ab 10
	// filter idx = 05, mode=11bit, id=7df, mask=7ff
	// << 21 1b 40 10  05 00 00 00  00 00 00 00  df 07 00 00  ff 07 00 00
	// filter idx = 07, channel = lin, id = 12
	// << 21 18 60 10  07 00 00 00  00 00 00 00  12 00 00 00  00 00 00 00
	// filter idx = 07, channel = lin, disable
	// << 22 1a 60 04  07 00 00 00

	bool extId = false;
	uint8_t filterNo;
	uint32_t id = 0, mask = 0;
	if (enable && cmd.DataLen1() == 16)
	{
		// enable, new version
		filterNo = cmd.Data1dw(0);
		extId = cmd.Data1dw(1) & 0x01;
		id = cmd.Data1dw(2);
		mask = cmd.Data1dw(3);
	}
	else if (!enable && cmd.DataLen1() == 4)
	{
		// disable, new version
		filterNo = cmd.Data1dw(0);
	}
	else
		return false;

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
	if (lin)
	{
		// TODO lin filters
	}

	send(cmd.Command() | 0x80);		// send ack
	return true;
}


bool CanHackerBinary::canGate(bool en)
{
	// new version: 43 = gate 1->2, 25 = gate 2->1
	if (cmd.Channel() == 0x43) canSettings[0].gate = en;
	else
	if (cmd.Channel() == 0x25) canSettings[1].gate = en;
	else return false;

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
	// new version
	// CAN ch=1 id=123 len=8 data=11...88
	// 40 22 00 20  12 00  00 00 00 00  23 01 00 00  08 00  11 22 33 44 55 66 77 88
	// CAN ch=2 RTR, 29bit, id=123, len=8
	// 40 24 00 40  0a 00  03 00 00 00  23 01 00 00  08 00
	// CAN ch=1 29bit, id=12345678, len=8, data=11...88 
	// 40 20 00 20  12 00  01 00 00 00  78 56 34 12  08 00  11 22 33 44 55 66 77 88

	// LIN id=C0 len=3 data=C4 EE 48
	// 40 23 00 60  0d 00  00 00 00 00  c0 00 00 00  03 00  c4 ee 48

	// channel mask also shifted!
	const bool ch1 = cmd.data[3] == cmd.maskCh1;
	const bool ch2 = cmd.data[3] == cmd.maskCh2;
	const bool lin = cmd.data[3] == cmd.maskLin;

	if (ch1 || ch2)
	{
		// ExtId = 0x01; rtr = 0x02; CAN-FD = 0x04; CAN-FD rate switch = 0x08; CAN-FD error status = 0x10; Block TX = 0x30000000
		//const uint32_t flags = cmd.Data2dw(0);
		// what it means? "Следует учесть, что если для CAN шины в message.flags не выставить флаг FLAG_MESSAGE_BLOCK_TX, то устройство
		// после отправки сообщения в шину вернёт его с соответствующим флагом."
		Can::Pkt pkt;
		pkt.id = cmd.Data2dw(1);

		pkt.data_len = cmd.Data2(8);
		if (pkt.data_len > std::size(pkt.data) ||
		 	(4 + 4 + 2 + pkt.data_len) != cmd.DataLen2())
			return false;
		for (int i = 0; i < pkt.data_len; i++)
			pkt.data[i] = cmd.Data2(10 + i);

		if (ch1 && canSettings[0].open)
			CanDrv::send(Can::CANch1, pkt);
		if (ch2 && canSettings[1].open)
			CanDrv::send(Can::CANch2, pkt);

		return true;
	}

	if (lin)
	{
		Lin::Pkt pkt;
		pkt.id = cmd.Data2dw(1);

		pkt.data_len = cmd.Data2(8);
		if (pkt.data_len > std::size(pkt.data) ||
			(4 + 4 + 2 + pkt.data_len) != cmd.DataLen2())
			return false;

		for (int i = 0; i < pkt.data_len; i++)
			pkt.data[i] = cmd.Data2(10 + i);

		// add checksum to data field
		if (pkt.data_len)
			pkt.addChecksum(linSettings.extCrc);

		if (linSettings.open)
			linBus1.send(pkt);

		return true;
	}
	return false;
}

bool CanHackerBinary::processPackets()
{
	// old version
	// receive CAN pkt = id=020 dlc=8 data=01 02 03 04 05 06 07 08
	// 40 01 20 00  00 15  01 b8 c5 0d  00 00 00 00  00 00 00 20  08  01 02 03 04 05 06 07 08
	// L_19200_ECRC_TX_C4_4_5515EDEF
	// 40 01 24 00  00 11  00 32 22 ea  00 00 00 f2  00 00 00 c4  04  55 15 ed ef

	// new version
	// 0  1  2  3   4  5   6  7  8  9   10 11 12 13  14 15 16 17  18 19 20 21  22 23  24 25 26 27 28 29 30 31
	// ch=2, flags=1000'0000, time=00535bd8, chksum=00, id=222, data=11..88
	// 40 01 00 40  1a 00  00 00 00 10  d8 5b 53 00  00 00 00 00  22 02 00 00  08 00  11 22 33 44 55 66 77 88
	// ch=1, flags=1000'0000, time=0103ac90, chksum=00, id=777, data=fffff
	// 40 02 00 20  1a 00  00 00 00 10  90 ac 03 01  00 00 00 00  77 07 00 00  08 00  ff ff ff ff ff ff ff ff
	// ch=2, flags=1000'0001, time=0069d9a8, chksum=00, id=1fff'ffff, data=ffffff
	// 40 18 00 40  1a 00  01 00 00 10  a8 d9 69 00  00 00 00 00  ff ff ff 1f  08 00  ff ff ff ff ff ff ff ff
	// lin normal, flags=0000'1000, time=00144f38, chksum=99, id=80, data=11..88
	// 40 01 00 20  1a 00  00 10 00 00  38 4f 14 00  99 00 00 00  80 00 00 00  08 00  11 22 33 44 55 66 77 88
	// lin extcrc, flags=0000'2000, time=024d8210, chksum=3f, id=80, data=40 00 00
	// 40 01 00 20  15 00  00 20 00 00  10 82 4d 02  3f 00 00 00  80 00 00 00  03 00  40 00 00
	// lin extcrc, flags=0000'2000, time=025215f0, chksum=e7, id=3c, data=20 ee ee ee ee ff ff 00
	// 40 0f 00 20  1a 00  00 20 00 00  f0 15 52 02  e7 00 00 00  3c 00 00 00  08 00  20 ee ee ee ee ff ff 00

	// Flags: bit0 - extId, bit1 - RTR, bit2 - CAN-FD, bit3 - CAN-FD switch rate, bit4 - CAN-FD error status
	// bit8 - LIN master req, bit9 - LIN slave response, bit12 - LIN classic CRC, bit13 - LIN extCRC
	// bit24 - error frame, bit28 - rx (?), bit29 - tx (?)


	auto convertAndSend = [this](uint8_t channel, uint32_t flags, const auto &pkt, uint32_t linChksum=0)
	{
		uint8_t tx[64];

		txCounter++;
		tx[0] = 0x40;
		tx[1] = txCounter;
		tx[2] = 0;
		tx[3] = channel;

		std::size_t txLen = 4 + 2 + 4 + 4 + 4 + 4 + 2 + pkt.data_len;
		__UNALIGNED_UINT16_WRITE(tx + 4, txLen - 4 - 2);

		__UNALIGNED_UINT32_WRITE(tx + 6, flags);
		__UNALIGNED_UINT32_WRITE(tx + 10, (pkt.timestamp % 60'000) * 1000);	// microseconds, max=59.9sec
		__UNALIGNED_UINT32_WRITE(tx + 14, linChksum);
		__UNALIGNED_UINT32_WRITE(tx + 18, pkt.id);

		__UNALIGNED_UINT16_WRITE(tx + 22, pkt.data_len);
		memcpy(tx + 24, pkt.data, pkt.data_len);

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

			uint8_t channel = ((ch == 0) ? cmd.maskCh1 : cmd.maskCh2);

			bool extId = pkt.id > 0x7ff;
			uint32_t flags = 0x1000'0000 | (extId ? 0x01 : 0x00);

			convertAndSend(channel, flags, pkt);
		}

	while (linPkt.Avail())
	{
		haveData = true;
		auto pkt = linPkt.Get();

		uint32_t flags = 0x1000'0000 | (linSettings.extCrc ? 0x0000'2000 : 0x0000'1000);

		uint8_t chksum = 0;
		if (pkt.data_len)
		{
			chksum = pkt.data[pkt.data_len-1];
			pkt.data_len--;
		}

		convertAndSend(cmd.maskLin, flags, pkt, chksum);
	}

	return haveData;
}

// cmd40 not supported by this function
void CanHackerBinary::send(uint8_t command, uint8_t channel, const void *buf, std::size_t bufLen)
{
	uint8_t tx[64];

	tx[0] = command;
	tx[1] = cmd.Counter();
	tx[2] = channel;
	tx[3] = bufLen;
	memcpy(tx + 4, buf, bufLen);
	std::size_t txLen = 4 + bufLen;

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
	CHLicense::Decrypt(CHLicense::key17, sessionKey, 16);

	uint8_t answer[8];
	memcpy(answer, deviceSerial, sizeof(answer));
	CHLicense::Encrypt((uint32_t*)sessionKey, answer, sizeof(answer));

	send(0x0F, 0, answer, sizeof(answer));
	return true;
}
