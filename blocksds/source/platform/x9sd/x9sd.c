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

extern void waitByLoop(u32 count);

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

// Waits for a non-0xF0 response from the card.
// This is only used in the sector read function.
// If the transfer length is finished without a non-0xF0 response, return false.
static bool X9SD_CardWaitDataReady(u64 command, u32 flags) {
    // Reset the card read state machine.
    X9SD_CardReadIndex = 4;

    // Start transfer.
    card_romSetCmd(command);
    card_romStartXfer(flags, false);
    do {
        if ((X9SD_CardRead1Byte() & 0xF0) == 0) return true;
    } while (card_romIsBusy());
    return false;
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
    // This might start the transfer immediately, so we do things a little differently. Data is in
    // half bytes. After a stream of 0xF0 bytes, and a single non-0xF0 byte, we have our real data.
    // Start looking
    bool data_ready = X9SD_CardWaitDataReady(X9SD_CMD_SDIO(17, sector), X9SD_CTRL_READ_512B);

    // If we didn't get any data yet, keep reading
    // We assume that if we didn't get any data, the previous 512 byte transfer completed
    while (!data_ready)
        data_ready = X9SD_CardWaitDataReady(X9SD_CMD_CARD_DATA_READ(sector), X9SD_CTRL_READ_512B);

    // Start tracking number of halfbytes
    u32 dataIdx = 0;

    // Scratch buffer for packing halfbytes to full bytes
    u8 scratch = 0;

    // Start reading our sector.
    // 0x200 byte sector == 0x400 halfbytes
    while (dataIdx < 0x400) {
        // Start a new transfer if the previous transfer has ended.
        if (!card_romIsBusy()) {
            card_romSetCmd(X9SD_CMD_CARD_DATA_READ(sector));
            card_romStartXfer(X9SD_CTRL_READ_1KB, false);
        }

        // Write to buffer.
        scratch |= ((X9SD_CardRead1Byte() & 0xF0) >> (dataIdx & 1 ? 4 : 0));
        if (dataIdx++ & 1) {
            *buffer++ = scratch;
            // reset scratch.
            scratch = 0;
        }
    }

    // Reset the card read state machine.
    X9SD_CardReadIndex = 4;

    // At this point, the console might still be trying to poll data, as we didn't get our data in
    // 512 byte aligned blocks. If so, we need to flush the transfer.
    while (card_romIsBusy()) {
        // Read data if available
        if (card_romIsDataReady()) {
            register u32 data = card_romGetData();
        }
    }
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
