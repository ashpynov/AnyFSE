#pragma once

#include "Button.hpp"

namespace FluentDesign
{
    // A button that records one main key and the RegisterHotKey-compatible modifiers.
    class HotkeyBox : public Button
    {
    public:
        struct Hotkey
        {
            UINT modifiers = 0;
            UINT key = 0;
        };

        explicit HotkeyBox(Theme& theme) : Button(theme) {}
        ~HotkeyBox() override;

        HotkeyBox& Create(HWND hParent, int x, int y, int width, int height) override;
        void SetHotkey(Hotkey hotkey);
        Hotkey GetHotkey() const { return m_hotkey; }
        bool IsRecording() const { return m_recording; }
        void BeginRecording();
        void CancelRecording();
        void SetPrompts(const std::wstring& emptyText, const std::wstring& recordingText);

        // OnChanged (inherited from Button) fires only for a committed value change.
        Event OnRecordingChanged;

    protected:
        void HandleClick() override;
        void DrawButton(HWND hWnd, HDC hdc, RECT rect) override;

    private:
        static constexpr int Layout_HotkeyBoxHeight = 38;
        static constexpr UINT RequiredModifiers = MOD_WIN | MOD_ALT | MOD_CONTROL;

        Hotkey m_hotkey;
        bool m_recording = false;
        UINT m_previewModifiers = 0;
        UINT m_activationKey = 0;
        UINT m_finishingKey = 0;
        std::wstring m_emptyText;
        std::wstring m_recordingText;

        static UINT ReadModifiers();
        static bool IsModifier(UINT key);
        static std::vector<std::wstring> GetKeyLabels(Hotkey hotkey);
        void UpdateCaption();
        void Commit(Hotkey hotkey, UINT finishingKey);
        static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR subclassId, DWORD_PTR refData);
    };
}
