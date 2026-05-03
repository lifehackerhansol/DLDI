/*
    EZ-Flash V
    Card IO routines

    Copyright (C) 2007 Michael Chisholm (Chishm)
    Copyright (C) 2007 SaTa
    Copyright (C) 2023 lifehackerhansol

    SPDX-License-Identifier: Zlib
*/

#include <common/libtwl_ext.h>
#include <common/sdio.h>
#include <libtwl/card/card.h>
#include <nds/ndstypes.h>

#include "ez5i.h"

static u32 isSDHC = 0;

static u32 EZ5I_SendCommand(const u64 command) {
    return cardExt_RomReadData4Byte(command, EZ5I_CTRL_READ_4B);
}

static bool EZ5I_SDSendSDIOCommand(u8 cmd, u32 parameter, u8* buffer, int size) {
    u32 data;
    u8* u8_data = (u8*)&data;
    int timeout = 99;

    // Either it's no response, 48 bits, or 136 bits.
    // If this isn't the case, then this function doesn't work.
    if (size != 0 && size != 6 && size != 17) return false;

    EZ5I_SendCommand(EZ5I_CMD_SDMC_SDIO(cmd, parameter));

    // R0 has no response.
    if (size == 0) return true;

    // Sends response in byte-swapped u32, with the starting marker
    // Search for starting marker, with a timeout
    do {
        data = EZ5I_SendCommand(EZ5I_CMD_SDMC_SEND_CLK(1));
        timeout--;
        if (!timeout) return false;
    } while (data & 0xFF);

    // Starting marker found. Start reading response
    if (buffer != NULL) {
        buffer[0] = u8_data[1];
        buffer[1] = u8_data[2];
        buffer[2] = u8_data[3];
    }

    // Read remaining data
    data = EZ5I_SendCommand(EZ5I_CMD_SDMC_SEND_CLK(2));
    if (buffer != NULL) {
        buffer[3] = u8_data[0];
        buffer[4] = u8_data[1];
        buffer[5] = u8_data[2];
        // if we're pulling an R1 response, then we have read all of our data here
        // otherwise keep going
        if (size != 6) return true;
        buffer[6] = u8_data[3];
    } else if (size == 6)
        return true;

    data = EZ5I_SendCommand(EZ5I_CMD_SDMC_SEND_CLK(3));
    if (buffer != NULL) {
        buffer[7] = u8_data[0];
        buffer[8] = u8_data[1];
        buffer[9] = u8_data[2];
        buffer[10] = u8_data[3];
    }
    data = EZ5I_SendCommand(EZ5I_CMD_SDMC_SEND_CLK(4));
    if (buffer != NULL) {
        buffer[11] = u8_data[0];
        buffer[12] = u8_data[1];
        buffer[13] = u8_data[2];
        buffer[14] = u8_data[3];
    }
    data = EZ5I_SendCommand(EZ5I_CMD_SDMC_SEND_CLK(5));
    if (buffer != NULL) {
        buffer[15] = u8_data[0];
        buffer[16] = u8_data[1];
    }

    return true;
}

// We write two words for each transfer
static void EZ5I_SDWriteData(const u32* buffer, u32 length) {
    // Send data start marker.
    u8 start_marker[2] = {0xFF, 0xF0};
    EZ5I_SendCommand(EZ5I_CMD_SDMC_WRITE_DATA(start_marker));

    // Transfer data.
    cardExt_RomSendCommand(EZ5I_CMD_BULK_TRANSFER_START, EZ5I_CTRL_READ_0);
    for (u32 i = 0; i < length; i += 2) {
        cardExt_RomReadData(EZ5I_CMD_TRANSFER_DATA(buffer + i), EZ5I_CTRL_READ_0, NULL, 0);
    }

    // End transfer.
    cardExt_RomSendCommand(EZ5I_CMD_BULK_TRANSFER_END, EZ5I_CTRL_READ_0);

    // Wait until CRC starts
    while (EZ5I_SendCommand(EZ5I_CMD_SDMC_SEND_CRC_STATUS) & 0x1);

    // Read CRC status
    EZ5I_SendCommand(EZ5I_CMD_SDMC_SEND_CRC_STATUS);
    while ((EZ5I_SendCommand(EZ5I_CMD_SDMC_SEND_CRC_STATUS) & 0x1) != 0x1);
}

u32 EZ5I_SRAMReadData(u32 address) {
    return EZ5I_SendCommand(EZ5I_CMD_SRAM_READ_DATA(address));
}

