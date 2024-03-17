// SPDX-License-Identifier: MIT
// SPDX-FileNotice: Modified from upstream (https://github.com/profi200/dsi_sdmmc) at commit 968744452e69a6607ea74989413297b62d90ee96
//
// Copyright (c) 2023 profi200

#include <nds/ndstypes.h>

// Based on UNSTUFF_BITS from linux/drivers/mmc/core/sd.c.
// Extracts up to 32 bits from a u32[4] array.
static inline u32 extractBits(const u32 resp[4], const u32 start, const u32 size)
{
	const u32 mask = (size < 32 ? 1u<<size : 0u) - 1;
	const u32 off = 3 - (start / 32);
	const u32 shift = start & 31u;

	u32 res = resp[off]>>shift;
	if(size + shift > 32)
		res |= resp[off - 1]<<((32u - shift) & 31u);

	return res & mask;
}

// This reads the CSD response and calculates the sector count of the SD card.
// This function is entirely based on `parseCsd` function upstream,
// with MMC checks removed as the Acekard family of hardware does not support MMC.
u32 calculateSDSectorCount(u32 * csd)
{
	const u8 structure = extractBits(csd, 126, 2); // [127:126]
	u32 sectors = 0;
	switch(structure)
	{
        case 0:
        {
            const u32 read_bl_len = extractBits(csd, 80, 4);  // [83:80]
            const u32 c_size      = extractBits(csd, 62, 12); // [73:62]
            const u32 c_size_mult = extractBits(csd, 47, 3);  // [49:47]

            // For SD cards with CSD 1.0 and <=2 GB (e)MMC this calculation is used.
            // Note: READ_BL_LEN is at least 9.
            // Modified/simplified to calculate sectors instead of bytes.
            sectors = (c_size + 1)<<(c_size_mult + 2 + read_bl_len - 9);
            break;
        }
        case 1:
        {
            // SD CSD version 3.0 format.
            // For version 2.0 this is 22 bits however the upper bits
            // are reserved and zero filled so this is fine.
            const u32 c_size = extractBits(csd, 48, 28); // [75:48]

            // Calculation for SD cards with CSD >1.0.
            sectors = (c_size + 1)<<10;
            break;
        }
        default:
            break;
	}
    return sectors;
}
