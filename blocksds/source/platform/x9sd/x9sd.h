/*
    Ninjapass X9
    Card IO routines

    SPDX-License-Identifier: Zlib
    SPDX-FileContributor: lifehackerhansol, 2025
*/

#pragma once

#include <libtwl/card/card.h>
#include <nds/ndstypes.h>

#ifndef NULL
#define NULL 0
#endif

// X9SD MCCNT1 flags

#define X9SD_CTRL_BASE                                                 \
    (MCCNT1_RESET_OFF | MCCNT1_CMD_SCRAMBLE | MCCNT1_CLOCK_SCRAMBLER | \
     MCCNT1_READ_DATA_DESCRAMBLE | MCCNT1_LATENCY2(24))
#define X9SD_CTRL_POLL (X9SD_CTRL_BASE | MCCNT1_LEN_0)
#define X9SD_CTRL_READ_4B (X9SD_CTRL_BASE | MCCNT1_LEN_4)
#define X9SD_CTRL_READ_512B (X9SD_CTRL_BASE | MCCNT1_LEN_512)
#define X9SD_CTRL_READ_1KB (X9SD_CTRL_BASE | MCCNT1_LEN_1024)
#define X9SD_CTRL_WRITE_512B (X9SD_CTRL_BASE | MCCNT1_DIR_WRITE | MCCNT1_LEN_512)

// Explicitly used for the 0x63 command.
// No clue what it does.
#define X9SD_CTRL_READ_512B_DELAY (X9SD_CTRL_BASE | MCCNT1_LATENCY_CLK | MCCNT1_LATENCY1(1018))

// X9SD commands
// TODO: what are the unknown commands?

static inline u64 X9SD_CMD_58(u8 parameter) {
    return (0x58ull << 56) | ((2ull | (u64)parameter) << 8);
}

static inline u64 X9SD_CMD_5C(u32 parameter) {
    return (0x5Cull << 56) | ((u64)parameter << 24);
}

static inline u64 X9SD_CMD_5D(u32 parameter) {
    return (0x5Dull << 56) | ((u64)parameter << 24);
}

static inline u64 X9SD_CMD_SDIO(u8 cmd, u32 parameter) {
    return (0x60ull << 56) | ((u64)parameter << 24) | ((u64)cmd);
}

static inline u64 X9SD_CMD_CARD_DATA_READ(u32 address) {
    return (0x62ull << 56) | ((u64)address << 24);
}

#define X9SD_CMD_63 (0x63ull << 56)

static inline u64 X9SD_CMD_CARD_DATA_WRITE(u32 address) {
    return (0x64ull << 56) | ((u64)address << 24);
}

// user API
void X9SD_Initialize(void);
void X9SD_SDReadSector(u32 sector, u8* buffer);
void X9SD_SDWriteSector(u32 sector, const void* buffer);
