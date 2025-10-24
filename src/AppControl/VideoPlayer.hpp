#pragma once

#include <string>
#include <windows.h>
#include <mfapi.h>
#include <mfplay.h>
#include <mfidl.h>

namespace AnyFSE::App::AppControl::Window
{
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

        volatile long m_refCount;


        void Cleanup();
        void ParseLoopTimings(const std::wstring& path);

        DWORD   GetDuration(IMFPMediaPlayer *player);
        HRESULT Rewind(IMFPMediaPlayer *player, DWORD position);

    public:
        SimpleVideoPlayer();
        ~SimpleVideoPlayer();


        // Initialize the player with a parent window
        HRESULT Stop();

        // Close and release all resources
        void Close();

        // Combined load and play
        HRESULT Load(const WCHAR *videoFile, bool mute, bool loop, HWND hwndParent);
        HRESULT Play();

        // Check if player is initialized
        BOOL IsInitialized() const { return m_bInitialized; }

        // Get the video window handle
        HWND GetVideoWindow() const { return m_hwndVideo; }

        // IUnknown
        ULONG AddRef();
        HRESULT QueryInterface(REFIID riid, void **ppv);
        ULONG Release();

        // IMFPMediaPlayerCallback
        void OnMediaPlayerEvent(MFP_EVENT_HEADER *pEventHeader);
    };
}