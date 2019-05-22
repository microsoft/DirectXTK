//--------------------------------------------------------------------------------------
// File: SoundCommon.h
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#include "Audio.h"
#include "PlatformHelpers.h"


namespace DirectX
{
    // Helper for getting a format tag from a WAVEFORMATEX
    inline uint32_t GetFormatTag(const WAVEFORMATEX* wfx)
    {
        if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            if (wfx->cbSize < (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)))
                return 0;

            static const GUID s_wfexBase = { 0x00000000, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

            auto wfex = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wfx);

            if (memcmp(reinterpret_cast<const BYTE*>(&wfex->SubFormat) + sizeof(DWORD),
                reinterpret_cast<const BYTE*>(&s_wfexBase) + sizeof(DWORD), sizeof(GUID) - sizeof(DWORD)) != 0)
            {
                return 0;
            }

            return wfex->SubFormat.Data1;
        }
        else
        {
            return wfx->wFormatTag;
        }
    }


    // Helper for validating wave format structure
    bool IsValid(_In_ const WAVEFORMATEX* wfx);


    // Helper for getting a default channel mask from channels
    uint32_t GetDefaultChannelMask(int channels);


    // Helpers for creating various wave format structures
    void CreateIntegerPCM(_Out_ WAVEFORMATEX* wfx, int sampleRate, int channels, int sampleBits);
    void CreateFloatPCM(_Out_ WAVEFORMATEX* wfx, int sampleRate, int channels);
    void CreateADPCM(_Out_writes_bytes_(wfxSize) WAVEFORMATEX* wfx, size_t wfxSize, int sampleRate, int channels, int samplesPerBlock);
#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
    void CreateXWMA(_Out_ WAVEFORMATEX* wfx, int sampleRate, int channels, int blockAlign, int avgBytes, bool wma3);
#endif
#if defined(_XBOX_ONE) && defined(_TITLE)
    void CreateXMA2(_Out_writes_bytes_(wfxSize) WAVEFORMATEX* wfx, size_t wfxSize, int sampleRate, int channels, int bytesPerBlock, int blockCount, int samplesEncoded);
