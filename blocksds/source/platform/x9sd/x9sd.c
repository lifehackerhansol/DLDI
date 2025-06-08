/*
    Ninjapass X9
    Card IO routines

    SPDX-License-Identifier: Zlib
    SPDX-FileContributor: lifehackerhansol, 2025
*/

#include <common/libtwl_ext.h>
#include <libtwl/card/card.h>
#include <nds/ndstypes.h>

#include "x9sd.h"

// Scratch buffer.
// Because this cart sucks and we need to pull entire 512 byte chunks.
// Make sure it's aligned.
static ALIGN(4) u8 scratch[0x800];

// Sends an SDIO command.
static void X9SD_SDSendCommand(u8 cmd, u32 parameter, void* buffer, u32 num_words) {
    cardExt_ReadData(X9SD_CMD_SDIO(cmd, parameter), X9SD_CTRL_READ_512B, buffer, num_words);
}

// TODO: what is this?
static void X9SD_CardSendCommand58(u8 parameter) {
    cardExt_SendCommand(X9SD_CMD_58(parameter), X9SD_CTRL_POLL);
}

// TODO: what is this?
static void X9SD_CardSendCommand5C(u32 parameter) {
    cardExt_SendCommand(X9SD_CMD_5C(parameter), X9SD_CTRL_POLL);
}

// TODO: what is this?
static void X9SD_CardSendCommand5D(u32 parameter) {
    cardExt_SendCommand(X9SD_CMD_5D(parameter), X9SD_CTRL_POLL);
}

// TODO: what is this?
static void X9SD_CardSendCommand63(void) {
    cardExt_ReadData(X9SD_CMD_63, X9SD_CTRL_READ_512B_DELAY, NULL, 0);
}

// Reads from the card, one byte at a time.
// A command should be started prior to this function, and the card should be busy.
static int X9SD_CardReadIndex = 4;
static u8 X9SD_CardRead1Byte(void) {
    static u32 data;
    if (X9SD_CardReadIndex == 4) {
        X9SD_CardReadIndex = 0;
        card_romWaitDataReady();
        data = card_romGetData();
    }
    return ((u8*)&data)[X9SD_CardReadIndex++];
}

// Initializes the card to a usable state for SD access.
void X9SD_Initialize(void) {
    X9SD_CardSendCommand58(0);
    X9SD_CardSendCommand5C(0);

    // It is possible to do a full SDIO init here.
    // However, since X9 only supports non-SDHC, and the bootloader already performs it, it's
    // pointless.
}

// Reads a block from the SD card.
void X9SD_SDReadSector(u32 sector, u8* buffer) {
    // CMD17 - Read single block
    // This might start the transfer immediately, so save everything to the scratch buffer.
    // Data is in half bytes. After a stream of 0xF0 bytes, and a single non-0xF0 byte, we have our
    // real data. Start looking
    cardExt_ReadData(X9SD_CMD_SDIO(17, sector), X9SD_CTRL_READ_512B, scratch, 128);

    // The data start marker is a 0xF000 pattern (checking upper halfbyte only.)
    // Everything before this pattern is a stream of 0xF0 bytes.
    // Did we get it right away?
    bool data_ready = false;
    int i = 0;
    // This will break when we see a non-0xF0 byte. If we get this, we have our data.
    for (i = 0; i < 512; i++) {
        if (!(scratch[i] & 0xF0)) {
            // We found our data. Let's move on
            data_ready = true;
            break;
        }
    }

    // If we looped through all 512 bytes... we didn't get it.
    // Let's keep polling until we find it
    while (!data_ready) {
        cardExt_ReadData(X9SD_CMD_CARD_DATA_READ(sector), X9SD_CTRL_READ_512B, scratch, 128);
        for (i = 0; i < 512; i++) {
            if (!(scratch[i] & 0xF0)) {
                // We found our data. Let's move on
                data_ready = true;
                break;
            }
        }
    }

    // The previous for loop should have stopped at an 0x00 byte.
    // We need to go one more byte from here, which is where our data stream starts.
    i++;

    // We probably didn't read our entire block yet. Let's pull the whole thing down in one go
    // Read above the last 512 byte that we received
    cardExt_ReadData(X9SD_CMD_CARD_DATA_READ(sector), X9SD_CTRL_READ_1KB, scratch + 0x200, 256);

    // Pack our data into bytes, and write it to the buffer.
    for (int j = 0; j < 0x400; j += 2)
        *buffer++ = (scratch[i + j] & 0xF0) | ((scratch[i + j + 1] & 0xF0) >> 4);

    // Seems we also need to do one last dummy poll...
    cardExt_ReadData(X9SD_CMD_CARD_DATA_READ(sector), X9SD_CTRL_READ_512B, NULL, 0);
}

void X9SD_SDWriteSector(u32 sector, const void* buffer) {
    // It is possible to submit the data beforehand.
    // This way the X9 can compute CRC right away.
    cardExt_WriteData(X9SD_CMD_CARD_DATA_WRITE(0), X9SD_CTRL_WRITE_512B, buffer, 128);

    // CMD24 - Write single block
    X9SD_SDSendCommand(24, sector, NULL, 0);

    // ????
    X9SD_CardSendCommand63();

    // Seems to wait until disk flush starts?
    bool ready = false;
    // Reset the card read state machine.
    X9SD_CardReadIndex = 4;
    while (!ready) {
        card_romSetCmd(X9SD_CMD_CARD_DATA_READ(0));
        card_romStartXfer(X9SD_CTRL_READ_512B, false);
        do {
            if ((X9SD_CardRead1Byte() & 0x10) == 0) ready = true;
        } while (card_romIsBusy());
    }

    // Seems to wait until disk flush ends?
    ready = false;
    // Reset the card read state machine.
    X9SD_CardReadIndex = 4;
    while (!ready) {
        card_romSetCmd(X9SD_CMD_CARD_DATA_READ(0));
        card_romStartXfer(X9SD_CTRL_READ_512B, false);
        do {
            if ((X9SD_CardRead1Byte() & 0x10) == 0x10) ready = true;
        } while (card_romIsBusy());
    }
}
