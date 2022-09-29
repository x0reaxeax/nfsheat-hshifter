#include <stdio.h>
#include <wchar.h>
#include <Windows.h>
#include <tlhelp32.h>

#include "memscan.h"

#define BASE_HEXADECIMAL    16

typedef enum shift_gear {
    GEAR_REVERSE = 0,
    GEAR_1       = 2,
    GEAR_2,
    GEAR_3,
    GEAR_4,
    GEAR_5,
    GEAR_6,
    GEAR_7,
    GEAR_8
} shiftgear_t;

struct targetproc {
    HANDLE    hproc;
    UINT32    pid;
    BOOL      is_scanned;
    uintptr_t gearaddr;
    uintptr_t scanaddr[18];
    UINT8     addr_index;
    UINT8     max_index;
    DWORD     lastkey;
};

struct targetproc heatproc = { 0 };

DWORD getpid(void) {
    DWORD pid = 0;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (!Process32First(snapshot, &entry)) {
        goto _SKIP;
    }

    while (Process32Next(snapshot, &entry)) {
        if (0 == wcscmp(entry.szExeFile, L"NeedForSpeedHeat.exe")) {
            pid = entry.th32ProcessID;
            break;
        }
    }

_SKIP:
    CloseHandle(snapshot);
    return pid;
}

int shift(shiftgear_t gear) {
    size_t bytes_written = 0;
    
    WriteProcessMemory(heatproc.hproc, (void *) heatproc.gearaddr, &gear, sizeof(int), &bytes_written);
    
    if (0 == bytes_written) {
        fprintf(stderr, "[-] Unable to write memory: %lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

shiftgear_t readgear(shiftgear_t *gear) {
    shiftgear_t cgear = GEAR_1;
    shiftgear_t *pgear = (NULL != gear) ? gear : &cgear;
    size_t bytes_read = 0;

    ReadProcessMemory(heatproc.hproc, (void *) heatproc.gearaddr, pgear, sizeof(int), &bytes_read);

    if (0 == bytes_read) {
        fprintf(stderr, "[-] Unable to read memory: %lu\n", GetLastError());
        return GEAR_1;
    }

    return *pgear;
}

INT64 CALLBACK callback_function(int nCode, WPARAM wParam, LPARAM lParam) {
    KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *) lParam;
    shiftgear_t gear = 0;

    if (HC_ACTION != nCode) {
        goto _SKIP;
    }

    if (WM_SYSKEYDOWN != wParam && WM_KEYDOWN != wParam) {
        goto _SKIP;
    }

    switch (p->vkCode) {

        case VK_END:
            printf("[+] Exiting..\n");
            PostQuitMessage(EXIT_SUCCESS);
            goto _SKIP;

        case VK_ADD:
            if (heatproc.is_scanned) {
                if (heatproc.addr_index >= (heatproc.max_index - 1)) {
                    printf("[*] Jumping back to first address in list..\n");
                    heatproc.addr_index = 0;
                } else {
                    heatproc.addr_index++;
                }

                printf("[+] Setting gear address to 0x%02llx. Shift twice to different gears..\n\n", heatproc.scanaddr[heatproc.addr_index]);
                heatproc.gearaddr = heatproc.scanaddr[heatproc.addr_index];
            }
            break;

        default:
            break;
    }

    if (!(p->vkCode >= 0x30 && p->vkCode <= 0x38)) {
        goto _SKIP;
    }

    gear = (p->vkCode - 48);

    shiftgear_t gearcheck = GEAR_1;
    if (p->vkCode != heatproc.lastkey || gear != readgear(&gearcheck)) {
        printf("[*] Shifting to gear %u (key: %lu)\n", (0 == gear) ? gear : (gear - 1), (p->vkCode - 48));
        shift(gear);
        heatproc.lastkey = p->vkCode;
    }

_SKIP:
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}


void scan_helper(DWORD pid, MEMBLOCK *scan) {
    char discardc = 0;
    puts("\n[*] Shift into 4th gear and press ENTER..");
    
    discardc = getchar();
    scangear(scan, pid, 5);

    puts("[*] Shift into 3rd gear and press ENTER..");
    discardc = getchar();
    scangear(scan, pid, 4);

    puts("[*] Shift into 2nd gear and press ENTER..");
    discardc = getchar();
    scangear(scan, pid, 3);


    puts("[*] Shift into 1st gear and press ENTER..");
    discardc = getchar();
    scangear(scan, pid, 2);


    puts("[*] Shift into REVERSE and press ENTER..");
    discardc = getchar();
    scangear(scan, pid, 0);
}

int main(void) {
    MSG msg;
    int msgret = 0;
    HANDLE prochandle = NULL;

    char input_addr[18] = { 0 };


    puts("*** H-SHIFTER FOR NEED FOR SPEED HEAT BY X0REAXEAX ***");
    puts("[*] Searching for Need for Speed Heat process..");
    
    heatproc.pid = getpid();

    if (0 == heatproc.pid) {
        fprintf(stderr, "[-] Unable to find 'NeedForSpeedHeat.exe' process\n");
        return EXIT_FAILURE;
    }

    printf("[+] Found NeedForSpeedHeat.exe -> PID: %u\n\n", heatproc.pid);

    while (0 == heatproc.gearaddr) {
        puts("[*] Automatic memory scan can TEMPORARILY consume huge amount of RAM!");
        printf("[*] Enter gear address or 's' to perform automatic scan: ");
        fflush(stdout);

        fgets(input_addr, sizeof(input_addr), stdin);

        if ('s' == input_addr[0]) {
            MEMBLOCK *scan = NULL;
            scan = new_scan(heatproc.pid);

            if (NULL == scan) {
                fprintf(stderr, "[-] Unable to initialize memory scanner..\n");
                return EXIT_FAILURE;
            }
            scan_helper(heatproc.pid, scan);

            heatproc.is_scanned = TRUE;
            
            print_matches(scan, sizeof(int), heatproc.scanaddr, &heatproc.max_index);
            free_scan(scan);

            heatproc.gearaddr = heatproc.scanaddr[0];
        } else {
            heatproc.gearaddr = strtoull(input_addr, NULL, BASE_HEXADECIMAL);
        }
    }

    printf(
        "[+] Gear Shift Address: 0x%02llx\n"
        "[+] Obtaining handle to PID %lu..\n",
        heatproc.gearaddr, heatproc.pid
    );

    prochandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, heatproc.pid);

    if (NULL == prochandle) {
        fprintf(stderr, "[-] Unable to obtain process handle: E%lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    heatproc.hproc = prochandle;

    HHOOK kbhook = SetWindowsHookExA(WH_KEYBOARD_LL, callback_function, GetModuleHandle(NULL), 0);

    if (NULL == kbhook) {
        fprintf(stderr, "[-] Failed to register hook E%ld\n", GetLastError());
        return EXIT_FAILURE;
    }

    printf("\n[+] Successfully registered low-level keyboard hook. Starting capture..\n\n");

    while ((msgret = GetMessage(&msg, NULL, 0, 0)) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(kbhook);
    CloseHandle(prochandle);
    system("pause");
    return EXIT_SUCCESS;
}
