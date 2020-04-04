#pragma once
#include <cstdint>



class Can {
public:
	enum Channel {
		CANch1,
		CANch2
	};


	struct Pkt
	{
		uint32_t	id;
		uint8_t		data[8];
		uint8_t		data_len;

		Pkt()	{ }

		Pkt (uint32_t pkt_id):
			id(pkt_id) { }

		// принимает строку, заполняет данные пакета. Неуказанные данные заполняются нулями
		template <int N>
		inline void SetData (const char (&data_str)[N])
		{
			int data_size = (N <= 8) ? (N - 1) : 8;
			for (int i = 0; i < data_size; i++)
				data[i] = data_str[i];
			for (int i = data_size; i < 8; i++)
				data[i] = 0x00;
		}

		inline void SetData (uint8_t A)
		{	data_len = 1;
			data[0] = A; data[1] = 0; data[2] = 0; data[3] = 0;
			data[4] = 0; data[5] = 0; data[6] = 0; data[7] = 0;
		}
		inline void SetData (uint8_t A, uint8_t B)
		{	data_len = 2;
			data[0] = A; data[1] = B; data[2] = 0; data[3] = 0;
			data[4] = 0; data[5] = 0; data[6] = 0; data[7] = 0;
		}
		inline void SetData (uint8_t A, uint8_t B, uint8_t C)
		{	data_len = 3;
			data[0] = A; data[1] = B; data[2] = C; data[3] = 0;
			data[4] = 0; data[5] = 0; data[6] = 0; data[7] = 0;
		}
		inline void SetData (uint8_t A, uint8_t B, uint8_t C, uint8_t D)
		{	data_len = 4;
			data[0] = A; data[1] = B; data[2] = C; data[3] = D;
			data[4] = 0; data[5] = 0; data[6] = 0; data[7] = 0;
		}
		inline void SetData (uint8_t A, uint8_t B, uint8_t C, uint8_t D, uint8_t E)
		{	data_len = 5;
			data[0] = A; data[1] = B; data[2] = C; data[3] = D;
			data[4] = E; data[5] = 0; data[6] = 0; data[7] = 0;
		}
		inline void SetData (uint8_t A, uint8_t B, uint8_t C, uint8_t D, uint8_t E, uint8_t F)
		{	data_len = 6;
			data[0] = A; data[1] = B; data[2] = C; data[3] = D;
			data[4] = E; data[5] = F; data[6] = 0; data[7] = 0;
		}
		inline void SetData (uint8_t A, uint8_t B, uint8_t C, uint8_t D, uint8_t E, uint8_t F, uint8_t G)
		{	data_len = 7;
			data[0] = A; data[1] = B; data[2] = C; data[3] = D;
			data[4] = E; data[5] = F; data[6] = G; data[7] = 0;
		}
		inline void SetData (uint8_t A, uint8_t B, uint8_t C, uint8_t D, uint8_t E, uint8_t F, uint8_t G, uint8_t H)
		{	data_len = 8;
			data[0] = A; data[1] = B; data[2] = C; data[3] = D;
			data[4] = E; data[5] = F; data[6] = G; data[7] = H;
		}
	};

	// объявление фильтров при инициализации
	struct Filter {
		Filter() : val(0) {}

		// добавить пару идентификаторов для точного соответствия
		static constexpr Filter List11 (uint16_t id1, uint16_t id2) {
			return (IdStd (id1) << 32) | IdStd (id2) | item_list;
		}
		static constexpr Filter List29 (uint32_t id1, uint32_t id2) {
			return (IdExt (id1) << 32) | IdExt (id2) | item_list;
		}

		// добавить один идентификатор
		static constexpr Filter List11 (uint16_t id) { return List11(id, id); }
		static constexpr Filter List29 (uint32_t id) { return List29(id, id); }

		// добавить один идентификатор с маской
		static constexpr Filter Mask11 (uint16_t id, uint16_t mask) {
			return (IdStd (id) << 32) | IdStd (mask) | item_mask;
		}
		static constexpr Filter Mask29 (uint32_t id, uint32_t mask) {
			return (IdExt (id) << 32) | IdExt (mask) | item_mask;
		}

		// маркер конца списка. Обязательно добавлять в массив фильтров!
		static constexpr Filter End() { return 0; }

	private:
		uint64_t val;
		friend class CanDrv;
		enum : uint64_t {
			item_mask = 1u,
			item_list = (1ull << 32) | 1u,
		};


		constexpr Filter(uint64_t x): val(x) {}

		// формирование ID для фильтров
		static constexpr uint64_t IdStd (uint16_t id)
		{	return (id & 0x7FFu) << 21; }
		static constexpr uint64_t IdExt (uint32_t id)
		{	return ((id & 0x1FFFFFFFu) << 3) | 0x04; }
	};
};

