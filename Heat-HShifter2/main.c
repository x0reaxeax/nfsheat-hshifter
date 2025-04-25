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

/// @file main.c
/// @brief The program retrieves 2 memory addresses from the game process
///  via memory artifacts, which are used as an offset base for the gear addresses.
/// 
///  The program uses a low-level keyboard hook to intercept key presses
///  and change gears in the game.
/// 
///  The hooked keys are:
///    *  - 0-9: Change gear (NOT NUMPAD!!)
///    *  - INSERT: Toggle between the gear display and the main console window
///    *  - DELETE: Re-scan for gear addresses (do this every time you enter and exit garage)
///    *  - END: Exit the program
///  
/// 
///  The program uses ReadProcessMemory and WriteProcessMemory as read/write primitives.
///   - github.con/x0reaxeax/nfsheat-hshifter
///  

#include <Windows.h>
#include <stdio.h>

#include "Utils.h"

// Verifies written gear value, but causes a 75ms delay
#define ENABLE_GEAR_VALIDATION

GLOBAL SHIFTER_CONFIG g_ShifterConfig = { 0 };

STATIC BOOLEAN ScanForGearAddresses(
    VOID
) {
    CONST BYTE abCurrentGearPattern[] = {
        0x9A, 0x99, 0x99, 0x3E, 0x66, 0x66, 0xE6, 0x3E,
        0xCD, 0xCC, 0x0C, 0x3F, 0x33, 0x33, 0x33, 0x3F,
        0x9A, 0x99, 0x59, 0x3F, 0x00, 0x00, 0x80, 0x3F,
        0xCD, 0xCC, 0x4C, 0x3E, 0x33, 0x33, 0x33, 0x3F,
        0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x00, 0x3F,
        0x00, 0x00, 0x40, 0x3F, 0x00, 0x00, 0x80, 0x3F,
        0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x7A, 0x44,
        0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xDA, 0x45
    };

    CONST BYTE abLastGearPattern[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x3F,
        0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x3F,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    LPCVOID lpCurrentGearArtifact = AobScan(
        abCurrentGearPattern,
        sizeof(abCurrentGearPattern),
        TARGET_GEAR_CURRENT
    );

    if (NULL == lpCurrentGearArtifact) {
        fprintf(
            stderr,
            "[-] Unable to find memory artifact.\n"
        );

        return FALSE;
    }

    printf(
        "[+] Memory artifact address (current gear): 0x%llX\n",
        (DWORD64) lpCurrentGearArtifact
    );

    LPCVOID lpLastGearArtifact = AobScan(
        abLastGearPattern,
        sizeof(abLastGearPattern),
        TARGET_GEAR_LAST
    );

    if (NULL == lpLastGearArtifact) {
        fprintf(
            stderr,
            "[-] Unable to find memory artifact.\n"
        );

        return FALSE;
    }

    printf(
        "[+] Memory artifact address (previous gear): 0x%llX\n",
        (DWORD64) lpLastGearArtifact
    );

    g_ShifterConfig.lpCurrentGearAddress = (LPVOID) (
        (DWORD64) lpCurrentGearArtifact +
        HEAT_CURRENT_GEAR_ARTIFACT_OFFSET
    );

    g_ShifterConfig.lpLastGearAddress = (LPVOID) (
        (DWORD64) lpLastGearArtifact +
        HEAT_LAST_GEAR_ARTIFACT_OFFSET
    );

    return TRUE;
}

STATIC BOOLEAN InitShifter(
    TARGET_MODE eTargetMode
) {
    g_ShifterConfig.dwCurrentGear = GEAR_1;
    g_ShifterConfig.bGearWindowEnabled = TRUE;

    g_ShifterConfig.dwGameProcessId = GetGameProcessId(
        L"NeedForSpeedHeat.exe"
    );

    if (0 == g_ShifterConfig.dwGameProcessId) {
        fprintf(
            stderr,
            "[-] Unable to initialize shifter - Heat process not found.\n"
        );
        return FALSE;
    }

    g_ShifterConfig.hGameProcess = OpenProcess(
        PROCESS_VM_OPERATION
        |    PROCESS_VM_READ
        |    PROCESS_VM_WRITE
        |    PROCESS_CREATE_THREAD
        |    PROCESS_QUERY_INFORMATION,
        FALSE,
        g_ShifterConfig.dwGameProcessId
    );

    if (NULL == g_ShifterConfig.hGameProcess) {
        fprintf(
            stderr,
            "[-] Unable to open process: E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    g_ShifterConfig.hConsoleWindow = GetStdHandle(STD_OUTPUT_HANDLE);
    if (INVALID_HANDLE_VALUE == g_ShifterConfig.hConsoleWindow) {
        fprintf(
            stderr,
            "[-] GetStdHandle(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    g_ShifterConfig.hGearConsoleWindow = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );

    if (INVALID_HANDLE_VALUE == g_ShifterConfig.hGearConsoleWindow) {
        fprintf(
            stderr,
            "[-] CreateConsoleScreenBuffer(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    if (TARGET_MODE_INVALID == eTargetMode) {
        CHAR cInputChar;
        printf(
            "[*] Select target mode:\n"
            "[1] Singleplayer\n"
            "[2] Multiplayer\n"
            "[$]: "
        );
        cInputChar = getchar();

        switch (cInputChar) {
            case '1':
                g_ShifterConfig.eTargetMode = TARGET_MODE_SINGLEPLAYER;
                break;
            case '2':
                g_ShifterConfig.eTargetMode = TARGET_MODE_MULTIPLAYER;
                break;
            default:
                fprintf(
                    stderr,
                    "[-] Invalid target mode.\n"
                );
                return FALSE;
        }
    }

    printf(
        "[*] Target mode: %s\n",
        (TARGET_MODE_SINGLEPLAYER == g_ShifterConfig.eTargetMode) ? "Singleplayer" : "Multiplayer"
    );

    printf(
        "[*] Scanning for memory artifact...\n"
    );

    if (!ScanForGearAddresses()) {
        fprintf(
            stderr,
            "[-] Unable to find gear addresses.\n"
        );
        return FALSE;
    }

    printf(
        "[+] Current gear address: 0x%llX\n"
        "[+] Last gear address: 0x%llX\n",
        (DWORD64) g_ShifterConfig.lpCurrentGearAddress,
        (DWORD64) g_ShifterConfig.lpLastGearAddress
    );

    CONST DWORD dwCurrentGear = ReadCurrentGear();
    CONST DWORD dwLastGear = ReadLastGear();

    if (dwCurrentGear != dwLastGear) {
        fprintf(
            stderr,
            "[-] Current gear doesn't correspond to last gear state (%lu != %lu)\n",
            dwCurrentGear,
            dwLastGear
        );
        return FALSE;
    }

    return TRUE;
}

STATIC BOOL ShiftGear(
    SHIFT_GEAR eTargetGear
) {

    SIZE_T cbBytesWritten = 0;

    // Writes go ASAP

    if (!WriteProcessMemory(
        g_ShifterConfig.hGameProcess,
        g_ShifterConfig.lpCurrentGearAddress,
        &eTargetGear,
        sizeof(DWORD),
        &cbBytesWritten
    )) {
        fprintf(
            stderr,
            "[-] WriteProcessMemory(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

    if (!WriteProcessMemory(
        g_ShifterConfig.hGameProcess,
        g_ShifterConfig.lpLastGearAddress,
        &eTargetGear,
        sizeof(DWORD),
        &cbBytesWritten
    )) {
        fprintf(
            stderr,
            "[-] WriteProcessMemory(): E%lu\n",
            GetLastError()
        );
        return FALSE;
    }

#ifdef ENABLE_GEAR_VALIDATION
    // Give the game some time to (in/)validate gear change
    Sleep(75);
#endif

    g_ShifterConfig.dwCurrentGear = ReadCurrentGear();//eTargetGear;

    if (g_ShifterConfig.bGearWindowEnabled) {
        DrawAsciiGearDisplay();
    }

    return TRUE;
}

VOID SwitchWindows(
    VOID
) {

    g_ShifterConfig.dwCurrentGear = ReadCurrentGear();

    HANDLE hTargetConsole = NULL;

    if (g_ShifterConfig.bGearWindowEnabled && g_ShifterConfig.bIsGearWindowVisible) {
        hTargetConsole = g_ShifterConfig.hConsoleWindow;
    } else if (g_ShifterConfig.bIsMainWindowVisible) {
        hTargetConsole = g_ShifterConfig.hGearConsoleWindow;
    } else {
        hTargetConsole = g_ShifterConfig.hConsoleWindow;
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

    if (hTargetConsole == g_ShifterConfig.hConsoleWindow) {
        g_ShifterConfig.bIsMainWindowVisible = TRUE;
        g_ShifterConfig.bIsGearWindowVisible = FALSE;
    } else {
        g_ShifterConfig.bIsMainWindowVisible = FALSE;
        g_ShifterConfig.bIsGearWindowVisible = TRUE;

        DrawAsciiGearDisplay();
    }
}

STATIC BOOLEAN SetMainWindowVisible(
    VOID
) {
    if (!SetConsoleActiveScreenBuffer(
        g_ShifterConfig.hConsoleWindow
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

INT64 CALLBACK KeyboardHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
) {
    LPKBDLLHOOKSTRUCT lpKbdHookStruct = (LPKBDLLHOOKSTRUCT) lParam;

    if (HC_ACTION != nCode) {
        goto _NEXT_HOOK;
    }

    if (WM_SYSKEYDOWN != wParam && WM_KEYDOWN != wParam) {
        goto _NEXT_HOOK;
    }

    if (VK_END == lpKbdHookStruct->vkCode) {
        PostQuitMessage(EXIT_SUCCESS);
        goto _NEXT_HOOK;
    }

    if (VK_DELETE == lpKbdHookStruct->vkCode) {
        SetMainWindowVisible();

        if (!ScanForGearAddresses()) {
            fprintf(
                stderr,
                "[-] Unable to find gear addresses.\n"
            );
            PostQuitMessage(EXIT_FAILURE);
        }

        if (g_ShifterConfig.bGearWindowEnabled) {
            SwitchWindows();
        }

        goto _NEXT_HOOK;
    }

    if (VK_INSERT == lpKbdHookStruct->vkCode) {
        SwitchWindows();
        goto _NEXT_HOOK;
    }

    if ((lpKbdHookStruct->vkCode >= 0x30 && lpKbdHookStruct->vkCode <= 0x38)) {
        ShiftGear(
            (SHIFT_GEAR) (lpKbdHookStruct->vkCode - 0x30)
        );
    }

_NEXT_HOOK:
    return CallNextHookEx(
        NULL,
        nCode,
        wParam,
        lParam
    );
}

int main(int argc, const char *argv[]) {
    MSG msg = { 0 };
    INT iMsgResult = 0;
    HHOOK hKeyboardHook = NULL;

    TARGET_MODE eTargetMode = TARGET_MODE_INVALID;

    if (argc >= 2) {
        if (EXIT_SUCCESS == strcmp(argv[1], "--single")) {
            eTargetMode = TARGET_MODE_SINGLEPLAYER;
        } else if (EXIT_SUCCESS == strcmp(argv[1], "--multi")) {
            eTargetMode = TARGET_MODE_MULTIPLAYER;
        } else {
            fprintf(
                stderr,
                "[-] Invalid target mode.\n"
            );
        }
    }

    printf(
        "*****************************************************\n"
        "***** Need for Speed Heat : Heat-HShifter v2   ******\n"
        "*****************************************************\n"
        "               github.com/x0reaxeax/nfsheat-hshifter\n\n"
    );

    printf(
        "[*] Initializing Shifter...\n"
    );

    if (!InitShifter(eTargetMode)) {
        fprintf(
            stderr,
            "[-] Unable to initialize shifter.\n"
        );
        return EXIT_FAILURE;
    }


    printf("[+] Shifter initialized.\n");

    hKeyboardHook = SetWindowsHookExA(
        WH_KEYBOARD_LL,
        KeyboardHookProc,
        GetModuleHandle(NULL),
        0
    );

    if (NULL == hKeyboardHook) {
        fprintf(
            stderr,
            "[-] SetWindowsHookExA(): E%lu\n",
            GetLastError()
        );
        return EXIT_FAILURE;
    }

    printf(
        "[+] Registered low-level keyboard hook.\n"
    );


    if (g_ShifterConfig.bGearWindowEnabled) {
        printf("[*] Switching to gear display console window..\n");
        
        if (!SetConsoleActiveScreenBuffer(
            g_ShifterConfig.hGearConsoleWindow
        )) {
            fprintf(
                stderr,
                "[-] SetConsoleActiveScreenBuffer(): E%lu\n",
                GetLastError()
            );
            return EXIT_FAILURE;
        }
    }

    // Read for initial gear display
    g_ShifterConfig.dwCurrentGear = ReadCurrentGear();

    if (g_ShifterConfig.bGearWindowEnabled) {
        DrawAsciiGearDisplay();
    }

    while ((iMsgResult = GetMessageA(
        &msg,
        NULL,
        0,
        0
    ))) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if (!SetConsoleActiveScreenBuffer(
        g_ShifterConfig.hConsoleWindow
    )) {
        fprintf(
            stderr,
            "[-] SetConsoleActiveScreenBuffer(): E%lu\n",
            GetLastError()
        );
    }

    ClearScreen(
        g_ShifterConfig.hGearConsoleWindow
    );

    printf(
        "[+] Unregistering low-level keyboard hook.\n"
    );

    if (!UnhookWindowsHookEx(
        hKeyboardHook
    )) {
        fprintf(
            stderr,
            "[-] UnhookWindowsHookEx(): E%lu\n",
            GetLastError()
        );
    }

    CloseHandle(
        g_ShifterConfig.hGearConsoleWindow
    );

    CloseHandle(
        g_ShifterConfig.hGameProcess
    );

    printf("[+] Exiting..\n");

    return EXIT_SUCCESS;
}