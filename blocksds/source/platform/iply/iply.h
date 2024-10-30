/*
    iPlayer
    Card IO routines

    SPDX-License-Identifier: Zlib
    SPDX-FileContributor: lifehackerhansol, 2025
*/

#pragma once

#include <nds/ndstypes.h>

#ifndef NULL
#define NULL 0
#endif

// iPlayer defines
// iPlayer MCCNT1 flags
#define IPLY_CTRL_BASE (MCCNT1_RESET_OFF | MCCNT1_LATENCY2(24) | MCCNT1_LEN_0 | MCCNT1_LATENCY1(16))
#define IPLY_CTRL_READ_4B (IPLY_CTRL_BASE | MCCNT1_LEN_4)
#define IPLY_CTRL_READ_512B (IPLY_CTRL_BASE | MCCNT1_LEN_512)
#define IPLY_CTRL_WRITE_512B (IPLY_CTRL_BASE | MCCNT1_DIR_WRITE | MCCNT1_LEN_512)

// iPlayer MCCMDs
#define IPLY_CMD_CARD_STATE (0xE000000000000000ull)
#define IPLY_CMD_SD_BEGIN_OPERATION (0xE100000000000000ull)
#define IPLY_CMD_SD_READ_DATA (0xE800000000000000ull)
#define IPLY_CMD_SD_WRITE_BUFFER (0xE900000000000000ull)

static inline u64 IPLY_CMD_SD_READ_SECTOR(u32 sector) {
    return (0x5000000000000200ull) | ((u64)sector << 24);
}

static inline u64 IPLY_CMD_SD_FLUSH_BUFFER(u32 sector) {
    return (0x5100000000000200ull) | ((u64)sector << 24);
}

// user API
void IPLY_SDReadSector(u32 sector, void* buffer);
void IPLY_SDWriteSector(u32 sector, const void* buffer);
