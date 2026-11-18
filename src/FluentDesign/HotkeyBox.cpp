#include "HotkeyBox.hpp"
#include "ComboBox.hpp"

namespace FluentDesign
{
    HotkeyBox::~HotkeyBox()
    {
        if (IsWindow(m_hWnd))
        {
            RemoveWindowSubclass(m_hWnd, SubclassProc, 0);
        }
    }

    HotkeyBox& HotkeyBox::Create(HWND hParent, int x, int y, int width, int height)
    {
        Button::Create(hParent, x, y, width, m_theme.DpiScale(Layout_HotkeyBoxHeight));
        if (m_hWnd)
        {
            // Install after Button and Theme so capture runs before their navigation handlers.
            SetWindowSubclass(m_hWnd, SubclassProc, 0, reinterpret_cast<DWORD_PTR>(this));
            UpdateCaption();
        }
        return *this;
    }

    void HotkeyBox::SetPrompts(const std::wstring& emptyText, const std::wstring& recordingText)
    {
        m_emptyText = emptyText;
        m_recordingText = recordingText;
        UpdateCaption();
    }

    void HotkeyBox::SetHotkey(Hotkey hotkey)
    {
        hotkey.modifiers &= MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_WIN;
        if (!hotkey.key)
            hotkey.modifiers = 0;
        m_hotkey = hotkey;
        UpdateCaption();
    }

