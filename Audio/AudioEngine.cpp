//--------------------------------------------------------------------------------------
// File: AudioEngine.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Audio.h"
#include "SoundCommon.h"

#include <unordered_map>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

//#define VERBOSE_TRACE

#ifdef VERBOSE_TRACE
#pragma message("NOTE: Verbose tracing enabled")
#endif

namespace
{
    struct EngineCallback : public IXAudio2EngineCallback
    {
        EngineCallback() noexcept(false)
        {
            mCriticalError.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
            if (!mCriticalError)
            {
                throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
            }
        }

        EngineCallback(EngineCallback&&) = default;
        EngineCallback& operator= (EngineCallback&&) = default;

        EngineCallback(EngineCallback const&) = delete;
        EngineCallback& operator= (EngineCallback const&) = delete;

        virtual ~EngineCallback() = default;

        STDMETHOD_(void, OnProcessingPassStart) () override {}
        STDMETHOD_(void, OnProcessingPassEnd)() override {}

        STDMETHOD_(void, OnCriticalError) (THIS_ HRESULT error)
        {
        #ifndef _DEBUG
            UNREFERENCED_PARAMETER(error);
        #endif
            DebugTrace("ERROR: AudioEngine encountered critical error (%08X)\n", static_cast<unsigned int>(error));
            SetEvent(mCriticalError.get());
        }

        ScopedHandle mCriticalError;
    };

    struct VoiceCallback : public IXAudio2VoiceCallback
    {
        VoiceCallback() noexcept(false)
        {
            mBufferEnd.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
            if (!mBufferEnd)
            {
                throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
            }
        }

        VoiceCallback(VoiceCallback&&) = default;
        VoiceCallback& operator=(VoiceCallback&&) = default;

        VoiceCallback(const VoiceCallback&) = delete;
        VoiceCallback& operator=(const VoiceCallback&) = delete;

        virtual ~VoiceCallback()
        {
        }

        STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32) override {}
        STDMETHOD_(void, OnVoiceProcessingPassEnd)() override {}
        STDMETHOD_(void, OnStreamEnd)() override {}
        STDMETHOD_(void, OnBufferStart)(void*) override {}

        STDMETHOD_(void, OnBufferEnd)(void* context) override
        {
            if (context)
            {
                auto inotify = static_cast<IVoiceNotify*>(context);
                inotify->OnBufferEnd();
                SetEvent(mBufferEnd.get());
            }
        }

        STDMETHOD_(void, OnLoopEnd)(void*) override {}
        STDMETHOD_(void, OnVoiceError)(void*, HRESULT) override {}

        ScopedHandle mBufferEnd;
    };

    static const XAUDIO2FX_REVERB_I3DL2_PARAMETERS gReverbPresets[] =
    {
        XAUDIO2FX_I3DL2_PRESET_DEFAULT,             // Reverb_Off
        XAUDIO2FX_I3DL2_PRESET_DEFAULT,             // Reverb_Default
        XAUDIO2FX_I3DL2_PRESET_GENERIC,             // Reverb_Generic
        XAUDIO2FX_I3DL2_PRESET_FOREST,              // Reverb_Forest
        XAUDIO2FX_I3DL2_PRESET_PADDEDCELL,          // Reverb_PaddedCell
        XAUDIO2FX_I3DL2_PRESET_ROOM,                // Reverb_Room
        XAUDIO2FX_I3DL2_PRESET_BATHROOM,            // Reverb_Bathroom
        XAUDIO2FX_I3DL2_PRESET_LIVINGROOM,          // Reverb_LivingRoom
        XAUDIO2FX_I3DL2_PRESET_STONEROOM,           // Reverb_StoneRoom
        XAUDIO2FX_I3DL2_PRESET_AUDITORIUM,          // Reverb_Auditorium
        XAUDIO2FX_I3DL2_PRESET_CONCERTHALL,         // Reverb_ConcertHall
        XAUDIO2FX_I3DL2_PRESET_CAVE,                // Reverb_Cave
        XAUDIO2FX_I3DL2_PRESET_ARENA,               // Reverb_Arena
        XAUDIO2FX_I3DL2_PRESET_HANGAR,              // Reverb_Hangar
        XAUDIO2FX_I3DL2_PRESET_CARPETEDHALLWAY,     // Reverb_CarpetedHallway
        XAUDIO2FX_I3DL2_PRESET_HALLWAY,             // Reverb_Hallway
        XAUDIO2FX_I3DL2_PRESET_STONECORRIDOR,       // Reverb_StoneCorridor
        XAUDIO2FX_I3DL2_PRESET_ALLEY,               // Reverb_Alley
        XAUDIO2FX_I3DL2_PRESET_CITY,                // Reverb_City
        XAUDIO2FX_I3DL2_PRESET_MOUNTAINS,           // Reverb_Mountains
        XAUDIO2FX_I3DL2_PRESET_QUARRY,              // Reverb_Quarry
        XAUDIO2FX_I3DL2_PRESET_PLAIN,               // Reverb_Plain
        XAUDIO2FX_I3DL2_PRESET_PARKINGLOT,          // Reverb_ParkingLot
        XAUDIO2FX_I3DL2_PRESET_SEWERPIPE,           // Reverb_SewerPipe
        XAUDIO2FX_I3DL2_PRESET_UNDERWATER,          // Reverb_Underwater
        XAUDIO2FX_I3DL2_PRESET_SMALLROOM,           // Reverb_SmallRoom
        XAUDIO2FX_I3DL2_PRESET_MEDIUMROOM,          // Reverb_MediumRoom
        XAUDIO2FX_I3DL2_PRESET_LARGEROOM,           // Reverb_LargeRoom
        XAUDIO2FX_I3DL2_PRESET_MEDIUMHALL,          // Reverb_MediumHall
        XAUDIO2FX_I3DL2_PRESET_LARGEHALL,           // Reverb_LargeHall
        XAUDIO2FX_I3DL2_PRESET_PLATE,               // Reverb_Plate
    };

    inline unsigned int makeVoiceKey(_In_ const WAVEFORMATEX* wfx) noexcept
    {
        assert(IsValid(wfx));

        if (wfx->nChannels > 0x7F)
            return 0;

        // This hash does not use nSamplesPerSec because voice reuse can change the source sample rate.

        // nAvgBytesPerSec and nBlockAlign are derived from other values in XAudio2 supported formats.

        union KeyGen
        {
            struct
            {
                unsigned int tag : 9;
                unsigned int channels : 7;
                unsigned int bitsPerSample : 8;
            } pcm;

            struct
            {
                unsigned int tag : 9;
                unsigned int channels : 7;
                unsigned int samplesPerBlock : 16;
            } adpcm;

        #ifdef DIRECTX_ENABLE_XMA2
            struct
            {
                unsigned int tag : 9;
                unsigned int channels : 7;
                unsigned int encoderVersion : 8;
            } xma;
        #endif

            unsigned int key;
        } result;

        static_assert(sizeof(KeyGen) == sizeof(unsigned int), "KeyGen is invalid");

        result.key = 0;

        if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            // We reuse EXTENSIBLE only if it is equivalent to the standard form
            auto wfex = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wfx);
            if (wfex->Samples.wValidBitsPerSample != 0 && wfex->Samples.wValidBitsPerSample != wfx->wBitsPerSample)
                return 0;

            if (wfex->dwChannelMask != 0 && wfex->dwChannelMask != GetDefaultChannelMask(wfx->nChannels))
                return 0;
        }

        uint32_t tag = GetFormatTag(wfx);
        switch (tag)
        {
            case WAVE_FORMAT_PCM:
                static_assert(WAVE_FORMAT_PCM < 0x1ff, "KeyGen tag is too small");
                result.pcm.tag = WAVE_FORMAT_PCM;
                result.pcm.channels = wfx->nChannels;
                result.pcm.bitsPerSample = wfx->wBitsPerSample;
                break;

            case WAVE_FORMAT_IEEE_FLOAT:
                static_assert(WAVE_FORMAT_IEEE_FLOAT < 0x1ff, "KeyGen tag is too small");

                if (wfx->wBitsPerSample != 32)
                    return 0;

                result.pcm.tag = WAVE_FORMAT_IEEE_FLOAT;
                result.pcm.channels = wfx->nChannels;
                result.pcm.bitsPerSample = 32;
                break;

            case WAVE_FORMAT_ADPCM:
                static_assert(WAVE_FORMAT_ADPCM < 0x1ff, "KeyGen tag is too small");
                result.adpcm.tag = WAVE_FORMAT_ADPCM;
                result.adpcm.channels = wfx->nChannels;

                {
                    auto wfadpcm = reinterpret_cast<const ADPCMWAVEFORMAT*>(wfx);
                    result.adpcm.samplesPerBlock = wfadpcm->wSamplesPerBlock;
                }
                break;

            #ifdef DIRECTX_ENABLE_XMA2
            case WAVE_FORMAT_XMA2:
                static_assert(WAVE_FORMAT_XMA2 < 0x1ff, "KeyGen tag is too small");
                result.xma.tag = WAVE_FORMAT_XMA2;
                result.xma.channels = wfx->nChannels;

                {
                    auto xmaFmt = reinterpret_cast<const XMA2WAVEFORMATEX*>(wfx);

                    if ((xmaFmt->LoopBegin > 0)
                        || (xmaFmt->PlayBegin > 0))
                        return 0;

                    result.xma.encoderVersion = xmaFmt->EncoderVersion;
                }
                break;
            #endif

            default:
                return 0;
        }

        return result.key;
    }
}

