#pragma once

#include <cstring>
#include <windows.h>

namespace Ally::Hid
{
    enum class Command : BYTE
    {
        Read = 0x05,
        Customize = 0xD1,
        SetButtons = 0x02,
        SetSticks = 0x03,
        ButtonStatus = 0x0A
    };

    enum class MappingActions : BYTE
    {
        None = 0,
        LeftClick = 1,
        RightClick = 2,
        ScrollUp = 4,
        ScrollDown = 5,
        MouseCursor = 6
    };

    enum class MCUMappingType : BYTE
    {
        Unknown = 0,
        Gamepad = 1,
        Keyboard = 2,
        Mouse = 3,
        Shortcut = 4,
        Media = 5
    };

    enum class KeyboardCode : BYTE
    {
        Escape = 0x76, // MCU keyboard values use PS/2 scan code set 2.
        ModeM2 = 0x8E,
        ModeM1 = 0x8F
    };

    enum class GamepadButtonCode : BYTE
    {
        LegacyView = 15,
        LegacyMenu = 16,
        View = 17,
        Menu = 18
    };

    enum class ButtonBlockIndex
    {
        DpadVertical = 0,
        DpadHorizontal,
        StickButtons,
        Shoulders,
        AB,
        XY,
        ViewMenu,
        RearButtons,
        Triggers
    };

#pragma pack(push, 1)
    struct ButtonAction
    {
        MCUMappingType type {};
        BYTE gamepadButton {};
        BYTE keyboardKey {};
        BYTE mediaAction {};
        MappingActions mouseAction {};
        BYTE shortcutLength {};
        BYTE shortcutKeys[5] {};
    };

    struct ButtonBinding
    {
        ButtonAction primary {};
        ButtonAction secondary {};
    };

    struct ButtonBlock
    {
        ButtonBinding first {};
        ButtonBinding second {};
    };

    struct ButtonMapping
    {
        ButtonBlock blocks[9] {};
        bool operator==(const ButtonMapping& other) const
        {
            return std::memcmp(this, &other, sizeof(*this)) == 0;
        }
    };

    struct StickMapping
    {
        MCUMappingType type {};
        BYTE gamepadAxis {};
        // Direction arrays follow the MCU order: up, down, left, right.
        BYTE keyboardDirections[4] {};
        BYTE mediaDirections[4] {};
        MappingActions mouseAction {};
        BYTE mouseDirections[4] {};
        bool operator==(const StickMapping& other) const
        {
            return std::memcmp(this, &other, sizeof(*this)) == 0;
        }
    };

    struct Mapping
    {
        ButtonMapping buttons {};
        StickMapping leftStick {};
        StickMapping rightStick {};
        bool operator==(const Mapping& other) const
        {
            return buttons == other.buttons
                && leftStick == other.leftStick
                && rightStick == other.rightStick;
        }
    };

    struct FeatureReport
    {
        BYTE reportId;
        Command command;
        USHORT address; // Little-endian on Windows.
        BYTE reserved;
        BYTE length;
        BYTE payload[58];

        explicit FeatureReport(USHORT blockAddress, size_t count);

        // Match the response header to the request; payload contains the returned data.
        bool operator!=(const FeatureReport& other) const
        {
            return reportId != other.reportId
                || command != other.command
                || address != other.address
                || reserved != other.reserved
                || length != other.length;
        }
    };

    struct ButtonStatusReport
    {
        BYTE reportId;
        Command command;
        Command subcommand;
        BYTE length;
        BYTE status; // Opaque response payload; readiness is acknowledged by the subcommand.
        BYTE reserved[59];

        explicit ButtonStatusReport();
    };

    struct SetButtonsReport
    {
        BYTE reportId;
        Command command;
        Command subcommand;
        BYTE index;
        BYTE length;
        ButtonBlock buttons;
        BYTE reserved[15];

        explicit SetButtonsReport(BYTE blockIndex, const ButtonBlock& block);
    };

    struct SetSticksReport
    {
        BYTE reportId;
        Command command;
        Command subcommand;
        BYTE length;
        StickMapping leftStick;
        StickMapping rightStick;
        BYTE reserved[30];

        explicit SetSticksReport(const StickMapping& left, const StickMapping& right);
    };
#pragma pack(pop)

    class McuLock
    {
    public:
        McuLock();
        ~McuLock();
        McuLock(const McuLock&) = delete;
        McuLock& operator=(const McuLock&) = delete;
        bool IsAcquired() const { return m_acquired; }

    private:
        void Release();
        HANDLE m_handles[3] {};
        bool m_acquired = false;
    };

    HANDLE Open();
    bool ReadMapping(HANDLE device, Mapping& mapping, bool& deviceLost);
    bool WriteMapping(HANDLE device, const Mapping& mapping);
}
