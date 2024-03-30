/*
	io_cf_common.c based on

	compact_flash.c
	By chishm (Michael Chisholm)

	Common hardware routines for using a compact flash card. This is not reentrant 
	and does not do range checking on the supplied addresses. This is designed to 
	be as fast as possible.

	CF routines modified with help from Darkfader

 Copyright (c) 2006 Michael "Chishm" Chisholm
	
 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "compact_flash.h"

//---------------------------------------------------------------
// DMA
#ifdef _IO_USE_DMA
	#ifndef NDS
		#include "gba_dma.h"
	#else
		#include <nds/dma.h>
		#ifdef ARM9
			#include <nds/arm9/cache.h>
		#endif
	#endif
#endif

//---------------------------------------------------------------
// CF Addresses & Commands

static CF_REGISTERS cfRegisters = {0};

//---------------------------------------------------------------
static bool StartupFinished = false;

#ifdef _IO_USEFASTCNT
static inline u16 setFastCNT(u16 originData) {
	//  2-3   32-pin GBA Slot ROM 1st Access Time (0-3 = 10, 8, 6, 18 cycles)
	//    4     32-pin GBA Slot ROM 2nd Access Time (0-1 = 6, 4 cycles)
    const u16 mask = ~(7<<2);//~ 000011100, clear bit 2-3 + 4
    const u16 setVal = ((2) << 2) | (1<<4);
    return (originData & mask) | setVal;
}
#endif

static bool CF_Block_Ready(void) {
	u32 i = 0;
	u32 TIMEOUT = CARD_TIMEOUT;

	if (!StartupFinished)TIMEOUT = CARD_INITTIMEOUT; // Use shorter time out to speed up initial init

	while ((*cfRegisters.status & CF_STS_BUSY) && (i < TIMEOUT)) {
		i++;
		while ((!(*cfRegisters.status & CF_STS_40)) && (i < TIMEOUT)) { i++; }
	} 

	if (i >= TIMEOUT)return false;

	return true;
}

static bool CF_Set_Features(u32 feature) {
	if (!CF_Block_Ready())return false;

	*cfRegisters.features = feature;
	*cfRegisters.sectorCount = 0x00;  // config???
	*cfRegisters.lba4 = 0x00;
	*cfRegisters.command = 0xEF;

	return true;
}

/*-----------------------------------------------------------------
_CF_isInserted
Is a compact flash card inserted?
bool return OUT:  true if a CF card is inserted
-----------------------------------------------------------------*/
bool _CF_isInserted (void) {
	return !CF_Set_Features(0xAA);
}


/*-----------------------------------------------------------------
_CF_clearStatus
Tries to make the CF card go back to idle mode
bool return OUT:  true if a CF card is idle
-----------------------------------------------------------------*/
bool _CF_clearStatus (void) {
	return CF_Block_Ready();
}


bool ReadBlocks_256 (u32 sector, int numSectors, u16* buff) {
	int i;
#ifdef _IO_ALLOW_UNALIGNED
	u8 *buff_u8 = (u8*)buff;
	int temp;
#endif

#if (defined _IO_USE_DMA) && (defined NDS) && (defined ARM9)
	DC_FlushRange(buffer, numSectors * BYTES_PER_READ);
#endif

	if (!CF_Block_Ready())return false;
	
	*cfRegisters.sectorCount = (numSectors == 256) ? 0 : numSectors;
	*cfRegisters.lba1 = sector;
	*cfRegisters.lba2 = sector >> 8;
	*cfRegisters.lba3 = sector >> 16;
	*cfRegisters.lba4 = ((sector >> 24) & 0x0F) | 0xE0;
	*cfRegisters.command = 0x20; // read sectors

	while (numSectors--)	{
		
		if (!CF_Block_Ready())return false;
#ifdef _IO_USE_DMA
	#ifdef NDS
		DMA3_SRC = (u32)(cfRegisters.data);
		DMA3_DEST = (u32)buff;
		DMA3_CR = 256 | DMA_COPY_HALFWORDS | DMA_SRC_FIX;
	#else
		DMA3COPY (cfRegisters.data, buff, 256 | DMA16 | DMA_ENABLE | DMA_SRC_FIXED);
	#endif
		buff += BYTES_PER_READ / 2;
#elif defined _IO_ALLOW_UNALIGNED
		i=256;
		if ((u32)buff_u8 & 1) {
			while(i--) {
				// if (!CF_Block_Ready())return false;
				temp = *cfRegisters.data;
				*buff_u8++ = temp & 0xFF;
				*buff_u8++ = temp >> 8;
			}
		} else {
			while(i--)*buff++ = *cfRegisters.data;
		}
#else
		i=256;
		while(i--)*buff++ = *cfRegisters.data;
#endif
	}

#if (defined _IO_USE_DMA) && (defined NDS)
	// Wait for end of transfer before returning
	while(DMA3_CR & DMA_BUSY);
#endif

	return true;
}


