//--------------------------------------------------------------------------------------
// File: WaveBank.cpp
//
// Copyright (c) Microsoft Corporation.
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
        assert(mEngine != nullptr);
        mEngine->RegisterNotify(this, false);
    }

    Impl(Impl&&) = default;
    Impl& operator= (Impl&&) = default;

    Impl(Impl const&) = delete;
    Impl& operator= (Impl const&) = delete;

    ~Impl() override
    {
        if (!mInstances.empty())
        {
            DebugTrace("WARNING: Destroying WaveBank \"%hs\" with %zu outstanding instances\n",
                mReader.BankName(), mInstances.size());

            for (auto it : mInstances)
            {
                assert(it != nullptr);
                it->OnDestroyParent();
            }

            mInstances.clear();
        }

        if (mOneShots > 0)
        {
            DebugTrace("WARNING: Destroying WaveBank \"%hs\" with %u outstanding one shot effects\n",
                mReader.BankName(), mOneShots);
        }

        if (mEngine)
        {
            mEngine->UnregisterNotify(this, true, false);
            mEngine = nullptr;
        }
    }

    HRESULT Initialize(_In_ AudioEngine* engine, _In_z_ const wchar_t* wbFileName) noexcept;

    void Play(unsigned int index, float volume, float pitch, float pan);

    // IVoiceNotify
    void __cdecl OnBufferEnd() override
    {
        InterlockedDecrement(&mOneShots);
    }

    void __cdecl OnCriticalError() override
    {
        mOneShots = 0;
    }

    void __cdecl OnReset() override
    {
        // No action required
    }

    void __cdecl OnUpdate() override
    {
        // We do not register for update notification
        assert(false);
    }

    void __cdecl OnDestroyEngine() noexcept override
    {
        mEngine = nullptr;
        mOneShots = 0;
    }

    void __cdecl OnTrim() override
    {
        // No action required
    }

    void __cdecl GatherStatistics(AudioStatistics& stats) const noexcept override
    {
        stats.playingOneShots += mOneShots;

        if (!mStreaming)
        {
            stats.audioBytes += mReader.BankAudioSize();

        #ifdef DIRECTX_ENABLE_XMA2
            if (mReader.HasXMA())
                stats.xmaAudioBytes += mReader.BankAudioSize();
        #endif
        }
    }

    void __cdecl OnDestroyParent() noexcept override
    {
    }

    AudioEngine*                        mEngine;
    std::list<IVoiceNotify*>            mInstances;
    WaveBankReader                      mReader;
    uint32_t                            mOneShots;
    bool                                mPrepared;
    bool                                mStreaming;
};


_Use_decl_annotations_
HRESULT WaveBank::Impl::Initialize(AudioEngine* engine, const wchar_t* wbFileName) noexcept
{
    if (!engine || !wbFileName)
        return E_INVALIDARG;

    HRESULT hr = mReader.Open(wbFileName);
    if (FAILED(hr))
        return hr;

    mStreaming = mReader.IsStreamingBank();

    return S_OK;
}


