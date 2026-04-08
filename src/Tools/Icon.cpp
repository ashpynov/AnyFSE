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


#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include <algorithm>
#include <shlobj_core.h>
#include "Logging/LogManager.hpp"
#include "Icon.hpp"
#include "Packages.hpp"

namespace AnyFSE::Tools::Icon
{
    static Logger log = LogManager::GetLogger("Tools/Icon");

    HICON LoadIconFromRaw(std::vector<uint8_t> imageData, int size)
    {
        if (imageData.empty()) return nullptr;

        // Create IStream from memory
        HICON hIcon = nullptr;
        IStream * pStream = nullptr;
        Gdiplus::Bitmap *bitmap = nullptr;

        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, imageData.size());

        try
        {
            if (!hGlobal)
            {
                throw;
            };

            void* pData = GlobalLock(hGlobal);
            memcpy(pData, imageData.data(), imageData.size());
            GlobalUnlock(hGlobal);

            HRESULT hr = CreateStreamOnHGlobal(hGlobal, TRUE, &pStream);
            if (FAILED(hr) || !pStream)
            {
                throw;
            }

            bitmap = new Gdiplus::Bitmap(pStream);
            if (bitmap->GetLastStatus() != Gdiplus::Ok)
            {
                throw;
            }
            int w = bitmap->GetHeight();
            int h = bitmap->GetWidth();

            bitmap->GetHICON(&hIcon);
        } catch (...) {};

        if (hGlobal) GlobalFree(hGlobal);
        if (bitmap)  delete bitmap;
        if (pStream) pStream->Release();

        return hIcon;
    }

    HICON LoadIconFromPNG(const std::wstring &pngPath, int size)
    {
        // Load the PNG image
        Gdiplus::Bitmap *bitmap = Gdiplus::Bitmap::FromFile(pngPath.c_str());
        if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok)
        {
            delete bitmap;
            return nullptr;
        }

        // Convert to HICON
        HICON hIcon = nullptr;
        bitmap->GetHICON(&hIcon);
        delete bitmap;

        return hIcon;
    }
    HICON LoadIcon(const std::wstring &icon, int iconSize)
    {
        if (icon.empty())
        {
            return 0;
        }

        int index = 0;

        std::wstring iconPath = icon;

        if (icon[0]==L'@')
        {
            return LoadIconFromRaw(Packages::GetAppDisplayLogoRawBytes(iconPath.c_str() + 1, 32), 32);
        }

        std::filesystem::path path(iconPath);

        size_t commaPos = iconPath.find(L',');
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
        if (_wcsicmp(path.extension().wstring().c_str(), L".png") == 0)
        {
            hIcon = LoadIconFromPNG(path.wstring().c_str(), iconSize);
            if (!hIcon)
            {
                log.Warn(log.APIError(), "Cant load icon from PNG");
                return NULL;
            }
            else
            {
                return hIcon;
            }
        }

        HRESULT hr = SHDefExtractIconW(path.wstring().c_str(), index, 0, &hIcon, NULL, (DWORD)iconSize);

        if (hr != 0 || !hIcon)
        {
            log.Warn(log.APIError(), "No icon with index %d", index);
            return NULL;
        }

        return hIcon;
    }

    Gdiplus::Bitmap * LoadBitmapFromIcon(const std::wstring &icon, int iconSize)
    {
        HICON hIcon = Icon::LoadIcon(icon, iconSize);
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
}

namespace Icon = AnyFSE::Tools::Icon;
