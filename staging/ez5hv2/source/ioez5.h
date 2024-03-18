/*
    EZ-Flash V
    Card IO routines

    Copyright (C) 2007 Michael Chisholm (Chishm)
    Copyright (C) 2007 SaTa
    Copyright (C) 2023 lifehackerhansol

    SPDX-License-Identifier: Zlib
*/

#pragma once

#include <nds/ndstypes.h>

#include "libtwl_card.h"
#include "ioez5.h"

#ifndef NULL
 #define NULL 0
#endif

// EZ5 defines
// EZ5 ROMCTRL flags
#define IOEZ5_CTRL_BASE      (MCCNT1_ENABLE | MCCNT1_RESET_OFF | MCCNT1_CMD_SCRAMBLE | MCCNT1_CLOCK_SCRAMBLER | MCCNT1_READ_DATA_DESCRAMBLE | MCCNT1_LATENCY2(24) | MCCNT1_LATENCY1(0))
#define IOEZ5_CTRL_READ_0    (IOEZ5_CTRL_BASE | MCCNT1_LEN_0)
#define IOEZ5_CTRL_READ_4B   (IOEZ5_CTRL_BASE | MCCNT1_LEN_4)
#define IOEZ5_CTRL_READ_512B (IOEZ5_CTRL_BASE | MCCNT1_LEN_512)

// EZ5 CARD_COMMANDs
#define IOEZ5_CMD_SRAM             (0xB700000000000000ull)
#define IOEZ5_CMD_SDMC             (0xB8F0000000000000ull)

#define IOEZ5_CMD_SD_READ_DATA     (IOEZ5_CMD_SDMC | (0x7ull << 48))

static inline u64 IOEZ5_CMD_SDIO(u8 cmd, u32 parameter)
{
    u64 command = IOEZ5_CMD_SDMC | (0xAull << 48);
    command |= (u64)(cmd | 0x40) << 32;
    command |= (u64)parameter;
    return command;
}

// Read back the SDIO response.
// 48-bit SDIO responses are 6 bytes, 136-bit SDIO responses are 17 bytes.
// responseByte is to specify which byte to retrieve.
static inline u64 IOEZ5_CMD_READ_RESPONSE(u8 responseByte)
{
    u64 command = IOEZ5_CMD_SDMC | (0xAull << 48);
    command |= (u64)responseByte << 40;
    return command;
}

static inline u64 IOEZ5_CMD_WAIT_BUSY()
{
    return IOEZ5_CMD_READ_RESPONSE(1);
}

static inline u64 IOEZ5_CMD_WAIT_WRITE_BUSY()
{
    return IOEZ5_CMD_SDMC | (0x8ull << 48);
}

static inline u64 IOEZ5_CMD_SDIO_READ_SINGLE_BLOCK(u32 sector)
{
    return IOEZ5_CMD_SDIO(17, sector);
}

static inline u64 IOEZ5_CMD_SDIO_WRITE_SINGLE_BLOCK(u32 sector)
{
	return IOEZ5_CMD_SDIO(24, sector);
}

#define IOEZ5_CMD_SDMC_WRITE_DATA_START (IOEZ5_CMD_SDMC | (0x6ull << 48) | 0xFFFFFFF0ull << 16)

static inline u64 IOEZ5_CMD_SDMC_WRITE_DATA(const u8 *data)
{
    u64 command = IOEZ5_CMD_SDMC | (0x6ull << 48);
    command |= ((u64)((data[0] >> 4) | 0xF0) << 40)
             | ((u64)((data[0]) | 0xF0) << 32)
             | ((u64)((data[1] >> 4) | 0xF0) << 24)
             | ((u64)((data[1]) | 0xF0) << 16);
	return command;
}

// user API
bool ioEZ5_SDReadSector(u32 sector, void *buffer);
bool ioEZ5_SDWriteSector(u32 sector, const u8 *buffer);