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

#pragma comment(lib, "psapi.lib")

namespace Handlers
{
    static Logger log = LogManager::GetLogger("Handlers");
    void SendKeyInput(const std::vector<WORD> &inputs)
    {

        std::vector<INPUT> input;
        size_t len = inputs.size();
        input.resize(len * 2);

        for (size_t i = 0; i < inputs.size(); i++)
        {
            input[i].type = INPUT_KEYBOARD;
            input[i].ki.wVk = inputs[i];
            input[i + len].type = INPUT_KEYBOARD;
            input[i + len].ki.wVk = inputs[i];
            input[i + len].ki.dwFlags = KEYEVENTF_KEYUP;
        }

        SendInput((UINT)len * 2, input.data(), sizeof(INPUT));
    }

    void TakeScreenShoot()
    {
        SendKeyInput({VK_LWIN, VK_MENU, VK_SNAPSHOT});
    }
    void ToggleRecord()
    {
        SendKeyInput({VK_LWIN, VK_MENU, 'R'});
    }

    void ToggleMicrophone()
    {
        PostMessage(GetForegroundWindow(), WM_APPCOMMAND, 0, 0x180000);
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

    void OpenLibrary()
    {
        Process::StartProtocol(L"anyfse://launcher");
    }

    void OpenTaskSwitcher()
    {
        Process::StartProtocol(L"shell:::{3080F90E-D7AD-11D9-BD98-0000947B0257}");
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
        HRESULT hr = CoInitialize(NULL);
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