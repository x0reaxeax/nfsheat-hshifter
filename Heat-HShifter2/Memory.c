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
    LPCVOID lpcTargetLiveMemory,
    SIZE_T cbLiveMemorySize
) {
    BOOL bRet = FALSE;
    if (IsIconic(
        g_ShifterConfig.hGameWindow
    )) {
        // Live memory is only live if the game is not minimized
        if (!MaximizeWindow(
            g_ShifterConfig.hGameWindow
        )) {
            fprintf(
                stderr,
                "[-] Failed to maximize game window.\n"
            );
            return FALSE;
        }

        g_ShifterConfig.bGameWasMinimized = TRUE;

        // Give the game some time to recover, hopefully this should only be done once.
        Sleep(1000);
    }

    LPBYTE alpReads[AOBSCAN_LIVE_MEMORY_ITERATIONS] = { 0 };
    SIZE_T cbBytesRead = 0;

    for (DWORD i = 0; i < AOBSCAN_LIVE_MEMORY_ITERATIONS; ++i) {
        alpReads[i] = VirtualAlloc(
            NULL,
            cbLiveMemorySize,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE
        );

        if (NULL == alpReads[i]) {
            fprintf(
                stderr,
                "[-] VirtualAlloc(): E%lu\n",
                GetLastError()
            );
            return FALSE;
        }
        
        if (!ReadProcessMemory(
            g_ShifterConfig.hGameProcess,
            lpcTargetLiveMemory,
            alpReads[i],
            cbLiveMemorySize,
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

    // Check if all reads are the same
    for (DWORD i = 0; i < AOBSCAN_LIVE_MEMORY_ITERATIONS; ++i) {
        for (DWORD j = i + 1; j < AOBSCAN_LIVE_MEMORY_ITERATIONS; ++j) {
            if (EXIT_SUCCESS != memcmp(
                alpReads[i],
                alpReads[j],
                cbLiveMemorySize
            )) {
                // Reads are not the same - live memory detected
                bRet = TRUE;
                goto _FINAL;
            }
        }
    }

_FINAL:
    for (DWORD i = 0; i < AOBSCAN_LIVE_MEMORY_ITERATIONS; ++i) {
        if (NULL != alpReads[i]) {
            VirtualFree(
                alpReads[i],
                0,
                MEM_RELEASE
            );
        }
    }

    return bRet;
}

STATIC BOOLEAN VerifyPlayerGear(
    LPCVOID lpcCurrentArtifactAddress,
    TARGET_GEAR eTargetGear
) {
    DWORD dwReadValue = 0;
    SIZE_T cbBytesRead = 0;
    SIZE_T cbLiveMemorySize = 0;

    LPCVOID lpcTargetAddressGear;
    LPCVOID lpcTargetLiveMemory;
    LPCVOID lpcTargetStaticMemory;

    switch (eTargetGear) {
        case TARGET_GEAR_CURRENT:
            lpcTargetAddressGear = (LPCVOID) (
                (DWORD64) lpcCurrentArtifactAddress 
                    + HEAT_CURRENT_GEAR_ARTIFACT_OFFSET
            );

            lpcTargetLiveMemory = (LPCVOID) (
                (DWORD64) lpcCurrentArtifactAddress 
                    + AOBSCAN_CURRENT_GEAR_LIVE_MEMORY_OFFSET
            );

            lpcTargetStaticMemory = (LPCVOID) (
                (DWORD64) lpcTargetLiveMemory - sizeof(DWORD64)
            );

            cbLiveMemorySize = HEAT_CURRENT_GEAR_ARTIFACT_SIZE;

            break;

        case TARGET_GEAR_LAST:
            lpcTargetAddressGear = (LPCVOID) (
                (DWORD64) lpcCurrentArtifactAddress 
                    + HEAT_LAST_GEAR_ARTIFACT_OFFSET
            );

            lpcTargetLiveMemory = (LPCVOID) (
                (DWORD64) lpcCurrentArtifactAddress 
                    - AOBSCAN_LAST_GEAR_LIVE_MEMORY_OFFSET
            );

            lpcTargetStaticMemory = (LPCVOID) (
                (DWORD64) lpcTargetLiveMemory + sizeof(DWORD64)
            );

            cbLiveMemorySize = HEAT_LAST_GEAR_ARTIFACT_SIZE;

            break;

        default:
            fprintf(
                stderr,
                "[-] Invalid target gear: %d\n",
                eTargetGear
            );
            return FALSE;
    }

    // Validate gear sanity check first, since it is faster
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

    if (g_ShifterConfig.bSecondGearScan) {
        if (GEAR_2 != dwReadValue) {
            WriteLog(
                "[-] => %s():%lu Invalid gear at address: 0x%016llX\n",
                __FUNCTION__,
                __LINE__,
                (DWORD64) lpcTargetAddressGear
            );

            return FALSE;
        }
    } else {
        // Omit GEAR_REVERSE from this check, so the car can't be in reverse gear!!
        if (dwReadValue < GEAR_NEUTRAL || dwReadValue > GEAR_8) {
            WriteLog(
                "[-] => %s():%lu Invalid gear at address: 0x%016llX\n",
                __FUNCTION__,
                __LINE__,
                (DWORD64) lpcTargetAddressGear
            );
            return FALSE;
        }
    }

    // Verify known live memory
    if (!IsValueLiveMemory(
        lpcTargetLiveMemory,
        cbLiveMemorySize
    )) {
        WriteLog(
            "[-] => %s():%lu Omitting static memory at address: 0x%016llX\n",
            __FUNCTION__,
            __LINE__,
            (DWORD64) lpcCurrentArtifactAddress
        );
        return FALSE;
    }

    // Verify known static memory
    if (IsValueLiveMemory(
        (LPCVOID) ((DWORD64) lpcTargetStaticMemory),
        sizeof(DWORD)
    )) {
        WriteLog(
            "[-] => %s():%lu Omitting live memory at address: 0x%016llX\n",
            __FUNCTION__,
            __LINE__,
            (DWORD64) lpcTargetStaticMemory
        );
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
        PAGE_SIZE,
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

    // Save cursor position, but don't check for errors
    // to avoid sacrificing scan speed even more
    BOOLEAN bCursorPositionSaved = TRUE;
    CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };
    if (!GetConsoleScreenBufferInfo(
        g_ShifterConfig.hShifterConsole,
        &csbi
    )) {
        bCursorPositionSaved = FALSE;
    }

    while ((DWORD64) lpCurrentAddress < AOBSCAN_HIGH_ADDRESS_LIMIT) {
        if (
            0 == ((DWORD64) lpCurrentAddress & AOBSCAN_UPDATE_CHECKPOINT) 
            && bCursorPositionSaved
        ) {
            // Restore cursor position
            SetConsoleCursorPosition(
                g_ShifterConfig.hShifterConsole,
                csbi.dwCursorPosition
            );

            printf(
                "[*] Scanning memory: 0x%012llX\n",
                (DWORD64) lpCurrentAddress
            );
        }

        if (sizeof(memInfo) != VirtualQueryEx(
            g_ShifterConfig.hGameProcess,
            lpCurrentAddress,
            &memInfo,
            sizeof(MEMORY_BASIC_INFORMATION)
        )) {
            lpCurrentAddress += PAGE_SIZE;
            continue;
        }

        if (!IsAddressStateValid(
            memInfo.State,
            memInfo.Protect
        )) {
            lpCurrentAddress = (LPCBYTE) memInfo.BaseAddress + memInfo.RegionSize;
            continue;
        }

        LPCBYTE lpRegionEnd = (LPCBYTE) memInfo.BaseAddress + memInfo.RegionSize;
        for (
            LPCBYTE lpPageAddr = (LPCBYTE) memInfo.BaseAddress;
            lpPageAddr < lpRegionEnd;
            lpPageAddr += PAGE_SIZE
        ) {
            if (!ReadProcessMemory(
                g_ShifterConfig.hGameProcess,
                lpPageAddr,
                lpReadBuffer,
                PAGE_SIZE,
                &cbBytesRead
            )) {
                continue;
            }

            for (DWORD64 qwIndex = 0; qwIndex + cbPatternSize <= cbBytesRead; ++qwIndex) {
                if (lpReadBuffer[qwIndex] != abyPattern[0]) { // next-level filter
                    continue;
                }

                LPCVOID lpTempMatch = lpPageAddr + qwIndex;
                if (TARGET_GEAR_CURRENT == eTargetGear) {
                    if (HEAT_CURRENT_GEAR_ARTIFACT_NIBBLE != GET_NIBBLE(lpTempMatch)) {
                        continue;
                    }
                }

                if (EXIT_SUCCESS != memcmp(
                    lpReadBuffer + qwIndex,
                    abyPattern,
                    cbPatternSize
                )) {
                    continue;
                }
                
                WriteLog(
                    "[*] Testing pattern at address: 0x%016llX\n",
                    (DWORD64) lpTempMatch
                );

                if (!VerifyPlayerGear(
                    lpTempMatch,
                    eTargetGear
                )) {
                    continue;
                }

                if (!g_ShifterConfig.bSecondGearScan) {
                    // Check if artifact itself is live memory
                    if (IsValueLiveMemory(
                        lpTempMatch,
                        cbPatternSize
                    )) {
                        WriteLog(
                            "[-] => %s():%lu Omitting live memory at address: 0x%016llX\n",
                            __FUNCTION__,
                            __LINE__,
                            (DWORD64) lpTempMatch
                        );
                        continue;
                    }
                }

                lpAobMatch = lpTempMatch;
                goto _FINAL;
            }
        }
        lpCurrentAddress = (LPCBYTE) memInfo.BaseAddress + memInfo.RegionSize;
    }

_FINAL:
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