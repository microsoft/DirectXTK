//--------------------------------------------------------------------------------------
// File: WaveBank.cpp
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
#include "WaveBankReader.h"
#include "PlatformHelpers.h"

#include <list>

using namespace DirectX;


//======================================================================================
// WaveBank
//======================================================================================

// Internal object implementation class.
class WaveBank::Impl : public IVoiceNotify
{
public:
    explicit Impl( _In_ AudioEngine* engine ) :
        mEngine( engine ),
        mOneShots( 0 ),
        mPrepared( false ),
        mStreaming( false )
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
            sprintf_s( buff, "WARNING: Destroying WaveBank \"%s\" with %Iu outstanding SoundEffectInstances\n", mReader.BankName(), mInstances.size() );
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
            sprintf_s( buff, "WARNING: Destroying WaveBank \"%s\" with %u outstanding one shot effects\n", mReader.BankName(), mOneShots );
            OutputDebugStringA( buff );
#endif
        }

        if ( mEngine )
        {
            mEngine->UnregisterNotify( this, true );
            mEngine = nullptr;
        }
    }

    HRESULT Initialize( _In_ AudioEngine* engine, _In_z_ const wchar_t* wbFileName );

    void Play( uint32_t index );

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

        if ( !mStreaming )
        {
            stats.audioBytes += mReader.BankAudioSize();
        }
    }

    AudioEngine*                        mEngine;
    std::list<SoundEffectInstance*>     mInstances;
    WaveBankReader                      mReader;
    uint32_t                            mOneShots;
    bool                                mPrepared;
    bool                                mStreaming;
};


_Use_decl_annotations_
HRESULT WaveBank::Impl::Initialize( AudioEngine* engine, const wchar_t* wbFileName )
{
    if ( !engine || !wbFileName )
        return E_INVALIDARG;

    HRESULT hr = mReader.Open( wbFileName );
    if ( FAILED(hr) )
        return hr;

    mStreaming = mReader.IsStreamingBank();

    return S_OK;
}


void WaveBank::Impl::Play( uint32_t index )
{
    if ( mStreaming )
    {
#ifdef _DEBUG
        OutputDebugStringA( "ERROR: One-shots can only be created from an in-memory wave bank\n");
#endif
        throw std::exception( "WaveBank::Play" );
    }

    if ( index >= mReader.Count() )
    {
#ifndef NDEBUG
        char buff[128];
        sprintf_s( buff, "WARNING: Index %u not found in wave bank with only %u entries, one-shot not triggered\n", index, mReader.Count() );
        OutputDebugStringA( buff );
#endif
        return;
    }

    if ( !mPrepared )
    {
        mReader.WaitOnPrepare();
        mPrepared = true;
    }

    char wfxbuff[64];
    auto wfx = reinterpret_cast<WAVEFORMATEX*>( wfxbuff );
    HRESULT hr = mReader.GetFormat( index, wfx, 64 );
    ThrowIfFailed( hr );

    IXAudio2SourceVoice* voice = nullptr;
    mEngine->AllocateVoice( wfx, SoundEffectInstance_Default, true, &voice );

    if ( !voice )
        return;

    hr = voice->Start( 0 );
    ThrowIfFailed( hr );

    XAUDIO2_BUFFER buffer;
    memset( &buffer, 0, sizeof(buffer) );

    hr = mReader.GetWaveData( index, &buffer.pAudioData, buffer.AudioBytes );
    ThrowIfFailed( hr );

    WaveBankReader::Metadata metadata;
    hr = mReader.GetMetadata( index, metadata );
    ThrowIfFailed( hr );

    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.pContext = this;

    hr = voice->SubmitSourceBuffer( &buffer, nullptr );
    if ( FAILED(hr) )
    {
#ifdef _DEBUG
        char buff[256];
        sprintf_s( buff, "ERROR: WaveBank failed (%08X) when submitting buffer:\n", hr );
        OutputDebugStringA( buff );
        
        sprintf_s( buff, "\tFormat Tag %u, %u channels, %u-bit, %u Hz, %u bytes\n", wfx->wFormatTag, 
                    wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, metadata.lengthBytes );
        OutputDebugStringA( buff );
#endif
        throw std::exception( "SubmitSourceBuffer" );
    }

    InterlockedIncrement( &mOneShots );
}


//--------------------------------------------------------------------------------------
// WaveBank
//--------------------------------------------------------------------------------------

// Public constructors.
_Use_decl_annotations_
WaveBank::WaveBank( AudioEngine* engine, const wchar_t* wbFileName )
  : pImpl(new Impl(engine) )
{
    HRESULT hr = pImpl->Initialize( engine, wbFileName );
    if ( FAILED(hr) )
    {
#ifdef _DEBUG
        char buff[1024];
        sprintf_s( buff, "ERROR: WaveBank failed (%08X) to intialize from .xwb file \"%S\"\n", hr, wbFileName );
        OutputDebugStringA( buff );
#endif
        throw std::exception( "WaveBank" );
    }

#ifdef _DEBUG
    char buff[1024];
    sprintf_s( buff, "INFO: WaveBank \"%s\" with %u entries loaded from .xwb file \"%S\"\n",
                pImpl->mReader.BankName(), pImpl->mReader.Count(), wbFileName );
    OutputDebugStringA( buff );
#endif
}


