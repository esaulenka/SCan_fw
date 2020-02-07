#include "canhacker.h"
#include "cdcacm.h"
#include "Pins.h"
#include "timer.h"
#include "Can/candrv.h"
#include "Debug.h"
#include <cstring>
#include <algorithm>


static const char version[] = "VF_01_01_2020\r";
static const char serial[] = "S0123456789ABCDEF\r";
static const char version2[] = "vSTM32\r";


// global object
CanHacker canHacker;


void CanHacker::packetReceived(Can::Channel channel, const Can::Pkt &packet)
{
	if (canSettings[channel].open)
	{
		TCanPktExt pktExt (packet, Timer::counter());
		canPkt[channel].Put(pktExt);
	}
}


bool CanHacker::processCmd()
{
	char rxBuf[32];
	auto rxLen = Usb::receive(rxBuf, sizeof(rxBuf));

	for (auto i = 0u; i < rxLen; i++)
	{
		cmd.push(rxBuf[i]);
		if (cmd.complete(rxBuf[i]))
			parse();
	}

	return rxLen != 0;	// some data proccessed
}

void CanHacker::parse()
{
	if (cmd.idx == 0) return;

	char buf[64] = {};
	memcpy(buf, cmd.data, cmd.idx);
	DBG("RX <%s>\n", buf);


	const char cmd0 = cmd.data[0];
	const char cmd1 = cmd.data[1];
	const char cmd2 = cmd.data[2];

	const auto channel = Can::Channel(parseDecimal(&cmd1, 1) - 1);
	const bool param2 = (cmd2 == '1');

	auto ack = []() {	Usb::send("\r", 1);		};

	switch(cmd0)
	{
	// Запрос версии прошивки "V" Ответ "VF_11_02_2019\r"
	// Запрос серийного номера "VS" Ответ "0123456789ABCDEF"
	case 'V':
		if (cmd1 == 'S' && cmd.idx == 2)
			Usb::send(serial, sizeof (serial)-1);
		else
			Usb::send(version, sizeof (version)-1);
		break;
	// Lawicell: Read detailed firmware version from device. "v"
	case 'v':
		Usb::send(version2, sizeof (version2)-1);
		break;

	// Открыть канал CAN "OxY"
	case 'O':
		if (cmd.idx == 3 &&
			canOpen(channel, param2))
			ack();
		break;
	// Закрыть канал CAN "Cx"
	case 'C':
		if (cmd.idx == 2 &&
			canClose(channel))
			ack();
		break;
	// Установить скорость CAN в выбранном канале "Sxy"
	case 'S':
		if (cmd.idx == 3 &&
			canSpeed(channel, cmd2))
			ack();
		break;

	// Задать значение аппаратного фильтра CAN "FXX12345678"
	// Задать значение маски аппаратного фильтра CAN "fXX12345678"
	// TODO

	// Отправить пакет в CAN "tXiiiLdddddddddddddddd", "T11234567825566"
	case 'T':
		if (canSend(channel, true))
			ack();
		break;
	case 't':
		if (canSend(channel, false))
			ack();
		break;

	// команды CAN шлюза "Gxx"
	case 'G':
		if (cmd.idx == 3 &&
			canGate(channel, param2))
			ack();
		break;

	// Блокировать прохождение пакета с заданным ID в задан-ном канале "LX123"
	case 'L':
		// TODO
		ack();
		break;


	// test pins
	case 'P':
		testPin();
		break;

	}

	// clear buffer
	cmd.idx = 0;
}


bool CanHacker::canSpeed(Can::Channel channel, char speed)
{
	uint32_t baudrate;
	switch (speed)
	{
	case '0': baudrate = CanDrv::Baudrate10; break;
	case '1': baudrate = CanDrv::Baudrate20; break;
	case '2': baudrate = CanDrv::Baudrate33_3; break;
	case '3': baudrate = CanDrv::Baudrate50; break;
	case '4': baudrate = CanDrv::Baudrate62_5; break;
	case '5': baudrate = CanDrv::Baudrate83_3; break;
	case '6': baudrate = CanDrv::Baudrate100; break;
	case '7': baudrate = CanDrv::Baudrate125; break;
	case '8': baudrate = CanDrv::Baudrate250; break;
	case '9': baudrate = CanDrv::Baudrate400; break;
	case 'A': baudrate = CanDrv::Baudrate500; break;
	case 'B': baudrate = CanDrv::Baudrate800; break;
	case 'C': baudrate = CanDrv::Baudrate1000; break;
	case 'D': baudrate = CanDrv::Baudrate95_2; break;
	default: return false;
	}

	if (channel > 1) return false;
	canSettings[channel].baudrate = baudrate;
	return true;
}

