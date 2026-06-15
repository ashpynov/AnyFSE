#pragma once

#include <cstdarg>
#include <string>
#include <vector>
#include <map>

namespace AnyFSE::Tools::Localization
{
    using ResourceLocales = std::map<std::wstring, std::string>;

    struct LocaleInfo
    {
        std::wstring code;
        std::wstring language;
    };

    bool Initialize();
    bool InitializeFromLocales(const ResourceLocales &resourceLocales);
    void SetPreferredLocale(const std::wstring &localeCode);
    std::wstring GetPreferredLocale();
    std::wstring GetCurrentLocale();
    std::vector<LocaleInfo> EnumerateLocales();
    std::vector<LocaleInfo> EnumerateLocales(const ResourceLocales &resourceLocales);
    std::wstring Translate(const std::wstring &key);
    std::wstring TranslateF(const wchar_t *key, ...);
    std::wstring VTranslateF(const wchar_t *key, va_list args);
}

using AnyFSE::Tools::Localization::Translate;
using AnyFSE::Tools::Localization::TranslateF;
