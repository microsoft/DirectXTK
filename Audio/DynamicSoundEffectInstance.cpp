//--------------------------------------------------------------------------------------
// File: DynamicSoundEffectInstance.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SoundCommon.h"

using namespace DirectX;


//======================================================================================
// DynamicSoundEffectInstance
//======================================================================================

// Internal object implementation class.
class DynamicSoundEffectInstance::Impl : public IVoiceNotify
{
public:
    Impl(_In_ AudioEngine* engine,
         _In_ DynamicSoundEffectInstance* object,
        std::function<void(DynamicSoundEffectInstance*)>& bufferNeeded,
         int sampleRate, int channels, int sampleBits,
        SOUND_EFFECT_INSTANCE_FLAGS flags) :
        mBase(),
        mBufferNeeded(nullptr),
        mObject(object)
    {
        if ((sampleRate < XAUDIO2_MIN_SAMPLE_RATE)
            || (sampleRate > XAUDIO2_MAX_SAMPLE_RATE))
        {
            DebugTrace("DynamicSoundEffectInstance sampleRate must be in range %u...%u\n", XAUDIO2_MIN_SAMPLE_RATE, XAUDIO2_MAX_SAMPLE_RATE);
            throw std::out_of_range("DynamicSoundEffectInstance");
        }

        if (!channels || (channels > 8))
        {
            DebugTrace("DynamicSoundEffectInstance channels must be in range 1...8\n");
            throw std::out_of_range("DynamicSoundEffectInstance channel count out of range");
        }

        switch (sampleBits)
        {
            case 8:
            case 16:
                break;

            default:
                DebugTrace("DynamicSoundEffectInstance sampleBits must be 8-bit or 16-bit\n");
                throw std::invalid_argument("DynamicSoundEffectInstance supports 8 or 16 bit");
        }

        mBufferEvent.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!mBufferEvent)
        {
            throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
        }

        CreateIntegerPCM(&mWaveFormat, sampleRate, channels, sampleBits);

        assert(engine != nullptr);
        engine->RegisterNotify(this, true);

        mBase.Initialize(engine, &mWaveFormat, flags);

        mBufferNeeded = bufferNeeded;
    }

    Impl(Impl&&) = default;
    Impl& operator= (Impl&&) = default;

    Impl(Impl const&) = delete;
    Impl& operator= (Impl const&) = delete;

    ~Impl() override
    {
        mBase.DestroyVoice();

        if (mBase.engine)
        {
            mBase.engine->UnregisterNotify(this, false, true);
            mBase.engine = nullptr;
        }
    }

    void Play();

    void Resume();

    void SubmitBuffer(_In_reads_bytes_(audioBytes) const uint8_t* pAudioData, uint32_t offset, size_t audioBytes);

    const WAVEFORMATEX* GetFormat() const noexcept { return &mWaveFormat; }

    // IVoiceNotify
    void __cdecl OnBufferEnd() override
    {
        SetEvent(mBufferEvent.get());
    }

    void __cdecl OnCriticalError() override
    {
        mBase.OnCriticalError();
    }

    void __cdecl OnReset() override
    {
        mBase.OnReset();
    }

    void __cdecl OnUpdate() override;

    void __cdecl OnDestroyEngine() noexcept override
    {
        mBase.OnDestroy();
    }

    void __cdecl OnTrim() override
    {
        mBase.OnTrim();
    }

    void __cdecl GatherStatistics(AudioStatistics& stats) const noexcept override
    {
        mBase.GatherStatistics(stats);
    }

    void __cdecl OnDestroyParent() noexcept override
    {
    }

    SoundEffectInstanceBase                             mBase;

private:
    ScopedHandle                                        mBufferEvent;
    std::function<void(DynamicSoundEffectInstance*)>    mBufferNeeded;
    DynamicSoundEffectInstance*                         mObject;
    WAVEFORMATEX                                        mWaveFormat;
};


void DynamicSoundEffectInstance::Impl::Play()
{
    if (!mBase.voice)
    {
        mBase.AllocateVoice(&mWaveFormat);
    }

    std::ignore = mBase.Play();

    if (mBase.voice && (mBase.state == PLAYING) && (mBase.GetPendingBufferCount() <= 2))
    {
        SetEvent(mBufferEvent.get());
    }
}


void DynamicSoundEffectInstance::Impl::Resume()
{
    if (mBase.voice && (mBase.state == PAUSED))
    {
        mBase.Resume();

        if ((mBase.state == PLAYING) && (mBase.GetPendingBufferCount() <= 2))
        {
            SetEvent(mBufferEvent.get());
        }
    }
}


_Use_decl_annotations_
void DynamicSoundEffectInstance::Impl::SubmitBuffer(const uint8_t* pAudioData, uint32_t offset, size_t audioBytes)
{
    if (!pAudioData || !audioBytes)
        throw std::invalid_argument("Invalid audio data buffer");

    if (audioBytes > UINT32_MAX)
        throw std::out_of_range("SubmitBuffer");

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = static_cast<UINT32>(audioBytes);
    buffer.pAudioData = pAudioData;

    if (offset)
    {
        assert(mWaveFormat.wFormatTag == WAVE_FORMAT_PCM);
        buffer.PlayBegin = offset / mWaveFormat.nBlockAlign;
        buffer.PlayLength = static_cast<UINT32>((audioBytes - offset) / mWaveFormat.nBlockAlign);
    }

    buffer.pContext = this;

    HRESULT hr = mBase.voice->SubmitSourceBuffer(&buffer, nullptr);
    if (FAILED(hr))
    {
    #ifdef _DEBUG
        DebugTrace("ERROR: DynamicSoundEffectInstance failed (%08X) when submitting buffer:\n", static_cast<unsigned int>(hr));

        DebugTrace("\tFormat Tag %u, %u channels, %u-bit, %u Hz, %zu bytes [%u offset)\n",
            mWaveFormat.wFormatTag, mWaveFormat.nChannels, mWaveFormat.wBitsPerSample, mWaveFormat.nSamplesPerSec, audioBytes, offset);
    #endif
        throw std::runtime_error("SubmitSourceBuffer");
    }
}


