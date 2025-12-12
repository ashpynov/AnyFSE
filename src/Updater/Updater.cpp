#include "Tools/Process.hpp"
#include "Updater/Updater.hpp"

namespace AnyFSE::Updater
{
    const std::wstring RELEASE_PAGE = L"https://github.com/ashpynov/AnyFSE/releases/tag/";
    
    void ShowVersion(const std::wstring &tag)
    {
        Process::StartProtocol(RELEASE_PAGE + tag);
    }
}