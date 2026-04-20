#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <initguid.h>
#include <vector>
#include <psapi.h>
#include "Ally/Handlers.hpp"
#include "Tools/Process.hpp"
#include "Tools/Unicode.hpp"
#include "Logging/LogManager.hpp"
#include "Handlers.hpp"

#pragma comment(lib, "psapi.lib")

namespace Ally::Handlers
{
    static Logger log = LogManager::GetLogger("Handlers");

    std::vector<HandlersDefinition> KnownHandlers =
    {
        { L"", L"None", NULL },
        { L"HomeApp", L"Home", Handlers::OpenLibrary },
        { L"TaskSwitcher", L"Task switcher", Handlers::OpenTaskSwitcher },
        { L"TaskSwitcherAlt", L"Task switcher (Win+Tab)", Handlers::OpenTaskSwitcherAlt },
        { L"GamebarCommandCenter", L"GameBar Command Center", Handlers::OpenGameBarComandCenter },
        { L"Gamebar", L"GameBar", Handlers::OpenGameBar },
        { L"CommandCenter", L"Command Center (Ctrl+Alt+C)", Handlers::OpenComandCenter },
        { L"CommandCenterF24", L"Command Center (F24)", Handlers::OpenComandCenterF24 },
        { L"ArmouryCrate", L"Armoury Crate SE", Handlers::OpenArmouryCrate },
        { L"SteamOverlay", L"Steam Overlay (Shift+Tab)", Handlers::OpenSteamOverlay },
        { L"SteamQAM", L"Steam Quick Menu (Ctrl+2)", Handlers::OpenSteamQuickMenu },
        { L"AnyFSESettings", L"AnyFSE Settings", Handlers::OpenAnyFSESettings },
        { L"Keyboard", L"On-Screen Keyboard", Handlers::ShowKeyboard },
    };

    std::function<void()> Handlers::GetByName(const std::wstring &handleName)
    {
        for (auto handle : KnownHandlers)
        {
            if (!_wcsicmp(handle.code.c_str(), handleName.c_str()))
            {
                return handle.handler;
            }
        }
        return NULL;
    }

    bool SteamIsActive()
    {
        std::wstring activeProcess = Unicode::to_lower(Process::GetWindowProcessName(WindowFromPoint(POINT{1, 1})));
        log.Trace("ActiveWindow: %s %x", Unicode::to_string(activeProcess).c_str(), WindowFromPoint(POINT{1, 1}));
        return false;
    }

    void SendKeyInput(const std::vector<WORD> &inputs, int delay)
    {

        std::vector<INPUT> input;
        size_t len = inputs.size();
        input.resize(len);

        for (size_t i = 0; i < inputs.size(); i++)
        {
            input[i].type = INPUT_KEYBOARD;
            input[i].ki.wVk = inputs[i];
        }
        SendInput((UINT)len, input.data(), sizeof(INPUT));
        if (delay)
        {
            Sleep(delay);
        }
        for (size_t i = 0; i < inputs.size(); i++)
        {
            input[i].type = INPUT_KEYBOARD;
            input[i].ki.wVk = inputs[i];
            input[i].ki.dwFlags = KEYEVENTF_KEYUP;
        }
        SendInput((UINT)len, input.data(), sizeof(INPUT));
    }

    void TakeScreenShoot()
    {
        Process::StartProtocol(L"ms-gamebar://hotkey/?ihkid=IHKID_CAPTURE_GAME_SCREENSHOT&source=GbEm");
    }
    void ToggleRecord()
    {
        Process::StartProtocol(L"ms-gamebar://hotkey/?ihkid=IHKID_TOGGLE_GAME_RECORDING&source=GbEm");
    }

    void ToggleMicrophone()
    {
        Process::StartProtocol(L"ms-gamebar://hotkey/?ihkid=IHKID_ALT_TOGGLE_MICROPHONE_CAPTURE&source=GbEm");
    }

    void Handlers::OpenArmouryCrate()
    {
        Process::StartProtocol(L"asusac://");
    }

