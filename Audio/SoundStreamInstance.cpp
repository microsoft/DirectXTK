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
#include "DirectXHelpers.h"
#include "PlatformHelpers.h"
#include "SoundCommon.h"

using namespace DirectX;

namespace
{
    const size_t DVD_SECTOR_SIZE = 2048;
    const size_t MEMORY_ALLOC_SIZE = 4096;
    const size_t MAX_BUFFER_COUNT = 3;

    size_t ComputeAsyncPacketSize(_In_ const WAVEFORMATEX* wfx)
    {
        if (!wfx)
            return 0;

        // TODO - for now, just a quick calculation
        size_t buffer = wfx->nBlockAlign * wfx->nChannels * wfx->nSamplesPerSec;
        buffer = AlignUp(buffer, MEMORY_ALLOC_SIZE);
        return buffer;
    }

    static_assert(MEMORY_ALLOC_SIZE >= DVD_SECTOR_SIZE, "Memory size should be larger than sector size");
    static_assert(MEMORY_ALLOC_SIZE >= DVD_SECTOR_SIZE || (MEMORY_ALLOC_SIZE% DVD_SECTOR_SIZE) == 0, "Memory size should be multiples of sector size");
}


//======================================================================================
// SoundStreamInstance
//======================================================================================

// Internal object implementation class.
class SoundStreamInstance::Impl : public IVoiceNotify
{
public:
    Impl(_In_ AudioEngine* engine,
        WaveBank* waveBank,
        uint32_t index,
        SOUND_EFFECT_INSTANCE_FLAGS flags) noexcept(false) :
        mBase(),
        mWaveBank(waveBank),
        mIndex(index),
        mLooped(false),
        mBuffers{},
        mRequest{},
        mPacketSize(0)
    {
        assert(engine != nullptr);
        engine->RegisterNotify(this, true);

        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(buff);
        assert(mWaveBank != nullptr);
        mBase.Initialize(engine, mWaveBank->GetFormat(index, wfx, sizeof(buff)), flags);

        mBufferEnd.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        mBufferRead.reset(CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!mBufferEnd || !mBufferRead)
        {
            throw std::exception("CreateEvent");
        }

        ThrowIfFailed(AllocateStreamingBuffers(wfx));

        // TODO - PCM only for now...
        if ((mPacketSize % wfx->nBlockAlign) != 0)
        {
            throw std::exception("ERROR: SoundStreamInstance requires block alignment matches packet size (PCM)");
        }
    }

    virtual ~Impl() override
    {
        mBase.DestroyVoice();

        if (mWaveBank && mWaveBank->GetAsyncHandle())
        {
            for (size_t j = 0; j < MAX_BUFFER_COUNT; ++j)
            {
                (void)CancelIoEx(mWaveBank->GetAsyncHandle(), &mRequest[j]);
            }
        }

        if (mBase.engine)
        {
            mBase.engine->UnregisterNotify(this, false, false);
            mBase.engine = nullptr;
        }

        memset(mBuffers, 0, sizeof(mBuffers));
        mPacketSize = 0;
    }

    void Play(bool loop);

    // IVoiceNotify
    virtual void __cdecl OnBufferEnd() override
    {
        SetEvent(mBufferEnd.get());
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

        stats.streamingBytes += mPacketSize * MAX_BUFFER_COUNT;
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

private:
    ScopedHandle                    mBufferEnd;
    ScopedHandle                    mBufferRead;

    uint8_t*                        mBuffers[MAX_BUFFER_COUNT];
    OVERLAPPED                      mRequest[MAX_BUFFER_COUNT];

    size_t                          mPacketSize;
    std::unique_ptr<uint8_t[], virtual_deleter> mStreamBuffer;

    HRESULT AllocateStreamingBuffers(const WAVEFORMATEX* wfx) noexcept;
};


HRESULT SoundStreamInstance::Impl::AllocateStreamingBuffers(const WAVEFORMATEX* wfx) noexcept
{
    if (!wfx)
        return E_INVALIDARG;

    size_t packetSize = ComputeAsyncPacketSize(wfx);
    if (!packetSize)
        return E_UNEXPECTED;

    if (mPacketSize < packetSize)
    {
        uint64_t totalSize = uint64_t(packetSize) * uint64_t(MAX_BUFFER_COUNT);
        if (totalSize > UINT32_MAX)
            return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

        mStreamBuffer.reset(reinterpret_cast<uint8_t*>(
            VirtualAlloc(nullptr, static_cast<SIZE_T>(totalSize), MEM_COMMIT, PAGE_READWRITE)
            ));

        if (!mStreamBuffer)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "ERROR: Failed allocating %llu bytes for SoundStreamInstance\n", totalSize);
            OutputDebugStringA(buff);
#endif
            mPacketSize = 0;
            return E_OUTOFMEMORY;
        }

        mPacketSize = packetSize;

        uint8_t* ptr = mStreamBuffer.get();
        for (size_t j = 0; j < MAX_BUFFER_COUNT; ++j)
        {
            mBuffers[j] = ptr;
            mRequest[j].Pointer = ptr;
            mRequest[j].hEvent = mBufferRead.get();
            ptr += packetSize;
        }
    }

    return S_OK;
}


void SoundStreamInstance::Impl::Play(bool loop)
{
    if (!mBase.voice)
    {
        if (!mWaveBank)
            return;

        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(buff);
        mBase.AllocateVoice(mWaveBank->GetFormat(mIndex, wfx, sizeof(buff)));
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
