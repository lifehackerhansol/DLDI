/*
    AceKard+
    Card IO routines

    SPDX-License-Identifier: Zlib
    SPDX-FileContributor: lifehackerhansol, 2025
*/

#include <common/libtwl_ext.h>
#include <common/sdio.h>
#include <libcart/sdcrc16.h>
#include <libcart/sdcrc7.h>
#include <libtwl/card/card.h>
#include <nds/ndstypes.h>

#include "acek.h"

// Switches between access to SD or flash.
static void ACEK_CardSetSlot1Mode(u8 mode) {
    cardExt_RomSendCommand(ACEK_CMD_SLOT1_MODE_SET(mode), ACEK_CTRL_ROM_POLL);
}

// Offsets the address where B7 command starts reading (seemingly).
// Acekard makes use of this when loading SRLs; if the target ROM is not fragmented it reads
// straight from SD.
static void ACEK_CardSetROMOffset(u32 offset, bool isFlashMode) {
    if (isFlashMode) offset -= 7;
    cardExt_RomSendCommand(ACEK_CMD_ROM_OFFSET_SET(offset), ACEK_CTRL_ROM_POLL);
}

// Switches to direct mode.
// In this mode the SDIO is exposed directly to the card interface.
static void ACEK_SDSetModeDirect(void) {
    cardExt_RomSendCommand(ACEK_CMD_DIRECT_SD_MODE, ACEK_CTRL_ROM_POLL);
    cardExt_RomSendCommand(ACEK_CMD_NULL, ACEK_CTRL_ROM_POLL);
}

// Switches to normal mode.
// In this mode normal card interface functions apply.
static void ACEK_SDSetModeNormal(void) {
    cardExt_RomSendCommand(ACEK_CMD_NORMAL_SD_MODE, ACEK_CTRL_ROM_POLL);
}

static void ACEK_SDSendClock(u32 count) {
    for (u32 i = 0; i < count; i += 8) cardExt_RomSendCommand(ACEK_CMD_NULL, ACEK_CTRL_SD_POLL);
}

static bool ACEK_SDIsBusy(void) {
    return !(cardExt_RomReadData4Byte(ACEK_CMD_SDIO_BUSY, ACEK_CTRL_SD_READ_4B) & 0x80808080);
}

static void ACEK_SDSendSDIOCommand(u8 cmd, u32 parameter) {
    u8 sdcmd[6];
    sdcmd[0] = cmd | 0x40;
    sdcmd[1] = (parameter >> 24) & 0xFF;
    sdcmd[2] = (parameter >> 16) & 0xFF;
    sdcmd[3] = (parameter >> 8) & 0xFF;
    sdcmd[4] = parameter & 0xFF;
    sdcmd[5] = __sd_crc7(sdcmd);

    for (u32 i = 0; i < 6; i++) {
        cardExt_RomSendCommand(ACEK_CMD_SDIO_CMD_BYTE(sdcmd[i]), ACEK_CTRL_SD_POLL);
    }
    ACEK_SDSendClock(128);
}

void ACEK_SDInitialize(void) {
    ACEK_CardSetSlot1Mode(ACEK_SLOT1MODE_SD);
    ACEK_CardSetROMOffset(0, false);
}

void ACEK_SDReadSector(u32 sector, void* buffer) {
    cardExt_RomReadData(ACEK_CMD_CARD_READ_DATA(sector << 9),
                        ACEK_CTRL_ROM_READ_512B | MCCNT1_LATENCY1(0x1000), buffer, 128);
}

void ACEK_SDWriteSector(u32 sector, const u8* buffer) {
    u64 crc16 = __sd_crc16(buffer);
    sector <<= 9;
    ACEK_SDSetModeDirect();
    ACEK_SDSendSDIOCommand(SDIO_CMD24_WRITE_SINGLE_BLOCK, sector);

    // Send starting token, 0xFFFFFFF0
    // We preemptively bswap this to pass to TRANSFER_DATA as an array
    const u32 token = 0xF0FFFFFF;
    cardExt_RomSendCommand(ACEK_CMD_TRANSFER_DATA((u8*)&token), ACEK_CTRL_SD_POLL);
    for (u32 i = 0; i < 512; i += 4) {
        cardExt_RomSendCommand(ACEK_CMD_TRANSFER_DATA(buffer + i), ACEK_CTRL_SD_POLL);
    }
    for (u32 i = 0; i < 8; i += 4) {
        cardExt_RomSendCommand(ACEK_CMD_TRANSFER_DATA((u8*)&crc16 + i), ACEK_CTRL_SD_POLL);
    }
    // Send end token
    cardExt_RomSendCommand(ACEK_CMD_FINISH_TRANSFER_DATA, ACEK_CTRL_SD_POLL);
    ACEK_SDSetModeNormal();
    while (ACEK_SDIsBusy());
}
