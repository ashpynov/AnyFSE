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

#include "Logging/LogManager.hpp"
#include "VideoPlayer.hpp"
#include "Tools/Unicode.hpp"

#include <filesystem>
#include <algorithm>
#include <string>
#include <shlwapi.h>
#include <iostream>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "shlwapi.lib")

namespace AnyFSE::App::AppControl::Window
{
    Logger log = LogManager::GetLogger("VideoPlayer");

    // IUnknown methods
    HRESULT SimpleVideoPlayer::QueryInterface(REFIID riid, void **ppv)
    {
        if (ppv == nullptr)
            return E_POINTER;

        if (riid == IID_IUnknown || riid == __uuidof(IMFPMediaPlayerCallback))
        {
            *ppv = static_cast<IMFPMediaPlayerCallback *>(this);
            AddRef();
            return S_OK;
        }

        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG SimpleVideoPlayer::AddRef()
    {
        return InterlockedIncrement(&m_refCount);
    }

    ULONG SimpleVideoPlayer::Release()
    {
        ULONG refCount = InterlockedDecrement(&m_refCount);
        if (refCount == 0)
        {
            delete this;
        }
        return refCount;
    }

    // IMFPMediaPlayerCallback methods
    void SimpleVideoPlayer::OnMediaPlayerEvent(MFP_EVENT_HEADER *pEventHeader)
    {
        CriticalSectionLock lock(&m_cs);

        switch (pEventHeader->eEventType)
        {
            case MFP_EVENT_TYPE_MEDIAITEM_CREATED:
            {
                MFP_MEDIAITEM_CREATED_EVENT* pEvent = (MFP_MEDIAITEM_CREATED_EVENT*)pEventHeader;
                if (pEventHeader->pMediaPlayer && pEvent->pMediaItem)
                {
                    HRESULT hr = pEventHeader->pMediaPlayer->SetMediaItem(pEvent->pMediaItem);
                    if (FAILED(hr))
                    {
                        log.Error(log.APIError(), "Cant set media data:");
                    }
                }
            }
            break;
            case MFP_EVENT_TYPE_MEDIAITEM_SET:
            {
                log.Debug("Video is loaded and ready to play");
                SIZE screenSize, videoSize;
                pEventHeader->pMediaPlayer->GetIdealVideoSize(nullptr, &screenSize);
                pEventHeader->pMediaPlayer->GetNativeVideoSize(&videoSize, nullptr);

                float zx = (float)screenSize.cx / videoSize.cx;
                float zy = (float)screenSize.cy / videoSize.cy;
                float zm = max(zx, zy);
                zx = (1.0f - zx / zm) / 2;
                zy = (1.0f - zy / zm) / 2;

                MFVideoNormalizedRect rect;
                rect.left = zx;
                rect.top = zy;
                rect.right = 1.0f - zx;
                rect.bottom = 1.0f - zy;

                pEventHeader->pMediaPlayer->SetVideoSourceRect(&rect);
                log.Debug("Video margins applied: %f, %f", zx,zy);

                m_duration = GetDuration(pEventHeader->pMediaPlayer);
                log.Debug("Video duration: %.3f s", (float)m_duration/1000);

                HWND mediaHwnd = NULL;
                if (m_desiredState == MFP_MEDIAPLAYER_STATE_PLAYING)
                {
                    pEventHeader->pMediaPlayer->Play();
                    log.Debug("Show in MFP_EVENT_TYPE_MEDIAITEM_SET");
                    ShowVideo(true);
                }
            }
            break;
            case MFP_EVENT_TYPE_PLAYBACK_ENDED:
            {
                log.Debug("Video Completed");
                if (m_loop)
                {
                    log.Debug("Video Loop");
                    pEventHeader->pMediaPlayer->Play();
                }
                else if (m_startLoop != 0)
                {
                    log.Debug("Rewind to %dms", m_startLoop);
                    pEventHeader->pMediaPlayer->SetMute(m_mutedLoop);
                    Rewind(pEventHeader->pMediaPlayer, m_startLoop);
                }
                else if (!m_pause)
                {
                    Close();
                }
            }
            break;
            case MFP_EVENT_TYPE_STOP:
            {
                log.Debug("Video Stopped");
                if (m_desiredState == MFP_MEDIAPLAYER_STATE_EMPTY)
                {
                    log.Debug("Clearing Media item");
                    pEventHeader->pMediaPlayer->ClearMediaItem();
                }
            }
            default:
                //log.Debug("Event recieved: %d", pEventHeader->eEventType);
                break;
        }
    }

    SimpleVideoPlayer::SimpleVideoPlayer()
        : m_pPlayer(nullptr)
        , m_refCount(1)
        , m_hwndVideo(nullptr)
        , m_bInitialized(FALSE)
        , m_loop(false)
        , m_pause(true)
        , m_desiredState(MFP_MEDIAPLAYER_STATE_EMPTY)
    {
        CoInitializeEx(NULL, COINIT_MULTITHREADED);
        InitializeCriticalSection(&m_cs);
    }

    SimpleVideoPlayer::~SimpleVideoPlayer()
    {
        {
            CriticalSectionLock lock(&m_cs);
            Close();
            if (m_pPlayer)
            {
                log.Debug("Cleanup player");
                HRESULT hr = m_pPlayer->Stop();

                hr = m_pPlayer->Shutdown();
                if (FAILED(hr))
                {
                    log.Error(log.APIError(), "Cant Shutdown player");
                }

                m_pPlayer->Release();
                m_pPlayer = nullptr;
            }

            MFShutdown();
            m_bInitialized = FALSE;
        }
        DeleteCriticalSection(&m_cs);
        CoUninitialize();
    }

    HRESULT SimpleVideoPlayer::Load(const WCHAR *videoFile, bool mute, bool loop, bool pause, HWND hwndParent)
    {
        CriticalSectionLock lock(&m_cs);

        if (m_bInitialized && videoFile && videoFile[0] && m_loadedVideo == videoFile)
        {
            log.Debug("Video: %s is preloaded already", Unicode::to_string(videoFile).c_str());
            return S_OK;
        }

        log.Debug("Load Video: %s", Unicode::to_string(videoFile).c_str());

        if (m_bInitialized)
        {
            log.Debug("Close old");
            Close();
        }

        if (!videoFile || !videoFile[0])
        {
            return S_OK;
        }

        m_loop = loop;
        m_pause = pause;

        ParseLoopTimings(videoFile);

        HRESULT hr = S_OK;

        if (!m_pPlayer)
        {
            hr = MFStartup(MF_VERSION);

            log.Trace("Creating Media Player");

            RECT parentRect;
            GetClientRect(hwndParent, &parentRect);


            m_hwndVideo = CreateWindowExW(
                0,
                L"STATIC",
                L"",
                WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                0, 0,
                parentRect.right, parentRect.bottom,
                hwndParent,
                NULL,
                GetModuleHandle(NULL),
                this);

            hr = MFPCreateMediaPlayer(
                NULL,
                FALSE,
                MFP_OPTION_FREE_THREADED_CALLBACK,
                this,
                m_hwndVideo,
                &m_pPlayer);

            if (FAILED(hr))
            {
                log.Error(log.APIError(), "Creating Media Player Failed");
                return hr;
            }

            if (m_pPlayer)
            {
                m_pPlayer->SetMute(mute);
            }

            m_bInitialized = TRUE;
        }
        ShowVideo(false);

        hr = m_pPlayer->CreateMediaItemFromURL(videoFile, FALSE, 0, NULL);
        if (FAILED(hr))
        {
            log.Error(log.APIError(), "Cant create media item");
        }
        m_loadedVideo = videoFile;

        return hr;
    }

    HRESULT SimpleVideoPlayer::Play()
    {
        CriticalSectionLock lock(&m_cs);

        m_desiredState = MFP_MEDIAPLAYER_STATE_PLAYING;

        if (!m_pPlayer || !m_bInitialized)
        {
            log.Error("Cannot play no player initialized");
            return E_FAIL;
        }

        MFP_MEDIAPLAYER_STATE state;
        HRESULT hr = m_pPlayer->GetState(&state);
        if (state == MFP_MEDIAPLAYER_STATE_EMPTY)
        {
            log.Debug("Play called while not loaded");
            return S_OK;
        }

        hr = m_pPlayer->Play();
        log.Debug("Show in Play");
        ShowVideo(true);

        if (FAILED(hr))
        {
            log.Error(log.APIError(), "Cannot play:");
        }
        return hr;
    }

    HRESULT SimpleVideoPlayer::Stop()
    {
        CriticalSectionLock lock(&m_cs);

        m_desiredState = MFP_MEDIAPLAYER_STATE_STOPPED;

        if (!m_pPlayer || !m_bInitialized)
        {
            return E_FAIL;
        }
        ShowVideo(false);
        m_pPlayer->SetMute(TRUE);
        MFP_MEDIAPLAYER_STATE state;
        HRESULT hr = m_pPlayer->GetState(&state);
        if (state == MFP_MEDIAPLAYER_STATE_EMPTY)
        {
            log.Debug("Stop called while not loaded");
            return S_OK;
        }

        log.Debug("Stopping");
        return m_pPlayer->Stop();
    }

    void SimpleVideoPlayer::Close()
    {
        CriticalSectionLock lock(&m_cs);
        m_desiredState = MFP_MEDIAPLAYER_STATE_EMPTY;
        if (!m_pPlayer || !m_bInitialized)
        {
            return;
        }
        ShowVideo(false);
        m_pPlayer->Stop();
        m_pPlayer->ClearMediaItem();
        m_loadedVideo = L"";
        if (FAILED(m_pPlayer->Shutdown()))
        {
            log.Error(log.APIError(), "Cant Shutdown player");
        }

        m_pPlayer->Release();
        m_pPlayer = nullptr;
        m_bInitialized = false;
    }

    void SimpleVideoPlayer::Resize()
    {
        if (!m_hwndVideo) return;

        RECT parentRect;
        GetClientRect(GetParent(m_hwndVideo), &parentRect);
        MoveWindow(m_hwndVideo, 0, 0, parentRect.right, parentRect.bottom, TRUE);
    }

    void SimpleVideoPlayer::ParseLoopTimings(const std::wstring &path)
    {
        m_startLoop = 0;
        m_endLoop = 0;
        m_duration = 0;
        m_mutedLoop = false;

        namespace fs = std::filesystem;
        std::wstring timings = fs::path(path).filename().replace_extension().extension().wstring();
        if (timings.size() < 2 )
        {
            return;
        }

        auto start = timings.begin() + 1;
        if (*start == L'm' || *start == L'M')
        {
            m_mutedLoop = true;
            start++;
        }

        auto delim = std::find(start, timings.end(), L'-');

        std::wstring startLoopStr = timings.substr(
            std::distance(timings.begin(), start),
            std::distance(start, delim));

        std::wstring endLoopStr = delim != timings.end()
            ? timings.substr(
                std::distance(timings.begin(), delim + 1) ,
                std::distance(delim + 1, timings.end()))
            : L"";

        try
        {
            m_startLoop = std::stoul(startLoopStr);
            m_endLoop = std::stoul(endLoopStr);
        }
        catch(...){}

    }
    DWORD SimpleVideoPlayer::GetDuration(IMFPMediaPlayer *player)
    {
        DWORD result = 0;
        if (!player)
        {
            return result;
        }

        PROPVARIANT varDuration;
        PropVariantInit(&varDuration);

        if ( SUCCEEDED(player->GetDuration(GUID_NULL, &varDuration)) )
        {
            result = (DWORD)(varDuration.hVal.QuadPart / 10000);
        }

        PropVariantClear(&varDuration);
        return result;
    }
    HRESULT SimpleVideoPlayer::Rewind(IMFPMediaPlayer *player, DWORD position)
    {
        PROPVARIANT varDuration;
        PropVariantInit(&varDuration);
        varDuration.vt = VT_I8;
        varDuration.hVal.QuadPart = position * 10000;

        HRESULT hr = player->SetPosition(GUID_NULL, &varDuration);
        if (SUCCEEDED(hr))
        {
             hr = player->Play();
        }
        PropVariantClear(&varDuration);
        return hr;

    }

    BOOL SimpleVideoPlayer::ShowVideo(bool bShow)
    {
        if (!m_pPlayer)
        {
            return false;
        }

        HWND hVideoWin = NULL;
        m_pPlayer->GetVideoWindow(&hVideoWin);
        if (hVideoWin)
        {
            if (bShow)
            {
                log.Debug("Show Video Window");
                Resize();
                ShowWindow(hVideoWin, SW_SHOW);
            }
            else
            {
                log.Debug("Hide Video Window");
                MoveWindow(hVideoWin, 0, 0, 0, 0, TRUE);
                ShowWindow(hVideoWin, SW_HIDE);
                UpdateWindow(GetParent(hVideoWin));
            }
        }
        return hVideoWin ? TRUE : FALSE;
    }
}