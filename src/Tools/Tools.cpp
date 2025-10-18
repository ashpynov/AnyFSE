#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include "Logging/LogManager.hpp"
#include "Tools.hpp"
#include <algorithm>
#include <shlobj_core.h> 

namespace AnyFSE::Tools
{
    static Logger log = LogManager::GetLogger("Tools");

    HICON LoadIcon(const std::wstring &icon, int iconSize)
    {
        if ( icon[0] == L'#')
        {
            return 0;
        }

        int index = 0;
        std::filesystem::path path(icon);

        size_t commaPos = icon.find(L',');
        if (commaPos != std::wstring::npos)
        {
            path = icon.substr(0, commaPos);
            std::wstring indexStr = icon.substr(commaPos + 1);
            try
            {
                index = std::stoi(indexStr);
            }
            catch (const std::exception& ex)
            {
                log.Warn(ex, "Index is invalid: %ls", indexStr);
            }
        }

        if (!std::filesystem::exists(path))
        {
            log.Warn("Icon file not found %s", path.string().c_str());
            return NULL;
        }

        HICON hIcon;
        
        HRESULT hr = SHDefExtractIconW(path.wstring().c_str(), index, 0, &hIcon, NULL, (DWORD)iconSize);

        if (!SUCCEEDED(hr) || !hIcon)
        {
            log.Warn(log.APIError(), "No icon with index %d", index);
            return NULL;
        }

        return hIcon;
    }
}
