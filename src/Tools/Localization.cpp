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
#include <set>

#include "Tools/Unicode.hpp"
#include "Tools/Paths.hpp"
#include "Tools/nlohmann/json.hpp"

namespace AnyFSE::Tools::Localization
{
    bool Initialize();
    std::vector<LocaleInfo> EnumerateLocales();

    namespace
    {
        std::map<std::wstring, std::wstring> g_dictionary;
        std::mutex g_lock;
        std::wstring g_forcedLocale;
        std::wstring g_currentLocale = L"en_US";

        BOOL CALLBACK EnumLocalizationResNameProc(HMODULE hModule, LPCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam)
        {
            auto *locales = reinterpret_cast<ResourceLocales *>(lParam);
            if (!locales || IS_INTRESOURCE(lpszName))
            {
                return TRUE;
            }

            HRSRC hResource = FindResourceW(hModule, lpszName, lpszType);
            if (!hResource)
            {
                return TRUE;
            }

            HGLOBAL hGlobal = LoadResource(hModule, hResource);
            if (!hGlobal)
            {
                return TRUE;
            }

            DWORD size = SizeofResource(hModule, hResource);
            const void *ptr = LockResource(hGlobal);
            if (!ptr || !size)
            {
                return TRUE;
            }

            std::wstring code = lpszName;
            if (code.size() >= 2 && code.front() == L'"' && code.back() == L'"')
            {
                code = code.substr(1, code.size() - 2);
            }
            std::transform(code.begin(), code.end(), code.begin(), towupper);
            (*locales)[code] = std::string(static_cast<const char *>(ptr), static_cast<size_t>(size));
            return TRUE;
        }

        ResourceLocales LoadResourceLocalesFromModule(HMODULE hModule)
        {
            ResourceLocales locales;
            EnumResourceNamesW(hModule, L"LOCALIZATION", EnumLocalizationResNameProc, reinterpret_cast<LONG_PTR>(&locales));
            return locales;
        }

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

        bool LoadLanguageJsonToMap(const nlohmann::json &parsed, std::map<std::wstring, std::wstring> &out)
        {
            if (!parsed.is_object())
            {
                return false;
            }

            for (const auto &[key, value] : parsed.items())
            {
                if (!value.is_string())
                {
                    continue;
                }

                out[Unicode::to_wstring(key)] = Unicode::to_wstring(value.get<std::string>());
            }

            return true;
        }

        bool LoadLanguageFileToMap(const std::wstring &filePath, std::map<std::wstring, std::wstring> &out)
        {
            std::string content;
            if (!ReadFileUtf8(filePath, content))
            {
                return false;
            }

            try
            {
                const auto parsed = nlohmann::json::parse(content);
                if (!LoadLanguageJsonToMap(parsed, out))
                {
                    return false;
                }
            }
            catch (...)
            {
                return false;
            }

            return true;
        }

        bool LoadLanguageContentToMap(const std::string &content, std::map<std::wstring, std::wstring> &out)
        {
            try
            {
                const auto parsed = nlohmann::json::parse(content);
                if (!LoadLanguageJsonToMap(parsed, out))
                {
                    return false;
                }
            }
            catch (...)
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

        bool MergeLocaleFromMap(
            const ResourceLocales &resourceLocales,
            const std::wstring &code)
        {
            std::wstring upperCode = code;
            std::transform(upperCode.begin(), upperCode.end(), upperCode.begin(), towupper);

            const auto it = resourceLocales.find(upperCode);
            if (it == resourceLocales.end())
            {
                return false;
            }

            std::map<std::wstring, std::wstring> parsed;
            if (!LoadLanguageContentToMap(it->second, parsed))
            {
                return false;
            }

            for (const auto &[k, v] : parsed)
            {
                g_dictionary[k] = v;
            }
            return true;
        }

        bool InitializeFromResourceLocales(const ResourceLocales &resourceLocales)
        {
            if (resourceLocales.empty())
            {
                return Initialize();
            }

            std::lock_guard<std::mutex> guard(g_lock);
            g_dictionary.clear();
            g_currentLocale = L"en_US";

            MergeLocaleFromMap(resourceLocales, L"en_US");

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
                for (wchar_t &ch : localeFileName)
                {
                    if (ch == L'-')
                    {
                        ch = L'_';
                    }
                }
            }

            if (!localeFileName.empty() && _wcsicmp(localeFileName.c_str(), L"en_US") != 0)
            {
                if (MergeLocaleFromMap(resourceLocales, localeFileName))
                {
                    g_currentLocale = localeFileName;
                }
            }

            return true;
        }

