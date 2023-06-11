// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <stdbool.h>
#include <stdint.h>
#include "iods2.h"

#define BYTES_PER_READ 512

// Initialize the driver. Returns true on success.
bool startup(void)
{
    ioDS2_Initialize();
    return true;
}

// Returns true if a card is present and initialized.
bool is_inserted(void)
{
    return true;
}

// Reads 512 byte sectors into a buffer that may be unaligned. Returns true on
// success.
bool read_sectors(uint32_t sector, uint32_t num_sectors, void *buffer)
{
	if (num_sectors == 1)
		ioDS2_SDReadSingleSector(sector, buffer);
	else
		ioDS2_SDReadMultiSector(sector, buffer, num_sectors);

	return true;
}

// Writes 512 byte sectors from a buffer that may be unaligned. Returns true on
// success.
bool write_sectors(uint32_t sector, uint32_t num_sectors, const void *buffer)
{
	if (num_sectors == 1)
		ioDS2_SDWriteSingleSector(sector, buffer);
	else
		ioDS2_SDWriteMultiSector(sector, buffer, num_sectors);

	return true;
}

// Clear error flags from the card. Returns true on success.
bool clear_status(void)
{
    return true;
}

// Shutdowns the card. This may never be called.
bool shutdown(void)
{
    return true;
}
