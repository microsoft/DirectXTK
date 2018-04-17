//--------------------------------------------------------------------------------------
// File: WaveBank.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Audio.h"
#include "WaveBankReader.h"
#include "SoundCommon.h"
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
    explicit Impl(_In_ AudioEngine* engine) :
        mEngine(engine),
        mOneShots(0),
        mPrepared(false),
        mStreaming(false)
    {
        assert(mEngine != 0);
        mEngine->RegisterNotify(this, false);
    }

    virtual ~Impl()
    {
        if (!mInstances.empty())
        {
            DebugTrace("WARNING: Destroying WaveBank \"%hs\" with %Iu outstanding SoundEffectInstances\n", mReader.BankName(), mInstances.size());

            for (auto it = mInstances.begin(); it != mInstances.end(); ++it)
            {
                assert(*it != 0);
                (*it)->OnDestroyParent();
            }

            mInstances.clear();
        }

        if (mOneShots > 0)
        {
            DebugTrace("WARNING: Destroying WaveBank \"%hs\" with %u outstanding one shot effects\n", mReader.BankName(), mOneShots);
        }

        if (mEngine)
        {
            mEngine->UnregisterNotify(this, true, false);
            mEngine = nullptr;
        }
    }

    HRESULT Initialize(_In_ AudioEngine* engine, _In_z_ const wchar_t* wbFileName);

    void Play(int index, float volume, float pitch, float pan);

    // IVoiceNotify
    virtual void __cdecl OnBufferEnd() override
    {
        InterlockedDecrement(&mOneShots);
    }

    virtual void __cdecl OnCriticalError() override
    {
        mOneShots = 0;
    }

    virtual void __cdecl OnReset() override
    {
        // No action required
    }

    virtual void __cdecl OnUpdate() override
    {
        // We do not register for update notification
        assert(false);
    }

    virtual void __cdecl OnDestroyEngine() override
    {
        mEngine = nullptr;
        mOneShots = 0;
    }

    virtual void __cdecl OnTrim() override
    {
        // No action required
    }

    virtual void __cdecl GatherStatistics(AudioStatistics& stats) const override
    {
        stats.playingOneShots += mOneShots;

        if (!mStreaming)
        {
            stats.audioBytes += mReader.BankAudioSize();

        #if defined(_XBOX_ONE) && defined(_TITLE)
            if (mReader.HasXMA())
                stats.xmaAudioBytes += mReader.BankAudioSize();
        #endif
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
HRESULT WaveBank::Impl::Initialize(AudioEngine* engine, const wchar_t* wbFileName)
{
    if (!engine || !wbFileName)
        return E_INVALIDARG;

    HRESULT hr = mReader.Open(wbFileName);
    if (FAILED(hr))
        return hr;

    mStreaming = mReader.IsStreamingBank();

    return S_OK;
}


void WaveBank::Impl::Play(int index, float volume, float pitch, float pan)
{
    assert(volume >= -XAUDIO2_MAX_VOLUME_LEVEL && volume <= XAUDIO2_MAX_VOLUME_LEVEL);
    assert(pitch >= -1.f && pitch <= 1.f);
    assert(pan >= -1.f && pan <= 1.f);

    if (mStreaming)
    {
        DebugTrace("ERROR: One-shots can only be created from an in-memory wave bank\n");
        throw std::exception("WaveBank::Play");
    }

    if (index < 0 || uint32_t(index) >= mReader.Count())
    {
        DebugTrace("WARNING: Index %d not found in wave bank with only %u entries, one-shot not triggered\n", index, mReader.Count());
        return;
    }

    if (!mPrepared)
    {
        mReader.WaitOnPrepare();
        mPrepared = true;
    }

    char wfxbuff[64] = {};
    auto wfx = reinterpret_cast<WAVEFORMATEX*>(wfxbuff);
    HRESULT hr = mReader.GetFormat(index, wfx, sizeof(wfxbuff));
    ThrowIfFailed(hr);

    IXAudio2SourceVoice* voice = nullptr;
    mEngine->AllocateVoice(wfx, SoundEffectInstance_Default, true, &voice);

    if (!voice)
        return;

    if (volume != 1.f)
    {
        hr = voice->SetVolume(volume);
        ThrowIfFailed(hr);
    }

    if (pitch != 0.f)
    {
        float fr = XAudio2SemitonesToFrequencyRatio(pitch * 12.f);

        hr = voice->SetFrequencyRatio(fr);
        ThrowIfFailed(hr);
    }

    if (pan != 0.f)
    {
        float matrix[16];
        if (ComputePan(pan, wfx->nChannels, matrix))
        {
            hr = voice->SetOutputMatrix(nullptr, wfx->nChannels, mEngine->GetOutputChannels(), matrix);
            ThrowIfFailed(hr);
        }
    }

    hr = voice->Start(0);
    ThrowIfFailed(hr);

    XAUDIO2_BUFFER buffer = {};
    hr = mReader.GetWaveData(index, &buffer.pAudioData, buffer.AudioBytes);
    ThrowIfFailed(hr);

    WaveBankReader::Metadata metadata;
    hr = mReader.GetMetadata(index, metadata);
    ThrowIfFailed(hr);

    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.pContext = this;

#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

    XAUDIO2_BUFFER_WMA wmaBuffer = {};

    uint32_t tag;
    hr = mReader.GetSeekTable(index, &wmaBuffer.pDecodedPacketCumulativeBytes, wmaBuffer.PacketCount, tag);
    ThrowIfFailed(hr);

    if (tag == WAVE_FORMAT_WMAUDIO2 || tag == WAVE_FORMAT_WMAUDIO3)
    {
        hr = voice->SubmitSourceBuffer(&buffer, &wmaBuffer);
    }
    else
    #endif
    {
        hr = voice->SubmitSourceBuffer(&buffer, nullptr);
    }
    if (FAILED(hr))
    {
        DebugTrace("ERROR: WaveBank failed (%08X) when submitting buffer:\n", hr);
        DebugTrace("\tFormat Tag %u, %u channels, %u-bit, %u Hz, %u bytes\n", wfx->wFormatTag,
                   wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, metadata.lengthBytes);
        throw std::exception("SubmitSourceBuffer");
    }

    InterlockedIncrement(&mOneShots);
}


//--------------------------------------------------------------------------------------
// WaveBank
//--------------------------------------------------------------------------------------

// Public constructors.
_Use_decl_annotations_
WaveBank::WaveBank(AudioEngine* engine, const wchar_t* wbFileName)
    : pImpl(std::make_unique<Impl>(engine))
{
    HRESULT hr = pImpl->Initialize(engine, wbFileName);
    if (FAILED(hr))
    {
        DebugTrace("ERROR: WaveBank failed (%08X) to intialize from .xwb file \"%ls\"\n", hr, wbFileName);
        throw std::exception("WaveBank");
    }

    DebugTrace("INFO: WaveBank \"%hs\" with %u entries loaded from .xwb file \"%ls\"\n",
               pImpl->mReader.BankName(), pImpl->mReader.Count(), wbFileName);
}


// Move constructor.
WaveBank::WaveBank(WaveBank&& moveFrom) throw()
    : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
WaveBank& WaveBank::operator= (WaveBank&& moveFrom) throw()
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
WaveBank::~WaveBank()
{
}


// Public methods.
void WaveBank::Play(int index)
{
    pImpl->Play(index, 1.f, 0.f, 0.f);
}


void WaveBank::Play(int index, float volume, float pitch, float pan)
{
    pImpl->Play(index, volume, pitch, pan);
}


void WaveBank::Play(_In_z_ const char* name)
{
    int index = static_cast<int>(pImpl->mReader.Find(name));
    if (index == -1)
    {
        DebugTrace("WARNING: Name '%hs' not found in wave bank, one-shot not triggered\n", name);
        return;
    }

    pImpl->Play(index, 1.f, 0.f, 0.f);
}


void WaveBank::Play(_In_z_ const char* name, float volume, float pitch, float pan)
{
    int index = static_cast<int>(pImpl->mReader.Find(name));
    if (index == -1)
    {
        DebugTrace("WARNING: Name '%hs' not found in wave bank, one-shot not triggered\n", name);
        return;
    }

    pImpl->Play(index, volume, pitch, pan);
}


std::unique_ptr<SoundEffectInstance> WaveBank::CreateInstance(int index, SOUND_EFFECT_INSTANCE_FLAGS flags)
{
    auto& wb = pImpl->mReader;

    if (pImpl->mStreaming)
    {
        DebugTrace("ERROR: SoundEffectInstances can only be created from an in-memory wave bank\n");
        throw std::exception("WaveBank::CreateInstance");
    }

    if (index < 0 || uint32_t(index) >= wb.Count())
    {
        // We don't throw an exception here as titles often simply ignore missing assets rather than fail
        return std::unique_ptr<SoundEffectInstance>();
    }

    if (!pImpl->mPrepared)
    {
        wb.WaitOnPrepare();
        pImpl->mPrepared = true;
    }

    auto effect = new SoundEffectInstance(pImpl->mEngine, this, index, flags);
    assert(effect != 0);
    pImpl->mInstances.emplace_back(effect);
    return std::unique_ptr<SoundEffectInstance>(effect);
}


std::unique_ptr<SoundEffectInstance> WaveBank::CreateInstance(_In_z_ const char* name, SOUND_EFFECT_INSTANCE_FLAGS flags)
{
    int index = static_cast<int>(pImpl->mReader.Find(name));
    if (index == -1)
    {
        // We don't throw an exception here as titles often simply ignore missing assets rather than fail
        return std::unique_ptr<SoundEffectInstance>();
    }

    return CreateInstance(index, flags);
}


void WaveBank::UnregisterInstance(_In_ SoundEffectInstance* instance)
{
    auto it = std::find(pImpl->mInstances.begin(), pImpl->mInstances.end(), instance);
    if (it == pImpl->mInstances.end())
        return;

    pImpl->mInstances.erase(it);
}


// Public accessors.
bool WaveBank::IsPrepared() const
{
    if (pImpl->mPrepared)
        return true;

    if (!pImpl->mReader.IsPrepared())
        return false;

    pImpl->mPrepared = true;
    return true;
}


bool WaveBank::IsInUse() const
{
    return (pImpl->mOneShots > 0) || !pImpl->mInstances.empty();
}


bool WaveBank::IsStreamingBank() const
{
    return pImpl->mReader.IsStreamingBank();
}


size_t WaveBank::GetSampleSizeInBytes(int index) const
{
    if (index < 0 || uint32_t(index) >= pImpl->mReader.Count())
        return 0;

    WaveBankReader::Metadata metadata;
    HRESULT hr = pImpl->mReader.GetMetadata(index, metadata);
    ThrowIfFailed(hr);
    return metadata.lengthBytes;
}


size_t WaveBank::GetSampleDuration(int index) const
{
    if (index < 0 || uint32_t(index) >= pImpl->mReader.Count())
        return 0;

    WaveBankReader::Metadata metadata;
    HRESULT hr = pImpl->mReader.GetMetadata(index, metadata);
    ThrowIfFailed(hr);
    return metadata.duration;
}


size_t WaveBank::GetSampleDurationMS(int index) const
{
    if (index < 0 || uint32_t(index) >= pImpl->mReader.Count())
        return 0;

    char buff[64] = {};
    auto wfx = reinterpret_cast<WAVEFORMATEX*>(buff);
    HRESULT hr = pImpl->mReader.GetFormat(index, wfx, sizeof(buff));
    ThrowIfFailed(hr);

    WaveBankReader::Metadata metadata;
    hr = pImpl->mReader.GetMetadata(index, metadata);
    ThrowIfFailed(hr);
    return static_cast<size_t>((uint64_t(metadata.duration) * 1000) / wfx->nSamplesPerSec);
}


_Use_decl_annotations_
const WAVEFORMATEX* WaveBank::GetFormat(int index, WAVEFORMATEX* wfx, size_t maxsize) const
{
    if (index < 0 || uint32_t(index) >= pImpl->mReader.Count())
        return nullptr;

    HRESULT hr = pImpl->mReader.GetFormat(index, wfx, maxsize);
    ThrowIfFailed(hr);
    return wfx;
}


_Use_decl_annotations_
int WaveBank::Find(const char* name) const
{
    return static_cast<int>(pImpl->mReader.Find(name));
}


#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

_Use_decl_annotations_
bool WaveBank::FillSubmitBuffer(int index, XAUDIO2_BUFFER& buffer, XAUDIO2_BUFFER_WMA& wmaBuffer) const
{
    memset(&buffer, 0, sizeof(buffer));
    memset(&wmaBuffer, 0, sizeof(wmaBuffer));

    HRESULT hr = pImpl->mReader.GetWaveData(index, &buffer.pAudioData, buffer.AudioBytes);
    ThrowIfFailed(hr);

    WaveBankReader::Metadata metadata;
    hr = pImpl->mReader.GetMetadata(index, metadata);
    ThrowIfFailed(hr);

    buffer.LoopBegin = metadata.loopStart;
    buffer.LoopLength = metadata.loopLength;

    uint32_t tag;
    hr = pImpl->mReader.GetSeekTable(index, &wmaBuffer.pDecodedPacketCumulativeBytes, wmaBuffer.PacketCount, tag);
    ThrowIfFailed(hr);

    return (tag == WAVE_FORMAT_WMAUDIO2 || tag == WAVE_FORMAT_WMAUDIO3);
}

#else

_Use_decl_annotations_
void WaveBank::FillSubmitBuffer(int index, XAUDIO2_BUFFER& buffer) const
{
    memset(&buffer, 0, sizeof(buffer));

    HRESULT hr = pImpl->mReader.GetWaveData(index, &buffer.pAudioData, buffer.AudioBytes);
    ThrowIfFailed(hr);

    WaveBankReader::Metadata metadata;
    hr = pImpl->mReader.GetMetadata(index, metadata);
    ThrowIfFailed(hr);

    buffer.LoopBegin = metadata.loopStart;
    buffer.LoopLength = metadata.loopLength;
}

#endif