bool CanHacker::canOpen(Can::Channel channel, bool silent)
{
	if (channel > 1) return false;

	const auto br = canSettings[channel].baudrate;
	if (br == 0) return false;

	CanDrv::init(channel, br, silent);
	// TODO filters
	static const uint64_t filters[] = {
			Can::Filter::Mask11 (0,0),
			Can::Filter::Mask29 (0,0),
			0	// end of filters mask
	};
	CanDrv::setFilter(channel, filters);

	canSettings[channel].open = true;
	return true;
}


bool CanHacker::canClose(Can::Channel channel)
{
	if (channel > 1) return false;
	canSettings[channel].open = false;
	return true;
}


bool CanHacker::canGate(Can::Channel channel, bool enable)
{
	if (channel > 1) return false;
	canSettings[channel].gate = enable;
	return true;
}

bool CanHacker::canSend(Can::Channel channel, bool id29bit)
{
	if (! canSettings[channel].open) return false;

	// "tXiiiLdddddddddddddddd", "TX12345678Ldd.."
	const uint8_t idLength = id29bit ? 8 : 3;

	const uint8_t dataLength = parseDecimal(&cmd.data[2 + idLength], 1);
	if (dataLength > 8) return false;
	if (cmd.idx < 2u + idLength + 1u + dataLength) return false;

	const uint32_t id = parseHex(&cmd.data[2], idLength);
	if ( id29bit && id > 0x1FFF'FFFF) return false;
	if (!id29bit && id > 0x7FF) return false;

	Can::Pkt pkt(id);
	pkt.data_len = dataLength;
	for (auto i = 0u; i < dataLength; i++)
		pkt.data[i] = parseHex(&cmd.data[2 + idLength + 1 + i *2], 2);

	//DBG("Can send: ch %d, id %X len %d\n", channel, id, dataLength);

	CanDrv::send(channel, pkt);
	return true;
}

bool CanHacker::processPackets()
{
	bool haveData = false;
	for (auto ch = 0; ch < 2; ch++)
		while (canPkt[ch].Avail())
		{
			haveData = true;
			auto pkt = canPkt[ch].Get();
			char buf[32];

			// "tXiiiLdddddddddddddddd1234\r"
			// "T112345678255660347\r"

			const bool id29bit = (pkt.id > 0x7FF);

			buf[0] = id29bit ? 'T' : 't';
			buf[1] = makeHex(ch + 1);
			const uint8_t idLength = id29bit ? 8 : 3;
			makeHex(&buf[2], pkt.id, idLength);
			buf[2 + idLength] = makeHex(pkt.data_len);
			for (auto i = 0u; i < pkt.data_len; i++)
				makeHex(&buf[2 + idLength + 1 + i*2], pkt.data[i], 2);

			const uint32_t timestamp = pkt.timestamp % 10000u;
			const uint32_t tsOffset = 2 + idLength + 1 + pkt.data_len * 2;
			makeHex(&buf[tsOffset], timestamp, 4);
			buf[tsOffset + 4] = '\r';

			Usb::send(buf, tsOffset + 4 + 1);
		}

	return haveData;
}






int CanHacker::parseDecimal(const char *str, uint32_t len)
{
	int res = 0;
	while(len--)
	{
		const char d = *str++;
		if (d < '0' || d > '9') return -1;
		res = res * 10 + d - '0';
	}
	return res;
}

int CanHacker::parseHex(const char *str, uint32_t len)
{
	int res = 0;
	while(len--)
	{
		char d = *str++;
		if (d >= '0' && d <= '9')
			d -= '0';
		else if (d >= 'A' && d <= 'F')
			d -= 'A';
		else if (d >= 'a' && d <= 'f')
			d -= 'a';
		else
			return -1;
		res = res * 16 + d;
	}
	return res;
}

void CanHacker::makeHex(char *buf, uint32_t value, uint32_t bufLen)
{
	for (auto i = 0u; i < bufLen; i++)
	{
		buf[i] = makeHex(value);
		value /= 16;
	}
	std::reverse(buf, buf + bufLen);
}
uint8_t CanHacker::makeHex(uint32_t value)
{
	static const char mask[] = "0123456789ABCDEF";
	return mask[value & 0x0F];
}


void CanHacker::testPin()
{
	// "Pxnn"
	// x - port name
	// nn - bit number (decimal)
	char port = cmd.data[1];
	int pin = parseDecimal(&cmd.data[2], 2);

	Usb::send(cmd.data, 6);
	if (cmd.idx < 4) return;
	if (port < 'A' || port > 'D') return;
	if (pin < 0 || pin > 15) return;

	setMode(port, pin, 1);	// output
	for (int i = 0; i < 100; i++)
	{
		setOut(port, pin, i & 1);
		Timer::delay(50);
	}
	setMode(port, pin, 4);	// input, nopull

	Usb::send("done\r\n", 6);

	PinBuzzer::Mode(OUTPUT_2MHZ);
	for (int i = 0; i < 50; i++)
	{
		PinBuzzer::Cpl();
		Timer::delay(3);
	}
	PinBuzzer::Off();
}
