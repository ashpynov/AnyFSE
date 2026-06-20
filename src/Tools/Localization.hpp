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

    ResourceLocales LoadResourceLocales();
    bool Initialize(const std::wstring &code = L"");
    bool InitializeFromLocales(const std::wstring &code = L"");
    std::wstring GetCurrentLocale();
    std::vector<LocaleInfo> EnumerateLocales();
    std::vector<LocaleInfo> EnumerateResourceLocales();
    std::wstring Translate(const std::wstring &key);
    std::wstring TranslateF(const wchar_t *key, ...);
    std::wstring VTranslateF(const wchar_t *key, va_list args);
}

using AnyFSE::Tools::Localization::Translate;
using AnyFSE::Tools::Localization::TranslateF;
