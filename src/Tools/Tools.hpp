// MIT License
//
// Copyright (c) 2025 Artem Shpynov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


#pragma once
#include <string>
#include <Windows.h>
#include "GdiPlus.hpp"

namespace AnyFSE::Tools
{
    HICON LoadIcon(const std::wstring& icon, int size = 256);
    Gdiplus::Bitmap * LoadBitmapFromIcon(const std::wstring &icon, int iconSize = 256);

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