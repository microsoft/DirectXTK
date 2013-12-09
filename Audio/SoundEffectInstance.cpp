//--------------------------------------------------------------------------------------
// File: SoundEffectInstance.cpp
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

#include "pch.h"
#include "Audio.h"
#include "WAVFileReader.h"
#include "PlatformHelpers.h"

using namespace DirectX;


//======================================================================================
// SoundEffectInstance
//======================================================================================

// Internal object implementation class.
class SoundEffectInstance::Impl : public IVoiceNotify
{
public:
    Impl( _In_ AudioEngine* engine, _In_ SoundEffect* effect, SOUND_EFFECT_INSTANCE_FLAGS flags ) :
        mVoice( nullptr ),
        mDirectVoice( nullptr ),
        mReverbVoice( nullptr ),
        mEffect( effect ),
        mWaveBank( nullptr ),
        mIndex( 0 ),
        mLooped( false ),
        mFlags( flags ),
        mState( STOPPED ),
        mEngine( engine )
    {
        assert( mEffect != 0 );
        assert( mEngine != 0 );
        mEngine->RegisterNotify( this );

        mDirectVoice = mEngine->GetMasterVoice();
        mReverbVoice = mEngine->GetReverbVoice();
        if ( mEngine->GetChannelMask() & SPEAKER_LOW_FREQUENCY )
            mFlags = mFlags | SoundEffectInstance_UseRedirectLFE;

        memset( &mDSPSettings, 0, sizeof(X3DAUDIO_DSP_SETTINGS) );

        assert( mEffect->GetFormat() != 0 );
        mDSPSettings.SrcChannelCount = mEffect->GetFormat()->nChannels;
        mDSPSettings.DstChannelCount = mEngine->GetOutputChannels();
    }

    Impl( _In_ AudioEngine* engine, _In_ WaveBank* waveBank, uint32_t index, SOUND_EFFECT_INSTANCE_FLAGS flags ) :
        mVoice( nullptr ),
        mDirectVoice( nullptr ),
        mReverbVoice( nullptr ),
        mEffect( nullptr ),
        mWaveBank( waveBank ),
        mIndex( index ),
        mLooped( false ),
        mFlags( flags ),
        mState( STOPPED ),
        mEngine( engine )
    {
        assert( mWaveBank != 0 );
        assert( mEngine != 0 );
        mEngine->RegisterNotify( this );

        mDirectVoice = mEngine->GetMasterVoice();
        mReverbVoice = mEngine->GetReverbVoice();
        if ( mEngine->GetChannelMask() & SPEAKER_LOW_FREQUENCY )
            mFlags = mFlags | SoundEffectInstance_UseRedirectLFE;

        memset( &mDSPSettings, 0, sizeof(X3DAUDIO_DSP_SETTINGS) );
 
        char buff[64];
        auto wfx = reinterpret_cast<WAVEFORMATEX*>( buff );
        mWaveBank->GetFormat( index, wfx, 64 );
        mDSPSettings.SrcChannelCount = wfx->nChannels;
        mDSPSettings.DstChannelCount = mEngine->GetOutputChannels();
    }

    ~Impl()
    {
        if ( mVoice )
        {
            mVoice->DestroyVoice();
            mVoice = nullptr;
        }

        if ( mEngine )
        {
            mEngine->UnregisterNotify( this, false );
            mEngine = nullptr;
        }
    }

    void Play( bool loop );
    void Stop( bool immediate );
    void Pause();
    void SetPan( float pan );

    void Apply3D( const AudioListener& listener, const AudioEmitter& emitter );

    SoundEffectInstance::SoundState GetState();

    void Halt()
    {
        if ( mVoice )
        {
            mVoice->Stop( 0 );
            mVoice->FlushSourceBuffers();
            mVoice = nullptr;
        }

        mState = STOPPED;
    }

    // IVoiceNotify
    virtual void OnBufferEnd() override
    {
        // We don't register for this notification for SoundEffectInstances, so this should not be invoked
        assert( false );
    }

    virtual void OnCriticalError() override
    {
        mVoice = nullptr;
        mDirectVoice = nullptr;
        mReverbVoice = nullptr;
        mState = STOPPED;
    }