static_assert(static_cast<unsigned int>(std::size(gReverbPresets)) == Reverb_MAX, "AUDIO_ENGINE_REVERB enum mismatch");


//======================================================================================
// AudioEngine
//======================================================================================

#define SAFE_DESTROY_VOICE(voice) if ( voice ) { voice->DestroyVoice(); voice = nullptr; }

// Internal object implementation class.
class AudioEngine::Impl
{
public:
    Impl() noexcept :
        mMasterVoice(nullptr),
        mReverbVoice(nullptr),
        masterChannelMask(0),
        masterChannels(0),
        masterRate(0),
        defaultRate(44100),
        maxVoiceOneshots(SIZE_MAX),
        maxVoiceInstances(SIZE_MAX),
        mMasterVolume(1.f),
        mX3DAudio{},
        mCriticalError(false),
        mReverbEnabled(false),
        mEngineFlags(AudioEngine_Default),
        mCategory(AudioCategory_GameEffects),
        mVoiceInstances(0)
    {
    }

    ~Impl() = default;

    Impl(Impl&&) = default;
    Impl& operator= (Impl&&) = default;

    Impl(Impl const&) = delete;
    Impl& operator= (Impl const&) = delete;

    HRESULT Initialize(AUDIO_ENGINE_FLAGS flags,
        _In_opt_ const WAVEFORMATEX* wfx,
        _In_opt_z_ const wchar_t* deviceId,
        AUDIO_STREAM_CATEGORY category);

    HRESULT Reset(_In_opt_ const WAVEFORMATEX* wfx, _In_opt_z_ const wchar_t* deviceId);

    void SetSilentMode();

    void Shutdown() noexcept;

    bool Update();

    void SetReverb(_In_opt_ const XAUDIO2FX_REVERB_PARAMETERS* native) noexcept;

    void SetMasteringLimit(int release, int loudness);

    AudioStatistics GetStatistics() const;

    void TrimVoicePool();

    void AllocateVoice(_In_ const WAVEFORMATEX* wfx,
        SOUND_EFFECT_INSTANCE_FLAGS flags, bool oneshot,
        _Outptr_result_maybenull_ IXAudio2SourceVoice** voice);
    void DestroyVoice(_In_ IXAudio2SourceVoice* voice) noexcept;

    void RegisterNotify(_In_ IVoiceNotify* notify, bool usesUpdate);
    void UnregisterNotify(_In_ IVoiceNotify* notify, bool oneshots, bool usesUpdate);

    ComPtr<IXAudio2>                    xaudio2;
    IXAudio2MasteringVoice*             mMasterVoice;
    IXAudio2SubmixVoice*                mReverbVoice;

    uint32_t                            masterChannelMask;
    uint32_t                            masterChannels;
    uint32_t                            masterRate;

    int                                 defaultRate;
    size_t                              maxVoiceOneshots;
    size_t                              maxVoiceInstances;
    float                               mMasterVolume;

    X3DAUDIO_HANDLE                     mX3DAudio;

    bool                                mCriticalError;
    bool                                mReverbEnabled;

    AUDIO_ENGINE_FLAGS                  mEngineFlags;

private:
    using notifylist_t = std::set<IVoiceNotify*>;
    using oneshotlist_t = std::list<std::pair<unsigned int, IXAudio2SourceVoice*>>;
    using voicepool_t = std::unordered_multimap<unsigned int, IXAudio2SourceVoice*>;

    AUDIO_STREAM_CATEGORY               mCategory;
    ComPtr<IUnknown>                    mReverbEffect;
    ComPtr<IUnknown>                    mVolumeLimiter;
    oneshotlist_t                       mOneShots;
    voicepool_t                         mVoicePool;
    notifylist_t                        mNotifyObjects;
    notifylist_t                        mNotifyUpdates;
    size_t                              mVoiceInstances;
    VoiceCallback                       mVoiceCallback;
    EngineCallback                      mEngineCallback;
};


_Use_decl_annotations_
HRESULT AudioEngine::Impl::Initialize(
    AUDIO_ENGINE_FLAGS flags,
    const WAVEFORMATEX* wfx,
    const wchar_t* deviceId,
    AUDIO_STREAM_CATEGORY category)
{
    mEngineFlags = flags;
    mCategory = category;

    return Reset(wfx, deviceId);
}


