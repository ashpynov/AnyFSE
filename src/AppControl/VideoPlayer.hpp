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
        
        public:
        SimpleVideoPlayer();
        ~SimpleVideoPlayer();     
        
        // Combined load and play
        HRESULT Load(const WCHAR *videoFile, bool mute, bool loop, HWND hwndParent);
        HRESULT Play();
        HRESULT Stop();
        void Close();

        ULONG AddRef();
        HRESULT QueryInterface(REFIID riid, void **ppv);
        ULONG Release();

        void OnMediaPlayerEvent(MFP_EVENT_HEADER *pEventHeader);
    };
}