#include "Tools/Localization.hpp"

#include <windows.h>

#include <cstdarg>
#include <cwctype>
#include <algorithm>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <filesystem>
#include <vector>

#include "Tools/Unicode.hpp"
#include "Tools/Paths.hpp"

namespace AnyFSE::Tools::Localization
{
    namespace
    {
        std::map<std::wstring, std::wstring> g_dictionary;
        std::mutex g_lock;
        std::wstring g_forcedLocale;
        std::wstring g_currentLocale = L"en_US";

        bool ReadFileUtf8(const std::wstring &filePath, std::string &content)
        {
            std::ifstream file(filePath, std::ios::binary);
            if (!file.is_open())
            {
                return false;
            }
            content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return true;
        }

        void SkipSpaces(const std::wstring &text, size_t &i)
        {
            while (i < text.size() && std::iswspace(text[i]))
            {
                ++i;
            }
        }

        bool ParseHex4(const std::wstring &text, size_t &i, wchar_t &ch)
        {
            if (i + 4 > text.size())
            {
                return false;
            }

            unsigned int value = 0;
            for (size_t n = 0; n < 4; ++n)
            {
                value <<= 4;
                const wchar_t c = text[i + n];
                if (c >= L'0' && c <= L'9')
                {
                    value |= static_cast<unsigned int>(c - L'0');
                }
                else if (c >= L'A' && c <= L'F')
                {
                    value |= static_cast<unsigned int>(10 + c - L'A');
                }
                else if (c >= L'a' && c <= L'f')
                {
                    value |= static_cast<unsigned int>(10 + c - L'a');
                }
                else
                {
                    return false;
                }
            }

            i += 4;
            ch = static_cast<wchar_t>(value);
            return true;
        }

        bool ParseJsonString(const std::wstring &text, size_t &i, std::wstring &result)
        {
            if (i >= text.size() || text[i] != L'"')
            {
                return false;
            }
            ++i;

            std::wstring out;
            while (i < text.size())
            {
                const wchar_t c = text[i++];
                if (c == L'"')
                {
                    result = out;
                    return true;
                }
                if (c == L'\\')
                {
                    if (i >= text.size())
                    {
                        return false;
                    }
                    const wchar_t esc = text[i++];
                    switch (esc)
                    {
                    case L'"': out.push_back(L'"'); break;
                    case L'\\': out.push_back(L'\\'); break;
                    case L'/': out.push_back(L'/'); break;
                    case L'b': out.push_back(L'\b'); break;
                    case L'f': out.push_back(L'\f'); break;
                    case L'n': out.push_back(L'\n'); break;
                    case L'r': out.push_back(L'\r'); break;
                    case L't': out.push_back(L'\t'); break;
                    case L'u':
                    {
                        wchar_t u = 0;
                        if (!ParseHex4(text, i, u))
                        {
                            return false;
                        }
                        out.push_back(u);
                        break;
                    }
                    default:
                        return false;
                    }
                }
                else
                {
                    out.push_back(c);
                }
            }
            return false;
        }

        bool ParseFlatJsonObject(const std::wstring &content, std::map<std::wstring, std::wstring> &out)
        {
            size_t i = 0;
            SkipSpaces(content, i);
            if (i >= content.size() || content[i] != L'{')
            {
                return false;
            }
            ++i;

            SkipSpaces(content, i);
            if (i < content.size() && content[i] == L'}')
            {
                return true;
            }

            while (i < content.size())
            {
                SkipSpaces(content, i);
                std::wstring key;
                if (!ParseJsonString(content, i, key))
                {
                    return false;
                }

                SkipSpaces(content, i);
                if (i >= content.size() || content[i] != L':')
                {
                    return false;
                }
                ++i;

                SkipSpaces(content, i);
                std::wstring value;
                if (!ParseJsonString(content, i, value))
                {
                    return false;
                }

                out[key] = value;
                SkipSpaces(content, i);

                if (i >= content.size())
                {
                    return false;
                }
                if (content[i] == L',')
                {
                    ++i;
                    continue;
                }
                if (content[i] == L'}')
                {
                    return true;
                }
                return false;
            }

            return false;
        }

        bool LoadLanguageFileToMap(const std::wstring &filePath, std::map<std::wstring, std::wstring> &out)
        {
            std::string content;
            if (!ReadFileUtf8(filePath, content))
            {
                return false;
            }
            std::wstring wideContent = Unicode::to_wstring(content);

            if (!ParseFlatJsonObject(wideContent, out))
            {
                return false;
            }
            return true;
        }

