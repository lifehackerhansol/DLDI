// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <stdbool.h>
#include <stdint.h>

#include "compact_flash.h"

#define REG_MMCF_STATUS        (vu16*)(0x080E0000)
#define REG_MMCF_FEATURES      (vu16*)(0x08020000)
#define REG_MMCF_SECTOR_COUNT  (vu16*)(0x08040000)
#define REG_MMCF_LBA1          (vu16*)(0x08060000)
#define REG_MMCF_LBA2          (vu16*)(0x08080000)
#define REG_MMCF_LBA3          (vu16*)(0x080A0000)
#define REG_MMCF_LBA4          (vu16*)(0x080C0000)
#define REG_MMCF_COMMAND       (vu16*)(0x080E0000)
#define REG_MMCF_DATA          (vu16*)(0x09000000)

static const CF_REGISTERS _MMCF_Registers = {
	REG_MMCF_DATA,
	REG_MMCF_STATUS,
	REG_MMCF_COMMAND,
	REG_MMCF_FEATURES,
	REG_MMCF_SECTOR_COUNT,
	REG_MMCF_LBA1,
	REG_MMCF_LBA2,
	REG_MMCF_LBA3,
	REG_MMCF_LBA4
};

// Initialize the driver. Returns true on success.
bool startup(void)
{
    return _CF_startup(&_MMCF_Registers);
}

// Returns true if a card is present and initialized.
bool is_inserted(void)
{
    return _CF_isInserted();
}

// Clear error flags from the card. Returns true on success.
bool clear_status(void)
{
    return _CF_clearStatus();
}

// Reads 512 byte sectors into a buffer that may be unaligned. Returns true on
// success.
bool read_sectors(uint32_t sector, uint32_t num_sectors, void *buffer)
{
    return _CF_readSectors(sector, num_sectors, buffer);
}

// Writes 512 byte sectors from a buffer that may be unaligned. Returns true on
// success.
bool write_sectors(uint32_t sector, uint32_t num_sectors, const void *buffer)
{
    return _CF_writeSectors(sector, num_sectors, buffer);
}

// Shutdowns the card. This may never be called.
bool shutdown(void)
{
    return _CF_shutdown();
}
