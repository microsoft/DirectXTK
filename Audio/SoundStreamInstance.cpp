//--------------------------------------------------------------------------------------
// File: SoundStreamInstance.cpp
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
// SoundStreamInstance
//======================================================================================

// Internal object implementation class.
class SoundStreamInstance::Impl : public IVoiceNotify
{
public:
    Impl(_In_ AudioEngine* engine, _In_ WaveBank* waveBank, uint32_t index, SOUND_EFFECT_INSTANCE_FLAGS flags) :
        mBase(),
        mWaveBank(waveBank),
        mIndex(index),
        mLooped(false)
    {
        assert(engine != nullptr);
        engine->RegisterNotify(this, true);

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
        // TODO -
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
        // TOOD -
    }

    virtual void __cdecl OnDestroyEngine() noexcept override
    {
        mBase.OnDestroy();
    }

    virtual void __cdecl OnTrim() override
    {
        mBase.OnTrim();
    }

    virtual void __cdecl GatherStatistics(AudioStatistics& stats) const noexcept override
    {
        mBase.GatherStatistics(stats);
    }

    virtual void __cdecl OnDestroyParent() noexcept override
    {
        mBase.OnDestroy();
        mWaveBank = nullptr;
    }


    SoundEffectInstanceBase         mBase;
    WaveBank*                       mWaveBank;
    uint32_t                        mIndex;
    bool                            mLooped;
};


void SoundStreamInstance::Impl::Play(bool loop)
{
    if (!mBase.voice)
    {
        if (mWaveBank)
        {
            char buff[64] = {};
            auto wfx = reinterpret_cast<WAVEFORMATEX*>(buff);
            mBase.AllocateVoice(mWaveBank->GetFormat(mIndex, wfx, sizeof(buff)));
        }
    }

    if (!mBase.Play())
        return;

    // TODO - buffer.pContext = this;
}


//--------------------------------------------------------------------------------------
// SoundStreamInstance
//--------------------------------------------------------------------------------------

// Private constructors
_Use_decl_annotations_
SoundStreamInstance::SoundStreamInstance(AudioEngine* engine, WaveBank* waveBank, unsigned int index, SOUND_EFFECT_INSTANCE_FLAGS flags) :
    pImpl(std::make_unique<Impl>(engine, waveBank, index, flags))
{
}


// Move constructor.
SoundStreamInstance::SoundStreamInstance(SoundStreamInstance&& moveFrom) noexcept
    : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
SoundStreamInstance& SoundStreamInstance::operator= (SoundStreamInstance&& moveFrom) noexcept
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
SoundStreamInstance::~SoundStreamInstance()
{
    if (pImpl)
    {
        if (pImpl->mWaveBank)
        {
            pImpl->mWaveBank->UnregisterInstance(pImpl.get());
            pImpl->mWaveBank = nullptr;
        }
    }
}


// Public methods.
void SoundStreamInstance::Play(bool loop)
{
    pImpl->Play(loop);
}


void SoundStreamInstance::Stop(bool immediate) noexcept
{
    pImpl->mBase.Stop(immediate, pImpl->mLooped);
}


void SoundStreamInstance::Pause() noexcept
{
    pImpl->mBase.Pause();
}


void SoundStreamInstance::Resume()
{
    pImpl->mBase.Resume();
}


void SoundStreamInstance::SetVolume(float volume)
{
    pImpl->mBase.SetVolume(volume);
}


void SoundStreamInstance::SetPitch(float pitch)
{
    pImpl->mBase.SetPitch(pitch);
}


void SoundStreamInstance::SetPan(float pan)
{
    pImpl->mBase.SetPan(pan);
}


void SoundStreamInstance::Apply3D(const AudioListener& listener, const AudioEmitter& emitter, bool rhcoords)
{
    pImpl->mBase.Apply3D(listener, emitter, rhcoords);
}


// Public accessors.
bool SoundStreamInstance::IsLooped() const noexcept
{
    return pImpl->mLooped;
}


SoundState SoundStreamInstance::GetState() noexcept
{
    return pImpl->mBase.GetState(true);
}


IVoiceNotify* SoundStreamInstance::GetVoiceNotify() const noexcept
{
    return pImpl.get();
}
