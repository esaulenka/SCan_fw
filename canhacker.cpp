#include "canhacker.h"
#include "cdcacm.h"
#include "Pins.h"
#include "timer.h"


static const char version[] = "VF_11_02_2019\r";
static const char serial[] = "0123456789ABCDEF\r";

CanHacker::CanHacker()
{

}


bool CanHacker::processCmd()
{
	char rxBuf[32];
	auto rxLen = Usb::receive(rxBuf, sizeof(rxBuf));

	for (auto i = 0u; i < rxLen; i++)
	{
		command.push(rxBuf[i]);
		if (command.complete())
			parse();
	}

	return rxLen != 0;
}

void CanHacker::parse()
{
	if (command.idx == 0) return;

	const char cmd0 = command.data[0];
	const char cmd1 = command.data[1];
	//const char cmd2 = command.data[2];

	auto ack = []() {
		Usb::send("\r", 1);
	};

	switch(cmd0)
	{
	// Запрос версии прошивки "V\r\n" Ответ "VF_11_02_2019\r"
	// Запрос серийного номера "VS\r\n" Ответ "0123456789ABCDEF"
	case 'V':
		if (cmd1 == 'S')
			Usb::send(serial, sizeof (serial));
		else
			Usb::send(version, sizeof (version));
		break;

	// Открыть канал CAN "OxY\r\n"
	case 'O':
		// TODO
		ack();
		break;
	// Закрыть канал CAN "Cx\r\n"
	case 'C':
		// TODO
		ack();
		break;
	// Установить скорость CAN в выбранном канале "Sxy\r\n"
	case 'S':
		// TODO
		ack();
		break;
	// Задать значение аппаратного фильтра CAN "FXX12345678\r\n"
	// Задать значение маски аппаратного фильтра CAN "fXX12345678\r\n"

	// Отправить пакет в CAN "tXiiiLdddddddddddddddd\r\n", "T11234567825566\r\n"

	// команды CAN шлюза "Gxx\r\n"
	case 'G':
		// TODO
		ack();
		break;

	// Блокировать прохождение пакета с заданным ID в задан-ном канале "LX123\r\n"
	case 'L':
		// TODO
		ack();
		break;


	// test pins
	case 'P':
	{
		// Pxnn\r\n
		// x - port name
		// nn - bit number (decimal)
		char port = cmd1;
		int pin = parseDecimal(&command.data[2], 2);

		Usb::send(command.data, 6);
		if (command.idx < 6) break;
		if (port < 'A' || port > 'D') break;
		if (pin < 0 || pin > 15) break;

		setMode(port, pin, 1);	// output
		for (int i = 0; i < 1000; i++)
		{
			setOut(port, pin, i & 1);
			Timer::delay(10);
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

		break;
	}

	}

	// clear buffer
	command.idx = 0;
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
