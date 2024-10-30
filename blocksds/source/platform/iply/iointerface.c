// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <iointerface.h>
#include <stdbool.h>
#include <stdint.h>

#include "iply.h"

#define BYTES_PER_READ 512

// Initialize the driver. Returns true on success.
bool IPLY_Startup(void) {
    return true;
}

// Returns true if a card is present and initialized.
bool IPLY_IsInserted(void) {
    return true;
}

// Clear error flags from the card. Returns true on success.
bool IPLY_ClearStatus(void) {
    return true;
}

// Reads 512 byte sectors into a buffer that may be unaligned. Returns true on
// success.
bool IPLY_ReadSectors(uint32_t sector, uint32_t num_sectors, void* buffer) {
    for (int i = 0; i < num_sectors; i++) {
        IPLY_SDReadSector(sector + i, buffer);
        buffer = (u8*)buffer + 0x200;
    }
    return true;
}

// Writes 512 byte sectors from a buffer that may be unaligned. Returns true on
// success.
bool IPLY_WriteSectors(uint32_t sector, uint32_t num_sectors, const void* buffer) {
    for (int i = 0; i < num_sectors; i++) {
        IPLY_SDWriteSector(sector + i, buffer);
        buffer = (u8*)buffer + 0x200;
    }
    return true;
}

// Shutdowns the card. This may never be called.
bool IPLY_Shutdown(void) {
    return true;
}

#ifdef PLATFORM_iply

disc_interface_t ioInterface = {.startup = IPLY_Startup,
                                .is_inserted = IPLY_IsInserted,
                                .read_sectors = IPLY_ReadSectors,
                                .write_sectors = IPLY_WriteSectors,
                                .clear_status = IPLY_ClearStatus,
                                .shutdown = IPLY_Shutdown};

#endif
