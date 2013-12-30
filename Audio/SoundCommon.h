//--------------------------------------------------------------------------------------
// File: SoundCommon.h
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#include "Audio.h"
#include "PlatformHelpers.h"


namespace DirectX
{
    // Helper for getting a format tag from a WAVEFORMATEX
    inline uint32_t GetFormatTag( const WAVEFORMATEX* wfx )
    {
        if ( wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
        {
            if ( wfx->cbSize < ( sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) ) )
                return 0;

            static const GUID s_wfexBase = {0x00000000, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};

            auto wfex = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>( wfx );

            if ( memcmp( reinterpret_cast<const BYTE*>(&wfex->SubFormat) + sizeof(DWORD),
                         reinterpret_cast<const BYTE*>(&s_wfexBase) + sizeof(DWORD), sizeof(GUID) - sizeof(DWORD) ) != 0 )
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
    bool IsValid( _In_ const WAVEFORMATEX* wfx );


    // Helper for getting a default channel mask from channels
    uint32_t GetDefaultChannelMask( int channels );


    // Helpers for creating various wave format structures
    void CreateIntegerPCM( _Out_ WAVEFORMATEX* wfx, int sampleRate, int channels, int sampleBits );
    void CreateFloatPCM( _Out_ WAVEFORMATEX* wfx, int sampleRate, int channels );
    void CreateADPCM( _Out_writes_bytes_(wfxSize) WAVEFORMATEX* wfx, size_t wfxSize, int sampleRate, int channels, int samplesPerBlock );
#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8)
    void CreateXWMA( _Out_ WAVEFORMATEX* wfx, int sampleRate, int channels, int blockAlign, int avgBytes, bool wma3 );
#endif
#if defined(_XBOX_ONE) && defined(_TITLE)
    void CreateXMA2( _Out_writes_bytes_(wfxSize) WAVEFORMATEX* wfx, size_t wfxSize, int sampleRate, int channels, int bytesPerBlock, int blockCount, int samplesEncoded );
#endif

    // Helper class for implementing SoundEffectInstance
    class SoundEffectInstanceBase
    {
    public:
        SoundEffectInstanceBase() :
            voice( nullptr ),
            state( STOPPED ),
            engine( nullptr ),
            mFlags( SoundEffectInstance_Default ),
            mDirectVoice( nullptr ),
            mReverbVoice( nullptr )
        {
        }

        ~SoundEffectInstanceBase()
        {
            assert( !voice );
        }

        void Initialize( _In_ AudioEngine* eng, _In_ const WAVEFORMATEX* wfx, SOUND_EFFECT_INSTANCE_FLAGS flags )
        {
            assert( eng != 0 );
            engine = eng;
            mDirectVoice = eng->GetMasterVoice();
            mReverbVoice = eng->GetReverbVoice();

            if ( eng->GetChannelMask() & SPEAKER_LOW_FREQUENCY )
                mFlags = flags | SoundEffectInstance_UseRedirectLFE;
            else
                mFlags = static_cast<SOUND_EFFECT_INSTANCE_FLAGS>( static_cast<int>(flags) & ~SoundEffectInstance_UseRedirectLFE );

            memset( &mDSPSettings, 0, sizeof(X3DAUDIO_DSP_SETTINGS) );
            assert( wfx != 0 );
            mDSPSettings.SrcChannelCount = wfx->nChannels;
            mDSPSettings.DstChannelCount = eng->GetOutputChannels();
        }

        void AllocateVoice( _In_ const WAVEFORMATEX* wfx )
        {
            if ( voice )
                return;

            assert( engine != 0 );
            engine->AllocateVoice( wfx, mFlags, false, &voice );
        }

        void DestroyVoice()
        {
            if ( voice )
            {
                assert( engine != 0 );
                engine->DestroyVoice( voice );
                voice = nullptr;
            }
        }

        bool Play() // Returns true if STOPPED -> PLAYING
        {
            if ( voice )
            {
                if ( state == PAUSED )
                {
                    state = PLAYING;
                    HRESULT hr = voice->Start( 0 );
                    ThrowIfFailed( hr );
                }
                else if ( state != PLAYING )
                {
                    HRESULT hr = voice->Start( 0 );
                    ThrowIfFailed( hr );
                    state = PLAYING;
                    return true;
                }
            }
            return false;
        }

        void Stop( bool immediate, bool& looped )
        {
            if ( !voice )
            {
                state = STOPPED;
                return;
            }

            if ( immediate )
            {
                state = STOPPED;
                voice->Stop( 0 );
            }
            else if ( looped )
            {
                looped = false;
                voice->ExitLoop();
            }
            else
            {
                voice->Stop( XAUDIO2_PLAY_TAILS );
            }
        }

        void Pause()
        {
            if ( voice && state == PLAYING )
            {
                state = PAUSED;

                voice->Stop( 0 );
            }
        }

        void Resume()
        {
            if ( voice && state == PAUSED )
            {
                HRESULT hr = voice->Start( 0 );
                ThrowIfFailed( hr );
                state = PLAYING;
            }
        }

        void SetPitch( float pitch )
        {
            if ( ( mFlags & SoundEffectInstance_NoSetPitch ) && pitch != 1.f )
            {
                DebugTrace( "ERROR: Sound effect instance was created with the NoSetPitch flag\n" );
                throw std::exception( "SetPitch" );
            }

            assert( pitch >= XAUDIO2_MIN_FREQ_RATIO && pitch <= XAUDIO2_DEFAULT_FREQ_RATIO );

            if ( voice )
            {
                HRESULT hr = voice->SetFrequencyRatio( pitch );
                ThrowIfFailed( hr );
            }
        }

        void SetPan( float pan );

        void Apply3D( const AudioListener& listener, const AudioEmitter& emitter );

        SoundState GetState( bool autostop )
        {
            if ( autostop && voice && ( state == PLAYING ) )
            {
                XAUDIO2_VOICE_STATE xstate;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
                voice->GetState( &xstate, XAUDIO2_VOICE_NOSAMPLESPLAYED );
#else
                voice->GetState( &xstate );
#endif

                if ( !xstate.BuffersQueued )
                {
                    // Automatic stop if the buffer has finished playing
                    voice->Stop();
                    state = STOPPED;
                }
            }

            return state;
        }

        int GetPendingBufferCount() const
        {
            if ( !voice )
                return 0;

            XAUDIO2_VOICE_STATE xstate;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
            voice->GetState( &xstate, XAUDIO2_VOICE_NOSAMPLESPLAYED );
#else
            voice->GetState( &xstate );
#endif
            return static_cast<int>( xstate.BuffersQueued );
        }

        void OnCriticalError()
        {
            voice = nullptr;
            state = STOPPED;
            mDirectVoice = nullptr;
            mReverbVoice = nullptr;
        }

        void OnReset()
        {
            assert( engine != 0 );
            mDirectVoice = engine->GetMasterVoice();
            mReverbVoice = engine->GetReverbVoice();

            if ( engine->GetChannelMask() & SPEAKER_LOW_FREQUENCY )
                mFlags = mFlags | SoundEffectInstance_UseRedirectLFE;
            else
                mFlags = static_cast<SOUND_EFFECT_INSTANCE_FLAGS>( static_cast<int>(mFlags) & ~SoundEffectInstance_UseRedirectLFE );

            mDSPSettings.DstChannelCount = engine->GetOutputChannels();
        }

        void OnDestroy()
        {
            if ( voice )
            {
                voice->Stop( 0 );
                voice->FlushSourceBuffers();
                voice = nullptr;
            }
            state = STOPPED;
            engine = nullptr;
            mDirectVoice = nullptr;
            mReverbVoice = nullptr;
        }

        void OnTrim()
        {
            if ( voice && ( state == STOPPED ) )
            {
                engine->DestroyVoice( voice );
                voice = nullptr;
            }
        }

        void GatherStatistics( AudioStatistics& stats ) const
        {
            ++stats.allocatedInstances;
            if ( voice )
            {
                ++stats.allocatedVoices;

                if ( mFlags & SoundEffectInstance_Use3D )
                    ++stats.allocatedVoices3d;

                if ( state == PLAYING )
                    ++stats.playingInstances;
            }
        }

        IXAudio2SourceVoice*        voice;
        SoundState                  state;
        AudioEngine*                engine;

    private:
        SOUND_EFFECT_INSTANCE_FLAGS mFlags;
        IXAudio2Voice*              mDirectVoice;
        IXAudio2Voice*              mReverbVoice;
        X3DAUDIO_DSP_SETTINGS       mDSPSettings;
   };
}