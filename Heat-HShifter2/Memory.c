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

#include <TlHelp32.h>

#include <stdio.h>

STATIC LPMODULEENTRY32 GetModuleInfo(
    LPCWSTR wszTargetModuleName
) {
    LPMODULEENTRY32 lpModuleEntry32 = NULL;
    MODULEENTRY32 me32 = {
        .dwSize = sizeof(MODULEENTRY32)
    };

    BOOLEAN bFound = FALSE;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE,
        g_ShifterConfig.dwGameProcessId
    );

    if (INVALID_HANDLE_VALUE == hSnapshot) {
        fprintf(
            stderr,
            "[-] CreateToolhelp32Snapshot(): E%lu\n",
            GetLastError()
        );
        return NULL;
    }

    if (Module32First(hSnapshot, &me32)) {
        do {
            if (EXIT_SUCCESS == wcscmp(
                me32.szModule,
                wszTargetModuleName
            )) {
                bFound = TRUE;
                break;
            }
        } while (Module32Next(hSnapshot, &me32));
    }

    if (!bFound) {
        return NULL;
    }

    lpModuleEntry32 = VirtualAlloc(
        NULL,
        sizeof(MODULEENTRY32),
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (NULL == lpModuleEntry32) {
        fprintf(
            stderr,
            "[-] VirtualAlloc(): E%lu\n",
            GetLastError()
        );
        CloseHandle(hSnapshot);
        return NULL;
    }

    memcpy(
        lpModuleEntry32,
        &me32,
        sizeof(MODULEENTRY32)
    );

    CloseHandle(hSnapshot);
    return lpModuleEntry32;
}

STATIC INLINE BOOLEAN IsAddressStateValid(
    DWORD dwState,
    DWORD dwProtect
) {
    if (MEM_COMMIT != dwState) {
        return FALSE;
    }

    if (PAGE_GUARD & dwProtect) {
        return FALSE;
    }

    if (PAGE_NOACCESS & dwProtect) {
        return FALSE;
    }

    if (PAGE_EXECUTE_READ & dwProtect) {
        return FALSE;
    }

    if (!(PAGE_READWRITE & dwProtect)) {
        return FALSE;
    }

    return TRUE;
}

