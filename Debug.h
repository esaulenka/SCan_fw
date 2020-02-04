#pragma once

#if defined DEBUG

	#include "SEGGER_RTT.h"
	#define DBG(...)	SEGGER_RTT_printf(0, __VA_ARGS__)

#else
	#define DBG(...)	do{}while(0)
#endif
