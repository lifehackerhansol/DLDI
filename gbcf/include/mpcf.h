/*
	SPDX-FileCopyrightText: 2006 Michael "Chishm" Chisholm
	SPDX-License-Identifier: BSD-3-Clause
*/

#include <nds/ndstypes.h>
#include <io_cf_common.h>
#include <iointerface.h>

//---------------------------------------------------------------
// CF Addresses
#define REG_MPCF_STS		((vu16*)0x098C0000)	// Status of the CF Card / Device control
#define REG_MPCF_CMD		((vu16*)0x090E0000)	// Commands sent to control chip and status return
#define REG_MPCF_ERR		((vu16*)0x09020000)	// Errors / Features

#define REG_MPCF_SEC		((vu16*)0x09040000)	// Number of sector to transfer
#define REG_MPCF_LBA1		((vu16*)0x09060000)	// 1st byte of sector address
#define REG_MPCF_LBA2		((vu16*)0x09080000)	// 2nd byte of sector address
#define REG_MPCF_LBA3		((vu16*)0x090A0000)	// 3rd byte of sector address
#define REG_MPCF_LBA4		((vu16*)0x090C0000)	// last nibble of sector address | 0xE0

#define REG_MPCF_DATA		((vu16*)0x09000000)	// Pointer to buffer of CF data transered from card

static const CF_REGISTERS _MPCF_Registers = {
	REG_MPCF_DATA,
	REG_MPCF_STS,
	REG_MPCF_CMD,
	REG_MPCF_ERR,
	REG_MPCF_SEC,
	REG_MPCF_LBA1,
	REG_MPCF_LBA2,
	REG_MPCF_LBA3,
	REG_MPCF_LBA4
};

static const disc_interface_t MPCF_IoInterface = {.startup = _CF_startup,
                                .is_inserted = _CF_isInserted,
                                .read_sectors = _CF_readSectors,
                                .write_sectors = _CF_writeSectors,
                                .clear_status = _CF_clearStatus,
                                .shutdown = _CF_shutdown};
