// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <stdbool.h>
#include <stdint.h>

#include <iointerface.h>
#include <io_cf_common.h>
#include <devices.h>

#define BYTES_PER_READ 512

static const disc_interface_t *device_interface;

// Initialize the driver. Returns true on success.
bool startup(void)
{
	for (int i=0; i < sizeof(GBCF_Interfaces); i++) {
		if (GBCF_Interfaces[i].startup(&GBCF_CFRegisters[i])) {
			device_interface = &GBCF_Interfaces[i];
			return true;
		}
	}
	return false;
}

// Returns true if a card is present and initialized.
bool is_inserted(void)
{
	return device_interface->is_inserted();
}

// Clear error flags from the card. Returns true on success.
bool clear_status(void)
{
	return device_interface->clear_status();
}

// Reads 512 byte sectors into a buffer that may be unaligned. Returns true on
// success.
bool read_sectors(uint32_t sector, uint32_t num_sectors, void *buffer)
{
	return device_interface->read_sectors(sector, num_sectors, buffer);
}

// Writes 512 byte sectors from a buffer that may be unaligned. Returns true on
// success.
bool write_sectors(uint32_t sector, uint32_t num_sectors, const void *buffer)
{
	return device_interface->write_sectors(sector, num_sectors, buffer);
}

// Shutdowns the card. This may never be called.
bool shutdown(void)
{
	return device_interface->shutdown();
}
