#include <Windows.h>
#include <stdio.h>

#include "Utils.h"

GLOBAL SHIFTER_CONFIG g_ShifterConfig = { 0 };

STATIC BOOLEAN InitShifter(
    VOID
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

    LPVOID lpRegionBaseAddress = GetGearRegionAddressBase();
    if (NULL == lpRegionBaseAddress) {
        fprintf(
            stderr,
            "[-] Unable to find gear region base address.\n"
        );
        return FALSE;
    }

    printf(
        "[+] Target region base address: 0x%llX\n",
        (DWORD64) lpRegionBaseAddress
    );

    printf(
        "[*] Scanning for string artifact...\n"
    );

    LPVOID lpStringArtifactAddress = ScanRegionForAsciiString(
        lpRegionBaseAddress,
        HEAT_GEAR_ADDRESS_REGION_SIZE,
        HEAT_ARTIFACT_VEHICLE_PHYSICS_JOB_STR
    );

    if (NULL == lpStringArtifactAddress) {
        fprintf(
            stderr,
            "[-] Unable to find string artifact.\n"
        );
        return FALSE;
    }

    printf(
        "[+] String artifact address: 0x%llX\n",
        (DWORD64) lpStringArtifactAddress
    );

    g_ShifterConfig.lpCurrentGearAddress = (LPVOID) (
        (DWORD64) lpStringArtifactAddress +
        HEAT_CURRENT_GEAR_ADDRESS_OFFSET
    );

    g_ShifterConfig.lpLastGearAddress = (LPVOID) (
        (DWORD64) g_ShifterConfig.lpCurrentGearAddress +
        HEAT_LAST_GEAR_ADDRESS_OFFSET
    );

    printf(
        "[+] Current gear address: 0x%llX\n"
        "[+] Last gear address: 0x%llX\n",
        (DWORD64) g_ShifterConfig.lpCurrentGearAddress,
        (DWORD64) g_ShifterConfig.lpLastGearAddress
    );

    return TRUE;
}

STATIC BOOL ShiftGear(
    SHIFT_GEAR eTargetGear
) {

    SIZE_T cbBytesWritten = 0;

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

    g_ShifterConfig.dwCurrentGear = eTargetGear;

    if (g_ShifterConfig.bGearWindowEnabled) {
        DrawAsciiGearDisplay();
    }

    return TRUE;
}

VOID SwitchWindows(
    VOID
) {
    g_ShifterConfig.dwCurrentGear = ReadGear();

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

    if (VK_INSERT == lpKbdHookStruct->vkCode) {
        g_ShifterConfig.bGearWindowEnabled = !g_ShifterConfig.bGearWindowEnabled;
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

    printf(
        "***************************************************\n"
        "***** Need for Speed Heat : Heat-HShifter2   ******\n"
        "***************************************************\n"
        "              github.com/x0reaxeax/Heat-HShifter2\n\n"
    );

    printf(
        "[*] Initializing Shifter...\n"
    );

    if (!InitShifter()) {
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

    // Read for initial gear display
    g_ShifterConfig.dwCurrentGear = ReadGear();

    DrawAsciiGearDisplay();

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