#endif

    // Helper for computing pan volume matrix
    bool ComputePan(float pan, unsigned int channels, _Out_writes_(16) float* matrix);

    // Helper class for implementing SoundEffectInstance
    class SoundEffectInstanceBase
    {
    public:
        SoundEffectInstanceBase() noexcept :
            voice(nullptr),
            state(STOPPED),
            engine(nullptr),
            mVolume(1.f),
            mPitch(0.f),
            mFreqRatio(1.f),
            mPan(0.f),
            mFlags(SoundEffectInstance_Default),
            mDirectVoice(nullptr),
            mReverbVoice(nullptr),
            mDSPSettings{}
        {
        }

        ~SoundEffectInstanceBase()
        {
            assert(voice == nullptr);
        }

        void Initialize(_In_ AudioEngine* eng, _In_ const WAVEFORMATEX* wfx, SOUND_EFFECT_INSTANCE_FLAGS flags)
        {
            assert(eng != nullptr);
            engine = eng;
            mDirectVoice = eng->GetMasterVoice();
            mReverbVoice = eng->GetReverbVoice();

            if (eng->GetChannelMask() & SPEAKER_LOW_FREQUENCY)
                mFlags = flags | SoundEffectInstance_UseRedirectLFE;
            else
                mFlags = static_cast<SOUND_EFFECT_INSTANCE_FLAGS>(static_cast<unsigned int>(flags) & ~static_cast<unsigned int>(SoundEffectInstance_UseRedirectLFE));

            memset(&mDSPSettings, 0, sizeof(X3DAUDIO_DSP_SETTINGS));
            assert(wfx != nullptr);
            mDSPSettings.SrcChannelCount = wfx->nChannels;
            mDSPSettings.DstChannelCount = eng->GetOutputChannels();
        }

        void AllocateVoice(_In_ const WAVEFORMATEX* wfx)
        {
            if (voice)
                return;

            assert(engine != nullptr);
            engine->AllocateVoice(wfx, mFlags, false, &voice);
        }

        void DestroyVoice()
        {
            if (voice)
            {
                assert(engine != nullptr);
                engine->DestroyVoice(voice);
                voice = nullptr;
            }
        }

        bool Play() // Returns true if STOPPED -> PLAYING
        {
            if (voice)
            {
                if (state == PAUSED)
                {
                    HRESULT hr = voice->Start(0);
                    ThrowIfFailed(hr);
                    state = PLAYING;
                }
                else if (state != PLAYING)
                {
                    if (mVolume != 1.f)
                    {
                        HRESULT hr = voice->SetVolume(mVolume);
                        ThrowIfFailed(hr);
                    }

                    if (mPitch != 0.f)
                    {
                        mFreqRatio = XAudio2SemitonesToFrequencyRatio(mPitch * 12.f);

                        HRESULT hr = voice->SetFrequencyRatio(mFreqRatio);
                        ThrowIfFailed(hr);
                    }

                    if (mPan != 0.f)
                    {
                        SetPan(mPan);
                    }

                    HRESULT hr = voice->Start(0);
                    ThrowIfFailed(hr);
                    state = PLAYING;
                    return true;
                }
            }
            return false;
        }

        void Stop(bool immediate, bool& looped)
        {
            if (!voice)
            {
                state = STOPPED;
                return;
            }

            if (immediate)
            {
                state = STOPPED;
                (void)voice->Stop(0);
                (void)voice->FlushSourceBuffers();
            }
            else if (looped)
            {
                looped = false;
                (void)voice->ExitLoop();
            }
            else
            {
                (void)voice->Stop(XAUDIO2_PLAY_TAILS);
            }
        }

        void Pause()
        {
            if (voice && state == PLAYING)
            {
                state = PAUSED;

                (void)voice->Stop(0);
            }
        }

        void Resume()
        {
            if (voice && state == PAUSED)
            {
                HRESULT hr = voice->Start(0);
                ThrowIfFailed(hr);
                state = PLAYING;
            }
        }

        void SetVolume(float volume)
        {
            assert(volume >= -XAUDIO2_MAX_VOLUME_LEVEL && volume <= XAUDIO2_MAX_VOLUME_LEVEL);

            mVolume = volume;

            if (voice)
            {
                HRESULT hr = voice->SetVolume(volume);
                ThrowIfFailed(hr);
            }
        }

        void SetPitch(float pitch)
        {
            assert(pitch >= -1.f && pitch <= 1.f);

            if ((mFlags & SoundEffectInstance_NoSetPitch) && pitch != 0.f)
            {
                DebugTrace("ERROR: Sound effect instance was created with the NoSetPitch flag\n");
                throw std::exception("SetPitch");
            }

            mPitch = pitch;

            if (voice)
            {
                mFreqRatio = XAudio2SemitonesToFrequencyRatio(mPitch * 12.f);

                HRESULT hr = voice->SetFrequencyRatio(mFreqRatio);
                ThrowIfFailed(hr);
            }
        }

        void SetPan(float pan);

        void Apply3D(const AudioListener& listener, const AudioEmitter& emitter, bool rhcoords);

        SoundState GetState(bool autostop)
        {
            if (autostop && voice && (state == PLAYING))
            {
                XAUDIO2_VOICE_STATE xstate;
            #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
                voice->GetState(&xstate, XAUDIO2_VOICE_NOSAMPLESPLAYED);
            #else
                voice->GetState(&xstate);
            #endif

                if (!xstate.BuffersQueued)
                {
                    // Automatic stop if the buffer has finished playing
                    (void)voice->Stop();
                    state = STOPPED;
                }
            }

            return state;
        }

        int GetPendingBufferCount() const
        {
            if (!voice)
                return 0;

            XAUDIO2_VOICE_STATE xstate;
        #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
            voice->GetState(&xstate, XAUDIO2_VOICE_NOSAMPLESPLAYED);
        #else
            voice->GetState(&xstate);
        #endif
            return static_cast<int>(xstate.BuffersQueued);
        }

        void OnCriticalError()
        {
            if (voice)
            {
                voice->DestroyVoice();
                voice = nullptr;
            }
            state = STOPPED;
            mDirectVoice = nullptr;
            mReverbVoice = nullptr;
        }

        void OnReset()
        {
            assert(engine != nullptr);
            mDirectVoice = engine->GetMasterVoice();
            mReverbVoice = engine->GetReverbVoice();

            if (engine->GetChannelMask() & SPEAKER_LOW_FREQUENCY)
                mFlags = mFlags | SoundEffectInstance_UseRedirectLFE;
            else
                mFlags = static_cast<SOUND_EFFECT_INSTANCE_FLAGS>(static_cast<unsigned int>(mFlags) & ~static_cast<unsigned int>(SoundEffectInstance_UseRedirectLFE));

            mDSPSettings.DstChannelCount = engine->GetOutputChannels();
        }

        void OnDestroy()
        {
            if (voice)
            {
                (void)voice->Stop(0);
                (void)voice->FlushSourceBuffers();
                voice->DestroyVoice();
                voice = nullptr;
            }
            state = STOPPED;
            engine = nullptr;
            mDirectVoice = nullptr;
            mReverbVoice = nullptr;
        }

        void OnTrim()
        {
            if (voice && (state == STOPPED))
            {
                engine->DestroyVoice(voice);
                voice = nullptr;
            }
        }

        void GatherStatistics(AudioStatistics& stats) const
        {
            ++stats.allocatedInstances;
            if (voice)
            {
                ++stats.allocatedVoices;

                if (mFlags & SoundEffectInstance_Use3D)
                    ++stats.allocatedVoices3d;

                if (state == PLAYING)
                    ++stats.playingInstances;
            }
        }

        IXAudio2SourceVoice*        voice;
        SoundState                  state;
        AudioEngine*                engine;

    private:
        float                       mVolume;
        float                       mPitch;
        float                       mFreqRatio;
        float                       mPan;
        SOUND_EFFECT_INSTANCE_FLAGS mFlags;
        IXAudio2Voice*              mDirectVoice;
        IXAudio2Voice*              mReverbVoice;
        X3DAUDIO_DSP_SETTINGS       mDSPSettings;
    };
}
