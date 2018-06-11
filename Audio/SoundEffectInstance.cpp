//--------------------------------------------------------------------------------------
// File: SoundEffectInstance.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SoundCommon.h"

using namespace DirectX;


//======================================================================================
// SoundEffectInstance
//======================================================================================

// Internal object implementation class.
class SoundEffectInstance::Impl : public IVoiceNotify
{
public:
    Impl(_In_ AudioEngine* engine, _In_ SoundEffect* effect, SOUND_EFFECT_INSTANCE_FLAGS flags) :
        mBase(),
        mEffect(effect),
        mWaveBank(nullptr),
        mIndex(0),
        mLooped(false)
    {
        assert(engine != nullptr);
        engine->RegisterNotify(this, false);

        assert(mEffect != nullptr);
        mBase.Initialize(engine, effect->GetFormat(), flags);
    }

    Impl(_In_ AudioEngine* engine, _In_ WaveBank* waveBank, uint32_t index, SOUND_EFFECT_INSTANCE_FLAGS flags) :
        mBase(),
        mEffect(nullptr),
        mWaveBank(waveBank),
        mIndex(index),
        mLooped(false)
    {
        assert(engine != nullptr);
        engine->RegisterNotify(this, false);

        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(buff);
        assert(mWaveBank != nullptr);
        mBase.Initialize(engine, mWaveBank->GetFormat(index, wfx, sizeof(buff)), flags);
    }

    virtual ~Impl() override
    {
        mBase.DestroyVoice();

        if (mBase.engine)
        {
            mBase.engine->UnregisterNotify(this, false, false);
            mBase.engine = nullptr;
        }
    }

    void Play(bool loop);

    // IVoiceNotify
    virtual void __cdecl OnBufferEnd() override
    {
        // We don't register for this notification for SoundEffectInstances, so this should not be invoked
        assert(false);
    }

    virtual void __cdecl OnCriticalError() override
    {
        mBase.OnCriticalError();
    }

    virtual void __cdecl OnReset() override
    {
        mBase.OnReset();
    }

    virtual void __cdecl OnUpdate() override
    {
        // We do not register for update notification
        assert(false);
    }

    virtual void __cdecl OnDestroyEngine() override
    {
        mBase.OnDestroy();
    }

    virtual void __cdecl OnTrim() override
    {
        mBase.OnTrim();
    }

    virtual void __cdecl GatherStatistics(AudioStatistics& stats) const override
    {
        mBase.GatherStatistics(stats);
    }

    SoundEffectInstanceBase         mBase;
    SoundEffect*                    mEffect;
    WaveBank*                       mWaveBank;
    uint32_t                        mIndex;
    bool                            mLooped;
};


void SoundEffectInstance::Impl::Play(bool loop)
{
    if (!mBase.voice)
    {
        if (mWaveBank)
        {
            char buff[64] = {};
            auto wfx = reinterpret_cast<WAVEFORMATEX*>(buff);
            mBase.AllocateVoice(mWaveBank->GetFormat(mIndex, wfx, sizeof(buff)));
        }
        else
        {
            assert(mEffect != nullptr);
            mBase.AllocateVoice(mEffect->GetFormat());
        }
    }

    if (!mBase.Play())
        return;

    // Submit audio data for STOPPED -> PLAYING state transition
    XAUDIO2_BUFFER buffer;

#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

    bool iswma = false;
    XAUDIO2_BUFFER_WMA wmaBuffer;
    if (mWaveBank)
    {
        iswma = mWaveBank->FillSubmitBuffer(mIndex, buffer, wmaBuffer);
    }
    else
    {
        assert(mEffect != nullptr);
        iswma = mEffect->FillSubmitBuffer(buffer, wmaBuffer);
    }

#else

    if (mWaveBank)
    {
        mWaveBank->FillSubmitBuffer(mIndex, buffer);
    }
    else
    {
        assert(mEffect != 0);
        mEffect->FillSubmitBuffer(buffer);
    }

#endif

    buffer.Flags = XAUDIO2_END_OF_STREAM;
    if (loop)
    {
        mLooped = true;
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    }
    else
    {
        mLooped = false;
        buffer.LoopCount = buffer.LoopBegin = buffer.LoopLength = 0;
    }
    buffer.pContext = nullptr;

    HRESULT hr;
#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
    if (iswma)
    {
        hr = mBase.voice->SubmitSourceBuffer(&buffer, &wmaBuffer);
    }
    else
    #endif
    {
        hr = mBase.voice->SubmitSourceBuffer(&buffer, nullptr);
    }

    if (FAILED(hr))
    {
    #ifdef _DEBUG
        DebugTrace("ERROR: SoundEffectInstance failed (%08X) when submitting buffer:\n", hr);

        char buff[64] = {};
        auto wfx = (mWaveBank) ? mWaveBank->GetFormat(mIndex, reinterpret_cast<WAVEFORMATEX*>(buff), sizeof(buff))
            : mEffect->GetFormat();

        size_t length = (mWaveBank) ? mWaveBank->GetSampleSizeInBytes(mIndex) : mEffect->GetSampleSizeInBytes();

        DebugTrace("\tFormat Tag %u, %u channels, %u-bit, %u Hz, %zu bytes\n", wfx->wFormatTag,
                   wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, length);
    #endif
        mBase.Stop(true, mLooped);
        throw std::exception("SubmitSourceBuffer");
    }
}


