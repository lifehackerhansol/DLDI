/*
    AceKard+
    Card IO routines

    SPDX-License-Identifier: Zlib
    SPDX-FileContributor: lifehackerhansol, 2025
*/

#include <libtwl/card/card.h>
#include <nds/ndstypes.h>

#define ACEK_CTRL_BASE (MCCNT1_RESET_OFF | MCCNT1_CLK_4_2_MHZ | MCCNT1_CMD_SCRAMBLE | MCCNT1_CLOCK_SCRAMBLER | MCCNT1_READ_DATA_DESCRAMBLE | MCCNT1_LATENCY2(0) | MCCNT1_LATENCY1(0))
#define ACEK_CTRL_POLL (ACEK_CTRL_BASE | MCCNT1_LEN_0)
#define ACEK_CTRL_READ_4B (ACEK_CTRL_BASE | MCCNT1_LEN_4)
#define ACEK_CTRL_READ_512B (ACEK_CTRL_BASE | MCCNT1_LEN_512)
#define ACEK_CTRL_SD_READ_512B (ACEK_CTRL_BASE | MCCNT1_LATENCY1(0x1000))

#define ACEK_CMD_NULL (0xFFFFFFFFFFFFFFFFull);
#define ACEK_CMD_DIRECT_SD_MODE (0xD3ull << 56)
#define ACEK_CMD_NORMAL_SD_MODE (0x7F7F7F7F7F7F7F7Full)
#define ACEK_CMD_SDIO_BUSY (0xD5ull << 56)

enum ACEK_Slot1Mode {
	ACEK_SLOT1MODE_FLASH = 0ull,
	ACEK_SLOT1MODE_SD = 0x80ull
};

static inline u64 ACEK_CMD_SLOT1_MODE_SET(u8 mode)
{
    return (0xD1ull << 56) | ((u64)mode << 48);
}

static inline u64 ACEK_CMD_ROM_OFFSET_SET(u32 offset)
{
    return (0xD0ull << 56) | ((u64)offset << 24);
}

static inline u64 ACEK_CMD_CARD_READ_DATA(u32 address) {
    return (0xB7ull << 56) | ((u64)address << 24);
}

// user API
void ACEK_SDInitialize(void);
void ACEK_SDReadSector(u32 sector, void* buffer);
void ACEK_SDWriteSector(u32 sector, const void* buffer);
