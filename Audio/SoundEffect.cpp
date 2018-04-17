//--------------------------------------------------------------------------------------
// File: SoundEffect.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "WAVFileReader.h"
#include "SoundCommon.h"

#include <list>

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <apu.h>
#endif

using namespace DirectX;


//======================================================================================
// SoundEffect
//======================================================================================

// Internal object implementation class.
class SoundEffect::Impl : public IVoiceNotify
{
public:
    explicit Impl(_In_ AudioEngine* engine) :
        mWaveFormat(nullptr),
        mStartAudio(nullptr),
        mAudioBytes(0),
        mLoopStart(0),
        mLoopLength(0),
        mEngine(engine),
        mOneShots(0)
    #if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
        , mSeekCount(0)
        , mSeekTable(nullptr)
    #endif
    #if defined(_XBOX_ONE) && defined(_TITLE)
        , mXMAMemory(nullptr)
    #endif
    {
        assert(mEngine != 0);
        mEngine->RegisterNotify(this, false);
    }

    virtual ~Impl()
    {
        if (!mInstances.empty())
        {
            DebugTrace("WARNING: Destroying SoundEffect with %Iu outstanding SoundEffectInstances\n", mInstances.size());

            for (auto it = mInstances.begin(); it != mInstances.end(); ++it)
            {
                assert(*it != 0);
                (*it)->OnDestroyParent();
            }

            mInstances.clear();
        }

        if (mOneShots > 0)
        {
            DebugTrace("WARNING: Destroying SoundEffect with %u outstanding one shot effects\n", mOneShots);
        }

        if (mEngine)
        {
            mEngine->UnregisterNotify(this, true, false);
            mEngine = nullptr;
        }

    #if defined(_XBOX_ONE) && defined(_TITLE)
        if (mXMAMemory)
        {
            ApuFree(mXMAMemory);
            mXMAMemory = nullptr;
        }
    #endif
    }

    HRESULT Initialize(_In_ AudioEngine* engine, _Inout_ std::unique_ptr<uint8_t[]>& wavData,
                       _In_ const WAVEFORMATEX* wfx, _In_reads_bytes_(audioBytes) const uint8_t* startAudio, size_t audioBytes,
                   #if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
                       _In_reads_opt_(seekCount) const uint32_t* seekTable, size_t seekCount,
                   #endif
                       uint32_t loopStart, uint32_t loopLength);

    void Play(float volume, float pitch, float pan);

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
        stats.audioBytes += mAudioBytes;

    #if defined(_XBOX_ONE) && defined(_TITLE)
        if (mXMAMemory)
            stats.xmaAudioBytes += mAudioBytes;
    #endif
    }

    const WAVEFORMATEX*                 mWaveFormat;
    const uint8_t*                      mStartAudio;
    uint32_t                            mAudioBytes;
    uint32_t                            mLoopStart;
    uint32_t                            mLoopLength;
    AudioEngine*                        mEngine;
    std::list<SoundEffectInstance*>     mInstances;
    uint32_t                            mOneShots;

#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
    uint32_t                            mSeekCount;
    const uint32_t*                     mSeekTable;
#endif

private:
    std::unique_ptr<uint8_t[]>          mWavData;

#if defined(_XBOX_ONE) && defined(_TITLE)
    void*                               mXMAMemory;
#endif
};