    virtual void OnReset() override
    {
        assert( mEngine != 0 );
        mDirectVoice = mEngine->GetMasterVoice();
        mReverbVoice = mEngine->GetReverbVoice();

        mFlags = static_cast<SOUND_EFFECT_INSTANCE_FLAGS>( static_cast<int>(mFlags) & ~SoundEffectInstance_UseRedirectLFE );
        if ( mEngine->GetChannelMask() & SPEAKER_LOW_FREQUENCY )
            mFlags = mFlags | SoundEffectInstance_UseRedirectLFE;

        mDSPSettings.DstChannelCount = mEngine->GetOutputChannels();
    }

    virtual void OnDestroyEngine() override
    {
        Halt();
        mDirectVoice = nullptr;
        mReverbVoice = nullptr;
        mEngine = nullptr;
    }

    virtual void GatherStatistics( AudioStatistics& stats ) const override
    {
        ++stats.allocatedInstances;
        if ( mVoice )
        {
            ++stats.allocatedVoices;

            if ( mFlags & SoundEffectInstance_Use3D )
                ++stats.allocatedVoices3d;

            if ( mState == PLAYING )
                ++stats.playingInstances;
        }
    }

    IXAudio2SourceVoice*            mVoice;
    IXAudio2Voice*                  mDirectVoice;
    IXAudio2Voice*                  mReverbVoice;
    SoundEffect*                    mEffect;
    WaveBank*                       mWaveBank;
    uint32_t                        mIndex;
    bool                            mLooped;
    SOUND_EFFECT_INSTANCE_FLAGS     mFlags;
    SoundEffectInstance::SoundState mState;

private:
    X3DAUDIO_DSP_SETTINGS           mDSPSettings;
    AudioEngine*                    mEngine;
};


void SoundEffectInstance::Impl::Play( bool loop )
{
    if ( !mVoice )
    {
        if ( mWaveBank )
        {
            char buff[64];
            mEngine->AllocateVoice( mWaveBank->GetFormat( mIndex, reinterpret_cast<WAVEFORMATEX*>( buff ), 64 ), mFlags, false, &mVoice );
        }
        else
        {
            assert( mEffect != 0 );
            mEngine->AllocateVoice( mEffect->GetFormat(), mFlags, false, &mVoice );
        }
    }

    if ( !mVoice )
        return;

    if ( mState == PAUSED )
    {
        mState = PLAYING;
        HRESULT hr = mVoice->Start( 0 );
        ThrowIfFailed( hr );
    }
    else if ( mState != PLAYING )
    {
        HRESULT hr = mVoice->Start( 0 );
        ThrowIfFailed( hr );

        XAUDIO2_BUFFER buffer;
        if ( mWaveBank )
        {
            mWaveBank->FillSubmitBuffer( mIndex, buffer );
        }
        else
        {
            assert( mEffect != 0 );
            mEffect->FillSubmitBuffer( buffer );
        }
    
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        if ( loop )
        {
            buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
        }
        else
        {
            buffer.LoopCount = buffer.LoopBegin = buffer.LoopLength = 0;
        }
        buffer.pContext = nullptr;

        hr = mVoice->SubmitSourceBuffer( &buffer, nullptr );
        if ( FAILED(hr) )
        {
#ifdef _DEBUG
            char buff[256];
            sprintf_s( buff, "ERROR: SoundEffect failed (%08X) when submitting buffer:\n", hr );
            OutputDebugStringA( buff );

            char buff2[64];
            auto wfx = ( mWaveBank ) ? mWaveBank->GetFormat( mIndex, reinterpret_cast<WAVEFORMATEX*>( buff2 ), 64 )
                                     : mEffect->GetFormat();

            uint32_t length = ( mWaveBank ) ? mWaveBank->SampleSizeInBytes( mIndex ) : mEffect->SampleSizeInBytes();

            sprintf_s( buff, "\tFormat Tag %u, %u channels, %u-bit, %u Hz, %u bytes\n", wfx->wFormatTag, 
                       wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, length );
            OutputDebugStringA( buff );
#endif
            throw std::exception( "SubmitSourceBuffer" );
        }

        mState = PLAYING;
    }
}


