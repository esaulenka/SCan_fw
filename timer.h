#pragma once
#include <cstdint>


extern "C" void SysTick_Handler();

class Timer
{
private:
	using T_Timer = uint32_t;

	// incremented every 1 ms
	static volatile T_Timer _counter;
	friend void SysTick_Handler();

	T_Timer _tmr;
public:
	Timer()
	{	_tmr = _counter;		}

	inline void restart ()
	{	_tmr = _counter;		}

	inline T_Timer value () const
	{	return _counter - _tmr;	}

	inline bool checkTimeout (T_Timer timeout) const
	{	return value() > timeout;			}

	inline void waitTimeout (T_Timer timeout) const
	{	while (! checkTimeout (timeout));	}

	static inline void delay (T_Timer timeout)
	{
		Timer tmr;
		tmr.waitTimeout (timeout);
	}

	static T_Timer counter()
	{	return _counter;	}

	static void init();

};