_Use_decl_annotations_
HRESULT SoundEffect::Impl::Initialize(AudioEngine* engine, std::unique_ptr<uint8_t[]>& wavData,
                                      const WAVEFORMATEX* wfx, const uint8_t* startAudio, size_t audioBytes,
                                  #if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
                                      const uint32_t* seekTable, size_t seekCount,
                                  #endif
                                      uint32_t loopStart, uint32_t loopLength)
{
    if (!engine || !IsValid(wfx) || !startAudio || !audioBytes || !wavData)
        return E_INVALIDARG;

    if (audioBytes > UINT32_MAX)
        return E_INVALIDARG;

    switch (GetFormatTag(wfx))
    {
        case WAVE_FORMAT_PCM:
        case WAVE_FORMAT_IEEE_FLOAT:
        case WAVE_FORMAT_ADPCM:
            // Take ownership of the buffer
            mWavData.reset(wavData.release());

            // WARNING: We assume the wfx and startAudio parameters are pointers into the wavData memory buffer
            mWaveFormat = wfx;
            mStartAudio = startAudio;
            break;

        #if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

        case WAVE_FORMAT_WMAUDIO2:
        case WAVE_FORMAT_WMAUDIO3:
            if (!seekCount || !seekTable)
            {
                DebugTrace("ERROR: SoundEffect format xWMA requires seek table\n");
                return E_FAIL;
            }

            if (seekCount > UINT32_MAX)
                return E_INVALIDARG;

            // Take ownership of the buffer
            mWavData.reset(wavData.release());

            // WARNING: We assume the wfx, startAudio, and mSeekTable parameters are pointers into the wavData memory buffer
            mWaveFormat = wfx;
            mStartAudio = startAudio;
            mSeekCount = static_cast<uint32_t>(seekCount);
            mSeekTable = seekTable;
            break;

        #endif // _XBOX_ONE || _WIN32_WINNT < _WIN32_WINNT_WIN8 || _WIN32_WINNT >= _WIN32_WINNT_WIN10

        #if defined(_XBOX_ONE) && defined(_TITLE)

        case WAVE_FORMAT_XMA2:
            if (!seekCount || !seekTable)
            {
                DebugTrace("ERROR: SoundEffect format XMA2 requires seek table\n");
                return E_FAIL;
            }

            if (seekCount > UINT32_MAX)
                return E_INVALIDARG;

            {
                HRESULT hr = ApuAlloc(&mXMAMemory, nullptr,
                                      static_cast<UINT32>(audioBytes), SHAPE_XMA_INPUT_BUFFER_ALIGNMENT);
                if (FAILED(hr))
                {
                    DebugTrace("ERROR: ApuAlloc failed. Did you allocate a large enough heap with ApuCreateHeap for all your XMA wave data?\n");
                    return hr;
                }
            }

            memcpy(mXMAMemory, startAudio, audioBytes);
            mStartAudio = reinterpret_cast<const uint8_t*>(mXMAMemory);

            mWavData.reset(new uint8_t[sizeof(XMA2WAVEFORMATEX) + (seekCount * sizeof(uint32_t))]);

            memcpy(mWavData.get(), wfx, sizeof(XMA2WAVEFORMATEX));
            mWaveFormat = reinterpret_cast<WAVEFORMATEX*>(mWavData.get());

            // XMA seek table is Big-Endian
            {
                auto dest = reinterpret_cast<uint32_t*>(mWavData.get() + sizeof(XMA2WAVEFORMATEX));
                for (size_t k = 0; k < seekCount; ++k)
                {
                    dest[k] = _byteswap_ulong(seekTable[k]);
                }
            }

            mSeekCount = static_cast<uint32_t>(seekCount);
            mSeekTable = reinterpret_cast<const uint32_t*>(mWavData.get() + sizeof(XMA2WAVEFORMATEX));

            wavData.reset();
            break;

        #endif // _XBOX_ONE && _TITLE

        default:
        {
            DebugTrace("ERROR: SoundEffect encountered an unsupported format tag (%u)\n", wfx->wFormatTag);
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
    }

    mAudioBytes = static_cast<uint32_t>(audioBytes);
    mLoopStart = loopStart;
    mLoopLength = loopLength;

    return S_OK;
}


void SoundEffect::Impl::Play(float volume, float pitch, float pan)
{
    assert(volume >= -XAUDIO2_MAX_VOLUME_LEVEL && volume <= XAUDIO2_MAX_VOLUME_LEVEL);
    assert(pitch >= -1.f && pitch <= 1.f);
    assert(pan >= -1.f && pan <= 1.f);

    IXAudio2SourceVoice* voice = nullptr;
    mEngine->AllocateVoice(mWaveFormat, SoundEffectInstance_Default, true, &voice);

    if (!voice)
        return;

    if (volume != 1.f)
    {
        HRESULT hr = voice->SetVolume(volume);
        ThrowIfFailed(hr);
    }

    if (pitch != 0.f)
    {
        float fr = XAudio2SemitonesToFrequencyRatio(pitch * 12.f);

        HRESULT hr = voice->SetFrequencyRatio(fr);
        ThrowIfFailed(hr);
    }

    if (pan != 0.f)
    {
        float matrix[16];
        if (ComputePan(pan, mWaveFormat->nChannels, matrix))
        {
            HRESULT hr = voice->SetOutputMatrix(nullptr, mWaveFormat->nChannels, mEngine->GetOutputChannels(), matrix);
            ThrowIfFailed(hr);
        }
    }

    HRESULT hr = voice->Start(0);
    ThrowIfFailed(hr);

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = mAudioBytes;
    buffer.pAudioData = mStartAudio;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.pContext = this;

#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

    uint32_t tag = GetFormatTag(mWaveFormat);
    if (tag == WAVE_FORMAT_WMAUDIO2 || tag == WAVE_FORMAT_WMAUDIO3)
    {
        XAUDIO2_BUFFER_WMA wmaBuffer = {};
        wmaBuffer.PacketCount = mSeekCount;
        wmaBuffer.pDecodedPacketCumulativeBytes = mSeekTable;

        hr = voice->SubmitSourceBuffer(&buffer, &wmaBuffer);
    }
    else
    #endif
    {
        hr = voice->SubmitSourceBuffer(&buffer, nullptr);
    }
    if (FAILED(hr))
    {
        DebugTrace("ERROR: SoundEffect failed (%08X) when submitting buffer:\n", hr);
        DebugTrace("\tFormat Tag %u, %u channels, %u-bit, %u Hz, %u bytes\n", mWaveFormat->wFormatTag,
                   mWaveFormat->nChannels, mWaveFormat->wBitsPerSample, mWaveFormat->nSamplesPerSec, mAudioBytes);
        throw std::exception("SubmitSourceBuffer");
    }

    InterlockedIncrement(&mOneShots);
}


//--------------------------------------------------------------------------------------
// SoundEffect
//--------------------------------------------------------------------------------------

// Public constructors.
_Use_decl_annotations_
SoundEffect::SoundEffect(AudioEngine* engine, const wchar_t* waveFileName)
    : pImpl(std::make_unique<Impl>(engine))
{
    WAVData wavInfo;
    std::unique_ptr<uint8_t[]> wavData;
    HRESULT hr = LoadWAVAudioFromFileEx(waveFileName, wavData, wavInfo);
    if (FAILED(hr))
    {
        DebugTrace("ERROR: SoundEffect failed (%08X) to load from .wav file \"%ls\"\n", hr, waveFileName);
        throw std::exception("SoundEffect");
    }

#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
    hr = pImpl->Initialize(engine, wavData, wavInfo.wfx, wavInfo.startAudio, wavInfo.audioBytes,
                           wavInfo.seek, wavInfo.seekCount,
                           wavInfo.loopStart, wavInfo.loopLength);
#else
    hr = pImpl->Initialize(engine, wavData, wavInfo.wfx, wavInfo.startAudio, wavInfo.audioBytes,
                           wavInfo.loopStart, wavInfo.loopLength);
#endif

    if (FAILED(hr))
    {
        DebugTrace("ERROR: SoundEffect failed (%08X) to intialize from .wav file \"%ls\"\n", hr, waveFileName);
        throw std::exception("SoundEffect");
    }
}


_Use_decl_annotations_
SoundEffect::SoundEffect(AudioEngine* engine, std::unique_ptr<uint8_t[]>& wavData,
                         const WAVEFORMATEX* wfx, const uint8_t* startAudio, size_t audioBytes)
    : pImpl(std::make_unique<Impl>(engine))
{
#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
    HRESULT hr = pImpl->Initialize(engine, wavData, wfx, startAudio, audioBytes, nullptr, 0, 0, 0);
#else
    HRESULT hr = pImpl->Initialize(engine, wavData, wfx, startAudio, audioBytes, 0, 0);
#endif
    if (FAILED(hr))
    {
        DebugTrace("ERROR: SoundEffect failed (%08X) to intialize\n", hr);
        throw std::exception("SoundEffect");
    }
}


_Use_decl_annotations_
SoundEffect::SoundEffect(AudioEngine* engine, std::unique_ptr<uint8_t[]>& wavData,
                         const WAVEFORMATEX* wfx, const uint8_t* startAudio, size_t audioBytes,
                         uint32_t loopStart, uint32_t loopLength)
    : pImpl(std::make_unique<Impl>(engine))
{
#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
    HRESULT hr = pImpl->Initialize(engine, wavData, wfx, startAudio, audioBytes, nullptr, 0, loopStart, loopLength);
#else
    HRESULT hr = pImpl->Initialize(engine, wavData, wfx, startAudio, audioBytes, loopStart, loopLength);
#endif
    if (FAILED(hr))
    {
        DebugTrace("ERROR: SoundEffect failed (%08X) to intialize\n", hr);
        throw std::exception("SoundEffect");
    }
}


#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

_Use_decl_annotations_
SoundEffect::SoundEffect(AudioEngine* engine, std::unique_ptr<uint8_t[]>& wavData,
                         const WAVEFORMATEX* wfx, const uint8_t* startAudio, size_t audioBytes,
                         const uint32_t* seekTable, size_t seekCount)
{
    HRESULT hr = pImpl->Initialize(engine, wavData, wfx, startAudio, audioBytes, seekTable, seekCount, 0, 0);
    if (FAILED(hr))
    {
        DebugTrace("ERROR: SoundEffect failed (%08X) to intialize\n", hr);
        throw std::exception("SoundEffect");
    }
}

#endif


// Move constructor.
SoundEffect::SoundEffect(SoundEffect&& moveFrom) throw()
    : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
SoundEffect& SoundEffect::operator= (SoundEffect&& moveFrom) throw()
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
    pImpl->Play(1.f, 0.f, 0.f);
}