        bool LoadLanguageFile(const std::wstring &filePath)
        {
            std::map<std::wstring, std::wstring> parsed;
            if (!LoadLanguageFileToMap(filePath, parsed))
            {
                return false;
            }
            for (const auto &[key, value] : parsed)
            {
                g_dictionary[key] = value;
            }
            return true;
        }
    }

    bool Initialize(const std::wstring &directoryPath)
    {
        namespace fs = std::filesystem;

        std::lock_guard<std::mutex> guard(g_lock);
        g_dictionary.clear();
        g_currentLocale = L"en_US";

        bool baseLoaded = LoadLanguageFile(directoryPath + L"\\en_US.json");

#ifdef _DEBUG
        if (!baseLoaded)
        {
            const fs::path debugPath = fs::path(Paths::GetExePath()) / L".." / L".." / L"Assets" / L"localization";
            baseLoaded = LoadLanguageFile(debugPath.lexically_normal().wstring() + L"\\en_US.json");
        }
#endif
        (void)baseLoaded;

        std::wstring localeFileName;
        if (!g_forcedLocale.empty())
        {
            localeFileName = g_forcedLocale;
        }
        else
        {
            wchar_t localeName[LOCALE_NAME_MAX_LENGTH] = {0};
            if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) <= 0)
            {
                return true;
            }
            localeFileName = localeName;
        }

        if (!localeFileName.empty())
        {
            for (wchar_t &ch : localeFileName)
            {
                if (ch == L'-')
                {
                    ch = L'_';
                }
            }

            if (_wcsicmp(localeFileName.c_str(), L"en_US") != 0)
            {
                bool loaded = LoadLanguageFile(directoryPath + L"\\" + localeFileName + L".json");
#ifdef _DEBUG
                const fs::path debugPath = fs::path(Paths::GetExePath()) / L".." / L".." / L"Assets" / L"localization";
                loaded = loaded || LoadLanguageFile(debugPath.lexically_normal().wstring() + L"\\" + localeFileName + L".json");
#endif
                if (loaded)
                {
                    g_currentLocale = localeFileName;
                }
            }
        }

        return true;
    }

    void SetPreferredLocale(const std::wstring &localeCode)
    {
        std::lock_guard<std::mutex> guard(g_lock);
        g_forcedLocale = localeCode;
    }

    std::wstring GetPreferredLocale()
    {
        std::lock_guard<std::mutex> guard(g_lock);
        return g_forcedLocale;
    }

    std::wstring GetCurrentLocale()
    {
        std::lock_guard<std::mutex> guard(g_lock);
        return g_currentLocale;
    }

    std::vector<LocaleInfo> EnumerateLocales(const std::wstring &directoryPath)
    {
        namespace fs = std::filesystem;
        std::vector<LocaleInfo> result;

        std::error_code ec;
        if (!fs::exists(directoryPath, ec))
        {
            return result;
        }

        for (const auto &entry : fs::directory_iterator(directoryPath, ec))
        {
            if (ec || !entry.is_regular_file())
            {
                continue;
            }
            if (entry.path().extension() != L".json")
            {
                continue;
            }

            LocaleInfo info;
            info.code = entry.path().stem().wstring();
            info.language = info.code;

            std::map<std::wstring, std::wstring> parsed;
            if (LoadLanguageFileToMap(entry.path().wstring(), parsed))
            {
                const auto it = parsed.find(L"language");
                if (it != parsed.end() && !it->second.empty())
                {
                    info.language = it->second;
                }
            }

            result.push_back(info);
        }

        std::sort(result.begin(), result.end(), [](const LocaleInfo &a, const LocaleInfo &b)
        {
            return _wcsicmp(a.language.c_str(), b.language.c_str()) < 0;
        });

        return result;
    }

    std::wstring Translate(const std::wstring &key)
    {
        std::lock_guard<std::mutex> guard(g_lock);
        const auto it = g_dictionary.find(key);
        if (it == g_dictionary.end())
        {
            return key;
        }
        return it->second;
    }

    std::wstring VTranslateF(const wchar_t *key, va_list args)
    {
        std::wstring format = Translate(key ? std::wstring(key) : std::wstring());

        va_list argsCopy;
        va_copy(argsCopy, args);
        const int len = _vscwprintf(format.c_str(), argsCopy);
        va_end(argsCopy);

        if (len <= 0)
        {
            return format;
        }

        std::wstring formatted(static_cast<size_t>(len), L'\0');
        _vsnwprintf_s(formatted.data(), formatted.size() + 1, _TRUNCATE, format.c_str(), args);
        return formatted;
    }

    std::wstring TranslateF(const wchar_t *key, ...)
    {
        va_list args;
        va_start(args, key);
        std::wstring result = VTranslateF(key, args);
        va_end(args);
        return result;
    }
}
