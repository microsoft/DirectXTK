//--------------------------------------------------------------------------------------
// File: SoundEffect.cpp
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

#include <list>

using namespace DirectX;


namespace
{
    uint32_t GetFormatTag( const WAVEFORMATEX* wfx )
    {
        if ( wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
        {
            if ( wfx->cbSize < 22 )
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
        else if ( wfx->wFormatTag == WAVE_FORMAT_ADPCM )
        {
            if ( wfx->cbSize < 32 )
                return 0;

            return WAVE_FORMAT_ADPCM;
        }
        else
        {
            return wfx->wFormatTag;
        }
    }
}


//======================================================================================
// SoundEffect
//======================================================================================

// Internal object implementation class.
class SoundEffect::Impl : public IVoiceNotify
{
public:
    explicit Impl( _In_ AudioEngine* engine ) :
        mWaveFormat( nullptr ),
        mStartAudio( nullptr ),
        mAudioBytes( 0 ),
        mLoopStart( 0 ),
        mLoopLength( 0 ),
        mEngine( engine ),
        mOneShots( 0 )
    {
        assert( mEngine != 0 );
        mEngine->RegisterNotify( this );
    }

    ~Impl()
    {
        if ( !mInstances.empty() )
        {
#ifdef _DEBUG
            char buff[256];
            sprintf_s( buff, "WARNING: Destroying SoundEffect with %Iu outstanding SoundEffectInstances\n", mInstances.size() );
            OutputDebugStringA( buff );
#endif

            for( auto it = mInstances.begin(); it != mInstances.end(); ++it )
            {
                assert( *it != 0 );
                (*it)->OnDestroyParent();
            }

            mInstances.clear();
        }

        if ( mOneShots > 0 )
        {
#ifdef _DEBUG
            char buff[256];
            sprintf_s( buff, "WARNING: Destroying SoundEffect with %u outstanding one shot effects\n", mOneShots );
            OutputDebugStringA( buff );
#endif
        }

        if ( mEngine )
        {
            mEngine->UnregisterNotify( this, true );
            mEngine = nullptr;
        }
    }

    HRESULT Initialize( _In_ AudioEngine* engine, _Inout_ std::unique_ptr<uint8_t[]>& wavData,
                        _In_ const WAVEFORMATEX* wfx, _In_ const uint8_t* startAudio,
                        uint32_t audioBytes, uint32_t loopStart, uint32_t loopLength );

    void Play();

    // IVoiceNotify
    virtual void OnBufferEnd() override
    {
        InterlockedDecrement( &mOneShots );
    }

    virtual void OnCriticalError() override
    {
        mOneShots = 0;
    }

    virtual void OnReset() override
    {
    }

    virtual void OnDestroyEngine() override
    {
        mEngine = nullptr;
        mOneShots = 0;
    }

    virtual void GatherStatistics( AudioStatistics& stats ) const override
    {
        stats.playingOneShots += mOneShots;
        stats.audioBytes += mAudioBytes;
    }

    const WAVEFORMATEX*                 mWaveFormat;
    const uint8_t*                      mStartAudio;
    uint32_t                            mAudioBytes;
    uint32_t                            mLoopStart;
    uint32_t                            mLoopLength;
    AudioEngine*                        mEngine;
    std::list<SoundEffectInstance*>     mInstances;
    uint32_t                            mOneShots;

private:
    std::unique_ptr<uint8_t[]>          mWavData;
};


_Use_decl_annotations_
HRESULT SoundEffect::Impl::Initialize( AudioEngine* engine, std::unique_ptr<uint8_t[]>& wavData,
                                       const WAVEFORMATEX* wfx, const uint8_t* startAudio,
                                       uint32_t audioBytes, uint32_t loopStart, uint32_t loopLength )
{
    if ( !engine || !wfx || !startAudio || !audioBytes || !wavData )
        return E_INVALIDARG;

    switch( GetFormatTag( wfx ) )
    {
    case WAVE_FORMAT_PCM:
    case WAVE_FORMAT_IEEE_FLOAT:
    case WAVE_FORMAT_ADPCM:
        break;

    default:
        {
#ifdef _DEBUG
            char buff[256];
            sprintf_s( buff, "ERROR: SoundEffect encountered an unsupported format tag (%u)\n", wfx->wFormatTag );
            OutputDebugStringA( buff );
#endif
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        }
    }

    // Take ownership of the buffer
    mWavData.reset( wavData.release() );

    // WARNING: We assume the wfx and startAudio parameters are pointers into the wavData memory buffer
    mWaveFormat = wfx;
    mStartAudio = startAudio;
    mAudioBytes = audioBytes;
    mLoopStart = loopStart;
    mLoopLength = loopLength;

    return S_OK;
}


void SoundEffect::Impl::Play()
{
    IXAudio2SourceVoice* voice = nullptr;
    mEngine->AllocateVoice( mWaveFormat, SoundEffectInstance_Default, true, &voice );
    
    if ( !voice )
        return;

    HRESULT hr = voice->Start( 0 );
    ThrowIfFailed( hr );

    XAUDIO2_BUFFER buffer;
    memset( &buffer, 0, sizeof(buffer) );
    buffer.AudioBytes = mAudioBytes;
    buffer.pAudioData = mStartAudio;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.pContext = this;

    hr = voice->SubmitSourceBuffer( &buffer, nullptr );
    if ( FAILED(hr) )
    {
#ifdef _DEBUG
        char buff[256];
        sprintf_s( buff, "ERROR: SoundEffect failed (%08X) when submitting buffer:\n", hr );
        OutputDebugStringA( buff );

        sprintf_s( buff, "\tFormat Tag %u, %u channels, %u-bit, %u Hz, %u bytes\n", mWaveFormat->wFormatTag, 
                   mWaveFormat->nChannels, mWaveFormat->wBitsPerSample, mWaveFormat->nSamplesPerSec, mAudioBytes );
        OutputDebugStringA( buff );
#endif
        throw std::exception( "SubmitSourceBuffer" );
    }

    InterlockedIncrement( &mOneShots );
}


//--------------------------------------------------------------------------------------
// SoundEffect
//--------------------------------------------------------------------------------------

// Public constructors.
_Use_decl_annotations_
SoundEffect::SoundEffect( AudioEngine* engine, const wchar_t* waveFileName )
  : pImpl(new Impl(engine) )
{
    WAVData wavInfo;
    std::unique_ptr<uint8_t[]> wavData;
    HRESULT hr = LoadWAVAudioFromFileEx( waveFileName, wavData, wavInfo );
    if ( FAILED(hr) )
    {
#ifdef _DEBUG
        char buff[1024];
        sprintf_s( buff, "ERROR: SoundEffect failed (%08X) to load from .wav file \"%S\"\n", hr, waveFileName );
        OutputDebugStringA( buff );
#endif
        throw std::exception( "SoundEffect" );
    }

    hr = pImpl->Initialize( engine, wavData, wavInfo.wfx, wavInfo.startAudio, wavInfo.audioBytes, wavInfo.loopStart, wavInfo.loopLength );
    if ( FAILED(hr) )
    {
#ifdef _DEBUG
        char buff[1024];
        sprintf_s( buff, "ERROR: SoundEffect failed (%08X) to intialize from .wav file \"%S\"\n", hr, waveFileName );
        OutputDebugStringA( buff );
#endif
        throw std::exception( "SoundEffect" );
    }
}


_Use_decl_annotations_
SoundEffect::SoundEffect( AudioEngine* engine, std::unique_ptr<uint8_t[]>& wavData, const WAVEFORMATEX* wfx, const uint8_t* startAudio, uint32_t audioBytes )
  : pImpl(new Impl(engine) )
{
    HRESULT hr = pImpl->Initialize( engine, wavData, wfx, startAudio, audioBytes, 0, 0 );
    if ( FAILED(hr) )
    {
#ifdef _DEBUG
        char buff[1024];
        sprintf_s( buff, "ERROR: SoundEffect failed (%08X) to intialize\n", hr );
        OutputDebugStringA( buff );
#endif
        throw std::exception( "SoundEffect" );
    }
}


_Use_decl_annotations_
SoundEffect::SoundEffect( AudioEngine* engine, std::unique_ptr<uint8_t[]>& wavData, const WAVEFORMATEX* wfx, const uint8_t* startAudio, uint32_t audioBytes,
                          uint32_t loopStart, uint32_t loopLength )
  : pImpl(new Impl(engine) )
{
    HRESULT hr = pImpl->Initialize( engine, wavData, wfx, startAudio, audioBytes, loopStart, loopLength );
    if ( FAILED(hr) )
    {
#ifdef _DEBUG
        char buff[1024];
        sprintf_s( buff, "ERROR: SoundEffect failed (%08X) to intialize\n", hr );
        OutputDebugStringA( buff );
#endif
        throw std::exception( "SoundEffect" );
    }
}


// Move constructor.
SoundEffect::SoundEffect(SoundEffect&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
SoundEffect& SoundEffect::operator= (SoundEffect&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
SoundEffect::~SoundEffect()
{
}


// Public methods.
void SoundEffect::Play()
{
    pImpl->Play();
}


std::unique_ptr<SoundEffectInstance> SoundEffect::CreateInstance( SOUND_EFFECT_INSTANCE_FLAGS flags )
{
    auto effect = new SoundEffectInstance( pImpl->mEngine, this, flags );
    assert( effect != 0 );
    pImpl->mInstances.emplace_back( effect );
    return std::unique_ptr<SoundEffectInstance>( effect );
}


void SoundEffect::UnregisterInstance( _In_ SoundEffectInstance* instance )
{
    auto it = std::find( pImpl->mInstances.begin(), pImpl->mInstances.end(), instance );
    if ( it == pImpl->mInstances.end() )
        return;

    pImpl->mInstances.erase( it );
}


// Public accessors.
bool SoundEffect::IsInUse() const
{
    return ( pImpl->mOneShots > 0 ) || !pImpl->mInstances.empty();
}


uint32_t SoundEffect::SampleSizeInBytes() const
{
    return pImpl->mAudioBytes;
}


uint32_t SoundEffect::SampleDuration() const
{
    if ( !pImpl->mWaveFormat )
        return 0;

    uint32_t bitsPerSample = ( GetFormatTag( pImpl->mWaveFormat ) == WAVE_FORMAT_ADPCM ) ? 4 : pImpl->mWaveFormat->wBitsPerSample;
    uint64_t div = uint64_t( bitsPerSample * pImpl->mWaveFormat->nChannels );
    return (div > 0) ? uint32_t( ( uint64_t(pImpl->mAudioBytes) * 8 ) / div ) : 0;
}


const WAVEFORMATEX* SoundEffect::GetFormat() const
{
    return pImpl->mWaveFormat;
}


void SoundEffect::FillSubmitBuffer( _Out_ XAUDIO2_BUFFER& buffer ) const
{
    memset( &buffer, 0, sizeof(buffer) );
    buffer.AudioBytes = pImpl->mAudioBytes;
    buffer.pAudioData = pImpl->mStartAudio;
    buffer.LoopBegin = pImpl->mLoopStart;
    buffer.LoopLength = pImpl->mLoopLength;
}
