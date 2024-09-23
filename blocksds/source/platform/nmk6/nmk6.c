/*
	Acekard RPG
	Card IO routines

	Copyright (C) 2024 lifehackerhansol

	SPDX-License-Identifier: Zlib
*/

#include <nds/ndstypes.h>
#include <nds/card.h>

#include <libtwl/card/card.h>
#include <common/libtwl_ext.h>
#include <common/sdio.h>
#include <libcart/sdcrc7.h>

static inline void NMK6_SpiControlSetFlags(u16 flags)
{
    REG_MCCNT0 = MCCNT0_ENABLE | MCCNT0_MODE_SPI | MCCNT0_SPI_HOLD_CS | flags;
}

static inline void NMK6_SpiControlDisable(u16 flags)
{
    REG_MCCNT0 = 0;
}

static void NMK6_SpiDataWriteSingle(u8 data)
{
    card_spiTransferLastByte(0, data);
}

static void NMK6_SpiDataWriteLoop(u8 * data, int size)
{
    if (size != 0) {
        for (int i=0; i < size - 1; i++) {
            card_spiTransferByte(0, data[i])
        }
        card_spiTransferLastByte(data[size - 1]);
    }
}

static void NMK6_SpiEnable(void)
{
    const u8 command[4] = { 0xfe, 0xfd, 0xfb, 0xf7 };
    NMK6_SpiDataWriteLoop(command, 4);
}

static void NMK6_SpiDisable(void)
{
    const u8 command[4] = { 0xfe, 0xfd, 0xfb, 0xf5 };
    NMK6_SpiDataWriteLoop(command, 4);
}

static void NMK6_SdioSendCommand(u8 sdio, u32 parameter)
{
    u8 command[6] = {
        (sdio | 0x40),
        (u8)((parameter >> 24) & 0xFF),
        (u8)((parameter >> 16) & 0xFF),
        (u8)((parameter >> 8) & 0xFF),
        (u8)(parameter & 0xFF),
        0
    }
    command[5] = __sd_crc7(command);
    cardExt_RomReadData4Byte(NMK6_CMD_WRITE(0xFF), NMK6_CTRL_READ_4B);
    cardExt_RomReadData4Byte(NMK6_CMD_WRITE(0xFF), NMK6_CTRL_READ_4B);
    for(int i=0; i < 6; i++)
        cardExt_RomReadData4Byte(NMK6_CMD_WRITE(command[i]), NMK6_CTRL_READ_4B);
}

static void NMK6_SdioReadResponse(u32 bits, u8 *response)
{
    // Loop until we receive the header bit
    while(cardExt_RomReadData4Byte(NMK6_CMD_READ(1), NMK6_CTRL_READ_4B) != 0);
    // Flush out the transmission bit and command index
    cardExt_RomReadData4Byte(NMK6_CMD_READ(7), NMK6_CTRL_READ_4B);
    if (bits == 48)
    {
        for (u32 i=0; i < 4; i++)
            *resp++ = (u8)(cardExt_RomReadData4Byte(NMK6_CMD_READ(8), NMK6_CTRL_READ_4B));
        // Flush out CRC and end bit
        cardExt_RomReadData4Byte(NMK6_CMD_READ(8), NMK6_CTRL_READ_4B);
    }
    else if (bits == 136)
    {
        for (u32 i=0; i < 16; i++)
            *resp++ = (u8)(cardExt_RomReadData4Byte(NMK6_CMD_READ(8), NMK6_CTRL_READ_4B));
    }
}

static u32 NMK6_SdioSendCommandR1(u8 sdio, u32 parameter)
{
    u32 response;
    NMK6_SdioSendCommand(sdio, parameter);
    NMK6_SdioReadResponse(48, &response);
    return response;    
}

// TODO: actually return this buffer and use it if needed
static void NMK6_SdioSendCommandR2(u8 sdio, u32 parameter)
{
    u32 response[4];
    NMK6_SdioSendCommand(sdio, parameter);
    NMK6_SdioReadResponse(136, response);
}

static void NMK6_SdioInitialize(void)
{
    u32 sdio_rca;
    int timeout;

    // clock issue?
    for(int i=0; i < 50; i++)
    {
        cardExt_RomReadData4Byte(NMK6_CMD_WRITE(0xFF), NMK6_CTRL_READ_4B);
    }

    // CMD0
    NMK6_SdioSendCommand(SDIO_CMD0_GO_IDLE_STATE, 0);

    for (timeout=0; i < 1024; i++)
    {
        NMK6_SdioSendCommandR1(SDIO_CMD55_APP_CMD, 0);
        if(NMK6_SdioSendCommandR1(SDIO_ACMD41_SEND_OP_COND, 0x00300000))
            break;
    }
    if(timeout == 1024)
        return false;

    NMK6_SdioSendCommand(SDIO_CMD2_ALL_SEND_CID, 0);
    sdio_rca = NMK6_SdioSendCommand(SDIO_CMD3_SEND_RELATIVE_ADDR, 0);
    NMK6_SdioSendCommand(SDIO_CMD55_APP_CMD, 0);
    NMK6_SdioSendCommand(SDIO_ACMD6_SET_BUS_WIDTH, 4);
}
