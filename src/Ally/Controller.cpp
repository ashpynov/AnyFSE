#include <windows.h>
#include "Ally/Controller.hpp"
#include "Ally/Ally.hpp"
#include "Ally/ProfileParser.hpp"
#include "Configuration/Config.hpp"
#include "Logging/LogManager.hpp"

namespace Ally
{
    static auto log = AnyFSE::Logging::LogManager::GetLogger("Controller");

    static void LogMappingDifferences(const char* operation, const Hid::Mapping& expected, const Hid::Mapping& actual)
    {
        const BYTE* expectedBytes = reinterpret_cast<const BYTE*>(&expected);
        const BYTE* actualBytes = reinterpret_cast<const BYTE*>(&actual);
        for (size_t offset = 0; offset < sizeof(Hid::Mapping); ++offset)
        {
            if (expectedBytes[offset] == actualBytes[offset])
                continue;
            const char* region = "buttons";
            size_t regionOffset = offset;
            unsigned block = unsigned(offset / sizeof(Hid::ButtonBlock)) + 1;
            if (offset < sizeof(Hid::ButtonMapping))
                regionOffset %= sizeof(Hid::ButtonBlock);
            else
            {
                regionOffset = offset - sizeof(Hid::ButtonMapping);
                block = 0;
                region = regionOffset < sizeof(Hid::StickMapping) ? "left stick" : "right stick";
                regionOffset %= sizeof(Hid::StickMapping);
            }
            log.Trace("%s mismatch: %s, block %u, byte %zu (mapping offset 0x%03zX): expected %02X, read %02X",
                operation, region, block, regionOffset, offset, unsigned(expectedBytes[offset]), unsigned(actualBytes[offset]));
        }
    }

    Controller::~Controller()
    {
        Stop();
        if (m_original)
            log.Warn("Controller destroyed with an unrestored temporary mapping");
        Close();
    }

    void Controller::Close()
    {
        if (m_device != INVALID_HANDLE_VALUE)
            CloseHandle(m_device);
        m_device = INVALID_HANDLE_VALUE;
        m_deviceLost = false;
    }

    bool Controller::Open()
    {
        if (m_device == INVALID_HANDLE_VALUE)
            m_device = Hid::Open();
        return m_device != INVALID_HANDLE_VALUE;
    }

    bool Controller::HasMouse(const Hid::StickMapping& stick)
    {
        return stick.mouseAction == Hid::MappingActions::MouseCursor;
    }

    bool Controller::Start(int mode)
    {
        if (mode != Config::CopyDesktopSticks && mode != Config::WindowsGamepadCursorMode)
            return false;
        if (m_original)
            return true;
        Hid::Mapping desktop;
        if (mode == Config::CopyDesktopSticks &&
            (!ProfileParser::Load(Profile::Mode::Desktop, desktop) || !(HasMouse(desktop.leftStick) || HasMouse(desktop.rightStick))))
            return false;
        Hid::McuLock lock;
        if (!lock.IsAcquired())
        {
            m_original.reset();
            m_applied.reset();
            Close();
            log.Warn("Controller locking failed; forgetting current Mode press and mapping snapshots");
            return false;
        }
        if (!Open())
            return false;

        Hid::Mapping before;
        if (!Hid::ReadMapping(m_device, before, m_deviceLost))
        {
            Close();
            return false;
        }
        if (HasMouse(before.leftStick) || HasMouse(before.rightStick))
        {
            Close();
            return true;
        }

        const bool scrollHotkeys = mode == Config::WindowsGamepadCursorMode && RegisterScrollHotkeys();

        // Keep the expected snapshot even on failed/uncertain writes. Stop may
        // restore only after reading an exact match; it never writes blindly.
        m_original = before;
        m_applied = before;
        for (Hid::ButtonBlock& block : m_applied->buttons.blocks)
        {
            block.first.primary = block.first.secondary;
            block.second.primary = block.second.secondary;
        }
        switch (mode)
        {
        case Config::CopyDesktopSticks:
            m_applied->leftStick = desktop.leftStick;
            m_applied->rightStick = desktop.rightStick;
            break;
        case Config::WindowsGamepadCursorMode:
        {
            m_applied->leftStick = {};
            m_applied->leftStick.type = Hid::MCUMappingType::Mouse;
            m_applied->leftStick.mouseAction = Hid::MappingActions::MouseCursor;

            m_applied->rightStick = {};
            if (scrollHotkeys)
            {
                m_applied->rightStick.type = Hid::MCUMappingType::Keyboard;
                // PS/2 set 2 codes for F14 through F23; these are not Windows virtual-key codes.
                static constexpr BYTE scrollScanCodes[] = { 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48, 0x50, 0x57 };
                for (size_t direction = 0; direction < 4; ++direction)
                    m_applied->rightStick.keyboardDirections[direction] = scrollScanCodes[m_scrollKeys[direction % 2] - VK_F14];
            }
            else
            {
                m_applied->rightStick.type = Hid::MCUMappingType::Mouse;
                for (size_t direction = 0; direction < 4; ++direction)
                    m_applied->rightStick.mouseDirections[direction] = static_cast<BYTE>(
                        direction % 2 == 0 ? Hid::MappingActions::ScrollUp : Hid::MappingActions::ScrollDown);
            }

            Hid::ButtonBlock& ab = m_applied->buttons.blocks[static_cast<size_t>(Hid::ButtonBlockIndex::AB)];
            ab.first.primary = {};
            ab.first.primary.type = Hid::MCUMappingType::Mouse;
            ab.first.primary.mouseAction = Hid::MappingActions::LeftClick;
            ab.first.secondary = ab.first.primary;
            ab.second.primary = {};
            ab.second.primary.type = Hid::MCUMappingType::Keyboard;
            ab.second.primary.keyboardKey = static_cast<BYTE>(Hid::KeyboardCode::Escape);
            ab.second.secondary = ab.second.primary;

            Hid::ButtonBinding& menu = m_applied->buttons.blocks[static_cast<size_t>(Hid::ButtonBlockIndex::ViewMenu)].second;
            menu.primary = {};
            menu.primary.type = Hid::MCUMappingType::Mouse;
            menu.primary.mouseAction = Hid::MappingActions::RightClick;
            menu.secondary = menu.primary;
            break;
        }
        }
        const bool written = Hid::WriteMapping(m_device, *m_applied);
        Hid::Mapping after;
        if (!Hid::ReadMapping(m_device, after, m_deviceLost))
        {
            log.Warn("Temporary mapping could not be verified; retaining recovery state");
            ReleaseScrollHotkeys();
            return false;
        }
        const bool matches = after == *m_applied;
        if (!matches)
            LogMappingDifferences("Temporary mapping", *m_applied, after);
        // Keep the actual result so a partial write can also be restored,
        // provided the mapping has not changed since this snapshot.
        m_applied = after;
        if (!written || !matches)
        {
            log.Warn("Temporary controller mapping was not fully applied; retaining recovery state");
            ReleaseScrollHotkeys();
            return false;
        }
        log.Debug("Temporary mouse mapping %d applied; controller mapping snapshot retained", mode);
        return true;
    }

