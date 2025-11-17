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
#include <windows.h>
#include <mfapi.h>
#include <mfplay.h>
#include <mfidl.h>

namespace AnyFSE::App::AppControl::Window
{
    class CriticalSectionLock {
    private:
        CRITICAL_SECTION* m_cs;

    public:
        explicit CriticalSectionLock(CRITICAL_SECTION* cs) : m_cs(cs) {
            EnterCriticalSection(m_cs);
        }

        ~CriticalSectionLock() {
            LeaveCriticalSection(m_cs);
        }

        // Prevent copying
        CriticalSectionLock(const CriticalSectionLock&) = delete;
        CriticalSectionLock& operator=(const CriticalSectionLock&) = delete;
    };

    class SimpleVideoPlayer : public IMFPMediaPlayerCallback
    {
    private:
        IMFPMediaPlayer *m_pPlayer;
        HWND m_hwndVideo;
        BOOL m_bInitialized;
        bool m_loop;
        bool m_pause;

        DWORD m_startLoop;
        DWORD m_endLoop;
        DWORD m_duration;
        bool m_mutedLoop;

        std::wstring m_loadedVideo;

        volatile long m_refCount;

        MFP_MEDIAPLAYER_STATE m_desiredState;
        CRITICAL_SECTION m_cs;

        void ParseLoopTimings(const std::wstring& path);

        DWORD   GetDuration(IMFPMediaPlayer *player);
        HRESULT Rewind(IMFPMediaPlayer *player, DWORD position);
        BOOL ShowVideo(bool bShow=true);

        public:
        SimpleVideoPlayer();
        ~SimpleVideoPlayer();

        // Combined load and play
        HRESULT Load(const WCHAR *videoFile, bool mute, bool loop, bool pause, HWND hwndParent);
        HRESULT Play();
        HRESULT Stop();
        void Close();
        void Resize();

        ULONG AddRef();
        HRESULT QueryInterface(REFIID riid, void **ppv);
        ULONG Release();

        void OnMediaPlayerEvent(MFP_EVENT_HEADER *pEventHeader);
    };
}