#include <algorithm>
#include <cmath>
#include <windows.h>
#include <GameInput.h>
#include <wrl/client.h>
#include "Ally/Gamepad.hpp"
#include "App/Constants.hpp"
#include "Logging/LogManager.hpp"

namespace Ally
{
    namespace c = AnyFSE::App::Constants;
    static AnyFSE::Logging::Logger log = AnyFSE::Logging::LogManager::GetLogger("Gamepad");
    static constexpr DWORD PollIntervalMs = 8;

    // Documented v0 flag omitted from the Windows SDK 10.0.26100.0 header.
    // https://learn.microsoft.com/gaming/gdk/docs/reference/input/gameinput/enums/gameinputfocuspolicy
    static constexpr GameInputFocusPolicy EnableBackgroundInput = static_cast<GameInputFocusPolicy>(0x00000040);

    Gamepad::~Gamepad()
    {
        Stop();
        if (m_stopEvent)
            CloseHandle(m_stopEvent);
        if (m_gameInput)
            m_gameInput->Release();
        if (m_module)
            FreeLibrary(m_module);
    }

    bool Gamepad::Initialize()
    {
        if (m_gameInput)
            return true;

        if (!m_module)
            m_module = LoadLibraryEx(c::GameInputDll, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!m_module)
        {
            log.Error(log.APIError(), "Could not load system GameInput");
            return false;
        }

        auto create = reinterpret_cast<decltype(&GameInputCreate)>(GetProcAddress(m_module, c::GameInputCreateExport));
        if (!create)
        {
            log.Error(log.APIError(), "Could not find GameInput factory");
            return false;
        }

        const HRESULT result = create(&m_gameInput);
        if (FAILED(result))
        {
            log.Error("Could not initialize GameInput: 0x%08lX", static_cast<unsigned long>(result));
            return false;
        }
        m_gameInput->SetFocusPolicy(EnableBackgroundInput);
        return true;
    }

    bool Gamepad::Start()
    {
        if (m_thread)
            return true;
        if (!Initialize())
            return false;

        if (!m_stopEvent)
            m_stopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (!m_stopEvent || !ResetEvent(m_stopEvent))
        {
            log.Error(log.APIError(), "Could not prepare gamepad stop event");
            return false;
        }

        m_thread = CreateThread(nullptr, 0, Listen, this, 0, nullptr);
        if (!m_thread)
        {
            log.Error(log.APIError(), "Could not start gamepad worker");
            return false;
        }
        return true;
    }

    void Gamepad::Stop()
    {
        if (!m_thread)
            return;

        SetEvent(m_stopEvent);
        WaitForSingleObject(m_thread, INFINITE);
        CloseHandle(m_thread);
        m_thread = nullptr;
    }

    DWORD WINAPI Gamepad::Listen(LPVOID context)
    {
        static_cast<Gamepad*>(context)->Run();
        return 0;
    }

    void Gamepad::Run()
    {
        constexpr double MaximumSpeed = 800.0; // Relative mouse units per second.
        constexpr double DeadZone = 8689.0 / 32767.0; // XInput right-stick dead zone, normalized for GameInput.
        ULONGLONG previousTick = 0;
        double remainderX = 0.0;
        double remainderY = 0.0;
        Microsoft::WRL::ComPtr<IGameInputDevice> device;

        for (DWORD wait = 0; WaitForSingleObject(m_stopEvent, wait) == WAIT_TIMEOUT; wait = PollIntervalMs)
        {
            if (device && !(device->GetDeviceStatus() & GameInputDeviceConnected))
            {
                device.Reset();
                previousTick = 0;
                remainderX = remainderY = 0.0;
            }

            Microsoft::WRL::ComPtr<IGameInputReading> reading;
            GameInputGamepadState state = {};
            if (FAILED(m_gameInput->GetCurrentReading(GameInputKindGamepad, device.Get(), reading.GetAddressOf())) ||
                !reading->GetGamepadState(&state))
            {
                previousTick = 0;
                remainderX = remainderY = 0.0;
                continue;
            }

            // Keep the selected controller until it disconnects or the modifier is released.
            if (!device)
                reading->GetDevice(device.GetAddressOf());

            const ULONGLONG now = GetTickCount64();
            const double elapsed = previousTick ? (std::min)(static_cast<double>(now - previousTick) / 1000.0, 0.05) : 0.016;
            previousTick = now;

            const double x = state.rightThumbstickX;
            const double y = state.rightThumbstickY;
            const double magnitude = std::sqrt(x * x + y * y);
            if (magnitude <= DeadZone)
            {
                remainderX = remainderY = 0.0;
                continue;
            }

            const double strength = (std::min)((magnitude - DeadZone) / (1.0 - DeadZone), 1.0);
            const double distance = MaximumSpeed * strength * strength * strength * elapsed;
            remainderX += x / magnitude * distance;
            remainderY -= y / magnitude * distance;

            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            input.mi.dx = static_cast<LONG>(remainderX);
            input.mi.dy = static_cast<LONG>(remainderY);
            if (!input.mi.dx && !input.mi.dy)
                continue;

            if (WaitForSingleObject(m_stopEvent, 0) != WAIT_TIMEOUT)
                break;

            if (SendInput(1, &input, sizeof(input)) == 1)
            {
                remainderX -= input.mi.dx;
                remainderY -= input.mi.dy;
            }
            else
            {
                remainderX = remainderY = 0.0;
            }
        }
    }
}
