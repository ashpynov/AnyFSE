#include <winrt/Windows.UI.StartScreen.h>   // WinRT header
#include <winrt/Windows.Foundation.h>       // Foundation types
#include <winrt/Windows.Foundation.Collections.h>

#pragma comment(lib, "windowsapp.lib")      // WinRT runtime

namespace AnyFSE::App::JumpList
{
    void RegisterJumpList()
    {
        winrt::init_apartment();  // WinRT initialization required

        auto jumpList = winrt::Windows::UI::StartScreen::JumpList::LoadCurrentAsync().get();

        auto fseItem = winrt::Windows::UI::StartScreen::JumpListItem::CreateWithArguments(
            L"/FSE", L"Enter full screen experience");
        fseItem.Logo(winrt::Windows::Foundation::Uri(L"ms-appx:///Assets/fullscreen-icon.png"));

        auto settingsItem = winrt::Windows::UI::StartScreen::JumpListItem::CreateWithArguments(
            L"/Settings", L"Configure");
        settingsItem.Logo(winrt::Windows::Foundation::Uri(L"ms-appx:///Assets/settings-icon.png"));


        jumpList.Items().Clear();
        jumpList.Items().Append(fseItem);
        jumpList.Items().Append(settingsItem);
        jumpList.SaveAsync().get();
    }
}