void DynamicSoundEffectInstance::Impl::OnUpdate()
{
    const DWORD result = WaitForSingleObjectEx(mBufferEvent.get(), 0, FALSE);
    switch (result)
    {
        case WAIT_TIMEOUT:
            break;

        case WAIT_OBJECT_0:
            if (mBufferNeeded)
            {
                // This callback happens on the same thread that called AudioEngine::Update()
                mBufferNeeded(mObject);
            }
            break;

        case WAIT_FAILED:
            throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "WaitForSingleObjectEx");
    }
}



//--------------------------------------------------------------------------------------
// DynamicSoundEffectInstance
//--------------------------------------------------------------------------------------

#pragma warning( disable : 4355 )

// Public constructors
_Use_decl_annotations_
DynamicSoundEffectInstance::DynamicSoundEffectInstance(
    AudioEngine* engine,
    std::function<void(DynamicSoundEffectInstance*)> bufferNeeded,
    int sampleRate,
    int channels,
    int sampleBits,
    SOUND_EFFECT_INSTANCE_FLAGS flags) :
    pImpl(std::make_unique<Impl>(engine, this, bufferNeeded, sampleRate, channels, sampleBits, flags))
{
}


DynamicSoundEffectInstance::DynamicSoundEffectInstance(DynamicSoundEffectInstance&&) noexcept = default;
DynamicSoundEffectInstance& DynamicSoundEffectInstance::operator= (DynamicSoundEffectInstance&&) noexcept = default;
DynamicSoundEffectInstance::~DynamicSoundEffectInstance() = default;


// Public methods.
void DynamicSoundEffectInstance::Play()
{
    pImpl->Play();
}


void DynamicSoundEffectInstance::Stop(bool immediate) noexcept
{
    bool looped = false;
    pImpl->mBase.Stop(immediate, looped);
}


void DynamicSoundEffectInstance::Pause() noexcept
{
    pImpl->mBase.Pause();
}


void DynamicSoundEffectInstance::Resume()
{
    pImpl->Resume();
}


void DynamicSoundEffectInstance::SetVolume(float volume)
{
    pImpl->mBase.SetVolume(volume);
}


void DynamicSoundEffectInstance::SetPitch(float pitch)
{
    pImpl->mBase.SetPitch(pitch);
}


void DynamicSoundEffectInstance::SetPan(float pan)
{
    pImpl->mBase.SetPan(pan);
}


void DynamicSoundEffectInstance::Apply3D(const X3DAUDIO_LISTENER& listener, const X3DAUDIO_EMITTER& emitter, bool rhcoords)
{
    pImpl->mBase.Apply3D(listener, emitter, rhcoords);
}


_Use_decl_annotations_
void DynamicSoundEffectInstance::SubmitBuffer(const uint8_t* pAudioData, size_t audioBytes)
{
    pImpl->SubmitBuffer(pAudioData, 0, audioBytes);
}


_Use_decl_annotations_
void DynamicSoundEffectInstance::SubmitBuffer(const uint8_t* pAudioData, uint32_t offset, size_t audioBytes)
{
    pImpl->SubmitBuffer(pAudioData, offset, audioBytes);
}


// Public accessors.
SoundState DynamicSoundEffectInstance::GetState() noexcept
{
    return pImpl->mBase.GetState(false);
}


size_t DynamicSoundEffectInstance::GetSampleDuration(size_t bytes) const noexcept
{
    auto wfx = pImpl->GetFormat();
    if (!wfx || !wfx->wBitsPerSample || !wfx->nChannels)
        return 0;

    return static_cast<size_t>((uint64_t(bytes) * 8)
                               / (uint64_t(wfx->wBitsPerSample) * uint64_t(wfx->nChannels)));
}


size_t DynamicSoundEffectInstance::GetSampleDurationMS(size_t bytes) const noexcept
{
    auto wfx = pImpl->GetFormat();
    if (!wfx || !wfx->nAvgBytesPerSec)
        return 0;

    return static_cast<size_t>((uint64_t(bytes) * 1000) / wfx->nAvgBytesPerSec);
}


size_t DynamicSoundEffectInstance::GetSampleSizeInBytes(uint64_t duration) const noexcept
{
    auto wfx = pImpl->GetFormat();
    if (!wfx || !wfx->nSamplesPerSec)
        return 0;

    return static_cast<size_t>(((duration * wfx->nSamplesPerSec) / 1000) * wfx->nBlockAlign);
}


int DynamicSoundEffectInstance::GetPendingBufferCount() const noexcept
{
    return pImpl->mBase.GetPendingBufferCount();
}


unsigned int DynamicSoundEffectInstance::GetChannelCount() const noexcept
{
    return pImpl->mBase.GetChannelCount();
}


const WAVEFORMATEX* DynamicSoundEffectInstance::GetFormat() const noexcept
{
    return pImpl->GetFormat();
}
