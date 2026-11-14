#pragma once

#include <windows.h>

struct IGameInput;

namespace Ally
{
    // Start and Stop are called by the HID listener thread; the worker owns all polling state.
    class Gamepad
    {
    public:
        Gamepad() = default;
        ~Gamepad();
        Gamepad(const Gamepad&) = delete;
        Gamepad& operator=(const Gamepad&) = delete;

        bool Start();
        void Stop();

    private:
        bool Initialize();
        static DWORD WINAPI Listen(LPVOID context);
        void Run();

        HMODULE m_module = nullptr;
        IGameInput* m_gameInput = nullptr;
        HANDLE m_stopEvent = nullptr;
        HANDLE m_thread = nullptr;
    };
}