_Use_decl_annotations_
HRESULT AudioEngine::Impl::Reset(const WAVEFORMATEX* wfx, const wchar_t* deviceId)
{
    if (wfx)
    {
        if (wfx->wFormatTag != WAVE_FORMAT_PCM)
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

        if (!wfx->nChannels || wfx->nChannels > XAUDIO2_MAX_AUDIO_CHANNELS)
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

        if (wfx->nSamplesPerSec < XAUDIO2_MIN_SAMPLE_RATE || wfx->nSamplesPerSec > XAUDIO2_MAX_SAMPLE_RATE)
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

        // We don't use other data members of WAVEFORMATEX here to describe the device format, so no need to fully validate
    }

    assert(!xaudio2);
    assert(mMasterVoice == nullptr);
    assert(mReverbVoice == nullptr);

    masterChannelMask = masterChannels = masterRate = 0;

    memset(&mX3DAudio, 0, X3DAUDIO_HANDLE_BYTESIZE);

    mCriticalError = false;
    mReverbEnabled = false;

    //
    // Create XAudio2 engine
    //
    HRESULT hr = XAudio2Create(xaudio2.ReleaseAndGetAddressOf(), 0u);
    if (FAILED(hr))
        return hr;

    if (mEngineFlags & AudioEngine_Debug)
    {
        XAUDIO2_DEBUG_CONFIGURATION debug = {};
        debug.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
        debug.BreakMask = XAUDIO2_LOG_ERRORS;
        xaudio2->SetDebugConfiguration(&debug, nullptr);
    #ifdef USING_XAUDIO2_9
        DebugTrace("INFO: XAudio 2.9 debugging enabled\n");
    #else // USING_XAUDIO2_8
            // To see the trace output, you need to view ETW logs for this application:
            //    Go to Control Panel, Administrative Tools, Event Viewer.
            //    View->Show Analytic and Debug Logs.
            //    Applications and Services Logs / Microsoft / Windows / XAudio2. 
            //    Right click on Microsoft Windows XAudio2 debug logging, Properties, then Enable Logging, and hit OK 
        DebugTrace("INFO: XAudio 2.8 debugging enabled\n");
    #endif
    }

    if (mEngineFlags & AudioEngine_DisableVoiceReuse)
    {
        DebugTrace("INFO: Voice reuse is disabled\n");
    }

    hr = xaudio2->RegisterForCallbacks(&mEngineCallback);
    if (FAILED(hr))
    {
        xaudio2.Reset();
        return hr;
    }

    //
    // Create mastering voice for device
    //
    hr = xaudio2->CreateMasteringVoice(&mMasterVoice,
        (wfx) ? wfx->nChannels : 0u /*XAUDIO2_DEFAULT_CHANNELS */,
        (wfx) ? wfx->nSamplesPerSec : 0u /* XAUDIO2_DEFAULT_SAMPLERATE */,
        0u, deviceId, nullptr, mCategory);
    if (FAILED(hr))
    {
        xaudio2.Reset();
        return hr;
    }

    DWORD dwChannelMask;
    hr = mMasterVoice->GetChannelMask(&dwChannelMask);
    if (FAILED(hr))
    {
        SAFE_DESTROY_VOICE(mMasterVoice)
        xaudio2.Reset();
        return hr;
    }

    XAUDIO2_VOICE_DETAILS details;
    mMasterVoice->GetVoiceDetails(&details);

    masterChannelMask = dwChannelMask;
    masterChannels = details.InputChannels;
    masterRate = details.InputSampleRate;

    DebugTrace("INFO: mastering voice has %u channels, %u sample rate, %08X channel mask\n",
        masterChannels, masterRate, masterChannelMask);

    if (mMasterVolume != 1.f)
    {
        hr = mMasterVoice->SetVolume(mMasterVolume);
        if (FAILED(hr))
        {
            SAFE_DESTROY_VOICE(mMasterVoice)
            xaudio2.Reset();
            return hr;
        }
    }

    //
    // Setup mastering volume limiter (optional)
    //
    if (mEngineFlags & AudioEngine_UseMasteringLimiter)
    {
        FXMASTERINGLIMITER_PARAMETERS params = {};
        params.Release = FXMASTERINGLIMITER_DEFAULT_RELEASE;
        params.Loudness = FXMASTERINGLIMITER_DEFAULT_LOUDNESS;

        hr = CreateFX(__uuidof(FXMasteringLimiter), mVolumeLimiter.ReleaseAndGetAddressOf(), &params, sizeof(params));
        if (FAILED(hr))
        {
            SAFE_DESTROY_VOICE(mMasterVoice)
            xaudio2.Reset();
            return hr;
        }

        XAUDIO2_EFFECT_DESCRIPTOR desc = {};
        desc.InitialState = TRUE;
        desc.OutputChannels = masterChannels;
        desc.pEffect = mVolumeLimiter.Get();

        XAUDIO2_EFFECT_CHAIN chain = { 1, &desc };
        hr = mMasterVoice->SetEffectChain(&chain);
        if (FAILED(hr))
        {
            SAFE_DESTROY_VOICE(mMasterVoice)
            mVolumeLimiter.Reset();
            xaudio2.Reset();
            return hr;
        }

        DebugTrace("INFO: Mastering volume limiter enabled\n");
    }

    //
    // Setup environmental reverb for 3D audio (optional)
    //
    if (mEngineFlags & AudioEngine_EnvironmentalReverb)
    {
        hr = XAudio2CreateReverb(mReverbEffect.ReleaseAndGetAddressOf(), 0u);
        if (FAILED(hr))
        {
            SAFE_DESTROY_VOICE(mMasterVoice)
            mVolumeLimiter.Reset();
            xaudio2.Reset();
            return hr;
        }

        XAUDIO2_EFFECT_DESCRIPTOR effects[] = { { mReverbEffect.Get(), TRUE, 1 } };
        XAUDIO2_EFFECT_CHAIN effectChain = { 1, effects };

        mReverbEnabled = true;

        hr = xaudio2->CreateSubmixVoice(&mReverbVoice, 1, masterRate,
            (mEngineFlags & AudioEngine_ReverbUseFilters) ? XAUDIO2_VOICE_USEFILTER : 0u, 0u,
            nullptr, &effectChain);
        if (FAILED(hr))
        {
            SAFE_DESTROY_VOICE(mMasterVoice)
            mReverbEffect.Reset();
            mVolumeLimiter.Reset();
            xaudio2.Reset();
            return hr;
        }

        XAUDIO2FX_REVERB_PARAMETERS native;
        ReverbConvertI3DL2ToNative(&gReverbPresets[Reverb_Default], &native);
        hr = mReverbVoice->SetEffectParameters(0, &native, sizeof(XAUDIO2FX_REVERB_PARAMETERS));
        if (FAILED(hr))
        {
            SAFE_DESTROY_VOICE(mReverbVoice)
            SAFE_DESTROY_VOICE(mMasterVoice)
            mReverbEffect.Reset();
            mVolumeLimiter.Reset();
            xaudio2.Reset();
            return hr;
        }

        DebugTrace("INFO: I3DL2 reverb effect enabled for 3D positional audio\n");
    }

    //
    // Setup 3D audio
    //
    constexpr float SPEEDOFSOUND = X3DAUDIO_SPEED_OF_SOUND;

    hr = X3DAudioInitialize(masterChannelMask, SPEEDOFSOUND, mX3DAudio);
    if (FAILED(hr))
    {
        SAFE_DESTROY_VOICE(mReverbVoice)
        SAFE_DESTROY_VOICE(mMasterVoice)
        mReverbEffect.Reset();
        mVolumeLimiter.Reset();
        xaudio2.Reset();
        return hr;
    }

    //
    // Inform any notify objects we are ready to go again
    //
    for (auto it = mNotifyObjects.begin(); it != mNotifyObjects.end(); ++it)
    {
        assert(*it != nullptr);
        (*it)->OnReset();
    }

    return S_OK;
}


