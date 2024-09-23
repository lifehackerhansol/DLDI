// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <iointerface.h>
#include <stdbool.h>
#include <stdint.h>

#define BYTES_PER_READ 512

// Initialize the driver. Returns true on success.
bool NMK6_Startup(void) {
    return true;
}

// Returns true if a card is present and initialized.
bool NMK6_IsInserted(void) {
    return true;
}

// Clear error flags from the card. Returns true on success.
bool NMK6_ClearStatus(void) {
    return true;
}

// Reads 512 byte sectors into a buffer that may be unaligned. Returns true on
// success.
bool NMK6_ReadSectors(uint32_t sector, uint32_t num_sectors, void* buffer) {
    return true;
}

// Writes 512 byte sectors from a buffer that may be unaligned. Returns true on
// success.
bool NMK6_WriteSectors(uint32_t sector, uint32_t num_sectors, const void* buffer) {
    return true;
}

// Shutdowns the card. This may never be called.
bool NMK6_Shutdown(void) {
    return true;
}

#ifdef PLATFORM_nmk6

disc_interface_t ioInterface = {.startup = NMK6_Startup,
                                .is_inserted = NMK6_IsInserted,
                                .read_sectors = NMK6_ReadSectors,
                                .write_sectors = NMK6_WriteSectors,
                                .clear_status = NMK6_ClearStatus,
                                .shutdown = NMK6_Shutdown};

#endif
