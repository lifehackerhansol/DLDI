/*
    AceKard+
    Card IO routines

    SPDX-License-Identifier: Zlib
    SPDX-FileContributor: lifehackerhansol, 2025
*/

#include <libcart/sdcrc16.h>
#include <libtwl/card/card.h>
#include <nds/ndstypes.h>

#define ACEK_CTRL_ROM_BASE                                                                  \
    (MCCNT1_RESET_OFF | MCCNT1_CLK_4_2_MHZ | MCCNT1_CMD_SCRAMBLE | MCCNT1_CLOCK_SCRAMBLER | \
     MCCNT1_READ_DATA_DESCRAMBLE | MCCNT1_LATENCY2(0) | MCCNT1_LATENCY1(0))
#define ACEK_CTRL_ROM_POLL (ACEK_CTRL_ROM_BASE | MCCNT1_LEN_0)
#define ACEK_CTRL_ROM_READ_4B (ACEK_CTRL_ROM_BASE | MCCNT1_LEN_4)
#define ACEK_CTRL_ROM_READ_512B (ACEK_CTRL_ROM_BASE | MCCNT1_LEN_512)

#define ACEK_CTRL_SD_BASE                                              \
    (MCCNT1_RESET_OFF | MCCNT1_CMD_SCRAMBLE | MCCNT1_CLOCK_SCRAMBLER | \
     MCCNT1_READ_DATA_DESCRAMBLE | MCCNT1_LATENCY2(0) | MCCNT1_LATENCY1(0))
#define ACEK_CTRL_SD_POLL (ACEK_CTRL_SD_BASE | MCCNT1_LEN_0)
#define ACEK_CTRL_SD_READ_4B (ACEK_CTRL_ROM_BASE | MCCNT1_LEN_4)

#define ACEK_CMD_NULL (0xFFFFFFFFFFFFFFFFull)
#define ACEK_CMD_DIRECT_SD_MODE (0xD3ull << 56)
#define ACEK_CMD_NORMAL_SD_MODE (0x7F7F7F7F7F7F7F7Full)
#define ACEK_CMD_SDIO_BUSY (0xD5ull << 56)

enum ACEK_Slot1Mode { ACEK_SLOT1MODE_FLASH = 0ull, ACEK_SLOT1MODE_SD = 0x80ull };

static inline u64 ACEK_CMD_SLOT1_MODE_SET(u8 mode) {
    return (0xD1ull << 56) | ((u64)mode << 48);
}

static inline u64 ACEK_CMD_ROM_OFFSET_SET(u32 offset) {
    return (0xD0ull << 56) | ((u64)offset << 24);
}

static inline u64 ACEK_CMD_CARD_READ_DATA(u32 address) {
    return (0xB7ull << 56) | ((u64)address << 24);
}

static inline u64 ACEK_CMD_SDIO_CMD_BYTE(u8 data) {
    /* Spread lower 8 bits into 64 bits */
    /* x =     **** **** **** **** **** **** abcd efgh */
    /* result: 000a 000b 000c 000d 000e 000f 000g 000h */
    u64 x = ((u64)data & 0x55ull) * 0x0002040810204081ull;
    x |= ((u64)data & 0xAAull) * 0x0002040810204081ull;
    x &= 0x0101010101010101ull;

    // Shift 4 to place in correct position for ACEK
    x <<= 4;
    // And add our magic for ACEK
    x |= 0xAFAFAFAFAFAFAFAFull;
    return x;
}

static inline u64 ACEK_CMD_TRANSFER_DATA(const u8* data) {
    u64 x = ((u64)data[3]) | (((u64)data[2]) << 8) | (((u64)data[1]) << 16) |
            (((u64)data[0]) << 24);

    /* Spread lower 8 bytes into 16 halfbytes */
    /* x =     **** **** **** **** abcd efgh ijkl mnop */
    /* result: 00ab 00cd 00ef 00gh 00ij 00kl 00mn 00op */
    x = (x | (x << 16)) & 0x0000FFFF0000FFFFull;
    x = (x | (x << 8)) & 0x00FF00FF00FF00FFull;
    x = (x | (x << 4)) & 0x0F0F0F0F0F0F0F0Full;

    return (x | 0xD0D0D0D0D0D0D0D0ull);
}

#define ACEK_CMD_FINISH_TRANSFER_DATA 0xDFFFFFFFFFFFFFFFull

// user API
void ACEK_SDInitialize(void);
void ACEK_SDReadSector(u32 sector, void* buffer);
void ACEK_SDWriteSector(u32 sector, const u8* buffer);