void AudioEngine::Impl::SetSilentMode()
{
    for (auto it = mNotifyObjects.begin(); it != mNotifyObjects.end(); ++it)
    {
        assert(*it != nullptr);
        (*it)->OnCriticalError();
    }

    for (auto it = mOneShots.begin(); it != mOneShots.end(); ++it)
    {
        assert(it->second != nullptr);
        it->second->DestroyVoice();
    }
    mOneShots.clear();

    for (auto it = mVoicePool.begin(); it != mVoicePool.end(); ++it)
    {
        assert(it->second != nullptr);
        it->second->DestroyVoice();
    }
    mVoicePool.clear();

    mVoiceInstances = 0;

    SAFE_DESTROY_VOICE(mReverbVoice)
    SAFE_DESTROY_VOICE(mMasterVoice)

    mReverbEffect.Reset();
    mVolumeLimiter.Reset();
    xaudio2.Reset();
}


void AudioEngine::Impl::Shutdown() noexcept
{
    for (auto it = mNotifyObjects.begin(); it != mNotifyObjects.end(); ++it)
    {
        assert(*it != nullptr);
        (*it)->OnDestroyEngine();
    }

    if (xaudio2)
    {
        xaudio2->UnregisterForCallbacks(&mEngineCallback);

        xaudio2->StopEngine();

        for (auto it = mOneShots.begin(); it != mOneShots.end(); ++it)
        {
            assert(it->second != nullptr);
            it->second->DestroyVoice();
        }
        mOneShots.clear();

        for (auto it = mVoicePool.begin(); it != mVoicePool.end(); ++it)
        {
            assert(it->second != nullptr);
            it->second->DestroyVoice();
        }
        mVoicePool.clear();

        mVoiceInstances = 0;

        SAFE_DESTROY_VOICE(mReverbVoice)
        SAFE_DESTROY_VOICE(mMasterVoice)

        mReverbEffect.Reset();
        mVolumeLimiter.Reset();
        xaudio2.Reset();

        masterChannelMask = masterChannels = masterRate = 0;

        mCriticalError = false;
        mReverbEnabled = false;

        memset(&mX3DAudio, 0, X3DAUDIO_HANDLE_BYTESIZE);
    }
}


bool AudioEngine::Impl::Update()
{
    if (!xaudio2)
        return false;

    HANDLE events[2] = { mEngineCallback.mCriticalError.get(), mVoiceCallback.mBufferEnd.get() };
    switch (WaitForMultipleObjectsEx(static_cast<DWORD>(std::size(events)), events, FALSE, 0, FALSE))
    {
        default:
        case WAIT_TIMEOUT:
            break;

        case WAIT_OBJECT_0:     // OnCritialError
            mCriticalError = true;

            SetSilentMode();
            return false;

        case WAIT_OBJECT_0 + 1: // OnBufferEnd
            // Scan for completed one-shot voices
            for (auto it = mOneShots.begin(); it != mOneShots.end(); )
            {
                assert(it->second != nullptr);

                XAUDIO2_VOICE_STATE xstate;
                it->second->GetState(&xstate, XAUDIO2_VOICE_NOSAMPLESPLAYED);

                if (!xstate.BuffersQueued)
                {
                    (void)it->second->Stop(0);
                    if (it->first)
                    {
                        // Put voice back into voice pool for reuse since it has a non-zero voiceKey
                    #ifdef VERBOSE_TRACE
                        DebugTrace("INFO: One-shot voice being saved for reuse (%08X)\n", it->first);
                    #endif
                        voicepool_t::value_type v(it->first, it->second);
                        mVoicePool.emplace(v);
                    }
                    else
                    {
                        // Voice is to be destroyed rather than reused
                    #ifdef VERBOSE_TRACE
                        DebugTrace("INFO: Destroying one-shot voice\n");
                    #endif
                        it->second->DestroyVoice();
                    }
                    it = mOneShots.erase(it);
                }
                else
                    ++it;
            }
            break;

        case WAIT_FAILED:
            throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "WaitForMultipleObjectsEx");
    }

    //
    // Inform any notify objects of updates
    //
    for (auto it = mNotifyUpdates.begin(); it != mNotifyUpdates.end(); ++it)
    {
        assert(*it != nullptr);
        (*it)->OnUpdate();
    }

    return true;
}


_Use_decl_annotations_
void AudioEngine::Impl::SetReverb(const XAUDIO2FX_REVERB_PARAMETERS* native) noexcept
{
    if (!mReverbVoice)
        return;

    if (native)
    {
        if (!mReverbEnabled)
        {
            mReverbEnabled = true;
            (void)mReverbVoice->EnableEffect(0);
        }

        (void)mReverbVoice->SetEffectParameters(0, native, sizeof(XAUDIO2FX_REVERB_PARAMETERS));
    }
    else if (mReverbEnabled)
    {
        mReverbEnabled = false;
        (void)mReverbVoice->DisableEffect(0);
    }
}


void AudioEngine::Impl::SetMasteringLimit(int release, int loudness)
{
    if (!mVolumeLimiter || !mMasterVoice)
        return;

    if ((release < FXMASTERINGLIMITER_MIN_RELEASE) || (release > FXMASTERINGLIMITER_MAX_RELEASE))
        throw std::out_of_range("AudioEngine::SetMasteringLimit");

    if ((loudness < FXMASTERINGLIMITER_MIN_LOUDNESS) || (loudness > FXMASTERINGLIMITER_MAX_LOUDNESS))
        throw std::out_of_range("AudioEngine::SetMasteringLimit");

    FXMASTERINGLIMITER_PARAMETERS params = {};
    params.Release = static_cast<UINT32>(release);
    params.Loudness = static_cast<UINT32>(loudness);

    HRESULT hr = mMasterVoice->SetEffectParameters(0, &params, sizeof(params));
    ThrowIfFailed(hr);
}


