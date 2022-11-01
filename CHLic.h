#pragma once

#include <cstdint>

class CHLicense
{
public:
	static constexpr uint32_t keyDflt[4] =	{ 0x34AA01BE, 0xDA891AF0, 0x78A1C4E5, 0xF98A5CD2 };
	static constexpr uint32_t key1[4] =		{ 0x68095DCB, 0x43B0A59B, 0x9A94188D, 0xB0922B57 };
	static constexpr uint32_t key2[4] =		{ 0x2ECDB8A3, 0xF76B7BF1, 0x2B7B56D9, 0xCA02B38F };
	static constexpr uint32_t key3[4] =		{ 0xBCBF6913, 0x39FB766B, 0x3FAC6AC5, 0x45BBACDB };
	static constexpr uint32_t key4[4] =		{ 0x59594207, 0x5487BD83, 0xC231BA49, 0xD0B02A83 };
	static constexpr uint32_t key5[4] =		{ 0x6C790337, 0xF5A515F3, 0xB3659CED, 0xFB7EA1C3 };
	static constexpr uint32_t key6[4] =		{ 0x4978C379, 0x382FCBDF, 0x805260AF, 0x8640F581 };
	static constexpr uint32_t key17[4] =	{ 0x32EDD1DF, 0x30553327, 0x373AD11B, 0x2508D42D };
	static constexpr uint32_t key19[4] =	{ 0xF681070F, 0x7CC61C15, 0x250DCC27, 0xAD03A629 };
	static constexpr uint32_t key20[4] =	{ 0xFAFB3A93, 0xE1A05683, 0x3033D9F3, 0x6A0A0957 };
	static constexpr uint32_t key36[4] =	{ 0x58200C4F, 0x3A9525DF, 0xD6F9DE43, 0xB35F6009 };
	static constexpr uint32_t key255[4] =	{ 0x5F441D91, 0x2CA91F81, 0xAD06E11D, 0x6DDA44D5 };

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

