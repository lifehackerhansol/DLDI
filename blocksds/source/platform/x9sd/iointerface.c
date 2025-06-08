// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <iointerface.h>
#include <stdbool.h>
#include <stdint.h>

#include "x9sd.h"

#define BYTES_PER_READ 512

// Initialize the driver. Returns true on success.
bool X9SD_Startup(void) {
    X9SD_Initialize();
    return true;
}

// Returns true if a card is present and initialized.
bool X9SD_IsInserted(void) {
    return true;
}

// Clear error flags from the card. Returns true on success.
bool X9SD_ClearStatus(void) {
    return true;
}

// Reads 512 byte sectors into a buffer that may be unaligned. Returns true on
// success.
bool X9SD_ReadSectors(uint32_t sector, uint32_t num_sectors, void* buffer) {
    for (int i = 0; i < num_sectors; i++) {
        X9SD_SDReadSector((sector + i) << 9, buffer);
        buffer = (u8*)buffer + 0x200;
    }
    return true;
}

// Writes 512 byte sectors from a buffer that may be unaligned. Returns true on
// success.
bool X9SD_WriteSectors(uint32_t sector, uint32_t num_sectors, const void* buffer) {
    for (int i = 0; i < num_sectors; i++) {
        X9SD_SDWriteSector((sector + i) << 9, buffer);
        buffer = (u8*)buffer + 0x200;
    }
    return true;
}

// Shutdowns the card. This may never be called.
bool X9SD_Shutdown(void) {
    return true;
}

#ifdef PLATFORM_x9sd

disc_interface_t ioInterface = {.startup = X9SD_Startup,
                                .is_inserted = X9SD_IsInserted,
                                .read_sectors = X9SD_ReadSectors,
                                .write_sectors = X9SD_WriteSectors,
                                .clear_status = X9SD_ClearStatus,
                                .shutdown = X9SD_Shutdown};

#endif