AudioStatistics AudioEngine::Impl::GetStatistics() const
{
    AudioStatistics stats = {};

    stats.allocatedVoices = stats.allocatedVoicesOneShot = mOneShots.size() + mVoicePool.size();
    stats.allocatedVoicesIdle = mVoicePool.size();

    for (auto it = mNotifyObjects.begin(); it != mNotifyObjects.end(); ++it)
    {
        assert(*it != nullptr);
        (*it)->GatherStatistics(stats);
    }

    assert(stats.allocatedVoices == (mOneShots.size() + mVoicePool.size() + mVoiceInstances));

    return stats;
}


void AudioEngine::Impl::TrimVoicePool()
{
    for (auto it = mNotifyObjects.begin(); it != mNotifyObjects.end(); ++it)
    {
        assert(*it != nullptr);
        (*it)->OnTrim();
    }

    for (auto it = mVoicePool.begin(); it != mVoicePool.end(); ++it)
    {
        assert(it->second != nullptr);
        it->second->DestroyVoice();
    }
    mVoicePool.clear();
}


_Use_decl_annotations_
void AudioEngine::Impl::AllocateVoice(
    const WAVEFORMATEX* wfx,
    SOUND_EFFECT_INSTANCE_FLAGS flags,
    bool oneshot,
    IXAudio2SourceVoice** voice)
{
    if (!wfx)
        throw std::invalid_argument("Wave format is required\n");

    // No need to call IsValid on wfx because CreateSourceVoice will do that

    if (!voice)
        throw std::invalid_argument("Voice pointer must be non-null");

    *voice = nullptr;

    if (!xaudio2 || mCriticalError)
        return;

#ifndef NDEBUG
    const float maxFrequencyRatio = XAudio2SemitonesToFrequencyRatio(12);
    assert(maxFrequencyRatio <= XAUDIO2_DEFAULT_FREQ_RATIO);
#endif

    unsigned int voiceKey = 0;
    if (oneshot)
    {
        if (flags & (SoundEffectInstance_Use3D | SoundEffectInstance_ReverbUseFilters | SoundEffectInstance_NoSetPitch))
        {
            DebugTrace((flags & SoundEffectInstance_NoSetPitch)
                       ? "ERROR: One-shot voices must support pitch-shifting for voice reuse\n"
                       : "ERROR: One-use voices cannot use 3D positional audio\n");
            throw std::invalid_argument("Invalid flags for one-shot voice");
        }

    #ifdef VERBOSE_TRACE
        if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            DebugTrace("INFO: Requesting one-shot: Format Tag EXTENSIBLE %u, %u channels, %u-bit, %u blkalign, %u Hz\n",
                GetFormatTag(wfx), wfx->nChannels, wfx->wBitsPerSample, wfx->nBlockAlign, wfx->nSamplesPerSec);
        }
        else
        {
            DebugTrace("INFO: Requesting one-shot: Format Tag %u, %u channels, %u-bit, %u blkalign, %u Hz\n",
                wfx->wFormatTag, wfx->nChannels, wfx->wBitsPerSample, wfx->nBlockAlign, wfx->nSamplesPerSec);
        }
    #endif

        if (!(mEngineFlags & AudioEngine_DisableVoiceReuse))
        {
            voiceKey = makeVoiceKey(wfx);
            if (voiceKey != 0)
            {
                auto it = mVoicePool.find(voiceKey);
                if (it != mVoicePool.end())
                {
                    // Found a matching (stopped) voice to reuse
                    assert(it->second != nullptr);
                    *voice = it->second;
                    mVoicePool.erase(it);

                    // Reset any volume/pitch-shifting
                    HRESULT hr = (*voice)->SetVolume(1.f);
                    ThrowIfFailed(hr);

                    hr = (*voice)->SetFrequencyRatio(1.f);
                    ThrowIfFailed(hr);

                    if (wfx->nChannels == 1 || wfx->nChannels == 2)
                    {
                        // Reset any panning
                        float matrix[16] = {};
                        ComputePan(0.f, wfx->nChannels, matrix);

                        hr = (*voice)->SetOutputMatrix(nullptr, wfx->nChannels, masterChannels, matrix);
                        ThrowIfFailed(hr);
                    }
                }
                else if ((mVoicePool.size() + mOneShots.size() + 1) >= maxVoiceOneshots)
                {
                    DebugTrace("WARNING: Too many one-shot voices in use (%zu + %zu >= %zu); one-shot not played\n",
                               mVoicePool.size(), mOneShots.size() + 1, maxVoiceOneshots);
                    return;
                }
                else
                {
                    // makeVoiceKey already constrained the supported wfx formats to those supported for reuse

                    char buff[64] = {};
                    auto wfmt = reinterpret_cast<WAVEFORMATEX*>(buff);

                    uint32_t tag = GetFormatTag(wfx);
                    switch (tag)
                    {
                        case WAVE_FORMAT_PCM:
                            CreateIntegerPCM(wfmt, defaultRate, wfx->nChannels, wfx->wBitsPerSample);
                            break;

                        case WAVE_FORMAT_IEEE_FLOAT:
                            CreateFloatPCM(wfmt, defaultRate, wfx->nChannels);
                            break;

                        case WAVE_FORMAT_ADPCM:
                        {
                            auto wfadpcm = reinterpret_cast<const ADPCMWAVEFORMAT*>(wfx);
                            CreateADPCM(wfmt, sizeof(buff), defaultRate, wfx->nChannels, wfadpcm->wSamplesPerBlock);
                        }
                        break;

                        #ifdef DIRECTX_ENABLE_XMA2
                        case WAVE_FORMAT_XMA2:
                            CreateXMA2(wfmt, sizeof(buff), defaultRate, wfx->nChannels, 65536, 2, 0);
                            break;
                        #endif
                    }

                #ifdef VERBOSE_TRACE
                    DebugTrace("INFO: Allocate reuse voice: Format Tag %u, %u channels, %u-bit, %u blkalign, %u Hz\n",
                        wfmt->wFormatTag, wfmt->nChannels, wfmt->wBitsPerSample, wfmt->nBlockAlign, wfmt->nSamplesPerSec);
                #endif

                    assert(voiceKey == makeVoiceKey(wfmt));

                    HRESULT hr = xaudio2->CreateSourceVoice(voice, wfmt, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &mVoiceCallback, nullptr, nullptr);
                    if (FAILED(hr))
                    {
                        DebugTrace("ERROR: CreateSourceVoice (reuse) failed with error %08X\n", static_cast<unsigned int>(hr));
                        throw std::runtime_error("CreateSourceVoice");
                    }
                }

                assert(*voice != nullptr);
                HRESULT hr = (*voice)->SetSourceSampleRate(wfx->nSamplesPerSec);
                if (FAILED(hr))
                {
                    DebugTrace("ERROR: SetSourceSampleRate failed with error %08X\n", static_cast<unsigned int>(hr));
                    throw std::runtime_error("SetSourceSampleRate");
                }
            }
        }
    }

    if (!*voice)
    {
        if (oneshot)
        {
            if ((mVoicePool.size() + mOneShots.size() + 1) >= maxVoiceOneshots)
            {
                DebugTrace("WARNING: Too many one-shot voices in use (%zu + %zu >= %zu); one-shot not played; see TrimVoicePool\n",
                           mVoicePool.size(), mOneShots.size() + 1, maxVoiceOneshots);
                return;
            }
        }
        else if ((mVoiceInstances + 1) >= maxVoiceInstances)
        {
            DebugTrace("ERROR: Too many instance voices (%zu >= %zu); see TrimVoicePool\n",
                mVoiceInstances + 1, maxVoiceInstances);
            throw std::runtime_error("Too many instance voices");
        }

        UINT32 vflags = (flags & SoundEffectInstance_NoSetPitch) ? XAUDIO2_VOICE_NOPITCH : 0u;

        HRESULT hr;
        if (flags & SoundEffectInstance_Use3D)
        {
            XAUDIO2_SEND_DESCRIPTOR sendDescriptors[2] = {};
            sendDescriptors[0].Flags = sendDescriptors[1].Flags = (flags & SoundEffectInstance_ReverbUseFilters)
                ? XAUDIO2_SEND_USEFILTER : 0u;
            sendDescriptors[0].pOutputVoice = mMasterVoice;
            sendDescriptors[1].pOutputVoice = mReverbVoice;
            const XAUDIO2_VOICE_SENDS sendList = { mReverbVoice ? 2U : 1U, sendDescriptors };

        #ifdef VERBOSE_TRACE
            DebugTrace("INFO: Allocate voice 3D: Format Tag %u, %u channels, %u-bit, %u blkalign, %u Hz\n",
                wfx->wFormatTag, wfx->nChannels, wfx->wBitsPerSample, wfx->nBlockAlign, wfx->nSamplesPerSec);
        #endif

            hr = xaudio2->CreateSourceVoice(voice, wfx, vflags, XAUDIO2_DEFAULT_FREQ_RATIO, &mVoiceCallback, &sendList, nullptr);
        }
        else
        {
        #ifdef VERBOSE_TRACE
            DebugTrace("INFO: Allocate voice: Format Tag %u, %u channels, %u-bit, %u blkalign, %u Hz\n",
                wfx->wFormatTag, wfx->nChannels, wfx->wBitsPerSample, wfx->nBlockAlign, wfx->nSamplesPerSec);
        #endif

            hr = xaudio2->CreateSourceVoice(voice, wfx, vflags, XAUDIO2_DEFAULT_FREQ_RATIO, &mVoiceCallback, nullptr, nullptr);
        }

        if (FAILED(hr))
        {
            DebugTrace("ERROR: CreateSourceVoice failed with error %08X\n", static_cast<unsigned int>(hr));
            throw std::runtime_error("CreateSourceVoice");
        }
        else if (!oneshot)
        {
            ++mVoiceInstances;
        }
    }

    if (oneshot)
    {
        assert(*voice != nullptr);
        mOneShots.emplace_back(std::make_pair(voiceKey, *voice));
    }
}


