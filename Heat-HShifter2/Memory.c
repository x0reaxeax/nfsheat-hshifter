/// H-shifter support for Need for Speed Heat.
/// Copyright (C) 2025  x0reaxeax
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/// @file Memory.c
/// @brief Helper functions for reading and writing memory of the game process.
/// 
///   - github.con/x0reaxeax/nfsheat-hshifter
/// 

#include "Utils.h"

#include <stdio.h>

LPVOID GetGearRegionAddressBase(
    VOID
) {
    LPVOID lpRegionAddress = NULL;

    MEMORY_BASIC_INFORMATION memInfo = { 0 };

    while (VirtualQueryEx(
        g_ShifterConfig.hGameProcess,
        lpRegionAddress,
        &memInfo,
        sizeof(memInfo)
    ) == sizeof(memInfo)) {
        if (HEAT_GEAR_ADDRESS_REGION_SIZE == memInfo.RegionSize) {
            return memInfo.BaseAddress;
        }
        lpRegionAddress = (LPVOID) (
            (ULONG_PTR)memInfo.BaseAddress + memInfo.RegionSize
        );
    }

    return NULL;
}

LPVOID ScanRegionForAsciiString(
    LPCVOID lpBaseAddress, 
    SIZE_T cbRegionSize, 
    LPCSTR szTargetString
) {
    CONST DWORD dwReadSize = 0x1000;
    CONST SIZE_T cbStrLen = sizeof(HEAT_ARTIFACT_VEHICLE_PHYSICS_JOB_STR);

    LPBYTE abyReadBuffer = VirtualAlloc(
        NULL, 
        dwReadSize, 
        MEM_COMMIT | MEM_RESERVE, 
        PAGE_READWRITE
    );

    if (NULL == abyReadBuffer) {
        fprintf(
            stderr, 
            "[-] VirtualAlloc(): E%lu\n",
            GetLastError()
        );
        return NULL;
    }

    DWORD64 qwOffset = 0;

    while (qwOffset < cbRegionSize) {
        SIZE_T cbBytesRead = 0;
        SIZE_T cbChunkLeft = (SIZE_T) (cbRegionSize - qwOffset);
        SIZE_T cbChunkSize = \
            (cbChunkLeft > dwReadSize) ? dwReadSize : cbChunkLeft;

        LPCVOID lpCurrentAddress = (LPCVOID) (
            (ULONG_PTR) lpBaseAddress + qwOffset
        );

        if (!ReadProcessMemory(
            g_ShifterConfig.hGameProcess,
            lpCurrentAddress,
            abyReadBuffer,
            cbChunkSize,
            &cbBytesRead
        )) {
            qwOffset += cbChunkSize;
            continue;
        }

        for (DWORD i = 0; i + cbStrLen < cbBytesRead; ++i) {
            if (EXIT_SUCCESS == memcmp(
                &abyReadBuffer[i], 
                szTargetString, 
                cbStrLen
            )) {
                DWORD64 qwFoundAddress = \
                    (DWORD64) (ULONG_PTR) lpCurrentAddress + i;

                VirtualFree(
                    abyReadBuffer, 
                    0, 
                    MEM_RELEASE
                );
                return (LPVOID) qwFoundAddress;
            }
        }

        qwOffset += cbChunkSize;
    }

    VirtualFree(
        abyReadBuffer, 
        0,
        MEM_RELEASE
    );

    return NULL;
}

DWORD ReadGear(
    VOID
) {
    DWORD dwCurrentGear = 0;
    SIZE_T cbBytesRead = 0;

    if (!ReadProcessMemory(
        g_ShifterConfig.hGameProcess,
        g_ShifterConfig.lpCurrentGearAddress,
        &dwCurrentGear,
        sizeof(DWORD),
        &cbBytesRead
    )) {
        fprintf(
            stderr,
            "[-] ReadProcessMemory(): E%lu\n",
            GetLastError()
        );
        return 0;
    }

    return dwCurrentGear;
}