// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include "Logging/LogManager.hpp"
#include "Tools.hpp"
#include <algorithm>
#include <shlobj_core.h>


namespace AnyFSE::Tools
{
    static Logger log = LogManager::GetLogger("Tools");

    HICON LoadIcon(const std::wstring &icon, int iconSize)
    {
        if (icon.empty())
        {
            return 0;
        }

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

        HICON hIcon;

        HRESULT hr = SHDefExtractIconW(path.wstring().c_str(), index, 0, &hIcon, NULL, (DWORD)iconSize);

        if (hr || !hIcon)
        {
            log.Warn(log.APIError(), "No icon with index %d", index);
            return NULL;
        }

        return hIcon;
    }

    Gdiplus::Bitmap * LoadBitmapFromIcon(const std::wstring &icon, int iconSize)
    {
        HICON hIcon = Tools::LoadIcon(icon, iconSize);
        if (!hIcon)
        {
            return nullptr;
        }

        //get the icon info
        ICONINFO ii;
        GetIconInfo(hIcon, &ii);

        using namespace Gdiplus;

        //create a bitmap from the ICONINFO so we can access the bitmapData
        Bitmap bmpIcon(ii.hbmColor, NULL);
        Rect rectBounds(0, 0, bmpIcon.GetWidth(), bmpIcon.GetHeight() );

        //get the BitmapData
        BitmapData bmData;
        bmpIcon.LockBits(&rectBounds, ImageLockModeRead, bmpIcon.GetPixelFormat(), &bmData);

        Bitmap * tmp = new Bitmap(bmData.Width, bmData.Height, bmData.Stride, PixelFormat32bppARGB, (BYTE*)bmData.Scan0);

        Bitmap * result = new Bitmap(bmpIcon.GetWidth(), bmpIcon.GetHeight(), PixelFormat32bppARGB);
        Graphics graphics(result);
        graphics.DrawImage(tmp, 0, 0, bmpIcon.GetWidth(), bmpIcon.GetHeight());

        delete tmp;

        bmpIcon.UnlockBits(&bmData);

        DestroyIcon(hIcon);
        DeleteObject(ii.hbmColor);
        DeleteObject(ii.hbmMask);

        return result;
    }

    BOOL GetChildRect(HWND hwnd, RECT * rect)
    {
        GetWindowRect(hwnd, rect);
        MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)rect, 2);
        return TRUE;
    }

    BOOL MoveWindow(HWND hwnd, int dx, int dy, BOOL bRepaint)
    {
        RECT rect;
        GetChildRect(hwnd, &rect);
        OffsetRect(&rect, dx, dy);
        ::MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, bRepaint);
        return TRUE;
    }

    BOOL MoveWindow(HWND hwnd, RECT* rect, BOOL bRepaint)
    {
        ::MoveWindow(hwnd, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, bRepaint);
        return TRUE;
    }

    BOOL MouseInClientRect(HWND hWnd, RECT *rect)
    {
        RECT hitRect;
        if (rect == NULL)
        {
            GetClientRect(hWnd, &hitRect);
        }
        else
        {
            hitRect = *rect;
        }

        POINT pt;
        GetCursorPos(&pt);
        MapWindowPoints(NULL, hWnd, &pt, 1);

        return PtInRect(&hitRect, pt);
    }
}
