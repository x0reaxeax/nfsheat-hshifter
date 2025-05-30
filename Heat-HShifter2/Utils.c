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
/// @file Utils.c
/// @brief Helper functions for console window management and process ID retrieval.
/// 
///  - github.con/x0reaxeax/nfsheat-hshifter
/// 

#include <Windows.h>
#include <TlHelp32.h>
#include <ShlObj.h>
#include <Shlwapi.h>

#include <stdio.h>
#include <stdarg.h>

#include "Utils.h"

#pragma comment (lib, "Shlwapi.lib")

DWORD GetGameProcessId(
    LPCWSTR wszProcessName
) {
    DWORD dwPid = 0;

    PROCESSENTRY32 pe32 = {
        .dwSize = sizeof(PROCESSENTRY32)
    };

    HANDLE hSnapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS,
        0
    );

    if (INVALID_HANDLE_VALUE == hSnapshot) {
        fprintf(
            stderr,
            "[-] CreateToolhelp32Snapshot(): E%lu\n",
            GetLastError()
        );
        return 0;
    }

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (EXIT_SUCCESS == wcscmp(
                pe32.szExeFile,
                wszProcessName
            )) {
                dwPid = pe32.th32ProcessID;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return dwPid;
}

HANDLE OpenLogFile(
    VOID
) {
    WCHAR wszLogFilePath[MAX_PATH] = { 0 };
    WCHAR wszLogFileName[MAX_PATH] = { 0 };

    g_ShifterConfig.szLogBuffer = VirtualAlloc(
        NULL,
        LOG_BUFFER_SIZE,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (NULL == g_ShifterConfig.szLogBuffer) {
        fprintf(
            stderr,
            "[-] VirtualAlloc(): E%lu\n",
            GetLastError()
        );
        return NULL;
    }

    // Remove config file name from the path
    WCHAR *wszLastSlash = wcsrchr(
        g_ShifterConfig.wszConfigFilePath,
        L'\\'
    );

    if (NULL == wszLastSlash) {
        fprintf(
            stderr,
            "[-] wcsrchr(): E%lu\n",
            GetLastError()
        );
        return NULL;
    }

    *wszLastSlash = L'\0';

    if (!PathCombineW(
        wszLogFilePath,
        g_ShifterConfig.wszConfigFilePath,
        LOG_FILE_NAME
    )) {
        fprintf(
            stderr,
            "[-] PathCombineW(): E%lu\n",
            GetLastError()
        );
        return NULL;
    }

    wprintf(
        L"[*] Log file path: '%s'\n",
        wszLogFilePath
    );

    HANDLE hLogFile = CreateFileW(
        wszLogFilePath,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (INVALID_HANDLE_VALUE == hLogFile) {
        fprintf(
            stderr,
            "[-] CreateFileW(): E%lu\n",
            GetLastError()
        );
        return NULL;
    }

    return hLogFile;
}

VOID CloseLogFile(
    VOID
) {
    if (NULL != g_ShifterConfig.hLogFile) {
        CloseHandle(g_ShifterConfig.hLogFile);
        g_ShifterConfig.hLogFile = NULL;
    }

    if (NULL != g_ShifterConfig.szLogBuffer) {
        VirtualFree(
            g_ShifterConfig.szLogBuffer,
            0,
            MEM_RELEASE
        );
        g_ShifterConfig.szLogBuffer = NULL;
    }
}

BOOLEAN WriteLog(
    LPCSTR szFormat,
    ...
) {
    if (NULL == g_ShifterConfig.hLogFile) {
        return FALSE;
    }

    if (!g_ShifterConfig.bEnableDebugLogging) {
        return FALSE;
    }

    va_list args;
    va_start(args, szFormat);
    vsnprintf(
        g_ShifterConfig.szLogBuffer, 
        LOG_BUFFER_SIZE - 1,
        szFormat, 
        args
    );
    va_end(args);

    DWORD dwBytesWritten = 0;
    if (!WriteFile(
        g_ShifterConfig.hLogFile,
        g_ShifterConfig.szLogBuffer,
        (DWORD) strlen(g_ShifterConfig.szLogBuffer),
        &dwBytesWritten,
        NULL
    )) {
        fprintf(
            stderr,
            "[-] WriteFile(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    return TRUE;
}

BOOLEAN CreateConfig(
    VOID
) {
    HRESULT hResult;
    LPWSTR wszPath = NULL;

    hResult = SHGetKnownFolderPath(
        &CONFIG_KNOWNFOLDERID_GUID,
        0,
        NULL,
        &wszPath
    );

    if (FAILED(hResult)) {
        fprintf(
            stderr,
            "[-] SHGetKnownFolderPath(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    WCHAR wszDirPath[MAX_PATH] = { 0 };
    if (!PathCombineW(
        wszDirPath,
        wszPath,
        CONFIG_DIRECTORY_NAME
    )) {
        fprintf(
            stderr,
            "[-] PathCombineW(): E%lu\n",
            GetLastError()
        );
        CoTaskMemFree(wszPath);
        return FALSE;
    }

    if (!PathFileExistsW(wszDirPath)) {
        if (!CreateDirectoryW(
            wszDirPath,
            NULL
        )) {
            fprintf(
                stderr,
                "[-] CreateDirectoryW(): E%lu\n",
                GetLastError()
            );
            CoTaskMemFree(wszPath);
            return FALSE;
        }
        wprintf(
            L"[*] Created config directory: '%s'\n",
            wszDirPath
        );
    }

    WCHAR wszConfigPath[MAX_PATH] = {0};
    if (!PathCombineW(
        wszConfigPath,
        wszDirPath,
        CONFIG_FILE_NAME
    )) {
        fprintf(
            stderr,
            "[-] PathCombineW(): E%lu\n",
            GetLastError()
        );
        CoTaskMemFree(wszPath);
        return FALSE;
    }

    memcpy(
        g_ShifterConfig.wszConfigFilePath,
        wszConfigPath,
        sizeof(g_ShifterConfig.wszConfigFilePath)
    );

    CoTaskMemFree(wszPath);

    if (PathFileExistsW(wszConfigPath)) {
        wprintf(
            L"[*] Config file found: '%s'\n",
            wszConfigPath
        );
        return TRUE;
    }

    HANDLE hFile = CreateFileW(
        wszConfigPath,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (INVALID_HANDLE_VALUE == hFile) {
        fprintf(
            stderr,
            "[-] CreateFileW(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    CHAR szConfigData[] = {
        "[CONFIG]\n"
        "GEAR_REVERSE=0x30\n"
        "GEAR_NEUTRAL=0x31\n"
        "GEAR_1=0x32\n"
        "GEAR_2=0x33\n"
        "GEAR_3=0x34\n"
        "GEAR_4=0x35\n"
        "GEAR_5=0x36\n"
        "GEAR_6=0x37\n"
        "GEAR_7=0x38\n"
        "GEAR_8=0x39\n"
    };

    DWORD dwBytesWritten = 0;
    if (!WriteFile(
        hFile,
        szConfigData,
        sizeof(szConfigData) - 1,
        &dwBytesWritten,
        NULL
    )) {
        fprintf(
            stderr,
            "[-] WriteFile(): E%lu\n",
            GetLastError()
        );
        CloseHandle(hFile);
        return FALSE;
    }

    wprintf(
        L"[*] Created config file: '%s'\n",
        wszConfigPath
    );

    CloseHandle(hFile);

    return TRUE;
}

VOID LoadConfig(
    VOID
) {
    CONST WCHAR awszGearNames[GEAR_8 + 1][64] = { 
        L"GEAR_REVERSE",
        L"GEAR_NEUTRAL",
        L"GEAR_1",
        L"GEAR_2",
        L"GEAR_3",
        L"GEAR_4",
        L"GEAR_5",
        L"GEAR_6",
        L"GEAR_7",
        L"GEAR_8"
    };

    for (INT i = 0; i <= GEAR_8; i++) {
        g_ShifterConfig.KeyboardMap.adwKeyCode[i] = GetPrivateProfileIntW(
            L"CONFIG",
            awszGearNames[i],
            0x30 + i,
            g_ShifterConfig.wszConfigFilePath
        );
    }

    wprintf(
        L"[+] Loaded config file: '%s'\n",
        g_ShifterConfig.wszConfigFilePath
    );
}

BOOL CALLBACK EnumWindowProc(
    HWND hwnd,
    LPARAM lParam
) {
    DWORD dwPid = 0;
    GetWindowThreadProcessId(hwnd, &dwPid);

    if (dwPid != g_ShifterConfig.dwGameProcessId) {
        return TRUE; // Continue enumeration
    }
    
    CHAR szWindowsTitle[256] = { 0 };
    LPCSTR szTargetTitle = "Need for Speed";
    if (0 == GetWindowTextA(
        hwnd,
        szWindowsTitle,
        sizeof(szWindowsTitle) - 1
    )) {
        return TRUE; // Continue enumeration
    }
    if (EXIT_SUCCESS != memcmp(
        szWindowsTitle,
        szTargetTitle,
        strlen(szTargetTitle) - 1
    )) {
        return TRUE; // Continue enumeration
    }

    g_ShifterConfig.hGameWindow = hwnd;
    
    // Set found flag
    *(PBOOLEAN) lParam = TRUE; 

    return FALSE; // Stop enumeration
}

BOOLEAN FindGameWindow(
    VOID
) {
    if (NULL != g_ShifterConfig.hGameWindow) {
        // Already found
        return TRUE;
    }
    
    BOOLEAN bFound = FALSE;
    EnumWindows(
        EnumWindowProc,
        (LPARAM) &bFound
    );

    return bFound;
}

BOOLEAN IsWindowForeground(
    FOREGROUND_WINDOW eTargetWindow
) {
    HWND hWnd = GetForegroundWindow();
    if (NULL == hWnd) {
        return FALSE;
    }

    DWORD dwWindowPid = 0;
    if (0 == GetWindowThreadProcessId(
        hWnd,
        &dwWindowPid
    )) {
        fprintf(
            stderr,
            "[-] GetWindowThreadProcessId(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    if (FGWIN_GAME == eTargetWindow) {
        return (dwWindowPid == g_ShifterConfig.dwGameProcessId);
    } else if (FGWIN_SHIFTER == eTargetWindow) {
        return (dwWindowPid == g_ShifterConfig.dwShifterProcessId);
    } else {
        fprintf(
            stderr,
            "[-] Invalid target window: %d\n",
            eTargetWindow
        );
    }

    return FALSE;
}

BOOLEAN MaximizeWindow(
    HWND hTargetWindow
) {
    if (!ShowWindow(
        hTargetWindow,
        SW_RESTORE
    )) {
        fprintf(
            stderr,
            "[-] ShowWindow: E%lu\n", 
            GetLastError()
        );
        return FALSE;
    }

    if (!ShowWindow(
        hTargetWindow,
        SW_SHOW
    )) {
        fprintf(
            stderr,
            "[-] ShowWindow: E%lu\n", 
            GetLastError()
        );
        return FALSE;
    }

    return TRUE;
}

VOID ClearScreen(
    HANDLE hConsole
) {
    COORD coordScreen = { 0 };
    DWORD dwCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;

    if (!GetConsoleScreenBufferInfo(
        hConsole,
        &csbi
    )) {
        fprintf(
            stderr,
            "[-] GetConsoleScreenBufferInfo(): E%lu\n",
            GetLastError()
        );
        return;
    }

    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    // Fill the entire screen with blanks.
    if (!FillConsoleOutputCharacterA(
        hConsole,
        ' ',
        dwConSize,
        coordScreen,
        &dwCharsWritten
    )) {
        fprintf(
            stderr,
            "[-] FillConsoleOutputCharacterA(): E%lu\n",
            GetLastError()
        );
        return;
    }

    if (!GetConsoleScreenBufferInfo(
        hConsole,
        &csbi
    )) {
        fprintf(
            stderr,
            "[-] GetConsoleScreenBufferInfo(): E%lu\n",
            GetLastError()
        );
        return;
    }

    if (!FillConsoleOutputAttribute(
        hConsole,
        csbi.wAttributes,
        dwConSize,
        coordScreen,
        &dwCharsWritten
    )) {
        fprintf(
            stderr,
            "[-] FillConsoleOutputAttribute(): E%lu\n",
            GetLastError()
        );
        return;
    }

    if (!SetConsoleCursorPosition(
        hConsole,
        coordScreen
    )) {
        fprintf(
            stderr,
            "[-] SetConsoleCursorPosition(): E%lu\n",
            GetLastError()
        );
    }
}

BOOLEAN MAYBE_UNUSED ForceForegroundWindow(
    HWND hTargetWindow
) {
    DWORD dwForegroundThreadId = GetWindowThreadProcessId(
        GetForegroundWindow(),
        NULL
    );

    if (0 == dwForegroundThreadId) {
        fprintf(
            stderr,
            "[-] GetWindowThreadProcessId(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    if (!AttachThreadInput(
        dwForegroundThreadId,
        g_ShifterConfig.dwShifterThreadId,
        TRUE
    )) {
        fprintf(
            stderr,
            "[-] AttachThreadInput(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    if (!SetForegroundWindow(
        hTargetWindow
    )) {
        fprintf(
            stderr,
            "[-] SetForegroundWindow(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    if (!BringWindowToTop(
        hTargetWindow
    )) {
        fprintf(
            stderr,
            "[-] BringWindowToTop(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    if (NULL == SetFocus(
        hTargetWindow
    )) {
        fprintf(
            stderr,
            "[-] SetFocus(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    if (!AttachThreadInput(
        dwForegroundThreadId,
        g_ShifterConfig.dwShifterThreadId,
        FALSE
    )) {
        fprintf(
            stderr,
            "[-] AttachThreadInput(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    return TRUE;
}

VOID SwitchWindows(
    VOID
) {

    g_ShifterConfig.dwCurrentGear = ReadCurrentGear();

    HANDLE hTargetConsole = NULL;

    if (g_ShifterConfig.bGearWindowEnabled && g_ShifterConfig.bIsGearWindowVisible) {
        hTargetConsole = g_ShifterConfig.hShifterConsole;
    } else if (g_ShifterConfig.bIsMainWindowVisible) {
        hTargetConsole = g_ShifterConfig.hGearDisplayConsole;
    } else {
        hTargetConsole = g_ShifterConfig.hShifterConsole;
    }

    if (!SetConsoleActiveScreenBuffer(
        hTargetConsole
    )) {
        fprintf(
            stderr,
            "[-] SetConsoleActiveScreenBuffer(): E%lu\n",
            GetLastError()
        );
        return;
    }

    if (hTargetConsole == g_ShifterConfig.hShifterConsole) {
        g_ShifterConfig.bIsMainWindowVisible = TRUE;
        g_ShifterConfig.bIsGearWindowVisible = FALSE;
    } else {
        g_ShifterConfig.bIsMainWindowVisible = FALSE;
        g_ShifterConfig.bIsGearWindowVisible = TRUE;

        DrawAsciiGearDisplay();
    }
}

BOOLEAN SetMainWindowVisible(
    VOID
) {
    if (!SetConsoleActiveScreenBuffer(
        g_ShifterConfig.hShifterConsole
    )) {
        fprintf(
            stderr,
            "[-] SetConsoleActiveScreenBuffer(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    g_ShifterConfig.bIsMainWindowVisible = TRUE;
    g_ShifterConfig.bIsGearWindowVisible = FALSE;
    return TRUE;
}

BOOLEAN MAYBE_UNUSED SetMinimizeButtonVisible(
    HWND hWnd, 
    BOOLEAN bVisible
) {
    LONG lStyle = GetWindowLong(
        hWnd, 
        GWL_STYLE
    );

    if (0 == lStyle) {
        fprintf(
            stderr,
            "[-] GetWindowLong(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    if (bVisible) {
        lStyle |= WS_MINIMIZEBOX;
    } else {
        lStyle &= ~WS_MINIMIZEBOX;
    }

    if (0 == SetWindowLong(
        hWnd, 
        GWL_STYLE, 
        lStyle
    )) {
        fprintf(
            stderr,
            "[-] SetWindowLong(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    return SetWindowPos(
        hWnd,
        NULL,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED
    );
}

BOOLEAN IsWindowInWindowedMode(
    HWND hWnd
) {
    if (!IsWindow(hWnd)) {
        return FALSE;
    }

    LONG lStyle = GetWindowLong(hWnd, GWL_STYLE);

    if (WS_OVERLAPPEDWINDOW & lStyle) {
        return TRUE;
    }

    if ((WS_CAPTION & lStyle) && (WS_SYSMENU & lStyle)) {
        return TRUE;
    }

    return FALSE;
}

VOID DrawAsciiGearDisplay(
    VOID
) {
    ClearScreen(
        g_ShifterConfig.hGearDisplayConsole
    );

    DWORD dwCharsToWrite;
    DWORD dwCharsWritten = 0;
    CHAR szOutputBuffer[1024] = { 0 };

    CHAR cTargetGear = '1';

    switch (g_ShifterConfig.dwCurrentGear) {
        case GEAR_REVERSE:
            cTargetGear = 'R';
            break;

        case GEAR_NEUTRAL:
            cTargetGear = 'N';
            break;

        default:
            cTargetGear = (CHAR) ('0' + g_ShifterConfig.dwCurrentGear - 1);
            break;
    }

    snprintf(
        szOutputBuffer, 
        sizeof(szOutputBuffer),
        "[ ******> HEAT H-Shifter v%u.%u <****** ]\n"
        "[ ----------------------------------- ]\n"
        "[ 0 - 9  - Gear Control               ]\n"
        "[ INSERT - Toggle Gear/Main Window    ]\n"
        "[ DELETE - Rescan Gear Addresses      ]\n"
        "[ END    - Exit H-Shifter             ]\n"
        "[*************************************]\n"
        "\n"
        "\n"
        "        ___________\n"
        "       /           \\\n"
        "      /             \\\n"
        "     /               \\\n"
        "    |       ___       |\n"
        "    |      /   \\      |\n"
        "    |     |  %c  |     |\n"
        "    |      \\___/      |\n"
        "     \\               /\n"
        "      \\             /\n"
        "       \\___________/\n",
        HSHIFTER_VERSION_MAJOR,
        HSHIFTER_VERSION_MINOR,
        cTargetGear
    );

    dwCharsToWrite = (DWORD) strlen(szOutputBuffer);

    if (!WriteConsoleA(
        g_ShifterConfig.hGearDisplayConsole,
        szOutputBuffer,
        dwCharsToWrite,
        &dwCharsWritten,
        NULL
    )) {
        fprintf(
            stderr,
            "[-] WriteConsoleA(): E%lu\n",
            GetLastError()
        );
    }
}