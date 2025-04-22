#ifndef _HEAT_HSHIFTER2_GAMEHELPER_H
#define _HEAT_HSHIFTER2_GAMEHELPER_H

#include <Windows.h>

#define GLOBAL
#define VOLATILE    volatile
#define STATIC      static

#define HEAT_GEAR_ADDRESS_NIBBLE                0x8
#define HEAT_LAST_GEAR_ADDRESS_NIBBLE           0x0
#define HEAT_LAST_GEAR_ADDRESS_OFFSET           0x6A8
#define HEAT_CURRENT_GEAR_ADDRESS_OFFSET        0x53D0
#define HEAT_ARTIFACT_VEHICLE_PHYSICS_JOB_STR   "vehiclePhysicsJob"

#define HEAT_GEAR_ADDRESS_REGION_SIZE           0x20000000U // 512MB

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
    GEAR_8
} SHIFT_GEAR, *LPSHIFT_GEAR;

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
///  Scans target memory for a known string artifact.
/// </summary>
/// <param name="lpBaseAddress"></param>
/// <param name="cbRegionSize"></param>
/// <param name="szTargetString"></param>
/// <returns>
///   Address of the string artifact if found, NULL on failure.
/// </returns>
LPVOID ScanRegionForAsciiString(
    LPCVOID lpBaseAddress,
    SIZE_T cbRegionSize,
    LPCSTR szTargetString
);

DWORD ReadGear(
    VOID
);

VOID ClearScreen(
    HANDLE hConsole
);

VOID DrawAsciiGearDisplay(
    VOID
);

#endif // _HEAT_HSHIFTER2_GAMEHELPER_H