bool WriteBlocks_256(u32 sector, int numSectors, const u16* buff) {
		
	int i;
#ifdef _IO_ALLOW_UNALIGNED
	u8 *buff_u8 = (u8*)buff;
	int temp;
#endif
	
#if defined _IO_USE_DMA && defined NDS && defined ARM9
	DC_FlushRange(buffer, numSectors * BYTES_PER_READ);
#endif

	if (!CF_Block_Ready())return false;
	
	*cfRegisters.sectorCount = (numSectors == 256) ? 0 : numSectors;
	*cfRegisters.lba1 = sector;
	*cfRegisters.lba2 = sector >> 8;
	*cfRegisters.lba3 = sector >> 16;
	*cfRegisters.lba4 = ((sector >> 24) & 0x0F) | 0xE0;
	*cfRegisters.command = 0x30; // write sectors
	
	while (numSectors--) {
		
		if (!CF_Block_Ready())return false;

#ifdef _IO_USE_DMA
	#ifdef NDS
		DMA3_SRC = (u32)buff;
		DMA3_DEST = (u32)(cfRegisters.data);
		DMA3_CR = 256 | DMA_COPY_HALFWORDS | DMA_DST_FIX;
	#else
		DMA3COPY(buff, cfRegisters.data, 256 | DMA16 | DMA_ENABLE | DMA_DST_FIXED);
	#endif
		buff += BYTES_PER_READ / 2;
#elif defined _IO_ALLOW_UNALIGNED
		i=256;
		if ((u32)buff_u8 & 1) {
			while(i--) {
				// if (!CF_Block_Ready())return false;
				temp = *buff_u8++;
				temp |= *buff_u8++ << 8;
				*cfRegisters.data = temp;
			}
		} else {
			while(i--)*cfRegisters.data = *buff++;
		}
#else
		i=256;
		while(i--)*cfRegisters.data = *buff++;
#endif
	}
#if defined _IO_USE_DMA && defined NDS
	// Wait for end of transfer before returning
	while(DMA3_CR & DMA_BUSY);
#endif
	return true;
}

/*-----------------------------------------------------------------
_CF_readSectors
Read 512 byte sector numbered "sector" into "buffer"
u32 sector IN: address of first 512 byte sector on CF card to read
u32 numSectors IN: number of 512 byte sectors to read,
 1 to 256 sectors can be read
void* buffer OUT: pointer to 512 byte buffer to store data in
bool return OUT: true if successful
-----------------------------------------------------------------*/
bool _CF_readSectors (u32 sector, u32 numSectors, void* buffer) {
	bool Result = false;
#ifdef _IO_USEFASTCNT
	u16 originMemStat = REG_EXMEMCNT;
	REG_EXMEMCNT = setFastCNT(originMemStat);
#endif
	while (numSectors > 0) {
		int sector_count = (numSectors > 256) ? 256 : numSectors;
		Result = ReadBlocks_256(sector, sector_count, (u16*)buffer);
		sector += sector_count;
		numSectors -= sector_count;
		buffer += (sector_count * BYTES_PER_READ);
	}
#ifdef _IO_USEFASTCNT
	REG_EXMEMCNT = originMemStat;
#endif	
	return Result;
}



/*-----------------------------------------------------------------
_CF_writeSectors
Write 512 byte sector numbered "sector" from "buffer"
u32 sector IN: address of 512 byte sector on CF card to read
u32 numSectors IN: number of 512 byte sectors to read,
 1 to 256 sectors can be read
void* buffer IN: pointer to 512 byte buffer to read data from
bool return OUT: true if successful
-----------------------------------------------------------------*/
bool _CF_writeSectors (u32 sector, u32 numSectors, const void* buffer) {
	bool Result = false;
#ifdef _IO_USEFASTCNT
	u16 originMemStat = REG_EXMEMCNT;
	REG_EXMEMCNT = setFastCNT(originMemStat);
#endif
	while (numSectors > 0) {
		int sector_count = (numSectors > 256) ? 256 : numSectors;
		Result = WriteBlocks_256(sector, sector_count, (u16*)buffer);
		sector += sector_count;
		numSectors -= sector_count;
		buffer += (sector_count * BYTES_PER_READ);
	}
#ifdef _IO_USEFASTCNT
	REG_EXMEMCNT = originMemStat;
#endif	
	return Result;
}

/*-----------------------------------------------------------------
_CF_shutdown
shutdown the CF interface
-----------------------------------------------------------------*/
bool _CF_shutdown(void) {
	return CF_Block_Ready() ;
}

/*-----------------------------------------------------------------
_CF_startUp
Initializes the CF interface using the supplied registers
returns true if successful, otherwise returns false
-----------------------------------------------------------------*/
bool _CF_startup(const CF_REGISTERS *usableCfRegs) {
	cfRegisters = *usableCfRegs;
	if(!_CF_isInserted()) return false;

	StartupFinished = true;
	return true;
}