bool EZ5I_SDInitialize(void) {
    u8 response[17] = {};
    register bool isSD20 = false;

    // Does this flush something?
    // iSmart does this loop
    for (int i = 0; i < 128; i++)
        cardExt_RomReadData(EZ5I_CMD_SDMC_READ_DATA, EZ5I_CTRL_READ_512B, NULL, 0);
    EZ5I_SDSendSDIOCommand(SDIO_CMD0_GO_IDLE_STATE, 0, NULL, 0);
    // it does it twice
    for (int i = 0; i < 128; i++)
        cardExt_RomReadData(EZ5I_CMD_SDMC_READ_DATA, EZ5I_CTRL_READ_512B, NULL, 0);

    // CMD8 SDHC init
    if (EZ5I_SDSendSDIOCommand(SDIO_CMD8_SEND_IF_COND, 0x1AA, response, 6))
        if (response[3] == 1 && response[4] == 0xAA) isSD20 = true;

    do {
        EZ5I_SDSendSDIOCommand(SDIO_CMD55_APP_CMD, 0, NULL, 6);
        u32 parameter = 0x00800000;
        if (isSD20) parameter |= BIT(30);
        EZ5I_SDSendSDIOCommand(SDIO_ACMD41_SD_SEND_OP_COND, parameter, response, 6);
    } while (!(response[1] & 0x80));
    isSDHC = response[1] & 0x40 ? 1 : 0;

    EZ5I_SDSendSDIOCommand(SDIO_CMD2_ALL_SEND_CID, 0, NULL, 17);
    do {
        EZ5I_SDSendSDIOCommand(SDIO_CMD3_SEND_RELATIVE_ADDR, 0, response, 6);
    } while ((response[3] & 0x1E) != 6);  // is standby

    u32 sdio_rca = (response[1] << 8) + response[2];

    EZ5I_SDSendSDIOCommand(SDIO_CMD9_SEND_CSD, (sdio_rca << 16), NULL, 17);
    EZ5I_SDSendSDIOCommand(SDIO_CMD7_SELECT_CARD, (sdio_rca << 16), NULL, 6);
    EZ5I_SDSendSDIOCommand(SDIO_CMD55_APP_CMD, (sdio_rca << 16), NULL, 6);
    EZ5I_SDSendSDIOCommand(SDIO_ACMD6_SET_BUS_WIDTH, 2, NULL, 6);
    EZ5I_SDSendSDIOCommand(SDIO_CMD16_SET_BLOCK_LEN, 512, NULL, 6);
    return true;
}

bool EZ5I_SDReadSector(u32 sector, void* buffer) {
    if (!isSDHC) sector <<= 9;

    EZ5I_SDSendSDIOCommand(17, sector, NULL, 0);

    // Wait for the start marker.
    u32 data = 0;
    u32 timeout = 0x10000;
    do {
        data = EZ5I_SendCommand(EZ5I_CMD_SDMC_SEND_CLK(1));
        timeout--;
        if (!timeout) return false;
    } while (data & 0xFF);

    cardExt_RomReadData(EZ5I_CMD_SDMC_READ_DATA, EZ5I_CTRL_READ_512B, buffer, 128);
    return true;
}

bool EZ5I_SDWriteSingleSector(u32 sector, const void* buffer) {
    if (!isSDHC) sector <<= 9;

    // CMD24
    if (!EZ5I_SDSendSDIOCommand(24, sector, NULL, 6)) return false;

    // Write data.
    EZ5I_SDWriteData(buffer, 128);

    // Wait until card ready
    while (EZ5I_SendCommand(EZ5I_CMD_SDMC_SEND_CLK(1)) & 0xFF);

    return true;
}

bool EZ5I_SDWriteMultiSector(u32 sector, u32 num_sectors, const void* buffer) {
    if (!isSDHC) sector <<= 9;

    // CMD25
    if (!EZ5I_SDSendSDIOCommand(25, sector, NULL, 6)) return false;

    for (u32 i = 0; i < num_sectors; i++) {
        // Write data.
        EZ5I_SDWriteData(buffer, 128);
        buffer = (u8*)buffer + 0x200;
    }

    // CMD12
    EZ5I_SDSendSDIOCommand(12, sector, NULL, 6);

    while ((EZ5I_SendCommand(EZ5I_CMD_SDMC_SEND_CRC_STATUS) & 0x1) != 0x1);

    // Wait until card ready
    while (EZ5I_SendCommand(EZ5I_CMD_SDMC_SEND_CLK(1)) & 0xFF);
    return true;
}
