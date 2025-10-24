#include "VideoPlayer.hpp"

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
            case MFP_EVENT_TYPE_MEDIAITEM_SET:
            {
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
                m_duration = GetDuration(pEventHeader->pMediaPlayer);
            }
            break;

            case MFP_EVENT_TYPE_PLAYBACK_ENDED:
            {
                if (m_loop)
                {
                    pEventHeader->pMediaPlayer->Play();
                }
                else if (m_startLoop != 0)
                {
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
        MFStartup(MF_VERSION);
    }

    SimpleVideoPlayer::~SimpleVideoPlayer()
    {
        Close();
        MFShutdown();
    }

    HRESULT SimpleVideoPlayer::Load(const WCHAR *videoFile, bool mute, bool loop, HWND hwndParent)
    {
        // Clean up any existing instance
        if (m_bInitialized)
        {
            Close();
        }

        m_loop = loop;

        ParseLoopTimings(videoFile);

        HRESULT hr = S_OK;

        hr = MFPCreateMediaPlayer(
            videoFile,
            FALSE,
            MFP_OPTION_NO_MMCSS | MFP_OPTION_NO_REMOTE_DESKTOP_OPTIMIZATION,
            this,
            hwndParent,
            &m_pPlayer);

        if (FAILED(hr))
        {
            return hr;
        }

        // Set volume to 100%
        if (m_pPlayer)
        {
            m_pPlayer->SetMute(mute);
        }

        m_bInitialized = TRUE;

        return hr;
    }

    HRESULT SimpleVideoPlayer::Play()
    {

        if (!m_pPlayer || !m_bInitialized)
        {
            return E_FAIL;
        }

        return m_pPlayer->Play();
    }

    HRESULT SimpleVideoPlayer::Stop()
    {
        if (!m_pPlayer || !m_bInitialized)
        {
            return E_FAIL;
        }

        return m_pPlayer->Stop();
    }

    void SimpleVideoPlayer::Close()
    {
        Cleanup();
    }

    void SimpleVideoPlayer::Cleanup()
    {
        if (m_pPlayer)
        {
            m_pPlayer->Shutdown();
            m_pPlayer->Release();
            m_pPlayer = nullptr;
        }

        if (m_bInitialized)
        {
            m_bInitialized = FALSE;
        }
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
            result = varDuration.hVal.QuadPart / 10000;
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