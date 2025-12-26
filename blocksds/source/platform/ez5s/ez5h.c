/*
	EZ-Flash V
	Card IO routines

	Copyright (C) 2007 Michael Chisholm (Chishm)
	Copyright (C) 2007 SaTa
	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: Zlib
*/

#include <common/libtwl_ext.h>
#include <libtwl/card/card.h>
#include <nds/ndstypes.h>

#include "ez5h.h"

static u32 EZ5H_SendCommand(const u64 command)
{
    return cardExt_ReadData4Byte(command, EZ5H_CTRL_READ_4B);
}

static bool EZ5H_SDSendSDIOCommand(u8 cmd, u32 parameter, u8 * buffer, int size) {
    u32 data;
    u8 *u8_data = (u8*)&data;
    int timeout = 99;

    // Either it's no response, 48 bits, or 136 bits.
    // If this isn't the case, then this function doesn't work.
    if (size != 0 && size != 6 && size != 17)
        return false;

    EZ5H_SendCommand(EZ5H_CMD_SDMC_SDIO(cmd, parameter));

    // R0 has no response.
    if (size == 0)
        return true;

    // Sends response in byte-swapped u32, with the starting marker
    // Search for starting marker, with a timeout
    do {
        data = EZ5H_SendCommand(EZ5H_CMD_SDMC_SEND_CLK(1));
        timeout--;
        if(!timeout)
            return false;
    } while (data & 0xFF);

    // Starting marker found. Start reading response
    if(buffer != NULL) {
        buffer[0] = u8_data[1];
        buffer[1] = u8_data[2];
        buffer[2] = u8_data[3];
    }

    // Read remaining data
    for (int i=2; i < size >> 2; i++) {
        data = EZ5H_SendCommand(EZ5H_CMD_SDMC_SEND_CLK(i));
        // here our buffer actually starts at 3...
        if(buffer != NULL) {
            buffer[(1 << i) - 1] = u8_data[0];
            buffer[(1 << i)] = u8_data[1];
            buffer[(1 << i) + 1] = u8_data[2];
            buffer[(1 << i) + 2] = u8_data[3];
        }
    }

    // if our size was 17 (R2 response)
    // then our previous loop didn't finish reading
    // one more time!
    if(size == 17)
    {
        data = EZ5H_SendCommand(EZ5H_CMD_SDMC_SEND_CLK(i));
        // here our buffer actually starts at 3...
        if(buffer != NULL) {
            buffer[15] = u8_data[0];
            buffer[16] = u8_data[1];
        }
    }

    return true;
}

// Calculate the CRC16 checksum for parallel 4 bit lines separately.
// When the SDIO bus operates in 4-bit mode, the CRC16 algorithm
// is applied to each line separately and generates total of
// 4 x 16 = 64 bits of checksum.
static uint64_t sdio_crc16_4bit_checksum(uint32_t *data, uint32_t num_words)
{
    uint64_t crc = 0;
    uint32_t *end = data + num_words;
    while (data < end)
    {
        for (int unroll = 0; unroll < 4; unroll++)
        {
            // Each 32-bit word contains 8 bits per line.
            // Reverse the bytes because SDIO protocol is big-endian.
            uint32_t data_in;
            if ((u32)data & 3) {
                ((u8*)&data_in)[0] = data[0];
                ((u8*)&data_in)[1] = data[1];
                ((u8*)&data_in)[2] = data[2];
                ((u8*)&data_in)[3] = data[3];
                data++;
            } else
                data_in = __builtin_bswap32(*data++);

            // Shift out 8 bits for each line
            uint32_t data_out = crc >> 32;
            crc <<= 32;

            // XOR outgoing data to itself with 4 bit delay
            data_out ^= (data_out >> 16);

            // XOR incoming data to outgoing data with 4 bit delay
            data_out ^= (data_in >> 16);

            // XOR outgoing and incoming data to accumulator at each tap
            uint64_t xorred = data_out ^ data_in;
            crc ^= xorred;
            crc ^= xorred << (5 * 4);
            crc ^= xorred << (12 * 4);
        }
    }

    return crc;
}

bool EZ5H_SDReadSector(u32 sector, void *buffer)
{
	EZ5H_SDSendSDIOCommand(17, sector, NULL, 6);

    // Wait for the start marker.
	u32 data = 0;
	u32 timeout = 0x10000;
	do
	{
		data = EZ5H_SendCommand(EZ5H_CMD_SDMC_SEND_CLK(1));
		timeout--;
		if (!timeout)
            return false;
	} while(data & 0xFF);

	cardExt_ReadData(EZ5H_CMD_SDMC_READ_DATA, EZ5H_CTRL_READ_512B, buffer, 128);
	return true;
}

bool EZ5H_SDWriteSector(u32 sector, const u8 *buffer)
{
	u64 crc16 = __builtin_bswap64(sdio_crc16_4bit_checksum(buffer, 128));

	// CMD24
    EZ5H_SDSendSDIOCommand(24, sector, NULL, 17);

	// This command needs an additional clock before sending data.
	EZ5H_SendCommand(EZ5H_CMD_SDMC_SEND_CLK(1));

	// Send data start marker.
    u8 start_marker[2] = {0xF0, 0x00};
	cardExt_SendCommand(EZ5H_CMD_SDMC_WRITE_DATA(start_marker), EZ5H_CTRL_READ_0);

	// Write data.
    for (u32 i=0; i < 512; i += 2) {
        cardExt_SendCommand(EZ5H_CMD_SDMC_WRITE_DATA(buffer + i), EZ5H_CTRL_READ_0);
    }
	// Write CRC data.
    for (u32 i=0; i < 8; i += 2) {
        cardExt_SendCommand(EZ5H_CMD_SDMC_WRITE_DATA(((u8*)&crc16) + i), EZ5H_CTRL_READ_0);
    }

	// Wait until CRC starts
	while(EZ5H_SendCommand(EZ5H_CMD_SDMC_SEND_CRC_STATUS) & 0x1);

	// Wait until CRC finishes
	EZ5H_SendCommand(EZ5H_CMD_SDMC_SEND_CRC_STATUS);
	while((EZ5H_SendCommand(EZ5H_CMD_SDMC_SEND_CRC_STATUS) & 0x1) != 0x1);

	// Wait until card ready
	while(EZ5H_SendCommand(EZ5H_CMD_SDMC_SEND_CLK(1)));
	return true;
}
