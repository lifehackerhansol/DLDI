/*
	SPDX-FileCopyrightText: 2006 Michael "Chishm" Chisholm
	SPDX-License-Identifier: BSD-3-Clause
*/

#include <nds/ndstypes.h>
#include <io_cf_common.h>
#include <io_m3_common.h>
#include <iointerface.h>

//---------------------------------------------------------------
// M3 CF Addresses
#define REG_M3CF_STS		((vu16*)0x080C0000)	// Status of the CF Card / Device control
#define REG_M3CF_CMD		((vu16*)0x088E0000)	// Commands sent to control chip and status return
#define REG_M3CF_ERR		((vu16*)0x08820000)	// Errors / Features

#define REG_M3CF_SEC		((vu16*)0x08840000)	// Number of sector to transfer
#define REG_M3CF_LBA1		((vu16*)0x08860000)	// 1st byte of sector address
#define REG_M3CF_LBA2		((vu16*)0x08880000)	// 2nd byte of sector address
#define REG_M3CF_LBA3		((vu16*)0x088A0000)	// 3rd byte of sector address
#define REG_M3CF_LBA4		((vu16*)0x088C0000)	// last nibble of sector address | 0xE0

#define REG_M3CF_DATA		((vu16*)0x08800000)		// Pointer to buffer of CF data transered from card

static const CF_REGISTERS _M3CF_Registers = {
	REG_M3CF_DATA,
	REG_M3CF_STS,
	REG_M3CF_CMD,
	REG_M3CF_ERR,
	REG_M3CF_SEC,
	REG_M3CF_LBA1,
	REG_M3CF_LBA2,
	REG_M3CF_LBA3,
	REG_M3CF_LBA4
};

bool M3CF_Startup(const CF_REGISTERS *usableCfRegs);

static const disc_interface_t M3CF_IoInterface = {.startup = M3CF_Startup,
                                .is_inserted = _CF_isInserted,
                                .read_sectors = _CF_readSectors,
                                .write_sectors = _CF_writeSectors,
                                .clear_status = _CF_clearStatus,
                                .shutdown = _CF_shutdown};