void SoundEffect::Play(float volume, float pitch, float pan)
{
    pImpl->Play(volume, pitch, pan);
}


std::unique_ptr<SoundEffectInstance> SoundEffect::CreateInstance(SOUND_EFFECT_INSTANCE_FLAGS flags)
{
    auto effect = new SoundEffectInstance(pImpl->mEngine, this, flags);
    assert(effect != 0);
    pImpl->mInstances.emplace_back(effect);
    return std::unique_ptr<SoundEffectInstance>(effect);
}


void SoundEffect::UnregisterInstance(_In_ SoundEffectInstance* instance)
{
    auto it = std::find(pImpl->mInstances.begin(), pImpl->mInstances.end(), instance);
    if (it == pImpl->mInstances.end())
        return;

    pImpl->mInstances.erase(it);
}


// Public accessors.
bool SoundEffect::IsInUse() const
{
    return (pImpl->mOneShots > 0) || !pImpl->mInstances.empty();
}


size_t SoundEffect::GetSampleSizeInBytes() const
{
    return pImpl->mAudioBytes;
}


size_t SoundEffect::GetSampleDuration() const
{
    if (!pImpl->mWaveFormat || !pImpl->mWaveFormat->nChannels)
        return 0;

    switch (GetFormatTag(pImpl->mWaveFormat))
    {
        case WAVE_FORMAT_ADPCM:
        {
            auto adpcmFmt = reinterpret_cast<const ADPCMWAVEFORMAT*>(pImpl->mWaveFormat);

            uint64_t duration = uint64_t(pImpl->mAudioBytes / adpcmFmt->wfx.nBlockAlign) * adpcmFmt->wSamplesPerBlock;
            int partial = pImpl->mAudioBytes % adpcmFmt->wfx.nBlockAlign;
            if (partial)
            {
                if (partial >= (7 * adpcmFmt->wfx.nChannels))
                    duration += (uint64_t(partial) * 2 / uint64_t(adpcmFmt->wfx.nChannels - 12));
            }
            return static_cast<size_t>(duration);
        }

    #if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

        case WAVE_FORMAT_WMAUDIO2:
        case WAVE_FORMAT_WMAUDIO3:
            if (pImpl->mSeekTable && pImpl->mSeekCount > 0)
            {
                return pImpl->mSeekTable[pImpl->mSeekCount - 1] / uint32_t(2 * pImpl->mWaveFormat->nChannels);
            }
            break;

        #endif

        #if defined(_XBOX_ONE) && defined(_TITLE)

        case WAVE_FORMAT_XMA2:
            return reinterpret_cast<const XMA2WAVEFORMATEX*>(pImpl->mWaveFormat)->SamplesEncoded;

        #endif

        default:
            if (pImpl->mWaveFormat->wBitsPerSample > 0)
            {
                return static_cast<size_t>((uint64_t(pImpl->mAudioBytes) * 8)
                                           / (uint64_t(pImpl->mWaveFormat->wBitsPerSample) * uint64_t(pImpl->mWaveFormat->nChannels)));
            }
    }

    return 0;
}


