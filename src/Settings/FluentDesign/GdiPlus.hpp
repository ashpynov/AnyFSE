#pragma once
#include <gdiplus.h>

namespace Gdiplus
{
    static void RoundRect(Graphics& graphics, const RectF& rect, REAL cornerRadius, const Brush *brush, const Pen &pen)
    {
        REAL radius = min(min(rect.Height, rect.Width), cornerRadius);
        GraphicsPath path;
        path.AddLine(rect.X + radius, rect.Y, rect.GetRight() - radius, rect.Y);
        path.AddArc(rect.GetRight() - radius, rect.Y, radius, radius, 270, 90);
        path.AddArc(rect.GetRight() - radius, rect.GetBottom() - radius, radius, radius, 0, 90);
        path.AddArc(rect.X, rect.GetBottom() - radius, radius, radius, 90, 90);
        path.AddArc(rect.X, rect.Y, radius, radius, 180, 90);
        path.CloseFigure();

        if (brush) graphics.FillPath(brush, &path);
        graphics.DrawPath(&pen, &path);
    }

    static RectF FromRECT(const RECT &rect)
    {
        return RectF((REAL)rect.left, (REAL)rect.top,
                    (REAL)(rect.right - rect.left),
                    (REAL)(rect.bottom - rect.top));
    }
}