void AudioEngine::Impl::DestroyVoice(_In_ IXAudio2SourceVoice* voice) noexcept
{
    if (!voice)
        return;

#ifndef NDEBUG
    for (auto it = mOneShots.cbegin(); it != mOneShots.cend(); ++it)
    {
        if (it->second == voice)
        {
            DebugTrace("ERROR: DestroyVoice should not be called for a one-shot voice\n");
            return;
        }
    }

    for (auto it = mVoicePool.cbegin(); it != mVoicePool.cend(); ++it)
    {
        if (it->second == voice)
        {
            DebugTrace("ERROR: DestroyVoice should not be called for a one-shot voice; see TrimVoicePool\n");
            return;
        }
    }
#endif

    assert(mVoiceInstances > 0);
    --mVoiceInstances;
    voice->DestroyVoice();
}


void AudioEngine::Impl::RegisterNotify(_In_ IVoiceNotify* notify, bool usesUpdate)
{
    assert(notify != nullptr);
    mNotifyObjects.insert(notify);

    if (usesUpdate)
    {
        mNotifyUpdates.insert(notify);
    }
}


void AudioEngine::Impl::UnregisterNotify(_In_ IVoiceNotify* notify, bool usesOneShots, bool usesUpdate)
{
    assert(notify != nullptr);
    mNotifyObjects.erase(notify);

    // Check for any pending one-shots for this notification object
    if (usesOneShots)
    {
        bool setevent = false;

        for (auto it = mOneShots.begin(); it != mOneShots.end(); ++it)
        {
            assert(it->second != nullptr);

            XAUDIO2_VOICE_STATE state;
            it->second->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

            if (state.pCurrentBufferContext == notify)
            {
                (void)it->second->Stop(0);
                (void)it->second->FlushSourceBuffers();
                setevent = true;
            }
        }

        if (setevent)
        {
            // Trigger scan on next call to Update...
            SetEvent(mVoiceCallback.mBufferEnd.get());
        }
    }

    if (usesUpdate)
    {
        mNotifyUpdates.erase(notify);
    }
}


//--------------------------------------------------------------------------------------
// AudioEngine
//--------------------------------------------------------------------------------------

// Public constructor.
_Use_decl_annotations_
AudioEngine::AudioEngine(
    AUDIO_ENGINE_FLAGS flags,
    const WAVEFORMATEX* wfx,
    const wchar_t* deviceId,
    AUDIO_STREAM_CATEGORY category) noexcept(false)
    : pImpl(std::make_unique<Impl>())
{
    HRESULT hr = pImpl->Initialize(flags, wfx, deviceId, category);
    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        {
            if (flags & AudioEngine_ThrowOnNoAudioHW)
            {
                DebugTrace("ERROR: AudioEngine found no default audio device\n");
                throw std::runtime_error("AudioEngineNoAudioHW");
            }
            else
            {
                DebugTrace("WARNING: AudioEngine found no default audio device; running in 'silent mode'\n");
            }
        }
        else
        {
            DebugTrace("ERROR: AudioEngine failed (%08X) to initialize using device [%ls]\n",
                static_cast<unsigned int>(hr),
                (deviceId) ? deviceId : L"default");
            throw std::runtime_error("AudioEngine");
        }
    }
}


// Move constructor.
AudioEngine::AudioEngine(AudioEngine&& moveFrom) noexcept
    : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
AudioEngine& AudioEngine::operator= (AudioEngine&& moveFrom) noexcept
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
AudioEngine::~AudioEngine()
{
    if (pImpl)
    {
        pImpl->Shutdown();
    }
}


