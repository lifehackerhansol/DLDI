// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <stdbool.h>
#include <stdint.h>

#include "iodsx.h"

#define BYTES_PER_READ 512

// Initialize the driver. Returns true on success.
bool startup(void)
{
    return true;
}

// Returns true if a card is present and initialized.
bool is_inserted(void)
{
    return true;
}

// Clear error flags from the card. Returns true on success.
bool clear_status(void)
{
    return true;
}

// Reads 512 byte sectors into a buffer that may be unaligned. Returns true on
// success.
bool read_sectors(uint32_t sector, uint32_t num_sectors, void *buffer)
{
    for(u32 i = 0; i < num_sectors; i++)
    {
        ioDSX_NANDReadSector(sector, buffer);
        sector++;
        buffer = (u8 *)buffer + 0x200;
    }
    return true;
}

// Writes 512 byte sectors from a buffer that may be unaligned. Returns true on
// success.
bool write_sectors(uint32_t sector, uint32_t num_sectors, const void *buffer)
{
    ioDSX_NANDWriteSectors(sector, num_sectors, buffer);
    return true;
}

// Shutdowns the card. This may never be called.
bool shutdown(void)
{
    return true;
}