STATIC BOOLEAN AreDwordsUnique(
    CONST LPDWORD adwArray,
    CONST SIZE_T dwMembCount
) {
    for (SIZE_T i = 0; i < dwMembCount; ++i) {
        for (SIZE_T j = i + 1; j < dwMembCount; ++j) {
            if (adwArray[i] == adwArray[j]) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

STATIC BOOLEAN IsValueLiveMemory(
    LPCVOID lpcCurrentArtifactAddress,
    TARGET_GEAR eTargetGear
) {
    LPCVOID lpcTargetAddress = NULL;

    switch (eTargetGear) {
        case TARGET_GEAR_CURRENT:
            lpcTargetAddress = (LPCVOID) (
                (DWORD64) lpcCurrentArtifactAddress + AOBSCAN_CURRENT_GEAR_LIVE_MEMORY_OFFSET
            );
            break;

        case TARGET_GEAR_LAST:
            lpcTargetAddress = (LPCVOID) (
                (DWORD64) lpcCurrentArtifactAddress - AOBSCAN_LAST_GEAR_LIVE_MEMORY_OFFSET
            );
            break;

        default:
            fprintf(
                stderr,
                "[-] Invalid target gear: %d\n",
                eTargetGear
            );
            return FALSE;
    }

    DWORD adwReads[AOBSCAN_LIVE_MEMORY_ITERATIONS] = { 0 };
    SIZE_T cbBytesRead = 0;

    for (INT i = 0; i < AOBSCAN_LIVE_MEMORY_ITERATIONS; ++i) {
        if (!ReadProcessMemory(
            g_ShifterConfig.hGameProcess,
            lpcTargetAddress,
            &adwReads[i],
            sizeof(DWORD),
            &cbBytesRead
        )) {
            fprintf(
                stderr,
                "[-] ReadProcessMemory(): E%lu\n",
                GetLastError()
            );
            return FALSE;
        }

        Sleep(AOBSCAN_LIVE_MEMORY_DELAY_MS);
    }

    if (!AreDwordsUnique(
        adwReads,
        AOBSCAN_LIVE_MEMORY_ITERATIONS
    )) {
        return FALSE;
    }

    return TRUE;
}

STATIC BOOLEAN VerifyPlayerGear(
    LPCVOID lpcCurrentArtifactAddress,
    TARGET_GEAR eTargetGear
) {
    DWORD dwScore = 0;
    if (IsWindowForeground(
        FGWIN_GAME
    )) {
        if (IsValueLiveMemory(
            lpcCurrentArtifactAddress,
            eTargetGear
        )) {
            dwScore++;
        }
    }

    DWORD dwReadValue = 0;
    SIZE_T cbBytesRead = 0;

    LPCVOID lpcTargetAddressGear = (LPCVOID) (
        (DWORD64) lpcCurrentArtifactAddress + HEAT_CURRENT_GEAR_ARTIFACT_OFFSET
    );

    if (!ReadProcessMemory(
        g_ShifterConfig.hGameProcess,
        lpcTargetAddressGear,
        &dwReadValue,
        sizeof(DWORD),
        &cbBytesRead
    )) {
        fprintf(
            stderr,
            "[-] ReadProcessMemory(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    if (dwReadValue < GEAR_REVERSE || dwReadValue > GEAR_8) {
        return FALSE;
    }

    return TRUE;
}

LPCVOID AobScan(
    LPCBYTE abyPattern,
    CONST SIZE_T cbPatternSize,
    TARGET_GEAR eTargetGear
) {
    SIZE_T cbBytesRead = 0;
    
    LPCBYTE lpAobMatch = NULL;
    LPCBYTE lpCurrentAddress = (LPCBYTE) AOBSCAN_LOW_ADDRESS_LIMIT;

    LPBYTE lpReadBuffer = VirtualAlloc(
        NULL,
        AOBSCAN_SCAN_CHUNK_SIZE,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (NULL == lpReadBuffer) {
        fprintf(
            stderr,
            "[-] VirtualAlloc(): E%lu\n",
            GetLastError()
        );
        return NULL;
    }
    
    MEMORY_BASIC_INFORMATION memInfo = { 0 };

    while ((DWORD64) lpCurrentAddress < AOBSCAN_HIGH_ADDRESS_LIMIT) {
        if (0 == ((DWORD64) lpCurrentAddress & 0x4000000)) {
            printf(
                "\r[*] Scanning memory: 0x%012llX",
                (DWORD64) lpCurrentAddress
            );
        }
        
        if (sizeof(memInfo) != VirtualQueryEx(
            g_ShifterConfig.hGameProcess,
            lpCurrentAddress,
            &memInfo,
            sizeof(MEMORY_BASIC_INFORMATION)
        )) {
            lpCurrentAddress += AOBSCAN_SCAN_CHUNK_SIZE;
            continue;
        }

        if (!IsAddressStateValid(
            memInfo.State,
            memInfo.Protect
        )) {
            lpCurrentAddress = (LPCBYTE) memInfo.BaseAddress + memInfo.RegionSize;
            continue;
        }
        
        LPCBYTE lpRegionBase = (LPCBYTE)memInfo.BaseAddress;
        SIZE_T cbRegionSize = memInfo.RegionSize;

        for (
            DWORD64 qwOffset = 0; 
            qwOffset < cbRegionSize; 
            qwOffset += AOBSCAN_SCAN_CHUNK_SIZE
        ) {
            SIZE_T cbChunkSize = 
                ((cbRegionSize - qwOffset) > AOBSCAN_SCAN_CHUNK_SIZE) 
                ? AOBSCAN_SCAN_CHUNK_SIZE 
                : (cbRegionSize - qwOffset);

            LPCBYTE lpChunkAddr = lpRegionBase + qwOffset;

            if (!ReadProcessMemory(
                g_ShifterConfig.hGameProcess,
                lpChunkAddr,
                lpReadBuffer,
                cbChunkSize,
                &cbBytesRead
            )) {
                continue;
            }

            for (DWORD64 qwIndex = 0; qwIndex + cbPatternSize <= cbBytesRead; ++qwIndex) {
                if (lpReadBuffer[qwIndex] != abyPattern[0]) { // next-level filter
                    continue;
                }

                if (EXIT_SUCCESS == memcmp(
                    lpReadBuffer + qwIndex,
                    abyPattern,
                    cbPatternSize
                )) {
                    LPCVOID lpTempMatch = lpChunkAddr + qwIndex;
                    if (TARGET_MODE_MULTIPLAYER == g_ShifterConfig.eTargetMode) {
                        if (!VerifyPlayerGear(
                            lpTempMatch,
                            eTargetGear
                        )) {
                            continue;
                        }
                    }

                    lpAobMatch = lpTempMatch;
                    goto _FINAL;
                }
            }
        }
        
        lpCurrentAddress = (LPCBYTE)memInfo.BaseAddress + memInfo.RegionSize;
    }

_FINAL:

    putchar('\n');

    VirtualFree(
        lpReadBuffer,
        0,
        MEM_RELEASE
    );

    return lpAobMatch;
}

SHIFT_GEAR ReadGear(
    CONST TARGET_GEAR eTargetGear
) {
    SHIFT_GEAR eGearValue = 0;
    SIZE_T cbBytesRead = 0;

    LPCVOID lpTargetAddress = (TARGET_GEAR_CURRENT == eTargetGear) 
        ? g_ShifterConfig.lpCurrentGearAddress 
        : g_ShifterConfig.lpLastGearAddress;


    if (!ReadProcessMemory(
        g_ShifterConfig.hGameProcess,
        lpTargetAddress,
        &eGearValue,
        sizeof(DWORD),
        &cbBytesRead
    )) {
        fprintf(
            stderr,
            "[-] ReadProcessMemory(): E%lu\n",
            GetLastError()
        );
        return GEAR_INVALID;
    }

    return eGearValue;
}