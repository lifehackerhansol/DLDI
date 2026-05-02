// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <iointerface.h>
#include <stdbool.h>
#include <stdint.h>

#include "dsx.h"

#define BYTES_PER_READ 512

// Initialize the driver. Returns true on success.
bool DSX_Startup(void) {
    return true;
}

// Returns true if a card is present and initialized.
bool DSX_IsInserted(void) {
    return true;
}

// Clear error flags from the card. Returns true on success.
bool DSX_ClearStatus(void) {
    return true;
}

// Reads 512 byte sectors into a buffer that may be unaligned. Returns true on
// success.
bool DSX_ReadSectors(uint32_t sector, uint32_t num_sectors, void* buffer) {
    sector += 0x6000;
    for(u32 i = 0; i < num_sectors; i++)
    {
        DSX_NANDReadSector(sector++, buffer);
        buffer = (u8 *)buffer + 0x200;
    }
    return true;
}

// Writes 512 byte sectors from a buffer that may be unaligned. Returns true on
// success.
bool DSX_WriteSectors(uint32_t sector, uint32_t num_sectors, const void* buffer) {
    DSX_NANDWriteSectors(sector + 0x6000, num_sectors, buffer);
    return true;
}

// Shutdowns the card. This may never be called.
bool DSX_Shutdown(void) {
    return false;
}

#ifdef PLATFORM_dsx

disc_interface_t ioInterface = {.startup = DSX_Startup,
                                .is_inserted = DSX_IsInserted,
                                .read_sectors = DSX_ReadSectors,
                                .write_sectors = DSX_WriteSectors,
                                .clear_status = DSX_ClearStatus,
                                .shutdown = DSX_Shutdown};

#endif