void WaveBank::Impl::Play(unsigned int index, float volume, float pitch, float pan)
{
    assert(volume >= -XAUDIO2_MAX_VOLUME_LEVEL && volume <= XAUDIO2_MAX_VOLUME_LEVEL);
    assert(pitch >= -1.f && pitch <= 1.f);
    assert(pan >= -1.f && pan <= 1.f);

    if (mStreaming)
    {
        DebugTrace("ERROR: One-shots can only be created from an in-memory wave bank\n");
        throw std::runtime_error("WaveBank::Play");
    }

    if (index >= mReader.Count())
    {
        DebugTrace("WARNING: Index %u not found in wave bank with only %u entries, one-shot not triggered\n",
            index, mReader.Count());
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

    #ifdef DIRECTX_ENABLE_XWMA

    XAUDIO2_BUFFER_WMA wmaBuffer = {};

    uint32_t tag;
    hr = mReader.GetSeekTable(index, &wmaBuffer.pDecodedPacketCumulativeBytes, wmaBuffer.PacketCount, tag);
    ThrowIfFailed(hr);

    if (tag == WAVE_FORMAT_WMAUDIO2 || tag == WAVE_FORMAT_WMAUDIO3)
    {
        hr = voice->SubmitSourceBuffer(&buffer, &wmaBuffer);
    }
    else
    #endif // xWMA
    {
        hr = voice->SubmitSourceBuffer(&buffer, nullptr);
    }
    if (FAILED(hr))
    {
        DebugTrace("ERROR: WaveBank failed (%08X) when submitting buffer:\n", static_cast<unsigned int>(hr));
        DebugTrace("\tFormat Tag %u, %u channels, %u-bit, %u Hz, %u bytes\n",
            wfx->wFormatTag, wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, metadata.lengthBytes);
        throw std::runtime_error("SubmitSourceBuffer");
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
        DebugTrace("ERROR: WaveBank failed (%08X) to intialize from .xwb file \"%ls\"\n",
            static_cast<unsigned int>(hr), wbFileName);
        throw std::runtime_error("WaveBank");
    }

    DebugTrace("INFO: WaveBank \"%hs\" with %u entries loaded from .xwb file \"%ls\"\n",
               pImpl->mReader.BankName(), pImpl->mReader.Count(), wbFileName);
}


WaveBank::WaveBank(WaveBank&&) noexcept = default;
WaveBank& WaveBank::operator= (WaveBank&&) noexcept = default;
WaveBank::~WaveBank() = default;


// Public methods (one-shots)
void WaveBank::Play(unsigned int index)
{
    pImpl->Play(index, 1.f, 0.f, 0.f);
}


void WaveBank::Play(unsigned int index, float volume, float pitch, float pan)
{
    pImpl->Play(index, volume, pitch, pan);
}


void WaveBank::Play(_In_z_ const char* name)
{
    unsigned int index = pImpl->mReader.Find(name);
    if (index == unsigned(-1))
    {
        DebugTrace("WARNING: Name '%hs' not found in wave bank, one-shot not triggered\n", name);
        return;
    }

    pImpl->Play(index, 1.f, 0.f, 0.f);
}


void WaveBank::Play(_In_z_ const char* name, float volume, float pitch, float pan)
{
    unsigned int index = pImpl->mReader.Find(name);
    if (index == unsigned(-1))
    {
        DebugTrace("WARNING: Name '%hs' not found in wave bank, one-shot not triggered\n", name);
        return;
    }

    pImpl->Play(index, volume, pitch, pan);
}


// Public methods (sound effect instance)
std::unique_ptr<SoundEffectInstance> WaveBank::CreateInstance(unsigned int index, SOUND_EFFECT_INSTANCE_FLAGS flags)
{
    auto& wb = pImpl->mReader;

    if (pImpl->mStreaming)
    {
        DebugTrace("ERROR: SoundEffectInstances can only be created from an in-memory wave bank\n");
        throw std::runtime_error("WaveBank::CreateInstance");
    }

    if (index >= wb.Count())
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
    assert(effect != nullptr);
    pImpl->mInstances.emplace_back(effect->GetVoiceNotify());
    return std::unique_ptr<SoundEffectInstance>(effect);
}


std::unique_ptr<SoundEffectInstance> WaveBank::CreateInstance(_In_z_ const char* name, SOUND_EFFECT_INSTANCE_FLAGS flags)
{
    unsigned int index = pImpl->mReader.Find(name);
    if (index == unsigned(-1))
    {
        // We don't throw an exception here as titles often simply ignore missing assets rather than fail
        return std::unique_ptr<SoundEffectInstance>();
    }

    return CreateInstance(index, flags);
}


// Public methods (sound stream instance)
std::unique_ptr<SoundStreamInstance> WaveBank::CreateStreamInstance(unsigned int index, SOUND_EFFECT_INSTANCE_FLAGS flags)
{
    auto& wb = pImpl->mReader;

    if (!pImpl->mStreaming)
    {
        DebugTrace("ERROR: SoundStreamInstances can only be created from a streaming wave bank\n");
        throw std::runtime_error("WaveBank::CreateStreamInstance");
    }

    if (index >= wb.Count())
    {
        // We don't throw an exception here as titles often simply ignore missing assets rather than fail
        return std::unique_ptr<SoundStreamInstance>();
    }

    if (!pImpl->mPrepared)
    {
        wb.WaitOnPrepare();
        pImpl->mPrepared = true;
    }

    auto effect = new SoundStreamInstance(pImpl->mEngine, this, index, flags);
    assert(effect != nullptr);
    pImpl->mInstances.emplace_back(effect->GetVoiceNotify());
    return std::unique_ptr<SoundStreamInstance>(effect);
}


std::unique_ptr<SoundStreamInstance> WaveBank::CreateStreamInstance(_In_z_ const char* name, SOUND_EFFECT_INSTANCE_FLAGS flags)
{
    unsigned int index = pImpl->mReader.Find(name);
    if (index == unsigned(-1))
    {
        // We don't throw an exception here as titles often simply ignore missing assets rather than fail
        return std::unique_ptr<SoundStreamInstance>();
    }

    return CreateStreamInstance(index, flags);
}


void WaveBank::UnregisterInstance(_In_ IVoiceNotify* instance)
{
    auto it = std::find(pImpl->mInstances.begin(), pImpl->mInstances.end(), instance);
    if (it == pImpl->mInstances.end())
        return;

    pImpl->mInstances.erase(it);
}


// Public accessors.
bool WaveBank::IsPrepared() const noexcept
{
    if (pImpl->mPrepared)
        return true;

    if (!pImpl->mReader.IsPrepared())
        return false;

    pImpl->mPrepared = true;
    return true;
}


bool WaveBank::IsInUse() const noexcept
{
    return (pImpl->mOneShots > 0) || !pImpl->mInstances.empty();
}


bool WaveBank::IsStreamingBank() const noexcept
{
    return pImpl->mReader.IsStreamingBank();
}


size_t WaveBank::GetSampleSizeInBytes(unsigned int index) const noexcept
{
    if (index >= pImpl->mReader.Count())
        return 0;

    WaveBankReader::Metadata metadata;
    HRESULT hr = pImpl->mReader.GetMetadata(index, metadata);
    if (FAILED(hr))
        return 0;

    return metadata.lengthBytes;
}


size_t WaveBank::GetSampleDuration(unsigned int index) const noexcept
{
    if (index >= pImpl->mReader.Count())
        return 0;

    WaveBankReader::Metadata metadata;
    HRESULT hr = pImpl->mReader.GetMetadata(index, metadata);
    if (FAILED(hr))
        return 0;

    return metadata.duration;
}


size_t WaveBank::GetSampleDurationMS(unsigned int index) const noexcept
{
    if (index >= pImpl->mReader.Count())
        return 0;

    char buff[64] = {};
    auto wfx = reinterpret_cast<WAVEFORMATEX*>(buff);
    HRESULT hr = pImpl->mReader.GetFormat(index, wfx, sizeof(buff));
    if (FAILED(hr))
        return 0;

    WaveBankReader::Metadata metadata;
    hr = pImpl->mReader.GetMetadata(index, metadata);
    if (FAILED(hr))
        return 0;

    return static_cast<size_t>((uint64_t(metadata.duration) * 1000) / wfx->nSamplesPerSec);
}


_Use_decl_annotations_
const WAVEFORMATEX* WaveBank::GetFormat(unsigned int index, WAVEFORMATEX* wfx, size_t maxsize) const noexcept
{
    if (index >= pImpl->mReader.Count())
        return nullptr;

    HRESULT hr = pImpl->mReader.GetFormat(index, wfx, maxsize);
    if (FAILED(hr))
        return nullptr;

    return wfx;
}


_Use_decl_annotations_
int WaveBank::Find(const char* name) const
{
    return static_cast<int>(pImpl->mReader.Find(name));
}


#ifdef DIRECTX_ENABLE_XWMA

_Use_decl_annotations_
bool WaveBank::FillSubmitBuffer(unsigned int index, XAUDIO2_BUFFER& buffer, XAUDIO2_BUFFER_WMA& wmaBuffer) const
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

#else // !xWMA

_Use_decl_annotations_
void WaveBank::FillSubmitBuffer(unsigned int index, XAUDIO2_BUFFER& buffer) const
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


HANDLE WaveBank::GetAsyncHandle() const noexcept
{
    if (pImpl)
    {
        return pImpl->mReader.GetAsyncHandle();
    }

    return nullptr;
}


_Use_decl_annotations_
bool WaveBank::GetPrivateData(unsigned int index, void* data, size_t datasize)
{
    if (index >= pImpl->mReader.Count())
        return false;

    if (!data)
        return false;

    switch (datasize)
    {
        case sizeof(WaveBankReader::Metadata):
        {
            auto ptr = reinterpret_cast<WaveBankReader::Metadata*>(data);
            return SUCCEEDED(pImpl->mReader.GetMetadata(index, *ptr));
        }

        case sizeof(WaveBankSeekData):
        {
            auto ptr = reinterpret_cast<WaveBankSeekData*>(data);
            return SUCCEEDED(pImpl->mReader.GetSeekTable(index, &ptr->seekTable, ptr->seekCount, ptr->tag));
        }

        default:
            return false;
    }
}
