#pragma once

#include <cstdint>


#if (BOARD == BOARD_SIGMA || BOARD == BOARD_CSAT || BOARD == BOARD_2CAN2LIN) \
	&& PROTOCOL == PROTOCOL_BINARY
	#define LIN_BUS_SUPPORTED	1
#else
	#define LIN_BUS_SUPPORTED	0
#endif

class Lin {

public:
	struct Pkt {
		uint8_t id = 0;
		uint8_t data_len = 0;
		uint8_t data[16] = {};

		void addChecksum(bool enhanced)
		{
			uint32_t sum = 0;
			if (enhanced) sum = id;
			for (auto i = 0u; i < data_len; ++i)
				sum += data[i];
			data[data_len++] = ~(sum % 255);
		}
		void setProtId(uint8_t unprotId)
		{
			// P1 = ~(ID1 ^       ID3 ^ ID4 ^ ID5)
			// P0 =   ID0 ^ ID1 ^ ID2 ^       ID4
			uint8_t b10 = (unprotId << 6);
			uint8_t b_1 = (unprotId << 5);
			uint8_t b32 = (unprotId << 4);
			uint8_t b4_ = (unprotId << 3);
			uint8_t b54 = (unprotId << 2);
			id = (unprotId & 0x3F) ^ 0x80 ^ ((b54 ^ b32 ^ b10) & 0xC0) ^ (b_1 & 0x40) ^ (b4_ & 0x80);
		}
	};
};
