/*
	SuperCard DSTWO
	Card IO routines

	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: Zlib
*/

#pragma once

#include <nds/ndstypes.h>

#ifndef NULL
    #define NULL 0
#endif

// IODS2 defines
// IODS2 MCCNT1 flags
#define IODS2_CTRL_CARD_PARAM    (MCCNT1_ENABLE | MCCNT1_RESET_OFF | MCCNT1_LATENCY2(24))
// SD parameters need higher latency
#define IODS2_CTRL_SD_READ_PARAM (IODS2_CTRL_CARD_PARAM | MCCNT1_LATENCY1(24))
#define IODS2_CTRL_READ_4B       (IODS2_CTRL_SD_READ_PARAM | MCCNT1_LEN_4)
#define IODS2_CTRL_READ_512B     (IODS2_CTRL_SD_READ_PARAM | MCCNT1_LEN_512)
// and writing to SD needs even higher latency
#define IODS2_CTRL_WRITE_512B    (IODS2_CTRL_CARD_PARAM | MCCNT1_DIR_WRITE | MCCNT1_LEN_512 | MCCNT1_LATENCY1(48))

// IODS2 MCCMDs
#define IODS2_CMD_CARD_RESET (0xE1ull << 56)
// Some sort of response testing?
// Must return 0xA5A55A5A
// note this uses CTRL_SD_READ_PARAM instead with higher latency
#define IODS2_CMD_SD_INIT   (0x735A5AA5A5ull << 24)

// idle: (ret & 0x100 == 0)
#define IODS2_CMD_CARD_BUSY         (0xE0ull << 56)

static inline u64 IODS2_CMD_SD_READ_SINGLE_BLOCK(u32 sector)
{
    return (0x30ull << 56) | ((u64)sector << 24) | 1ull;
}

static inline u64 IODS2_CMD_SD_READ_MULTI_BLOCK(u32 sector, u32 num_sectors)
{
    return (0x31ull << 56) | ((u64)sector << 24) | ((u64)num_sectors & 0xFFFFFF);
}

static inline u64 IODS2_CMD_SD_WRITE_SINGLE_BLOCK(u32 sector)
{
    return (0x35ull << 56) | ((u64)sector << 24) | 1ull;
}

static inline u64 IODS2_CMD_SD_WRITE_MULTI_BLOCK(u32 sector, u32 num_sectors)
{
    return (0x36ull << 56) | ((u64)sector << 24) | ((u64)num_sectors & 0xFFFFFF);
}

// user API
void ioDS2_Initialize(void);
void ioDS2_SDReadSingleSector(u32 sector, void *buffer);
void ioDS2_SDReadMultiSector(u32 sector, void *buffer, u32 num_sectors);
void ioDS2_SDWriteSingleSector(u32 sector, const void *buffer);
void ioDS2_SDWriteMultiSector(u32 sector, const void *buffer, u32 num_sectors);