// Public methods.
bool AudioEngine::Update()
{
    return pImpl->Update();
}


_Use_decl_annotations_
bool AudioEngine::Reset(const WAVEFORMATEX* wfx, const wchar_t* deviceId)
{
    if (pImpl->xaudio2)
    {
        DebugTrace("WARNING: Called Reset for active audio graph; going silent in preparation for migration\n");
        pImpl->SetSilentMode();
    }

    HRESULT hr = pImpl->Reset(wfx, deviceId);
    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        {
            if (pImpl->mEngineFlags & AudioEngine_ThrowOnNoAudioHW)
            {
                DebugTrace("ERROR: AudioEngine found no default audio device on Reset\n");
                throw std::runtime_error("AudioEngineNoAudioHW");
            }
            else
            {
                DebugTrace("WARNING: AudioEngine found no default audio device on Reset; running in 'silent mode'\n");
                return false;
            }
        }
        else
        {
            DebugTrace("ERROR: AudioEngine failed (%08X) to Reset using device [%ls]\n",
                static_cast<unsigned int>(hr), (deviceId) ? deviceId : L"default");
            throw std::runtime_error("AudioEngine::Reset");
        }
    }

    DebugTrace("INFO: AudioEngine Reset using device [%ls]\n", (deviceId) ? deviceId : L"default");

    return true;
}


void AudioEngine::Suspend() noexcept
{
    if (!pImpl->xaudio2)
        return;

    pImpl->xaudio2->StopEngine();
}


void AudioEngine::Resume()
{
    if (!pImpl->xaudio2)
        return;

    HRESULT hr = pImpl->xaudio2->StartEngine();
    ThrowIfFailed(hr);
}


float AudioEngine::GetMasterVolume() const noexcept
{
    return pImpl->mMasterVolume;
}


void AudioEngine::SetMasterVolume(float volume)
{
    assert(volume >= -XAUDIO2_MAX_VOLUME_LEVEL && volume <= XAUDIO2_MAX_VOLUME_LEVEL);

    pImpl->mMasterVolume = volume;

    if (pImpl->mMasterVoice)
    {
        HRESULT hr = pImpl->mMasterVoice->SetVolume(volume);
        ThrowIfFailed(hr);
    }
}


void AudioEngine::SetReverb(AUDIO_ENGINE_REVERB reverb)
{
    if (reverb >= Reverb_MAX)
        throw std::invalid_argument("reverb parameter is invalid");

    if (reverb == Reverb_Off)
    {
        pImpl->SetReverb(nullptr);
    }
    else
    {
        XAUDIO2FX_REVERB_PARAMETERS native;
        ReverbConvertI3DL2ToNative(&gReverbPresets[reverb], &native);
        pImpl->SetReverb(&native);
    }
}


_Use_decl_annotations_
void AudioEngine::SetReverb(const XAUDIO2FX_REVERB_PARAMETERS* native)
{
    pImpl->SetReverb(native);
}


void AudioEngine::SetMasteringLimit(int release, int loudness)
{
    pImpl->SetMasteringLimit(release, loudness);
}


// Public accessors.
AudioStatistics AudioEngine::GetStatistics() const
{
    return pImpl->GetStatistics();
}


WAVEFORMATEXTENSIBLE AudioEngine::GetOutputFormat() const noexcept
{
    WAVEFORMATEXTENSIBLE wfx = {};

    if (!pImpl->xaudio2)
        return wfx;

    wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfx.Format.wBitsPerSample = wfx.Samples.wValidBitsPerSample = 16; // This is a guess
    wfx.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

    wfx.Format.nChannels = static_cast<WORD>(pImpl->masterChannels);
    wfx.Format.nSamplesPerSec = pImpl->masterRate;
    wfx.dwChannelMask = pImpl->masterChannelMask;

    wfx.Format.nBlockAlign = static_cast<WORD>(wfx.Format.nChannels * wfx.Format.wBitsPerSample / 8);
    wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;

    static const GUID s_pcm = { WAVE_FORMAT_PCM, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };
    memcpy(&wfx.SubFormat, &s_pcm, sizeof(GUID));

    return wfx;
}


uint32_t AudioEngine::GetChannelMask() const noexcept
{
    return pImpl->masterChannelMask;
}


unsigned int AudioEngine::GetOutputChannels() const noexcept
{
    return pImpl->masterChannels;
}


bool AudioEngine::IsAudioDevicePresent() const noexcept
{
    return pImpl->xaudio2 && !pImpl->mCriticalError;
}


bool AudioEngine::IsCriticalError() const noexcept
{
    return pImpl->mCriticalError;
}


// Voice management.
void AudioEngine::SetDefaultSampleRate(int sampleRate)
{
    if ((sampleRate < XAUDIO2_MIN_SAMPLE_RATE) || (sampleRate > XAUDIO2_MAX_SAMPLE_RATE))
        throw std::out_of_range("Default sample rate is out of range");

    pImpl->defaultRate = sampleRate;
}


void AudioEngine::SetMaxVoicePool(size_t maxOneShots, size_t maxInstances)
{
    if (maxOneShots > 0)
        pImpl->maxVoiceOneshots = maxOneShots;

    if (maxInstances > 0)
        pImpl->maxVoiceInstances = maxInstances;
}


void AudioEngine::TrimVoicePool()
{
    pImpl->TrimVoicePool();
}


_Use_decl_annotations_
void AudioEngine::AllocateVoice(
    const WAVEFORMATEX* wfx,
    SOUND_EFFECT_INSTANCE_FLAGS flags,
    bool oneshot,
    IXAudio2SourceVoice** voice)
{
    pImpl->AllocateVoice(wfx, flags, oneshot, voice);
}


void AudioEngine::DestroyVoice(_In_ IXAudio2SourceVoice* voice) noexcept
{
    pImpl->DestroyVoice(voice);
}


void AudioEngine::RegisterNotify(_In_ IVoiceNotify* notify, bool usesUpdate)
{
    pImpl->RegisterNotify(notify, usesUpdate);
}


void AudioEngine::UnregisterNotify(_In_ IVoiceNotify* notify, bool oneshots, bool usesUpdate)
{
    pImpl->UnregisterNotify(notify, oneshots, usesUpdate);
}


IXAudio2* AudioEngine::GetInterface() const noexcept
{
    return pImpl->xaudio2.Get();
}


IXAudio2MasteringVoice* AudioEngine::GetMasterVoice() const noexcept
{
    return pImpl->mMasterVoice;
}


IXAudio2SubmixVoice* AudioEngine::GetReverbVoice() const noexcept
{
    return pImpl->mReverbVoice;
}


X3DAUDIO_HANDLE& AudioEngine::Get3DHandle() const noexcept
{
    return pImpl->mX3DAudio;
}


