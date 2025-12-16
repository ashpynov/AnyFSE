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

#include "SettingsDialog.hpp"

namespace AnyFSE::App::AppSettings::Settings
{
    using namespace FluentDesign;

    template<class T>
    FluentDesign::SettingsLine& SettingsDialog::AddSettingsLine(std::list<SettingsLine>& page,
        ULONG &top, const std::wstring& name, const std::wstring& desc, T& control,
        int height, int padding, int contentMargin, int contentWidth, int contentHeight )
    {
        RECT rect;
        GetDialogRect(&rect);
        int width = rect.right - rect.left
            - m_theme.DpiScale(Layout::MarginLeft - Layout::Border)
            - m_theme.DpiScale(Layout::MarginRight - Layout::Border);

        page.emplace_back(
            m_theme, m_hScrollView, name, desc,
            m_theme.DpiScale(Layout::MarginLeft - Layout::Border),
            top, width,
            m_theme.DpiScale(height),
            padding,
            [&](HWND parent) { return control.Create(parent, 0, 0,
                m_theme.DpiScale(contentWidth), m_theme.DpiScale(contentHeight)); });

        top += m_theme.DpiScale(height + padding);
        if (contentMargin)
        {
            page.back().SetLeftMargin(contentMargin);
        }

        return page.back();
    }
}

