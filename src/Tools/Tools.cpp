#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include "Logging/LogManager.h"
#include "Tools.h"

namespace AnyFSE::Tools
{
    static Logger log = LogManager::GetLogger("Tools");

    HICON LoadIcon(const std::wstring &icon)
    {
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

        HICON hIcon = ExtractIcon(GetModuleHandle(NULL), path.wstring().c_str(), index);
        
        if (!hIcon)
        {
            log.Warn(log.APIError(), "No icon with index %d", index);
            return NULL;
        }
        
        return hIcon;
    }
    std::wstring to_wstring(const std::string &str)
    {
        return std::wstring();
    }
    std::string to_string(const std::wstring &wstr)
    {
        if (wstr.empty()) return "";
        
        std::string result;
        result.reserve(wstr.size() * 3); // Reserve space for worst-case UTF-8
        
        for (wchar_t wc : wstr) {
            if (wc <= 0x7F) {
                // ASCII character
                result += static_cast<char>(wc);
            }
            else if (wc <= 0x7FF) {
                // 2-byte UTF-8
                result += static_cast<char>(0xC0 | ((wc >> 6) & 0x1F));
                result += static_cast<char>(0x80 | (wc & 0x3F));
            }
            else if (wc <= 0xFFFF) {
                // 3-byte UTF-8 (most common for BMP)
                result += static_cast<char>(0xE0 | ((wc >> 12) & 0x0F));
                result += static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (wc & 0x3F));
            }
            else {
                // 4-byte UTF-8 (surrogate pairs would need special handling)
                #pragma warning(suppress: 4333)
                result += static_cast<char>(0xF0 | ((wc >> 18) & 0x07));
                result += static_cast<char>(0x80 | ((wc >> 12) & 0x3F));
                result += static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (wc & 0x3F));
            }
        }
        return result;
    }
}