// Static methods.
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_GAMES)
#include <mmdeviceapi.h>
#elif defined(_XBOX_ONE)
#include <Windows.Media.Devices.h>
#include <wrl.h>
#elif defined(USING_XAUDIO2_REDIST) || defined(_GAMING_DESKTOP)
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#elif (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
#pragma comment(lib,"runtimeobject.lib")
#pragma warning(push)
#pragma warning(disable: 4471 5204)
#include <windows.devices.enumeration.h>
#pragma warning(pop)
#include <wrl.h>
#endif

std::vector<AudioEngine::RendererDetail> AudioEngine::GetRendererDetails()
{
    std::vector<RendererDetail> list;

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_GAMES)

    ComPtr<IMMDeviceEnumerator> devEnum;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(devEnum.GetAddressOf()));
    ThrowIfFailed(hr);

    ComPtr<IMMDeviceCollection> devices;
    hr = devEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices);
    ThrowIfFailed(hr);

    ComPtr<IMMDevice> endpoint;
    ThrowIfFailed(devices->Item(0, endpoint.GetAddressOf()));

    LPWSTR id = nullptr;
    ThrowIfFailed(endpoint->GetId(&id));

    RendererDetail device;
    device.deviceId = id;
    device.description = L"Default";

    CoTaskMemFree(id);

    list.emplace_back(device);

#elif defined(_XBOX_ONE)

    using namespace Microsoft::WRL;
    using namespace Microsoft::WRL::Wrappers;
    using namespace ABI::Windows::Foundation;
    using namespace ABI::Windows::Media::Devices;

    ComPtr<IMediaDeviceStatics> mdStatics;
    HRESULT hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_Media_Devices_MediaDevice).Get(), &mdStatics);
    ThrowIfFailed(hr);

    HString id;
    hr = mdStatics->GetDefaultAudioRenderId(AudioDeviceRole_Default, id.GetAddressOf());
    ThrowIfFailed(hr);

    RendererDetail device;
    device.deviceId = id.GetRawBuffer(nullptr);
    device.description = L"Default";
    list.emplace_back(device);

#elif defined(USING_XAUDIO2_REDIST) || defined(_GAMING_DESKTOP)

    ComPtr<IMMDeviceEnumerator> devEnum;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(devEnum.GetAddressOf()));
    ThrowIfFailed(hr);

    ComPtr<IMMDeviceCollection> devices;
    hr = devEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices);
    ThrowIfFailed(hr);

    UINT count = 0;
    ThrowIfFailed(devices->GetCount(&count));

    if (!count)
        return list;

    for (UINT j = 0; j < count; ++j)
    {
        ComPtr<IMMDevice> endpoint;
        hr = devices->Item(j, endpoint.GetAddressOf());
        ThrowIfFailed(hr);

        LPWSTR id = nullptr;
        ThrowIfFailed(endpoint->GetId(&id));

        RendererDetail device;
        device.deviceId = id;
        CoTaskMemFree(id);

        ComPtr<IPropertyStore> props;
        if (SUCCEEDED(endpoint->OpenPropertyStore(STGM_READ, props.GetAddressOf())))
        {
            PROPVARIANT var;
            PropVariantInit(&var);

            if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &var)))
            {
                if (var.vt == VT_LPWSTR)
                {
                    device.description = var.pwszVal;
                }
                PropVariantClear(&var);
            }
        }

        list.emplace_back(device);
    }

#elif (_WIN32_WINNT >= _WIN32_WINNT_WIN8)

#if defined(__cplusplus_winrt)

    // Enumerating with WinRT using C++/CX (Windows Store apps)
    using Windows::Devices::Enumeration::DeviceClass;
    using Windows::Devices::Enumeration::DeviceInformation;
    using Windows::Devices::Enumeration::DeviceInformationCollection;

    auto operation = DeviceInformation::FindAllAsync(DeviceClass::AudioRender);
    while (operation->Status == Windows::Foundation::AsyncStatus::Started) { Sleep(100); }
    if (operation->Status != Windows::Foundation::AsyncStatus::Completed)
    {
        throw std::runtime_error("FindAllAsync");
    }

    DeviceInformationCollection^ devices = operation->GetResults();

    for (unsigned i = 0; i < devices->Size; ++i)
    {
        using Windows::Devices::Enumeration::DeviceInformation;

        DeviceInformation^ d = devices->GetAt(i);

        RendererDetail device;
        device.deviceId = d->Id->Data();
        device.description = d->Name->Data();
        list.emplace_back(device);
    }
#else

    // Enumerating with WinRT using WRL (Win32 desktop app for Windows 8.x)
    using namespace Microsoft::WRL;
    using namespace Microsoft::WRL::Wrappers;
    using namespace ABI::Windows::Foundation;
    using namespace ABI::Windows::Foundation::Collections;
    using namespace ABI::Windows::Devices::Enumeration;

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
    RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    ThrowIfFailed(initialize);
#endif

    ComPtr<IDeviceInformationStatics> diFactory;
    HRESULT hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_Devices_Enumeration_DeviceInformation).Get(), &diFactory);
    ThrowIfFailed(hr);

    ComPtr<IAsyncOperation<DeviceInformationCollection*>> operation;
    hr = diFactory->FindAllAsyncDeviceClass(DeviceClass_AudioRender, operation.GetAddressOf());
    ThrowIfFailed(hr);

    ComPtr<IAsyncInfo> asyncinfo;
    hr = operation.As(&asyncinfo);
    ThrowIfFailed(hr);

    AsyncStatus status;
    hr = asyncinfo->get_Status(&status);
    ThrowIfFailed(hr);

    while (status == ABI::Windows::Foundation::AsyncStatus::Started)
    {
        Sleep(100);
        hr = asyncinfo->get_Status(&status);
        ThrowIfFailed(hr);
    }

    if (status != ABI::Windows::Foundation::AsyncStatus::Completed)
    {
        throw std::runtime_error("FindAllAsyncDeviceClass");
    }

    ComPtr<IVectorView<DeviceInformation*>> devices;
    hr = operation->GetResults(devices.GetAddressOf());
    ThrowIfFailed(hr);

    unsigned int count = 0;
    hr = devices->get_Size(&count);
    ThrowIfFailed(hr);

    if (!count)
        return list;

    for (unsigned int j = 0; j < count; ++j)
    {
        ComPtr<IDeviceInformation> deviceInfo;
        hr = devices->GetAt(j, deviceInfo.GetAddressOf());
        if (SUCCEEDED(hr))
        {
            RendererDetail device;

            HString id;
            if (SUCCEEDED(deviceInfo->get_Id(id.GetAddressOf())))
            {
                device.deviceId = id.GetRawBuffer(nullptr);
            }

            HString name;
            if (SUCCEEDED(deviceInfo->get_Name(name.GetAddressOf())))
            {
                device.description = name.GetRawBuffer(nullptr);
            }

            list.emplace_back(device);
        }
    }
#endif
#else
#error DirectX Tool Kit for Audio not supported on this platform
#endif

    return list;
}
