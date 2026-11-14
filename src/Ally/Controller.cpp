#include <windows.h>
#include "Ally/Controller.hpp"
#include "Ally/ProfileParser.hpp"
#include "Configuration/Config.hpp"
#include "Logging/LogManager.hpp"

namespace Ally
{
    static auto log = AnyFSE::Logging::LogManager::GetLogger("Controller");

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
            m_applied->rightStick.type = Hid::MCUMappingType::Mouse;
            m_applied->rightStick.mouseDirections[0] = static_cast<BYTE>(Hid::MappingActions::ScrollUp);
            m_applied->rightStick.mouseDirections[1] = static_cast<BYTE>(Hid::MappingActions::ScrollDown);
            m_applied->rightStick.mouseDirections[2] = static_cast<BYTE>(Hid::MappingActions::ScrollUp);
            m_applied->rightStick.mouseDirections[3] = static_cast<BYTE>(Hid::MappingActions::ScrollDown);

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
            return false;
        }
        const bool matches = after == *m_applied;
        // Keep the actual result so a partial write can also be restored,
        // provided the mapping has not changed since this snapshot.
        m_applied = after;
        if (!written || !matches)
        {
            log.Warn("Temporary controller mapping was not fully applied; retaining recovery state");
            return false;
        }
        log.Debug("Temporary mouse mapping %d applied; controller mapping snapshot retained", mode);
        return true;
    }

    void Controller::Stop()
    {
        if (!m_original)
            return;
        Hid::McuLock lock;
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
            m_applied = restored;
            if (!written || !(restored == *m_original))
            {
                log.Warn("Controller mapping was not fully restored; retaining recovery state");
                return;
            }
            log.Debug("Original controller mapping restored");
        }
        else
            log.Debug("Controller mapping changed externally; skipping restoration");
        m_original.reset();
        m_applied.reset();
        Close();
    }
}
