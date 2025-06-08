/*
    AceKard+
    Card IO routines

    SPDX-License-Identifier: Zlib
    SPDX-FileContributor: lifehackerhansol, 2025
*/

#include <common/libtwl_ext.h>
#include <libtwl/card/card.h>
#include <nds/ndstypes.h>

#include "acek.h"

// Switches between access to SD or flash.
void ACEK_CardSetSlot1Mode(u8 mode) {
    cardExt_SendCommand(ACEK_CMD_SLOT1_MODE_SET(mode), ACEK_CTRL_POLL);
}

// Offsets the address where B7 command starts reading (seemingly).
// Acekard makes use of this when loading SRLs; if the target ROM is not fragmented it reads straight from SD.
void ACEK_CardSetROMOffset(u32 offset, bool isFlashMode) {
    if (isFlashMode)
        offset -= 7;
    cardExt_SendCommand(ACEK_CMD_ROM_OFFSET_SET(offset), ACEK_CTRL_POLL);
}

// Switches to direct mode.
// In this mode the SDIO is exposed directly to the card interface.
void ACEK_SDSetModeDirect(void) {
    cardExt_SendCommand(ACEK_CMD_DIRECT_SD_MODE, ACEK_CTRL_POLL);
    cardExt_SendCommand(ACEK_CMD_NULL, ACEK_CTRL_POLL);
}

// Switches to normal mode.
// In this mode normal card interface functions apply.
void ACEK_SDSetModeNormal(void) {
    cardExt_SendCommand(ACEK_CMD_NORMAL_SD_MODE, ACEK_CTRL_POLL);
}

void ACEK_SDInitialize(void) {
    ACEK_CardSetSlot1Mode(ACEK_SLOT1MODE_SD);
    ACEK_CardSetROMOffset(0, false);
}

void ACEK_SDReadSector(u32 sector, void * buffer)
{
    cardExt_ReadData(ACEK_CMD_CARD_READ_DATA(sector << 9), ACEK_CTRL_SD_READ_512B, buffer, 128);
}

void ACEK_SDWriteSector(u32 sector, void * buffer)
{
    return;
}