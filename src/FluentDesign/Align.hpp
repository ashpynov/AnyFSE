#pragma once
#include <windows.h>
#include <gdiplus.h>

namespace FluentDesign::Align
{
    typedef BYTE Anchor;
    enum Side: Anchor
    {
        None = 0,
        Left = 1,
        Right = 2,
        Center = 3,
        Top = Left,
        Bottom = Right
    };

    struct Margins
    {
        float Left;
        float Top;
        float Right;
        float Bottom;
    };

    Side LeftSide(Anchor anchor);
    Side TopSide(Anchor anchor);
    Side RightSide(Anchor anchor);
    Side BottomSide(Anchor anchor);

    Anchor Align(Side left, Side top, Side right = None, Side bottom = None);

    Anchor TopLeft();
    Anchor TopRight();
    Anchor BottomLeft();
    Anchor BottomRight();
    Anchor Client();
    Anchor TopClient();
    Anchor BottomClient();

    float HSide(Side side, const Gdiplus::RectF &rect, float size);
    float VSide(Side side, const Gdiplus::RectF &rect, float size);
}