// Move constructor.
WaveBank::WaveBank(WaveBank&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
WaveBank& WaveBank::operator= (WaveBank&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
WaveBank::~WaveBank()
{
}


// Public methods.
void WaveBank::Play( uint32_t index )
{
    pImpl->Play( index );
}


void WaveBank::Play( _In_z_ const char* name )
{
    uint32_t index = pImpl->mReader.Find( name );
    if ( index == uint32_t(-1) )
    {
#ifndef NDEBUG
        char buff[128];
        sprintf_s( buff, "WARNING: Name '%s' not found in wave bank, one-shot not triggered\n", name );
        OutputDebugStringA( buff );
#endif
        return;
    }

    pImpl->Play( index );
}


std::unique_ptr<SoundEffectInstance> WaveBank::CreateInstance( uint32_t index, SOUND_EFFECT_INSTANCE_FLAGS flags )
{
    auto& wb = pImpl->mReader;

    if ( pImpl->mStreaming )
    {
#ifdef _DEBUG
        OutputDebugStringA( "ERROR: SoundEffectInstances can only be created from an in-memory wave bank\n");
#endif
        throw std::exception( "WaveBank::CreateInstance" );
    }

    if ( index >= wb.Count() )
    {
        // We don't throw an exception here as titles often simply ignore missing assets rather than fail
        return std::unique_ptr<SoundEffectInstance>();
    }

    if ( !pImpl->mPrepared )
    {
        wb.WaitOnPrepare();
        pImpl->mPrepared = true;
    }

    auto effect = new SoundEffectInstance( pImpl->mEngine, this, index, flags );
    assert( effect != 0 );
    pImpl->mInstances.emplace_back( effect );
    return std::unique_ptr<SoundEffectInstance>( effect );
}


std::unique_ptr<SoundEffectInstance> WaveBank::CreateInstance( _In_z_ const char* name, SOUND_EFFECT_INSTANCE_FLAGS flags )
{
    uint32_t index = pImpl->mReader.Find( name );
    if ( index == uint32_t(-1) )
    {
        // We don't throw an exception here as titles often simply ignore missing assets rather than fail
        return std::unique_ptr<SoundEffectInstance>();
    }

    return CreateInstance( index, flags );
}


void WaveBank::UnregisterInstance( _In_ SoundEffectInstance* instance )
{
    auto it = std::find( pImpl->mInstances.begin(), pImpl->mInstances.end(), instance );
    if ( it == pImpl->mInstances.end() )
        return;

    pImpl->mInstances.erase( it );
}


// Public accessors.
bool WaveBank::IsPrepared() const
{
    if ( pImpl->mPrepared )
        return true;

    if ( !pImpl->mReader.IsPrepared() )
        return false;

    pImpl->mPrepared = true;
    return true;
}


bool WaveBank::IsInUse() const
{
    return ( pImpl->mOneShots > 0 ) || !pImpl->mInstances.empty();
}


bool WaveBank::IsStreamingBank() const
{
    return pImpl->mReader.IsStreamingBank();
}


uint32_t WaveBank::SampleSizeInBytes( uint32_t index ) const
{
    if ( index >= pImpl->mReader.Count() )
        return 0;

    WaveBankReader::Metadata metadata;
    HRESULT hr = pImpl->mReader.GetMetadata( index, metadata );
    ThrowIfFailed( hr );
    return metadata.lengthBytes;
}


uint32_t WaveBank::SampleDuration( uint32_t index ) const
{
    if ( index >= pImpl->mReader.Count() )
        return 0;

    WaveBankReader::Metadata metadata;
    HRESULT hr = pImpl->mReader.GetMetadata( index, metadata );
    ThrowIfFailed( hr );
    return metadata.duration;
}


_Use_decl_annotations_
const WAVEFORMATEX* WaveBank::GetFormat( uint32_t index, WAVEFORMATEX* wfx, size_t maxsize ) const
{
    if ( index >= pImpl->mReader.Count() )
        return nullptr;

    HRESULT hr = pImpl->mReader.GetFormat( index, wfx, maxsize );
    ThrowIfFailed( hr );
    return wfx;
}


_Use_decl_annotations_
uint32_t WaveBank::Find( const char* name ) const
{
    return pImpl->mReader.Find( name );
}


_Use_decl_annotations_
void WaveBank::FillSubmitBuffer( uint32_t index, XAUDIO2_BUFFER& buffer ) const
{
    memset( &buffer, 0, sizeof(buffer) );

    HRESULT hr = pImpl->mReader.GetWaveData( index, &buffer.pAudioData, buffer.AudioBytes );
    ThrowIfFailed( hr );

    WaveBankReader::Metadata metadata;
    hr = pImpl->mReader.GetMetadata( index, metadata );
    ThrowIfFailed( hr );
    buffer.LoopBegin = metadata.loopStart;
    buffer.LoopLength = metadata.loopLength;
}