    UINT HotkeyBox::ReadModifiers()
    {
        UINT modifiers = 0;
        if (GetKeyState(VK_CONTROL) & 0x8000) modifiers |= MOD_CONTROL;
        if (GetKeyState(VK_MENU) & 0x8000) modifiers |= MOD_ALT;
        if (GetKeyState(VK_SHIFT) & 0x8000) modifiers |= MOD_SHIFT;
        if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) modifiers |= MOD_WIN;
        return modifiers;
    }

    bool HotkeyBox::IsModifier(UINT key)
    {
        return key == VK_CONTROL || key == VK_LCONTROL || key == VK_RCONTROL
            || key == VK_MENU || key == VK_LMENU || key == VK_RMENU
            || key == VK_SHIFT || key == VK_LSHIFT || key == VK_RSHIFT
            || key == VK_LWIN || key == VK_RWIN;
    }

    std::vector<std::wstring> HotkeyBox::GetKeyLabels(Hotkey hotkey)
    {
        std::vector<std::wstring> labels;
        if (hotkey.modifiers & MOD_WIN) labels.emplace_back(L"Win");
        if (hotkey.modifiers & MOD_CONTROL) labels.emplace_back(L"Ctrl");
        if (hotkey.modifiers & MOD_ALT) labels.emplace_back(L"Alt");
        if (hotkey.modifiers & MOD_SHIFT) labels.emplace_back(L"Shift");
        if (hotkey.key)
        {
            const UINT scanCode = MapVirtualKey(hotkey.key, MAPVK_VK_TO_VSC_EX);
            const LONG keyData = static_cast<LONG>(((scanCode & 0xff) << 16) | ((scanCode & 0xff00) ? (1 << 24) : 0));
            wchar_t name[128]{};
            if (scanCode && GetKeyNameText(keyData, name, ARRAYSIZE(name)))
                labels.emplace_back(name);
            else
                labels.emplace_back(L"VK " + std::to_wstring(hotkey.key));
        }
        return labels;
    }

    void HotkeyBox::UpdateCaption()
    {
        if (!m_hWnd)
            return;
        const Hotkey displayed = m_recording ? Hotkey{m_previewModifiers, 0} : m_hotkey;
        const std::vector<std::wstring> labels = GetKeyLabels(displayed);
        std::wstring caption;
        for (const std::wstring& label : labels)
        {
            if (!caption.empty()) caption += L" + ";
            caption += label;
        }
        if (m_recording && !caption.empty()) caption += L" +";
        if (caption.empty()) caption = m_recording ? m_recordingText : m_emptyText;
        // The native button name exposes the value to accessibility clients.
        SetWindowText(m_hWnd, caption.c_str());
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }

    void HotkeyBox::HandleClick()
    {
        BeginRecording();
    }

    void HotkeyBox::BeginRecording()
    {
        if (m_recording || !IsWindowEnabled(m_hWnd))
            return;
        m_theme.SwapFocus(m_hWnd);
        if (GetFocus() != m_hWnd)
            return;
        m_previewModifiers = ReadModifiers();
        if (m_previewModifiers && !(m_previewModifiers & RequiredModifiers))
            return;
        m_recording = true;
        UpdateCaption();
        OnRecordingChanged.Notify();
    }

    void HotkeyBox::CancelRecording()
    {
        if (!m_recording)
            return;
        m_recording = false;
        m_activationKey = 0;
        UpdateCaption();
        OnRecordingChanged.Notify();
    }

    void HotkeyBox::Commit(Hotkey hotkey, UINT finishingKey)
    {
        const bool changed = m_hotkey.key != hotkey.key || m_hotkey.modifiers != hotkey.modifiers;
        m_recording = false;
        m_activationKey = 0;
        m_finishingKey = finishingKey;
        SetHotkey(hotkey);
        OnRecordingChanged.Notify();
        if (changed)
            OnChanged.Notify();
    }

    LRESULT CALLBACK HotkeyBox::SubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR subclassId, DWORD_PTR refData)
    {
        HotkeyBox* control = reinterpret_cast<HotkeyBox*>(refData);
        const UINT key = static_cast<UINT>(wParam);
        switch (msg)
        {
        case WM_GETDLGCODE:
            if (control->m_recording || control->m_finishingKey)
                return DLGC_WANTALLKEYS;
            break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (control->m_finishingKey == key || control->m_activationKey == key)
                return 0;
            if (control->m_recording)
            {
                control->m_previewModifiers = ReadModifiers();
                if (IsModifier(key))
                {
                    if (!(control->m_previewModifiers & RequiredModifiers))
                    {
                        control->m_finishingKey = key;
                        control->CancelRecording();
                    }
                    else
                        control->UpdateCaption();
                }
                else if (!(lParam & (static_cast<LPARAM>(1) << 30)))
                {
                    if (!control->m_previewModifiers && (key == VK_BACK || key == VK_DELETE))
                        control->Commit({}, key);
                    else if (!(control->m_previewModifiers & RequiredModifiers))
                    {
                        control->m_finishingKey = key;
                        control->CancelRecording();
                    }
                    else
                        control->Commit({control->m_previewModifiers, key}, key);
                }
                return 0;
            }
            if ((key == VK_RETURN || key == VK_SPACE || key == VK_GAMEPAD_A) && !(lParam & (static_cast<LPARAM>(1) << 30)))
            {
                control->m_activationKey = key;
                control->BeginRecording();
                return 0;
            }
            break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (control->m_activationKey == key) control->m_activationKey = 0;
            if (control->m_finishingKey == key) control->m_finishingKey = 0;
            if (control->m_recording)
            {
                control->m_previewModifiers = ReadModifiers();
                control->UpdateCaption();
            }
            // Do not let the native button or menu process the recorded Space/Alt release.
            return 0;
        case WM_CHAR:
        case WM_SYSCHAR:
            return 0;
        case WM_KILLFOCUS:
        case WM_CANCELMODE:
            control->CancelRecording();
            control->m_finishingKey = 0;
            break;
        case WM_ENABLE:
            if (!wParam) control->CancelRecording();
            break;
        case WM_SHOWWINDOW:
            if (!wParam) control->CancelRecording();
            break;
        case WM_NCDESTROY:
        {
            control->m_recording = false;
            RemoveWindowSubclass(hWnd, SubclassProc, subclassId);
            const LRESULT result = DefSubclassProc(hWnd, msg, wParam, lParam);
            control->m_hWnd = nullptr;
            return result;
        }
        }
        return DefSubclassProc(hWnd, msg, wParam, lParam);
    }

    void HotkeyBox::DrawButton(HWND hWnd, HDC hdc, RECT rect)
    {
        Gdiplus::Graphics graphics(hdc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        const bool enabled = IsWindowEnabled(hWnd) != FALSE;
        const Theme::Colors background = !enabled ? Theme::Colors::ComboDisabled
            : m_mousePressed ? Theme::Colors::ComboPressed
            : IsMouseOver() ? Theme::Colors::ComboHover : Theme::Colors::Combo;
        Gdiplus::RectF backgroundRect = Gdiplus::ToRectF(rect);
        backgroundRect.Width -= 1;
        backgroundRect.Height -= 1;
        Gdiplus::SolidBrush backgroundBrush(m_theme.GetColor(background));
        Gdiplus::Pen backgroundBorder(m_theme.GetColor(Theme::Colors::ComboBorder), m_theme.DpiScaleF(1));
        Gdiplus::RoundRect(graphics, backgroundRect, static_cast<float>(m_theme.GetSize_FocusCornerSize()), &backgroundBrush, backgroundBorder);
        Gdiplus::Font font(hdc, m_theme.GetFont_Text());
        Gdiplus::StringFormat format;
        format.SetAlignment(Gdiplus::StringAlignmentNear);
        format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
        Gdiplus::SolidBrush textBrush(m_theme.GetColor(enabled ? Theme::Colors::Text : Theme::Colors::TextSecondary));
        Gdiplus::RectF content(static_cast<float>(rect.left), static_cast<float>(rect.top),
            static_cast<float>(rect.right - rect.left), static_cast<float>(rect.bottom - rect.top));
        content.Inflate(-m_theme.DpiScaleF(16), -m_theme.DpiScaleF(8));
        if (content.Width <= 0 || content.Height <= 0)
            return;
        graphics.SetClip(content);
        std::vector<std::wstring> labels = GetKeyLabels(m_recording ? Hotkey{m_previewModifiers, 0} : m_hotkey);
        if (labels.empty())
        {
            Gdiplus::SolidBrush promptBrush(m_theme.GetColor(Theme::Colors::TextDisabled));
            format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
            graphics.DrawString((m_recording ? m_recordingText : m_emptyText).c_str(), -1, &font, content, &format, &promptBrush);
        }
        else
        {
            format.SetAlignment(Gdiplus::StringAlignmentCenter);
            const float gap = m_theme.DpiScaleF(3);
            const float padding = m_theme.DpiScaleF(6);
            Gdiplus::RectF plusMeasured;
            graphics.MeasureString(L"+", -1, &font, Gdiplus::PointF(), &format, &plusMeasured);
            const float separatorWidth = plusMeasured.Width + gap * 2;
            std::vector<float> widths;
            float total = 0;
            for (const std::wstring& label : labels)
            {
                Gdiplus::RectF measured;
                graphics.MeasureString(label.c_str(), -1, &font, Gdiplus::PointF(), &format, &measured);
                widths.push_back(measured.Width + padding);
                total += widths.back();
            }
            const size_t separatorCount = labels.size() - (m_recording ? 0 : 1);
            total += separatorWidth * static_cast<float>(separatorCount);
            // Keep every key visible for long combinations and small controls.
            const float scale = total > content.Width ? content.Width / total : 1.0f;
            const Gdiplus::GraphicsState saved = graphics.Save();
            graphics.TranslateTransform(content.X, content.Y);
            graphics.ScaleTransform(scale, 1.0f);
            Gdiplus::SolidBrush back(m_theme.GetColor(Theme::Colors::Edit));
            Gdiplus::Pen border(m_theme.GetColor(Theme::Colors::EditAccent), m_theme.DpiScaleF(1));
            float x = 0;
            for (size_t index = 0; index < labels.size(); ++index)
            {
                Gdiplus::RectF keyRect(x, 0, widths[index], content.Height);
                Gdiplus::RoundRect(graphics, keyRect, m_theme.DpiScaleF(4), &back, border);
                graphics.DrawString(labels[index].c_str(), -1, &font, keyRect, &format, &textBrush);
                x += widths[index];
                if (index < separatorCount)
                {
                    const Gdiplus::RectF plusRect(x + gap, 0, plusMeasured.Width, content.Height);
                    graphics.DrawString(L"+", -1, &font, plusRect, &format, &textBrush);
                    x += separatorWidth;
                }
            }
            graphics.Restore(saved);
        }
        graphics.ResetClip();
        if (m_recording)
        {
            Gdiplus::Pen accent(m_theme.GetColor(Theme::Colors::EditAccentFocus), m_theme.DpiScaleF(2));
            graphics.DrawLine(&accent, m_theme.DpiScaleF(8), static_cast<float>(rect.bottom) - m_theme.DpiScaleF(2),
                static_cast<float>(rect.right) - m_theme.DpiScaleF(8), static_cast<float>(rect.bottom) - m_theme.DpiScaleF(2));
        }
    }
}
