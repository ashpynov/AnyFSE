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

#define byte ::byte
#include <gdiplus.h>

namespace Gdiplus
{

    enum FrameFlags
    {
        CORNER_NW = 0x01, // Northwest corner rounded
        CORNER_NE = 0x02, // Northeast corner rounded
        CORNER_SE = 0x04, // Southeast corner rounded
        CORNER_SW = 0x08, // Southwest corner rounded

        CORNER_ALL = CORNER_NW | CORNER_NE | CORNER_SE | CORNER_SW,

        SIDE_TOP = 0x10,    // Draw top side
        SIDE_RIGHT = 0x20,  // Draw right side
        SIDE_BOTTOM = 0x40, // Draw bottom side
        SIDE_LEFT = 0x80,   // Draw left side

        SIDE_ALL = SIDE_TOP | SIDE_RIGHT | SIDE_BOTTOM | SIDE_LEFT,

        SIDE_NO_TOP = SIDE_RIGHT | SIDE_BOTTOM | SIDE_LEFT,
        SIDE_NO_BOTTOM = SIDE_RIGHT | SIDE_TOP | SIDE_LEFT,
        CORNER_BOTTOM = CORNER_SE | CORNER_SW,
        CORNER_TOP = CORNER_NE | CORNER_NW,
    };

    static void AddCorner(GraphicsPath &path, UINT corner, UINT round, const RectF& rect, REAL dia)
    {
        switch (corner)
        {
            case CORNER_NE:
                round   ? path.AddArc(rect.GetRight() - dia, rect.Y, dia, dia, 270, 90)
                        : path.AddLine(rect.GetRight(), rect.Y, rect.GetRight(), rect.Y);
                break;

            case CORNER_SE:
                round   ? path.AddArc(rect.GetRight() - dia, rect.GetBottom()-dia, dia, dia, 0, 90)
                        : path.AddLine(rect.GetRight(), rect.GetBottom(), rect.GetRight(), rect.GetBottom());
                break;

            case CORNER_SW:
                round   ? path.AddArc(rect.X, rect.GetBottom() - dia, dia, dia, 90, 90)
                        : path.AddLine(rect.X, rect.GetBottom(), rect.X, rect.GetBottom());
                break;

            case CORNER_NW:
                round   ? path.AddArc(rect.X, rect.Y, dia, dia, 180, 90)
                        : path.AddLine(rect.X, rect.Y, rect.X, rect.Y);
                break;

            case CORNER_NW | SIDE_TOP:
                round   ? path.AddLine(rect.X, rect.Y+dia/2, rect.X, rect.Y+dia/2)
                        : path.AddLine(rect.X, rect.Y, rect.X, rect.Y);
                break;
        }
    }

    static void RoundRect(Graphics& graphics, const RectF& origRect, REAL cornerRadius, const Brush *brush, const Pen &pen,
        UINT flags = SIDE_ALL | CORNER_ALL
    )
    {
        RectF rect = origRect;
        REAL dia = min(min(rect.Height, rect.Width), cornerRadius);
        REAL radius = dia / 2;

        if (brush)
        {
            GraphicsPath path;
            AddCorner(path, CORNER_NE, flags & CORNER_NE, rect, dia);
            AddCorner(path, CORNER_SE, flags & CORNER_SE, rect, dia);
            AddCorner(path, CORNER_SW, flags & CORNER_SW, rect, dia);
            AddCorner(path, CORNER_NW, flags & CORNER_NW, rect, dia);
            path.CloseFigure();
            graphics.FillPath(brush, &path);
        }

        RectF strokeRect = origRect;
        strokeRect.Inflate(-.5, -.5);
        dia = min(min(strokeRect.Height, strokeRect.Width), cornerRadius);
        GraphicsPath strokes[4];
        int n = 0;
        if (flags & SIDE_TOP)
        {
            AddCorner(strokes[++n], CORNER_NW, flags & CORNER_NW, strokeRect, dia);
            AddCorner(strokes[n], CORNER_NE, flags & CORNER_NE, strokeRect, dia);
        }
        if (flags & SIDE_RIGHT)
        {
            if (!(flags & SIDE_TOP))
            {
                AddCorner(strokes[++n], CORNER_NE, flags & CORNER_NE, strokeRect, dia);
            }
            AddCorner(strokes[n], CORNER_SE, flags & CORNER_SE, strokeRect, dia);
        }
        if (flags & SIDE_BOTTOM)
        {
            if (!(flags & SIDE_RIGHT))
            {
                AddCorner(strokes[++n], CORNER_SE, flags & CORNER_SE, strokeRect, dia);
            }
            AddCorner(strokes[n], CORNER_SW, flags & CORNER_SW, strokeRect, dia);
        }
        if (flags & SIDE_LEFT)
        {
            if (!(flags & SIDE_BOTTOM))
            {
                AddCorner(strokes[++n], CORNER_SW, flags & CORNER_SW, strokeRect, dia);
            }
            AddCorner(strokes[n], CORNER_NW | (flags & SIDE_TOP), flags & CORNER_NW, strokeRect, dia);
        }

        for (int i = 0; i <= n; i++)
        {
            graphics.DrawPath(&pen, &strokes[i]);
        }
    }

    static RectF ToRectF(const RECT &rect)
    {
        return RectF((REAL)rect.left, (REAL)rect.top,
                    (REAL)(rect.right - rect.left),
                    (REAL)(rect.bottom - rect.top));
    }

    static Rect ToRect(const RECT &rect)
    {
        return Rect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    }

    static Image* LoadImageFromResource(HMODULE hMod, const wchar_t* resid, const wchar_t* restype)
    {
        IStream* pStream = nullptr;
        Image* pBmp = nullptr;
        HGLOBAL hGlobal = nullptr;

        HRSRC hrsrc = FindResourceW(hMod, resid, restype);     // get the handle to the resource
        if (hrsrc)
        {
            DWORD dwResourceSize = SizeofResource(hMod, hrsrc);
            if (dwResourceSize > 0)
            {
                HGLOBAL hGlobalResource = LoadResource(hMod, hrsrc); // load it
                if (hGlobalResource)
                {
                    void* imagebytes = LockResource(hGlobalResource); // get a pointer to the file bytes

                    // copy image bytes into a real hglobal memory handle
                    hGlobal = ::GlobalAlloc(GHND, dwResourceSize);
                    if (hGlobal)
                    {
                        void* pBuffer = ::GlobalLock(hGlobal);
                        if (pBuffer)
                        {
                            memcpy(pBuffer, imagebytes, dwResourceSize);
                            HRESULT hr = CreateStreamOnHGlobal(hGlobal, TRUE, &pStream);
                            if (SUCCEEDED(hr))
                            {
                                // pStream now owns the global handle and will invoke GlobalFree on release
                                hGlobal = nullptr;
                                pBmp = new Image(pStream);
                            }
                        }
                    }
                }
            }
        }

        if (pStream)
        {
            pStream->Release();
            pStream = nullptr;
        }

        if (hGlobal)
        {
            GlobalFree(hGlobal);
            hGlobal = nullptr;
        }

        return pBmp;
    }

}