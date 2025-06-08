// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <iointerface.h>
#include <stdbool.h>
#include <stdint.h>

#include "acek.h"

#define BYTES_PER_READ 512

// Initialize the driver. Returns true on success.
bool ACEK_Startup(void) {
    ACEK_SDInitialize();
    return true;
}

// Returns true if a card is present and initialized.
bool ACEK_IsInserted(void) {
    return true;
}

// Clear error flags from the card. Returns true on success.
bool ACEK_ClearStatus(void) {
    return true;
}

// Reads 512 byte sectors into a buffer that may be unaligned. Returns true on
// success.
bool ACEK_ReadSectors(uint32_t sector, uint32_t num_sectors, void* buffer) {
    for(int i=0; i < num_sectors; i++) {
        ACEK_SDReadSector(sector + i, buffer);
        buffer = (u8*)buffer + 0x200;
    }
    return true;
}

// Writes 512 byte sectors from a buffer that may be unaligned. Returns true on
// success.
bool ACEK_WriteSectors(uint32_t sector, uint32_t num_sectors, const void* buffer) {
    return false;
}

// Shutdowns the card. This may never be called.
bool ACEK_Shutdown(void) {
    return true;
}

#ifdef PLATFORM_acek

disc_interface_t ioInterface = {.startup = ACEK_Startup,
                                .is_inserted = ACEK_IsInserted,
                                .read_sectors = ACEK_ReadSectors,
                                .write_sectors = ACEK_WriteSectors,
                                .clear_status = ACEK_ClearStatus,
                                .shutdown = ACEK_Shutdown};

#endif
