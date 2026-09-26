#pragma once
#include <windows.h>

namespace AnyFSE::App::Constants::Optimization
{
    inline constexpr wchar_t StateKey[] = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\AnyFSE\\GameBoost";
    inline constexpr wchar_t Journal[] = L"JournalV1";
    inline constexpr wchar_t Lock[] = L"Global\\AnyFSE.GameBoost.Lock";
    inline constexpr wchar_t GameBarKey[] = L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\GameBar";
    inline constexpr wchar_t GameDvrKey[] = L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GameDVR";
    inline constexpr wchar_t GamesKey[] =
        L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games";
    inline constexpr char PowerPlan[] = "power.plan";
    inline constexpr char CpuMinimum[] = "power.cpuMinimumAC";
    inline constexpr char UsbSuspend[] = "power.usbSuspendAC";
    inline constexpr char PciPower[] = "power.pciPowerAC";
    inline constexpr char Mouse[] = "mouse.acceleration";
    inline constexpr GUID Ultimate = {0xe9a42b02, 0xd5df, 0x448d, {0xaa, 0x00, 0x03, 0xf1, 0x47, 0x49, 0xeb, 0x61}};
    inline constexpr GUID High = {0x8c5e7fda, 0xe8bf, 0x4a96, {0x9a, 0x85, 0xa6, 0xe2, 0x3a, 0x8c, 0x63, 0x5c}};
    inline constexpr GUID CpuGroup = {0x54533251, 0x82be, 0x4824, {0x96, 0xc1, 0x47, 0xb6, 0x0b, 0x74, 0x0d, 0x00}};
    inline constexpr GUID CpuMin = {0x893dee8e, 0x2bef, 0x41e0, {0x89, 0xc6, 0xb5, 0x5d, 0x09, 0x29, 0x96, 0x4c}};
    inline constexpr GUID UsbGroup = {0x2a737441, 0x1930, 0x4402, {0x8d, 0x77, 0xb2, 0xbe, 0xbb, 0xa3, 0x08, 0xa3}};
    inline constexpr GUID UsbSetting = {0x48e6b7a6, 0x50f5, 0x4782, {0xa5, 0xd4, 0x53, 0xbb, 0x8f, 0x07, 0xe2, 0x26}};
    inline constexpr GUID PciGroup = {0x501a4d13, 0x42af, 0x4429, {0x9f, 0xd1, 0xa8, 0x21, 0x8c, 0x26, 0x8e, 0x20}};
    inline constexpr GUID PciSetting = {0xee12f906, 0xd277, 0x404b, {0xb6, 0xda, 0xe5, 0xfa, 0x1a, 0x57, 0x6d, 0xf5}};

    struct RegistrySetting { const char* id; const wchar_t* key; const wchar_t* name; DWORD number; const wchar_t* text; };
    inline constexpr RegistrySetting RegistrySettings[] = {
        {"gameMode.auto", GameBarKey, L"AutoGameModeEnabled", 1, nullptr},
        {"gameMode.allow", GameBarKey, L"AllowAutoGameMode", 1, nullptr},
        {"gameBar.capture", GameDvrKey, L"AppCaptureEnabled", 1, nullptr},
        {"mmcss.priority", GamesKey, L"Priority", 6, nullptr},
        {"mmcss.category", GamesKey, L"Scheduling Category", 0, L"High"}
    };
}
