#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <initguid.h>
#include <vector>
#include "Ally/Handlers.hpp"

namespace Handlers
{
    void SendKeyInput(const std::vector<WORD> &inputs)
    {

        std::vector<INPUT> input;
        int len = inputs.size();
        input.resize(len * 2);

        for (size_t i = 0; i < inputs.size(); i++)
        {
            input[i].type = INPUT_KEYBOARD;
            input[i].ki.wVk = inputs[i];
            input[i + len].type = INPUT_KEYBOARD;
            input[i + len].ki.wVk = inputs[i];
            input[i + len].ki.dwFlags = KEYEVENTF_KEYUP;
        }

        SendInput(len * 2, input.data(), sizeof(INPUT));
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

    void OpenComandCenterAlt()
    {
        ShellExecute(NULL, L"open", L"ms-gamebar://launchForeground/activate/B9ECED6F.ASUSCommandCenter_qmba6cd70vzyy_App_Widget", NULL, NULL, SW_SHOWNORMAL);
    }

    void OpenComandCenter()
    {
        SendKeyInput({VK_CONTROL, VK_MENU, VK_SHIFT, VK_F23});
    }

    void OpenLibrary()
    {
        ShellExecute(NULL, L"open", L"anyfse://", NULL, NULL, SW_SHOWNORMAL);
    }

    void OpenTaskSwitcher()
    {
        ShellExecute(NULL, L"open",
                     L"shell:::{3080F90E-D7AD-11D9-BD98-0000947B0257}",
                     NULL, NULL, SW_SHOWDEFAULT);
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