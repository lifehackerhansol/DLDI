/*
    iPlayer
    Card IO routines

    SPDX-License-Identifier: Zlib
    SPDX-FileContributor: lifehackerhansol, 2025
*/

#include <common/libtwl_ext.h>
#include <libtwl/card/card.h>
#include <nds/ndstypes.h>

#include "iply.h"

void IPLY_SDReadSector(u32 sector, void* buffer) {
    // Initiate an operation
    cardExt_ReadData4Byte(IPLY_CMD_SD_BEGIN_OPERATION, IPLY_CTRL_READ_4B);

    // Initiate a sector read
    cardExt_ReadData4Byte(IPLY_CMD_SD_READ_SECTOR(sector), IPLY_CTRL_READ_4B);

    // Wait for data ready
    while (cardExt_ReadData4Byte(IPLY_CMD_CARD_STATE, IPLY_CTRL_READ_4B) & BIT(28));

    // retrieve data
    cardExt_ReadData(IPLY_CMD_SD_READ_DATA, IPLY_CTRL_READ_512B, buffer, 128);
}

void IPLY_SDWriteSector(u32 sector, const void* buffer) {
    // Initiate an operation
    cardExt_ReadData4Byte(IPLY_CMD_SD_BEGIN_OPERATION, IPLY_CTRL_READ_4B);

    // Write to buffer
    cardExt_WriteData(IPLY_CMD_SD_WRITE_BUFFER, IPLY_CTRL_WRITE_512B, buffer, 128);

    // Flush the buffer
    cardExt_ReadData4Byte(IPLY_CMD_SD_FLUSH_BUFFER(sector), IPLY_CTRL_READ_4B);

    // Wait for completion
    while (cardExt_ReadData4Byte(IPLY_CMD_CARD_STATE, IPLY_CTRL_READ_4B) & BIT(21));
}
