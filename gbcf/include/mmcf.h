/*
	SPDX-FileCopyrightText: 2006 Michael "Chishm" Chisholm
	SPDX-License-Identifier: Zlib
*/

#include <nds/ndstypes.h>
#include <io_cf_common.h>
#include <iointerface.h>

//---------------------------------------------------------------
// CF Addresses
#define REG_MMCF_STATUS        (vu16*)(0x080E0000)
#define REG_MMCF_FEATURES      (vu16*)(0x08020000)
#define REG_MMCF_SECTOR_COUNT  (vu16*)(0x08040000)
#define REG_MMCF_SECTOR_NO     (vu16*)(0x08060000)
#define REG_MMCF_CYLINDER_LOW  (vu16*)(0x08080000)
#define REG_MMCF_CYLINDER_HIGH (vu16*)(0x080A0000)
#define REG_MMCF_SEL_HEAD      (vu16*)(0x080C0000)
#define REG_MMCF_COMMAND       (vu16*)(0x080E0000)
#define REG_MMCF_DATA          (vu16*)(0x09000000) // Pointer to buffer of CF data transered from card

static const CF_REGISTERS _MMCF_Registers = {
	REG_MMCF_DATA,
	REG_MMCF_STATUS,
	REG_MMCF_COMMAND,
	REG_MMCF_FEATURES,
	REG_MMCF_SECTOR_COUNT,
	REG_MMCF_SECTOR_NO,
	REG_MMCF_CYLINDER_LOW,
	REG_MMCF_CYLINDER_HIGH,
	REG_MMCF_SEL_HEAD
};

static const disc_interface_t MMCF_IoInterface = {.startup = _CF_startup,
                                .is_inserted = _CF_isInserted,
                                .read_sectors = _CF_readSectors,
                                .write_sectors = _CF_writeSectors,
                                .clear_status = _CF_clearStatus,
                                .shutdown = _CF_shutdown};