        std::vector<LocaleInfo> EnumerateResourceLocales(const ResourceLocales &resourceLocales)
        {
            if (resourceLocales.empty())
            {
                return EnumerateLocales();
            }

            std::vector<LocaleInfo> result;
            std::set<std::wstring> seen;

            for (const auto &[code, content] : resourceLocales)
            {
                std::wstring upperCode = code;
                std::transform(upperCode.begin(), upperCode.end(), upperCode.begin(), towupper);

                if (seen.find(upperCode) != seen.end())
                {
                    continue;
                }

                LocaleInfo info;
                info.code = upperCode;
                info.language = upperCode;
                std::map<std::wstring, std::wstring> parsed;
                if (LoadLanguageContentToMap(content, parsed))
                {
                    const auto it = parsed.find(L"language");
                    if (it != parsed.end() && !it->second.empty())
                    {
                        info.language = it->second;
                    }
                }

                seen.insert(info.code);
                result.push_back(info);
            }

            std::sort(result.begin(), result.end(), [](const LocaleInfo &a, const LocaleInfo &b)
            {
                return _wcsicmp(a.language.c_str(), b.language.c_str()) < 0;
            });

            return result;
        }

    }

    bool Initialize()
    {
        namespace fs = std::filesystem;

        std::lock_guard<std::mutex> guard(g_lock);
        g_dictionary.clear();
        g_currentLocale = L"en_US";

        const fs::path programDataPath = fs::path(Paths::GetDataPath()) / L"Localization";
        const fs::path exeAssetsPath = fs::path(Paths::GetExePath()) / L"Localization";

        // Base language: first existing source wins (ProgramData has priority).
        LoadLanguageFile((programDataPath / L"en_US.json").wstring()) ||
        LoadLanguageFile((exeAssetsPath / L"en_US.json").wstring());


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
                const bool loaded =
                    LoadLanguageFile((programDataPath / (localeFileName + L".json")).wstring()) ||
                    LoadLanguageFile((exeAssetsPath / (localeFileName + L".json")).wstring());
                if (loaded)
                {
                    g_currentLocale = localeFileName;
                }
            }
        }

        return true;
    }

    bool InitializeFromLocales()
    {
        return InitializeFromResourceLocales(LoadResourceLocales());
    }

    ResourceLocales LoadResourceLocales()
    {
        return LoadResourceLocalesFromModule(GetModuleHandleW(NULL));
    }

    std::vector<LocaleInfo> EnumerateLocales()
    {
        namespace fs = std::filesystem;
        std::vector<LocaleInfo> result;
        std::set<std::wstring> seen;

        auto scan = [&](const fs::path &path)
        {
            std::error_code ec;
            if (!fs::exists(path, ec))
            {
                return;
            }

            for (const auto &entry : fs::directory_iterator(path, ec))
            {
                if (ec || !entry.is_regular_file() || entry.path().extension() != L".json")
                {
                    continue;
                }

                LocaleInfo info;
                info.code = entry.path().stem().wstring();
                if (seen.find(info.code) != seen.end())
                {
                    continue;
                }

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

                seen.insert(info.code);
                result.push_back(info);
            }
        };

        // ProgramData has priority for duplicates.
        scan(fs::path(Paths::GetDataPath()) / L"Localization");
        scan(fs::path(Paths::GetExePath()) / L"localization");

        std::sort(result.begin(), result.end(), [](const LocaleInfo &a, const LocaleInfo &b)
        {
            return _wcsicmp(a.language.c_str(), b.language.c_str()) < 0;
        });

        return result;
    }

    std::vector<LocaleInfo> EnumerateResourceLocales()
    {
        return EnumerateResourceLocales(LoadResourceLocales());
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
