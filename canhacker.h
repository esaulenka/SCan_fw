#pragma once
#include <cstdint>


class CanHacker
{
public:
	CanHacker();

	bool processCmd();


private:

	void parse();

	int parseDecimal(const char * str, uint32_t len);
	int parseHex(const char * str, uint32_t len);

	struct {
		char data[32];
		uint32_t idx = 0;

		void push(char b) {
			data[idx++] = b;
			idx %= sizeof(data);
		}
		bool complete() const {
			if (idx < 2) return false;
			return (data[idx-2] == '\r') && (data[idx-1] == '\n');
		}
	} command;

};

