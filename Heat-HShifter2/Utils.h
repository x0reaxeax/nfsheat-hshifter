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
/// 

/// @file GameHelper.h
///   - github.con/x0reaxeax/nfsheat-hshifter

#ifndef _HEAT_HSHIFTER2_GAMEHELPER_H
#define _HEAT_HSHIFTER2_GAMEHELPER_H

#include <Windows.h>

#define GLOBAL
#define VOLATILE    volatile
#define STATIC      static
#define INLINE      inline

#define PAGE_SIZE   0x1000

#define HEAT_GEAR_ADDRESS_NIBBLE                0x8
#define HEAT_LAST_GEAR_ADDRESS_NIBBLE           0x0
#define HEAT_CURRENT_GEAR_ARTIFACT_OFFSET       0x98
#define HEAT_LAST_GEAR_ARTIFACT_OFFSET          0x48

#define AOBSCAN_LOW_ADDRESS_LIMIT               0x92400000ULL
#define AOBSCAN_HIGH_ADDRESS_LIMIT              0x2FFFFFFFFULL
#define AOBSCAN_SCAN_CHUNK_SIZE                 0x40000U        // 256KB

//#define HEAT_LAST_GEAR_ADDRESS_OFFSET           0x6A8
//#define HEAT_LAST_GEAR_ADDRESS_OFFSET_SECONDARY 0x428
//#define HEAT_CURRENT_GEAR_ADDRESS_OFFSET        0x53D0
//#define HEAT_ARTIFACT_VEHICLE_PHYSICS_JOB_STR   "vehiclePhysicsJob"

//#define HEAT_GEAR_ADDRESS_REGION_SIZE           0x20000000U // 512MB
//#define HEAT_SECONDARY_GEAR_ADDRESS_REGION_SIZE 0x20F0000   //0x1040000U

#define GET_NIBBLE(value) \
    ((value) & 0x0F) << 4 | \
    ((value) & 0xF0) >> 4

typedef enum _SHIFT_GEAR {
    GEAR_REVERSE = 0,
    GEAR_NEUTRAL,
    GEAR_1,
    GEAR_2,
    GEAR_3,
    GEAR_4,
    GEAR_5,
    GEAR_6,
    GEAR_7,
    GEAR_8,
    GEAR_INVALID = 0xFFFFFFFF
} SHIFT_GEAR, *LPSHIFT_GEAR;

typedef enum _TARGET_GEAR {
    TARGET_GEAR_CURRENT = 0,
    TARGET_GEAR_LAST
} TARGET_GEAR, *LPTARGET_GEAR;

typedef struct _SHIFTER_CONFIG {
    HANDLE hGameProcess;
    DWORD dwGameProcessId;

    DWORD dwCurrentGear;
    
    HANDLE hConsoleWindow;
    HANDLE hGearConsoleWindow;
    BOOLEAN bGearWindowEnabled;
    BOOLEAN bIsMainWindowVisible;
    BOOLEAN bIsGearWindowVisible;

    LPVOID lpCurrentGearAddress;
    LPVOID lpLastGearAddress;
} SHIFTER_CONFIG, *LPSHIFTER_CONFIG;

EXTERN_C GLOBAL SHIFTER_CONFIG g_ShifterConfig;

/// <summary>
///  Retrieves process ID of the target game process.
/// </summary>
/// <param name="wszProcessName"></param>
/// <returns>
///  Process ID of the target game process if found, 0 on failure.
/// </returns>
DWORD GetGameProcessId(
    LPCWSTR wszProcessName
);

/// <summary>
///  Retrieves base address of the target gear region.
/// </summary>
/// <returns>
///  Base address of the target gear region if found, NULL on failure.
/// </returns>
LPVOID GetGearRegionAddressBase(
    VOID
);

/// <summary>
///  Scans target memory for a known signature/artifact.
/// </summary>
/// <param name="abyPattern"></param>
/// <param name="cbPatternSize"></param>
/// <returns>
///   Address of the signature/artifact if found, NULL on failure.
/// </returns>
LPCVOID AobScan(
    LPCBYTE abyPattern,
    CONST SIZE_T cbPatternSize
);

DWORD ReadGear(
    CONST TARGET_GEAR eTargetGear
);

#define ReadCurrentGear() \
    ReadGear(TARGET_GEAR_CURRENT)
#define ReadLastGear() \
    ReadGear(TARGET_GEAR_LAST)

VOID ClearScreen(
    HANDLE hConsole
);

VOID DrawAsciiGearDisplay(
    VOID
);

#endif // _HEAT_HSHIFTER2_GAMEHELPER_H