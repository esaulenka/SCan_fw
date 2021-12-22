#pragma once

#include <cstdint>

class CHLicense
{
public:
	static constexpr uint32_t FixedKey[4] =
	{ 0x34AA01BE, 0xDA891AF0, 0x78A1C4E5, 0xF98A5CD2 };

	static void Encrypt(const uint32_t key[4], void *buf, std::size_t bufLen)
	{
		auto st = (uint32_t*)buf;
		for (auto i = bufLen / 8; i != 0; i--) {
			EncryptBlock(key, st, st + 1);
			st += 2;
		}
	}

	static void Decrypt(const uint32_t key[4], void *buf, std::size_t bufLen)
	{
		auto st = (uint32_t*)buf;
		for (auto i = bufLen / 8; i != 0; i--) {
			DecryptBlock(key, st, st + 1);
			st += 2;
		}
	}

private:
	static void EncryptBlock(const uint32_t key[4], uint32_t *a, uint32_t *b)
	{
		for (int i = 0; i < 0x30; ) {
			*a += *b + (*b << 6 ^ *b >> 8) + key[i % 4] + i;
			i++;
			*b += *a + (*a << 6 ^ *a >> 8) + key[i % 4] + i;
			i++;
		}
	}
	static void DecryptBlock(const uint32_t key[4], uint32_t *a, uint32_t *b)
	{
		for (int i = 0x30; i != 0; ) {
			i--;
			*b -= *a + (*a << 6 ^ *a >> 8) + key[i % 4] + i;
			i--;
			*a -= *b + (*b << 6 ^ *b >> 8) + key[i % 4] + i;
		}
	}

};

