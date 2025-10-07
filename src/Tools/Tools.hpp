#pragma once
#include <string>
#include <Windows.h>

namespace AnyFSE::Tools
{
    HICON LoadIcon(const std::wstring& icon);
    std::wstring to_wstring(const std::string& str);
    std::string to_string(const std::wstring& wstr);
    std::wstring to_lower(const std::wstring &str);

    template <typename T, typename V>
    size_t index_of(const T& list, const V &value);

    const size_t npos = static_cast<size_t>(-1);

    template<typename T, typename V>
    size_t index_of(const T& list, const V& value) {
        auto it = std::find(list.begin(), list.end(), value);
        return (it != list.end()) ? std::distance(list.begin(), it) : npos;
    }

    template<typename T, typename Predicate>
    size_t index_of_if(const T& list, Predicate pred) {
        auto it = std::find_if(list.begin(), list.end(), pred);
        return (it != list.end()) ? std::distance(list.begin(), it) : npos;
    }


}

namespace Tools = AnyFSE::Tools;