    void Handlers::OpenAnyFSESettings()
    {
        Process::StartProtocol(L"anyfse://settings");
    }

    void Handlers::OpenGameBar()
    {
        Process::StartProtocol(L"ms-gamebar://hotkey/?ihkid=IHKID_GAME_GUIDE_OVERLAY&source=GbEm");
    }

    void Handlers::OpenSteamOverlay()
    {
        if (SteamIsActive())
        {
            SendKeyInput({VK_CONTROL, '1'}, 200);
        }
        else
        {
            SendKeyInput({VK_SHIFT, VK_TAB}, 200);
        }
    }

    void Handlers::OpenSteamQuickMenu()
    {
        if (!SteamIsActive())
        {
            SendKeyInput({VK_SHIFT, VK_TAB}, 100);
        }
        SendKeyInput({VK_CONTROL, '2'}, 200);
    }


    void OpenGameBarComandCenter()
    {
        std::wstring activeProcess = Unicode::to_lower(Process::GetWindowProcessName(WindowFromPoint(POINT{1, 1})));
        log.Trace("ActiveWindow: %s %x", Unicode::to_string(activeProcess).c_str(), WindowFromPoint(POINT{1, 1}));
        if (activeProcess == L"gamebar.exe" || activeProcess == L"classiccommandcenter.exe")
        {
            SendKeyInput({VK_ESCAPE});
        }
        else
        {
            Process::StartProtocol(L"ms-gamebar://launchForeground/activate/B9ECED6F.ASUSCommandCenter_qmba6cd70vzyy_App_Widget");
        }
    }

    void OpenComandCenter()
    {
        SendKeyInput({VK_CONTROL, VK_MENU, 'C'});
    }

    void OpenComandCenterF24()
    {
        SendKeyInput({VK_F24});
    }

    void OpenLibrary()
    {
        Process::StartProtocol(L"anyfse://launcher");
    }

    void OpenTaskSwitcher()
    {
        Process::StartProtocol(L"shell:::{3080F90E-D7AD-11D9-BD98-0000947B0257}");
    }

    void OpenTaskSwitcherAlt()
    {
        SendKeyInput({VK_LWIN, VK_TAB});
    }

    // {4CE576FA-83DC-4F88-951C-9D0782B4E376}
    DEFINE_GUID(CLSID_UIHostNoLaunch,
                0x4ce576fa, 0x83dc, 0x4f88, 0x95, 0x1c, 0x9d, 0x07, 0x82, 0xb4, 0xe3, 0x76);

    // {37C994E7-432B-4834-A2F7-DCE1F13B834B}
    DEFINE_GUID(IID_ITipInvocation,
                0x37c994e7, 0x432b, 0x4834, 0xa2, 0xf7, 0xdc, 0xe1, 0xf1, 0x3b, 0x83, 0x4b);

    // ITipInvocation interface (undocumented)
    struct ITipInvocation : public IUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE Toggle(HWND hwnd) = 0;
    };

    void ShowKeyboard()
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (SUCCEEDED(hr))
        {
            ITipInvocation *pTipInvocation = NULL;

            // Try to connect to the running TabTip instance
            hr = CoCreateInstance(CLSID_UIHostNoLaunch, NULL,
                                  CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
                                  IID_ITipInvocation, (void **)&pTipInvocation);

            if (SUCCEEDED(hr))
            {
                // Toggle the keyboard on screen
                pTipInvocation->Toggle(GetDesktopWindow());
                pTipInvocation->Release();
            }
            else if (hr == REGDB_E_CLASSNOTREG)
            {
                // TabTip isn't running at all - launch it
                ShellExecuteA(NULL, "open",
                              "C:\\Program Files\\Common Files\\microsoft shared\\ink\\TabTip.exe",
                              NULL, NULL, SW_SHOW);
                // Note: On some systems, you may need to call Toggle again after a short delay
            }

            CoUninitialize();
        }
    }
}