size_t SoundEffect::GetSampleDurationMS() const
{
    if (!pImpl->mWaveFormat || !pImpl->mWaveFormat->nSamplesPerSec)
        return 0;

    uint64_t samples = GetSampleDuration();
    return static_cast<size_t>((samples * 1000) / pImpl->mWaveFormat->nSamplesPerSec);
}


const WAVEFORMATEX* SoundEffect::GetFormat() const
{
    return pImpl->mWaveFormat;
}


#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

bool SoundEffect::FillSubmitBuffer(_Out_ XAUDIO2_BUFFER& buffer, _Out_ XAUDIO2_BUFFER_WMA& wmaBuffer) const
{
    memset(&buffer, 0, sizeof(buffer));
    memset(&wmaBuffer, 0, sizeof(wmaBuffer));

    buffer.AudioBytes = pImpl->mAudioBytes;
    buffer.pAudioData = pImpl->mStartAudio;
    buffer.LoopBegin = pImpl->mLoopStart;
    buffer.LoopLength = pImpl->mLoopLength;

    uint32_t tag = GetFormatTag(pImpl->mWaveFormat);
    if (tag == WAVE_FORMAT_WMAUDIO2 || tag == WAVE_FORMAT_WMAUDIO3)
    {
        wmaBuffer.PacketCount = pImpl->mSeekCount;
        wmaBuffer.pDecodedPacketCumulativeBytes = pImpl->mSeekTable;
        return true;
    }

    return false;
}

#else

void SoundEffect::FillSubmitBuffer(_Out_ XAUDIO2_BUFFER& buffer) const
{
    memset(&buffer, 0, sizeof(buffer));
    buffer.AudioBytes = pImpl->mAudioBytes;
    buffer.pAudioData = pImpl->mStartAudio;
    buffer.LoopBegin = pImpl->mLoopStart;
    buffer.LoopLength = pImpl->mLoopLength;
}

#endif
