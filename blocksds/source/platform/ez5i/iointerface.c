// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <iointerface.h>
#include <stdbool.h>
#include <stdint.h>

#include "ez5i.h"

#define BYTES_PER_READ 512

// Initialize the driver. Returns true on success.
bool EZ5I_Startup(void) {
    return EZ5I_SDInitialize();
}

// Returns true if a card is present and initialized.
bool EZ5I_IsInserted(void) {
    return true;
}

// Clear error flags from the card. Returns true on success.
bool EZ5I_ClearStatus(void) {
    return true;
}

// Reads 512 byte sectors into a buffer that may be unaligned. Returns true on
// success.
bool EZ5I_ReadSectors(uint32_t sector, uint32_t num_sectors, void* buffer) {
    for (int i = 0; i < num_sectors; i++) {
        bool result = EZ5I_SDReadSector(sector, buffer);
        if (!result) return false;
        sector++;
        buffer = (u8*)buffer + 0x200;
    }
    return true;
}

// Writes 512 byte sectors from a buffer that may be unaligned. Returns true on
// success.
bool EZ5I_WriteSectors(uint32_t sector, uint32_t num_sectors, const void* buffer) {
    if (num_sectors == 1)
        return EZ5I_SDWriteSingleSector(sector, buffer);
    else
        return EZ5I_SDWriteMultiSector(sector, num_sectors, buffer);
    return true;
}

// Shutdowns the card. This may never be called.
bool EZ5I_Shutdown(void) {
    return true;
}

#ifdef PLATFORM_ez5i

disc_interface_t ioInterface = {.startup = EZ5I_Startup,
                                .is_inserted = EZ5I_IsInserted,
                                .read_sectors = EZ5I_ReadSectors,
                                .write_sectors = EZ5I_WriteSectors,
                                .clear_status = EZ5I_ClearStatus,
                                .shutdown = EZ5I_Shutdown};

#endif
