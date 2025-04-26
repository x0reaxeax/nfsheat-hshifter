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
#include <KnownFolders.h>

#define GLOBAL
#define VOLATILE    volatile
#define STATIC      static
#define INLINE      inline

#define PAGE_SIZE   0x1000

#define CONFIG_KNOWNFOLDERID_GUID               FOLDERID_Documents   // AppDataDocuments
#define CONFIG_DIRECTORY_NAME                   L"Heat-HShifter2"
#define CONFIG_FILE_NAME                        L"config.ini"

#define HEAT_GEAR_ADDRESS_NIBBLE                0x8
#define HEAT_LAST_GEAR_ADDRESS_NIBBLE           0x0
#define HEAT_CURRENT_GEAR_ARTIFACT_OFFSET       0x98            // To be added to artifact
#define HEAT_LAST_GEAR_ARTIFACT_OFFSET          0x48

#define AOBSCAN_LOW_ADDRESS_LIMIT               0x10000ULL      //0x92400000ULL
#define AOBSCAN_HIGH_ADDRESS_LIMIT              0x2FFFFFFFFULL
#define AOBSCAN_SCAN_CHUNK_SIZE                 0x40000U        // 256KB

#define AOBSCAN_CURRENT_GEAR_LIVE_MEMORY_OFFSET 0xB4            // To be added
#define AOBSCAN_LAST_GEAR_LIVE_MEMORY_OFFSET    0xC             // To be subtracted
#define AOBSCAN_LIVE_MEMORY_ITERATIONS          3               // Number of different live memory values to check
#define AOBSCAN_LIVE_MEMORY_DELAY_MS            350             // Delay between each live memory check

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
    TARGET_GEAR_LAST,
    TARGET_GEAR_INVALID = 0xFFFFFFFF
} TARGET_GEAR, *LPTARGET_GEAR;

typedef enum _TARGET_MODE {
    TARGET_MODE_SINGLEPLAYER = 0,
    TARGET_MODE_MULTIPLAYER,
    TARGET_MODE_INVALID = 0xFFFFFFFF
} TARGET_MODE, *LPTARGET_MODE;

typedef struct _KEYBOARD_MAP {
    DWORD adwKeyCode[GEAR_8 + 1];
} KEYBOARD_MAP, *LPKEYBOARD_MAP;

typedef struct _SHIFTER_CONFIG {
    HANDLE hGameProcess;
    DWORD dwGameProcessId;
    DWORD dwShifterProcessId;

    DWORD dwCurrentGear;

    TARGET_MODE eTargetMode;
    
    HANDLE hConsoleWindow;
    HANDLE hGearConsoleWindow;
    BOOLEAN bGearWindowEnabled;
    BOOLEAN bIsMainWindowVisible;
    BOOLEAN bIsGearWindowVisible;

    KEYBOARD_MAP KeyboardMap;   // WIP config remapping
    WCHAR wszConfigFilePath[MAX_PATH];

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
///  Scans target memory for a known signature/artifact.
/// </summary>
/// <param name="abyPattern"></param>
/// <param name="cbPatternSize"></param>
/// <param name="eTargetGear"></param>
/// <returns>
///   Address of the signature/artifact if found, NULL on failure.
/// </returns>
LPCVOID AobScan(
    LPCBYTE abyPattern,
    CONST SIZE_T cbPatternSize,
    TARGET_GEAR eTargetGear
);

/// <summary>
///  Reads the current or last gear value from the target memory.
/// </summary>
/// <param name="eTargetGear"></param>
/// <returns>
///  Current or last gear value if found, GEAR_INVALID on failure.
SHIFT_GEAR ReadGear(
    CONST TARGET_GEAR eTargetGear
);

#define ReadCurrentGear() \
    ReadGear(TARGET_GEAR_CURRENT)
#define ReadLastGear() \
    ReadGear(TARGET_GEAR_LAST)

/// <summary>
///  Prompts the user to select the target mode.
/// </summary>
/// /// <returns>
///  TRUE if the target mode was successfully set, FALSE on failure.
/// </returns>
BOOLEAN ChangeModePrompt(
    VOID
);

/// <summary>
///  Switches between the main console window and the gear console window.
/// </summary>
VOID SwitchWindows(
    VOID
);

/// <summary>
///  Sets the main console window to be visible.
/// </summary>
/// <returns>
///  TRUE if the main console window was successfully set to be visible, FALSE on failure.
/// </returns>
BOOLEAN SetMainWindowVisible(
    VOID
);

/// <summary>
///  Clears target console window.
/// </summary>
/// /// <param name="hConsole"></param>
VOID ClearScreen(
    HANDLE hConsole
);

/// <summary>
///  Draws the current gear value in the target console window.
/// </summary>
VOID DrawAsciiGearDisplay(
    VOID
);

/// <summary>
///  Checks if the target game window is in the foreground.
/// </summary>
/// /// <returns>
///  TRUE if the target game window is in the foreground, FALSE otherwise.
/// </returns>
BOOLEAN IsGameWindowForeground(
    VOID
);

BOOLEAN CreateConfig(
    VOID
);

VOID LoadConfig(
    VOID
);

#endif // _HEAT_HSHIFTER2_GAMEHELPER_H