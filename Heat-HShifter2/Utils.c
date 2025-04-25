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

#include <stdio.h>

#include "Utils.h"

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

VOID DrawAsciiGearDisplay(
    VOID
) {
    ClearScreen(
        g_ShifterConfig.hGearConsoleWindow
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
        "[ *****> HEAT H-Shifter v2.0 <***** ]\n"
        "[ --------------------------------- ]\n"
        "[ 0 - 9  - Gear Control             ]\n"
        "[ INSERT - Toggle Gear/Main Window  ]\n"
        "[ DELETE - Rescan Gear Addresses    ]\n"
        "[ END    - Exit H-Shifter           ]\n"
        "[***********************************]\n"
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
        cTargetGear
    );

    dwCharsToWrite = (DWORD) strlen(szOutputBuffer);

    if (!WriteConsoleA(
        g_ShifterConfig.hGearConsoleWindow,
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