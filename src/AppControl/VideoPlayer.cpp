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
                if (SUCCEEDED(pEventHeader->pMediaPlayer->GetVideoWindow(&mediaHwnd))
                    && IsWindowVisible(mediaHwnd))
                {
                    pEventHeader->pMediaPlayer->Play();
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
            }
            break;
        }
    }

    SimpleVideoPlayer::SimpleVideoPlayer()
        : m_pPlayer(nullptr)
        , m_refCount(1)
        , m_hwndVideo(nullptr)
        , m_bInitialized(FALSE)
        , m_loop(false)
    {

    }

    SimpleVideoPlayer::~SimpleVideoPlayer()
    {
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

    HRESULT SimpleVideoPlayer::Load(const WCHAR *videoFile, bool mute, bool loop, HWND hwndParent)
    {
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

        ParseLoopTimings(videoFile);
        
        HRESULT hr = S_OK;

        if (!m_pPlayer)
        {
            MFStartup(MF_VERSION);

            log.Trace("Creating Media Player");
            hr = MFPCreateMediaPlayer(
                NULL,
                FALSE,
                MFP_OPTION_NO_REMOTE_DESKTOP_OPTIMIZATION,
                this,
                hwndParent,
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

        m_nextVideo = videoFile;
        
        hr = m_pPlayer->CreateMediaItemFromURL(videoFile, FALSE, 0, NULL);
        if (FAILED(hr))
        {
            log.Error(log.APIError(), "Cant create media item");
        }

        return hr;
    }

    HRESULT SimpleVideoPlayer::Play()
    {

        if (!m_pPlayer || !m_bInitialized)
        {
            log.Error("Cannot play no player initialized");
            return E_FAIL;
        }

        HRESULT hr = m_pPlayer->Play();
        if (FAILED(hr))
        {
            log.Error(log.APIError(), "Cannot play:");
        }
        return hr;
    }

    HRESULT SimpleVideoPlayer::Stop()
    {
        if (!m_pPlayer || !m_bInitialized)
        {
            return E_FAIL;
        }

        log.Debug("Stopping");
        return m_pPlayer->Stop();
    }

    void SimpleVideoPlayer::Close()
    {
        HRESULT hr = m_pPlayer->Stop();
        hr = m_pPlayer->ClearMediaItem();
        m_nextVideo = L"";
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
}