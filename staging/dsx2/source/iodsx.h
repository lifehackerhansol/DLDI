/*
	DS-Xtreme (v2)
	Card IO routines

	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: Zlib
*/

#pragma once

#include <nds/ndstypes.h>
#include "libtwl_card.h"

#ifndef NULL
 #define NULL 0
#endif

#define IODSX_CTRL_BASE (MCCNT1_ENABLE | MCCNT1_RESET_OFF | MCCNT1_CMD_SCRAMBLE | MCCNT1_CLOCK_SCRAMBLER | MCCNT1_READ_DATA_DESCRAMBLE | MCCNT1_LATENCY2(24))

// Card commands

#define IODSX_CMD_WAIT_BUSY (0x2ull << 56)
#define IODSX_CMD_RESET_FPGA_ADDRESS (0x3ull << 56)

#define IODSX_CMD_STOP_TRANSMISSION (0xBCull << 56)

static inline u64 IODSX_CMD_READ_LBA(u32 lba)
{
	// this is where the actual sector is located in NAND
	lba += 0x6000;
	return (0xBFull << 56) | ((u64)lba << 24);
}

static inline u64 IODSX_CMD_WRITE_LBA_TRANSFER_DATA(u32 data)
{
	return (0x4ull << 56) | ((u64)data << 24);
}

static inline u64 IODSX_CMD_WRITE_LBA_COMMIT_DATA(u32 lba)
{
	// this is where the actual sector is located in NAND
	lba += 0x6000;
	return (0x5ull << 56) | ((u64)lba << 24);
}

// user API
void ioDSX_NANDReadSector(u32 sector, void* buffer);
void ioDSX_NANDWriteSectors(u32 sector, u32 num_sectors, const u32 *buffer);