//--------------------------------------------------------------------------------------
// SoundEffectInstance
//--------------------------------------------------------------------------------------

// Private constructors
_Use_decl_annotations_
SoundEffectInstance::SoundEffectInstance(AudioEngine* engine, SoundEffect* effect, SOUND_EFFECT_INSTANCE_FLAGS flags) :
    pImpl(std::make_unique<Impl>(engine, effect, flags))
{
}

_Use_decl_annotations_
SoundEffectInstance::SoundEffectInstance(AudioEngine* engine, WaveBank* waveBank, int index, SOUND_EFFECT_INSTANCE_FLAGS flags) :
    pImpl(std::make_unique<Impl>(engine, waveBank, index, flags))
{
}


// Move constructor.
SoundEffectInstance::SoundEffectInstance(SoundEffectInstance&& moveFrom) noexcept
    : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
SoundEffectInstance& SoundEffectInstance::operator= (SoundEffectInstance&& moveFrom) noexcept
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
SoundEffectInstance::~SoundEffectInstance()
{
    if (pImpl)
    {
        if (pImpl->mWaveBank)
        {
            pImpl->mWaveBank->UnregisterInstance(this);
            pImpl->mWaveBank = nullptr;
        }

        if (pImpl->mEffect)
        {
            pImpl->mEffect->UnregisterInstance(this);
            pImpl->mEffect = nullptr;
        }
    }
}


// Public methods.
void SoundEffectInstance::Play(bool loop)
{
    pImpl->Play(loop);
}


void SoundEffectInstance::Stop(bool immediate)
{
    pImpl->mBase.Stop(immediate, pImpl->mLooped);
}


void SoundEffectInstance::Pause()
{
    pImpl->mBase.Pause();
}


void SoundEffectInstance::Resume()
{
    pImpl->mBase.Resume();
}


void SoundEffectInstance::SetVolume(float volume)
{
    pImpl->mBase.SetVolume(volume);
}


void SoundEffectInstance::SetPitch(float pitch)
{
    pImpl->mBase.SetPitch(pitch);
}


void SoundEffectInstance::SetPan(float pan)
{
    pImpl->mBase.SetPan(pan);
}


void SoundEffectInstance::Apply3D(const AudioListener& listener, const AudioEmitter& emitter, bool rhcoords)
{
    pImpl->mBase.Apply3D(listener, emitter, rhcoords);
}


// Public accessors.
bool SoundEffectInstance::IsLooped() const
{
    return pImpl->mLooped;
}


SoundState SoundEffectInstance::GetState()
{
    return pImpl->mBase.GetState(true);
}


// Notifications.
void SoundEffectInstance::OnDestroyParent()
{
    pImpl->mBase.OnDestroy();
    pImpl->mWaveBank = nullptr;
    pImpl->mEffect = nullptr;
}
