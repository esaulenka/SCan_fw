#pragma once
#include <cstdint>


// простейший кольцевой буфер.
// проверки при добавлении/извлечении данных отсутствуют, надо проверять отдельно!!

template <typename Type = char, int Size = 32>
class CircularBuffer
{
private:
        Type 		buff[Size];
        volatile uint32_t	putIdx;
        volatile uint32_t	getIdx;

public:
        CircularBuffer()
        {
                Flush ();
        }

        void Put (const Type & data)
        {
                buff[putIdx] = data;
                putIdx = (putIdx + 1) % Size;
        }

        Type Get ()
        {
                Type data = buff[getIdx];
                getIdx = (getIdx + 1) % Size;
                return data;
        }

        Type & View ()
        {
                return buff[getIdx];
        }

        uint32_t Avail () const
        {
                int32_t avail = putIdx - getIdx;
                if (avail < 0) avail += Size;
                return avail;
        }

        uint32_t Free () const
        {
                int32_t free = getIdx - putIdx - 1;
                if (free < 0) free += Size;
                return free;
        }

        void Flush ()
        {
                putIdx = getIdx = 0;
        }


};


