// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#pragma once
#include <string>
#include <Windows.h>

namespace AnyFSE::Tools
{
    HICON LoadIcon(const std::wstring& icon, int size = 256);

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

    BOOL GetChildRect(HWND hwnd, RECT *rect);
    BOOL MoveWindow(HWND hwnd, RECT * rect, BOOL bRepaint);
    BOOL MoveWindow(HWND hwnd, int dx, int dy, BOOL bRepaint);
    BOOL MouseInClientRect(HWND, RECT *rect);
}

namespace Tools = AnyFSE::Tools;