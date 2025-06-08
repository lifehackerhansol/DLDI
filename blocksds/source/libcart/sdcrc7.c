// clang-format off

// SPDX-License-Identifier: MIT-0
// SPDX-FileCopyrightText: 2022-2023 devwizard

#include <stdint.h>

uint8_t __sd_crc7(const uint8_t *src)
{
	int i;
	int n;
	int crc = 0;
	for (i = 0; i < 5; i++)
	{
		crc ^= src[i];
		for (n = 0; n < 8; n++)
		{
			if ((crc <<= 1) & 0x100) crc ^= 0x12;
		}
	}
	return (crc & 0xFE) | 1;
}
