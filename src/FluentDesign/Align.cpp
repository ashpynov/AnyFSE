#include <windows.h>
#include "Align.hpp"

namespace FluentDesign::Align
{
    Anchor Align(Side left, Side top, Side right, Side bottom)
    {
        return (left << 6) | (top << 4) | (right << 2) | bottom;
    }

    Side LeftSide(Anchor anchor)    { return (Side)((anchor >> 6 ) & Side::Center); }
    Side TopSide(Anchor anchor)     { return (Side)((anchor >> 4 ) & Side::Center); }
    Side RightSide(Anchor anchor)   { return (Side)((anchor >> 2 ) & Side::Center); }
    Side BottomSide(Anchor anchor)  { return (Side)(anchor & Side::Center); }

    Anchor TopLeft()      { return Align(Side::Left,  Side::Top); }
    Anchor TopRight()     { return Align(Side::Right, Side::Top); }
    Anchor BottomLeft()   { return Align(Side::Left,  Side::Bottom); }
    Anchor BottomRight()  { return Align(Side::Right, Side::Bottom); }
    Anchor Client()       { return Align(Side::Left,  Side::Top,    Side::Right, Side::Bottom); }
    Anchor TopClient()    { return Align(Side::Left,  Side::Top,    Side::Right, Side::Top);    }
    Anchor BottomClient() { return Align(Side::Left,  Side::Top,    Side::Right, Side::Top);    }

    float HSide(Side side, const Gdiplus::RectF& rect, float size)
    {
        switch (side)
        {
            case Align::Side::Left:
                return rect.GetLeft();
            case Align::Side::Right:
                return rect.GetRight();
            case Align::Side::Center:
                return rect.GetLeft() + rect.Width / 2;
        }
        return size;
    }

    float VSide(Side side, const Gdiplus::RectF& rect, float size)
    {
        switch (side)
        {
            case Align::Side::Top:
                return rect.GetTop();
            case Align::Side::Bottom:
                return rect.GetBottom();
            case Align::Side::Center:
                return rect.GetTop() + rect.Height/ 2;
        }
        return size;
    }
}