    void Controller::Stop()
    {
        ReleaseScrollHotkeys();
        if (!m_original)
            return;
        Hid::McuLock lock;
        if (!lock.IsAcquired())
        {
            m_original.reset();
            m_applied.reset();
            Close();
            log.Warn("Controller locking failed; forgetting current Mode press and mapping snapshots");
            return;
        }
        // Never reopen a disconnected controller and restore a stale snapshot.
        Hid::Mapping current;
        if (!Hid::ReadMapping(m_device, current, m_deviceLost))
        {
            if (m_deviceLost)
            {
                m_original.reset();
                m_applied.reset();
                Close();
                log.Warn("Controller disconnected; discarding temporary mapping snapshot");
                return;
            }
            log.Warn("Could not check temporary mapping; restoration deferred");
            return;
        }
        if (current == *m_applied)
        {
            const bool written = Hid::WriteMapping(m_device, *m_original);
            if (written)
                m_applied = m_original;
            Hid::Mapping restored;
            if (!Hid::ReadMapping(m_device, restored, m_deviceLost))
                return;
            const bool matches = restored == *m_original;
            if (!matches)
                LogMappingDifferences("Original mapping", *m_original, restored);
            m_applied = restored;
            if (!written || !matches)
            {
                log.Warn("Controller mapping was not fully restored; retaining recovery state");
                return;
            }
            log.Debug("Original controller mapping restored");
        }
        else
        {
            LogMappingDifferences("Current vs retained mapping", *m_applied, current);
            log.Debug("Controller mapping changed externally; skipping restoration");
        }
        m_original.reset();
        m_applied.reset();
        Close();
    }

    bool Controller::RegisterScrollHotkeys()
    {
        ReleaseScrollHotkeys();
        int count = 0;
        for (UINT key = VK_F14; key <= VK_F23 && count < 2; ++key)
        {
            // Keep keyboard repeat enabled: each WM_HOTKEY produces one wheel step.
            if (RegisterHotKey(nullptr, count == 0 ? HotkeyScrollUp : HotkeyScrollDown, 0, key))
            {
                m_scrollKeys[count++] = key;
                log.Debug("Registered cursor scroll hotkey F%u", key - VK_F1 + 1);
            }
            else
            {
                const DWORD error = GetLastError();
                log.Debug("Cursor scroll hotkey F%u unavailable: error %lu", key - VK_F1 + 1, error);
            }
        }
        if (count == 2)
            return true;
        log.Warn("Could not register two cursor scroll hotkeys from F14-F23; using MCU mouse wheel actions");
        ReleaseScrollHotkeys();
        return false;
    }

    void Controller::ReleaseScrollHotkeys()
    {
        for (int index = 0; index < 2; ++index)
        {
            if (m_scrollKeys[index])
            {
                UnregisterHotKey(nullptr, index == 0 ? HotkeyScrollUp : HotkeyScrollDown);
                m_scrollKeys[index] = 0;
            }
        }
    }

}