void SoundEffectInstance::Impl::Stop( bool immediate )
{
    if ( !mVoice )
    {
        mState = STOPPED;
        return;
    }

    if ( immediate )
    {
        mState = STOPPED;
        mVoice->Stop( 0 );
    }
    else if ( mLooped )
    {
        mLooped = false;
        mVoice->ExitLoop();
    }
    else
    {
        mVoice->Stop( XAUDIO2_PLAY_TAILS );
    }
}


void SoundEffectInstance::Impl::Pause()
{
    if ( mState != PLAYING )
        return;

    mState = PAUSED;

    if ( mVoice )
    {
        mVoice->Stop( 0 );
    }
}


void SoundEffectInstance::Impl::SetPan( float pan )
{
    if ( mDSPSettings.SrcChannelCount != 1 )
    {
#if _DEBUG
        char buff2[256];
        sprintf_s( buff2, "ERROR: SoundEffectInstance only supports panning on mono source data\n" );
        OutputDebugStringA( buff2 );
#endif
        throw std::exception( "SoundEffectInstance::SetPan" );
    }

    if ( !mVoice )
        return;

    pan = std::min<float>( 1.f, pan );
    pan = std::max<float>( -1.f, pan );

    float left = ( pan >= 0 ) ? ( 1.f - pan ) : 1.f;
    float right = ( pan <= 0 ) ? ( - pan - 1.f ) : 1.f;

    float matrix[8];
    for( size_t j = 0; j < 8; ++j )
        matrix[j] = 1.f;

    switch( mEngine->GetChannelMask() )
    {
    case SPEAKER_STEREO:
    case SPEAKER_2POINT1:
    case SPEAKER_SURROUND:
        matrix[ 0 ] = left;
        matrix[ 1 ] = right;
        break;

    case SPEAKER_QUAD:
        matrix[ 0 ] = matrix[ 2 ] = left;
        matrix[ 1 ] = matrix[ 3 ] = right;
        break;

    case SPEAKER_4POINT1:
        matrix[ 0 ] = matrix[ 3 ] = left;
        matrix[ 1 ] = matrix[ 4 ] = right;
        break;

    case SPEAKER_5POINT1:
    case SPEAKER_7POINT1:
    case SPEAKER_5POINT1_SURROUND:
        matrix[ 0 ] = matrix[ 4 ] = left;
        matrix[ 1 ] = matrix[ 5 ] = right;
        break;

    case SPEAKER_7POINT1_SURROUND:
        matrix[ 0 ] = matrix[ 4 ] = matrix[ 6 ] = left;
        matrix[ 1 ] = matrix[ 5 ] = matrix[ 7 ] = right;
        break;

    case SPEAKER_MONO:
    default:
        // No panning...
        return;
    }

    HRESULT hr = mVoice->SetOutputMatrix( nullptr, 1, mEngine->GetOutputChannels(), matrix );
    ThrowIfFailed( hr );
}


void SoundEffectInstance::Impl::Apply3D( const AudioListener& listener, const AudioEmitter& emitter )
{
    if ( !mVoice )
        return;

    if ( !( mFlags & SoundEffectInstance_Use3D ) )
    {
#ifdef _DEBUG
        OutputDebugStringA( "ERROR: SoundEffectInstance::Apply3D called for an instance created without SoundEffectInstance_Use3D set" );
#endif
        throw std::exception( "SoundEffectInstance::Apply3D" );
    }

    DWORD dwCalcFlags = X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER | X3DAUDIO_CALCULATE_LPF_DIRECT;

    if ( mFlags & SoundEffectInstance_UseRedirectLFE )
    {
        // On devices with an LFE channel, allow the mono source data to be routed to the LFE destination channel.
        dwCalcFlags |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE;
    }

    auto reverb = mReverbVoice;
    if ( reverb )
    {
        dwCalcFlags |= X3DAUDIO_CALCULATE_LPF_REVERB | X3DAUDIO_CALCULATE_REVERB;
    }

    float matrix[ XAUDIO2_MAX_AUDIO_CHANNELS * 8 ];
    assert( mDSPSettings.SrcChannelCount <= XAUDIO2_MAX_AUDIO_CHANNELS );
    assert( mDSPSettings.DstChannelCount <= 8 );
    mDSPSettings.pMatrixCoefficients = matrix;

    X3DAudioCalculate( mEngine->Get3DHandle(), &listener, &emitter, dwCalcFlags, &mDSPSettings );

    mDSPSettings.pMatrixCoefficients = nullptr;

    auto voice = mVoice;
    voice->SetFrequencyRatio( mDSPSettings.DopplerFactor );

    auto direct = mDirectVoice;
    voice->SetOutputMatrix( direct, mDSPSettings.SrcChannelCount, mDSPSettings.DstChannelCount, matrix );

    if ( reverb )
    {
        voice->SetOutputMatrix( reverb, 1, 1, &mDSPSettings.ReverbLevel );
    }

    if ( mFlags & SoundEffectInstance_ReverbUseFilters )
    {
        XAUDIO2_FILTER_PARAMETERS filterDirect = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI/6.0f * mDSPSettings.LPFDirectCoefficient), 1.0f };
        // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
        voice->SetOutputFilterParameters( direct, &filterDirect );

        if ( reverb )
        {
            XAUDIO2_FILTER_PARAMETERS filterReverb = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI/6.0f * mDSPSettings.LPFReverbCoefficient), 1.0f };
            // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
            voice->SetOutputFilterParameters( reverb, &filterReverb );
        }
    }
}


