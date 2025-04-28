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

#define HSHIFTER_VERSION_MAJOR           2
#define HSHIFTER_VERSION_MINOR           0
#define HSHIFTER_VERSION_PATCH           0

#define PAGE_SIZE   0x1000

#define CONFIG_KNOWNFOLDERID_GUID               FOLDERID_Documents   // AppDataDocuments
#define CONFIG_DIRECTORY_NAME                   L"Heat-HShifter2"
#define CONFIG_FILE_NAME                        L"config.ini"
#define LOG_FILE_NAME                           L"Shifter.log"
#define LOG_BUFFER_SIZE                         0x1000

#define HEAT_GEAR_ADDRESS_NIBBLE                0x8
#define HEAT_LAST_GEAR_ADDRESS_NIBBLE           0x0
#define HEAT_CURRENT_GEAR_ARTIFACT_NIBBLE       0x4
#define HEAT_CURRENT_GEAR_ARTIFACT_OFFSET       0x14                // To be added to artifact
#define HEAT_LAST_GEAR_ARTIFACT_OFFSET          0x48                // To be added to artifact
#define HEAT_CURRENT_GEAR_ARTIFACT_SIZE         0x14
#define HEAT_LAST_GEAR_ARTIFACT_SIZE            0x28

#define AOBSCAN_LOW_ADDRESS_LIMIT               0x10000ULL
#define AOBSCAN_HIGH_ADDRESS_LIMIT              0x2FFFFFFFFULL
#define AOBSCAN_SCAN_CHUNK_SIZE                 0x40000U            // 256KB

#define AOBSCAN_CURRENT_GEAR_LIVE_MEMORY_OFFSET 0x30                // To be added
#define AOBSCAN_LAST_GEAR_LIVE_MEMORY_OFFSET    0xC                 // To be subtracted
#define AOBSCAN_LIVE_MEMORY_ITERATIONS          4                   // Number of different live memory values to check
#define AOBSCAN_LIVE_MEMORY_DELAY_MS            450                 // Delay between each live memory check
#define AOBSCAN_UPDATE_CHECKPOINT               0x6000000           // Visual updates are displayed per this threshold.
                                                                    //  - Setting this too low will cause performance issues

#define GET_NIBBLE(value) ((DWORD64)(value) & 0xF)

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

typedef enum _FOREGROUND_WINDOW {
    FGWIN_GAME = 0,
    FGWIN_SHIFTER,
    FGWIN_INVALID = 0xFFFFFFFF
} FOREGROUND_WINDOW, *LPFOREGROUND_WINDOW;

typedef struct _KEYBOARD_MAP {
    DWORD adwKeyCode[GEAR_8 + 1];
} KEYBOARD_MAP, *LPKEYBOARD_MAP;

typedef struct _SHIFTER_CONFIG {
    HANDLE hGameProcess;
    DWORD dwGameProcessId;
    DWORD dwShifterProcessId;
    DWORD dwShifterThreadId;

    DWORD dwCurrentGear;
    
    HWND hGameWindow;
    BOOL bGameWasMinimized;
    
    HWND hShifterWindow;

    HANDLE hShifterConsole;
    HANDLE hGearDisplayConsole;
    BOOLEAN bGearWindowEnabled;
    BOOLEAN bIsMainWindowVisible;
    BOOLEAN bIsGearWindowVisible;

    HANDLE hLogFile;
    LPBYTE szLogBuffer;
    BOOLEAN bEnableDebugLogging;

    KEYBOARD_MAP KeyboardMap;
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
///  Maximizes the target console window.
/// </summary>
/// <param name="hTargetWindow"></param>
/// <returns>
///  TRUE if the target window was successfully maximized, FALSE on failure.
/// </returns>
BOOLEAN MaximizeWindow(
    HWND hTargetWindow
);

/// <summary>
///  Forces the target window to the foreground.
/// </summary>
/// /// <param name="hTargetWindow"></param>
/// <returns>
///  TRUE if the target window was successfully forced to the foreground, FALSE on failure.
/// </returns>
BOOL ForceForegroundWindow(
    HWND hTargetWindow
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
///  Obtains the handle to the target game window.
/// </summary>
/// <param name=""></param>
/// <returns>
///  TRUE if the target window was successfully found, FALSE on failure.
/// </returns>
BOOLEAN FindGameWindow(
    VOID
);

/// <summary>
///  Checks if the target window is in the foreground.
/// </summary>
/// /// <returns>
///  TRUE if the target window is in the foreground, FALSE otherwise.
/// </returns>
BOOLEAN IsWindowForeground(
    FOREGROUND_WINDOW eTargetWindow
);

/// <summary>
///  Opens the log file for writing.
/// </summary>
/// /// <returns>
///  Handle to the log file if opened successfully, NULL on failure.
/// </returns>
HANDLE OpenLogFile(
    VOID
);

/// <summary>
///  Closes the log file.
/// </summary>
VOID CloseLogFile(
    VOID
);

/// <summary>
///  Writes a formatted message to the log file.
/// </summary>
/// /// <param name="szFormat"></param>
/// /// <param name="..."></param>
/// /// <returns>
///  TRUE if the message was successfully written, FALSE on failure.
/// </returns>
BOOLEAN WriteLog(
    LPCSTR szFormat,
    ...
);

/// <summary>
///  Checks if config file exists, and if not, creates it and saves its path.
/// </summary>
/// /// <returns>
///  TRUE if the config file was successfully created or located, FALSE on failure.
/// </returns>
BOOLEAN CreateConfig(
    VOID
);

/// <summary>
///  Loads the config file and retrieves the keyboard mapping.
/// </summary>
VOID LoadConfig(
    VOID
);

#endif // _HEAT_HSHIFTER2_GAMEHELPER_H