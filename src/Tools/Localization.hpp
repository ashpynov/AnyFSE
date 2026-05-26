#pragma once

#include <cstdarg>
#include <string>
#include <vector>

namespace AnyFSE::Tools::Localization
{
    struct LocaleInfo
    {
        std::wstring code;
        std::wstring language;
    };

    bool Initialize(const std::wstring &directoryPath);
    void SetPreferredLocale(const std::wstring &localeCode);
    std::wstring GetPreferredLocale();
    std::wstring GetCurrentLocale();
    std::vector<LocaleInfo> EnumerateLocales(const std::wstring &directoryPath);
    std::wstring Translate(const std::wstring &key);
    std::wstring TranslateF(const wchar_t *key, ...);
    std::wstring VTranslateF(const wchar_t *key, va_list args);
}

using AnyFSE::Tools::Localization::Translate;
using AnyFSE::Tools::Localization::TranslateF;