SoundEffectInstance::SoundState SoundEffectInstance::Impl::GetState()
{
    if ( !mVoice || mState != PLAYING )
        return mState;

	XAUDIO2_VOICE_STATE state;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
	mVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED );
#else
	mVoice->GetState(&state);
#endif

    if ( !state.BuffersQueued )
    {
        // Automatic stop if the buffer has finished playing
        mVoice->Stop();
        mState = STOPPED;
    }

    return mState;
}


//--------------------------------------------------------------------------------------
// SoundEffectInstance
//--------------------------------------------------------------------------------------

// Private constructors
SoundEffectInstance::SoundEffectInstance( AudioEngine* engine, SoundEffect* effect, SOUND_EFFECT_INSTANCE_FLAGS flags ) :
    pImpl( new Impl( engine, effect, flags ) )
{
}

SoundEffectInstance::SoundEffectInstance( AudioEngine* engine, WaveBank* waveBank, uint32_t index, SOUND_EFFECT_INSTANCE_FLAGS flags ) :
    pImpl( new Impl( engine, waveBank, index, flags ) )
{
}


// Move constructor.
SoundEffectInstance::SoundEffectInstance(SoundEffectInstance&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
SoundEffectInstance& SoundEffectInstance::operator= (SoundEffectInstance&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
SoundEffectInstance::~SoundEffectInstance()
{
    if( pImpl )
    {
        if ( pImpl->mWaveBank )
        {
            pImpl->mWaveBank->UnregisterInstance( this );
            pImpl->mWaveBank = nullptr;
        }

        if ( pImpl->mEffect )
        {
            pImpl->mEffect->UnregisterInstance( this );
            pImpl->mEffect = nullptr;
        }
    }
}


// Public methods.
void SoundEffectInstance::Play( bool loop )
{
    pImpl->Play( loop );
}


void SoundEffectInstance::Stop( bool immediate )
{
    pImpl->Stop( immediate );
}


void SoundEffectInstance::Pause()
{
    pImpl->Pause();
}


void SoundEffectInstance::SetVolume( float volume )
{
    if ( pImpl->mVoice )
    {
        pImpl->mVoice->SetVolume( volume );
    }
}


void SoundEffectInstance::SetPitch( float pitch )
{
    if ( pImpl->mVoice )
    {
        pImpl->mVoice->SetFrequencyRatio( pitch );
    }
}


void SoundEffectInstance::SetPan( float pan )
{
    pImpl->SetPan( pan );
}


void SoundEffectInstance::Apply3D( const AudioListener& listener, const AudioEmitter& emitter )
{
    pImpl->Apply3D( listener, emitter );
}


// Public accessors.
bool SoundEffectInstance::IsLooped() const
{
    return pImpl->mLooped;
}

SoundEffectInstance::SoundState SoundEffectInstance::GetState()
{
    return pImpl->GetState();
}


// Notifications.
void SoundEffectInstance::OnDestroyParent()
{
    pImpl->Halt();
    pImpl->mWaveBank = nullptr;
    pImpl->mEffect = nullptr;
}