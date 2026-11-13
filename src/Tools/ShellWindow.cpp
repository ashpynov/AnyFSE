#include "ShellWindow.hpp"
#include <inspectable.h>
#include <objbase.h>
#include <objectarray.h>
#include <servprov.h>
#include <wrl/client.h>
#include "Logging/LogManager.hpp"

#pragma comment(lib, "ole32.lib")

namespace AnyFSE::Tools::ShellWindow
{
    namespace
    {
        Logger log = LogManager::GetLogger("ShellWindow");
        namespace wrl = Microsoft::WRL;

        class __declspec(uuid("C2F03A33-21F5-47FA-B4BB-156362A2F239")) ImmersiveShell;

        // Only the ABI prefix through SetCloak is needed. IInspectable occupies slots 0-5.
        // Reference: https://github.com/izantech/winspaces/blob/main/crates/winspaces-win32/src/shell_cloak.rs
        struct IApplicationView : IInspectable
        {
            virtual HRESULT STDMETHODCALLTYPE SetFocus() = 0;
            virtual HRESULT STDMETHODCALLTYPE SwitchTo() = 0;
            virtual HRESULT STDMETHODCALLTYPE TryInvokeBack(IUnknown* callback) = 0;
            virtual HRESULT STDMETHODCALLTYPE GetThumbnailWindow(HWND* window) = 0;
            virtual HRESULT STDMETHODCALLTYPE GetMonitor(IUnknown** monitor) = 0;
            virtual HRESULT STDMETHODCALLTYPE GetVisibility(int* visibility) = 0;
            virtual HRESULT STDMETHODCALLTYPE SetCloak(UINT type, int flags) = 0;
        };

        struct __declspec(uuid("1841C6D7-4F9D-42C0-AF41-8747538F10E5")) IApplicationViewCollection : IUnknown
        {
            virtual HRESULT STDMETHODCALLTYPE GetViews(IObjectArray** views) = 0;
            virtual HRESULT STDMETHODCALLTYPE GetViewsByZOrder(IObjectArray** views) = 0;
            virtual HRESULT STDMETHODCALLTYPE GetViewsByAppUserModelId(PCWSTR appUserModelId, IObjectArray** views) = 0;
            virtual HRESULT STDMETHODCALLTYPE GetViewForHwnd(HWND window, IApplicationView** view) = 0;
        };
    }

    HRESULT SwitchTo(HWND window)
    {
        wrl::ComPtr<IServiceProvider> shell;
        HRESULT result = CoCreateInstance(__uuidof(ImmersiveShell), nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&shell));
        if (FAILED(result))
            return result;

        wrl::ComPtr<IApplicationViewCollection> collection;
        result = shell->QueryService(__uuidof(IApplicationViewCollection), IID_PPV_ARGS(&collection));
        if (FAILED(result))
            return result;

        wrl::ComPtr<IApplicationView> view;
        result = collection->GetViewForHwnd(window, &view);
        if (FAILED(result) || !view)
            return FAILED(result) ? result : E_NOINTERFACE;

        // A failed foreground transfer must not prevent the shell from attempting activation.
        const HRESULT foregroundResult = CoAllowSetForegroundWindow(shell.Get(), nullptr);
        if (FAILED(foregroundResult))
            log.Warn("Cannot transfer foreground privilege to shell: HRESULT=%#lx", static_cast<unsigned long>(foregroundResult));

        // AVCT_DEFAULT = 1; flags = 0 removes the shell cloak.
        result = view->SetCloak(1, 0);
        return SUCCEEDED(result) ? view->SwitchTo() : result;
    }
}
