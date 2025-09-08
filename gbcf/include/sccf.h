/*
	SPDX-FileCopyrightText: 2006 Michael "Chishm" Chisholm
	SPDX-License-Identifier: BSD-3-Clause
*/

#include <nds/ndstypes.h>
#include <io_cf_common.h>
#include <iointerface.h>

//---------------------------------------------------------------
// CF Addresses
#define REG_SCCF_STS		((vu16*)0x098C0000)	// Status of the CF Card / Device control
#define REG_SCCF_CMD		((vu16*)0x090E0000)	// Commands sent to control chip and status return
#define REG_SCCF_ERR		((vu16*)0x09020000)	// Errors / Features

#define REG_SCCF_SEC		((vu16*)0x09040000)	// Number of sector to transfer
#define REG_SCCF_LBA1		((vu16*)0x09060000)	// 1st byte of sector address
#define REG_SCCF_LBA2		((vu16*)0x09080000)	// 2nd byte of sector address
#define REG_SCCF_LBA3		((vu16*)0x090A0000)	// 3rd byte of sector address
#define REG_SCCF_LBA4		((vu16*)0x090C0000)	// last nibble of sector address | 0xE0

#define REG_SCCF_DATA		((vu16*)0x09000000)	// Pointer to buffer of CF data transered from card

static const CF_REGISTERS _SCCF_Registers = {
	REG_SCCF_DATA,
	REG_SCCF_STS,
	REG_SCCF_CMD,
	REG_SCCF_ERR,
	REG_SCCF_SEC,
	REG_SCCF_LBA1,
	REG_SCCF_LBA2,
	REG_SCCF_LBA3,
	REG_SCCF_LBA4
};

/*-----------------------------------------------------------------
changeMode (was SC_Unlock)
Added by MightyMax
Modified by Chishm
Modified again by loopy
1=ram(readonly), 5=ram, 3=SD interface?
-----------------------------------------------------------------*/
#define SC_MODE_RAM 0x5
#define SC_MODE_MEDIA 0x3 
#define SC_MODE_RAM_RO 0x1
static inline void SCCF_ChangeMode(u8 mode) {
	vu16 *unlockAddress = (vu16*)0x09FFFFFE;
	*unlockAddress = 0xA55A ;
	*unlockAddress = 0xA55A ;
	*unlockAddress = mode ;
	*unlockAddress = mode ;
}

bool SCCF_Startup(const CF_REGISTERS *usableCfRegs);

static const disc_interface_t SCCF_IoInterface = {.startup = SCCF_Startup,
                                .is_inserted = _CF_isInserted,
                                .read_sectors = _CF_readSectors,
                                .write_sectors = _CF_writeSectors,
                                .clear_status = _CF_clearStatus,
                                .shutdown = _CF_shutdown};
