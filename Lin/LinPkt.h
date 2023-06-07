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
	};
};
