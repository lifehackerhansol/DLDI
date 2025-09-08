/*
    SPDX-License-Identifier: Zlib
    SPDX-FileContributor: lifehackerhansol, 2025
*/

#include <nds/ndstypes.h>
#include <iointerface.h>
#include <m3cf.h>
#include <mmcf.h>
#include <mpcf.h>
#include <sccf.h>

static const disc_interface_t GBCF_Interfaces[] = {
    M3CF_IoInterface,
    MMCF_IoInterface,
    MPCF_IoInterface,
    SCCF_IoInterface
};

static const CF_REGISTERS GBCF_CFRegisters[] = {
    _M3CF_Registers,
    _MMCF_Registers,
    _MPCF_Registers,
    _SCCF_Registers
};
