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
#include "WaveBankReader.h"
#include "PlatformHelpers.h"
#include "SoundCommon.h"

using namespace DirectX;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#pragma warning(disable : 4061 4062)

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
        mPlaying(false),
        mLooped(false),
        mPackets{},
        mCurrentDiskReadBuffer(0),
        mCurrentPlayBuffer(0),
        mPlayingBuffers(0),
        mCurrentPosition(0),
        mOffsetBytes(0),
        mLengthInBytes(0),
        mPacketSize(0)
    {
        assert(engine != nullptr);
        engine->RegisterNotify(this, true);

        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(buff);
        assert(mWaveBank != nullptr);
        mBase.Initialize(engine, mWaveBank->GetFormat(index, wfx, sizeof(buff)), flags);

        WaveBankReader::Metadata metadata;
        (void)mWaveBank->GetPrivateData(index, &metadata, sizeof(metadata));

        mOffsetBytes = metadata.offsetBytes;
        mLengthInBytes = metadata.lengthBytes;
        // TODO: loop points?

        mBufferEnd.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        mBufferRead.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
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

        ThrowIfFailed(ReadBuffers());
    }

    virtual ~Impl() override
    {
        mBase.DestroyVoice();

        if (mWaveBank && mWaveBank->GetAsyncHandle())
        {
            for (size_t j = 0; j < MAX_BUFFER_COUNT; ++j)
            {
                (void)CancelIoEx(mWaveBank->GetAsyncHandle(), &mPackets[j].request);
            }
        }

        if (mBase.engine)
        {
            mBase.engine->UnregisterNotify(this, false, false);
            mBase.engine = nullptr;
        }

        memset(mPackets, 0, sizeof(mPackets));
        mPacketSize = 0;
    }

    void Play(bool loop)
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

        mLooped = loop;

        ThrowIfFailed(PlayBuffers());
    }

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
        if (!mPlaying)
            return;

        HANDLE events[] = { mBufferRead.get(), mBufferEnd.get() };
        switch (WaitForMultipleObjectsEx(_countof(events), events, FALSE, 0, FALSE))
        {
        case WAIT_TIMEOUT:
            break;

        case WAIT_OBJECT_0:
            ThrowIfFailed(PlayBuffers());
            break;

        case (WAIT_OBJECT_0 + 1):
            ThrowIfFailed(ReadBuffers());
            break;

        case WAIT_FAILED:
            throw std::exception("WaitForMultipleObjects");
        }
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
    bool                            mPlaying;
    bool                            mLooped;

private:
    ScopedHandle                    mBufferEnd;
    ScopedHandle                    mBufferRead;

    enum class State : uint32_t
    {
        FREE = 0,
        PENDING,
        READY,
        PLAYING,
    };

    struct Packets
    {
        State       state;
        uint8_t*    buffer;
        uint32_t    valid;
        OVERLAPPED  request;
    };

    Packets                         mPackets[MAX_BUFFER_COUNT];

    uint32_t                        mCurrentDiskReadBuffer;
    uint32_t                        mCurrentPlayBuffer;
    uint32_t                        mPlayingBuffers;
    size_t                          mCurrentPosition;
    size_t                          mOffsetBytes;
    size_t                          mLengthInBytes;

    size_t                          mPacketSize;
    std::unique_ptr<uint8_t[], virtual_deleter> mStreamBuffer;

    HRESULT AllocateStreamingBuffers(const WAVEFORMATEX* wfx) noexcept;
    HRESULT ReadBuffers() noexcept;
    HRESULT PlayBuffers() noexcept;
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
            mPackets[j].buffer = ptr;
            mPackets[j].request.hEvent = mBufferRead.get();
            ptr += packetSize;
        }
    }

    return S_OK;
}


HRESULT SoundStreamInstance::Impl::ReadBuffers() noexcept
{
    if (mCurrentPosition >= mLengthInBytes)
    {
        if (!mLooped)
            return S_FALSE;

        mCurrentPosition = 0;
    }

    HANDLE async = mWaveBank->GetAsyncHandle();

    uint32_t readBuffer = mCurrentDiskReadBuffer;
    for (uint32_t j = 0; j < MAX_BUFFER_COUNT; ++j)
    {
        uint32_t entry = (j + readBuffer) % MAX_BUFFER_COUNT;
        switch (mPackets[entry].state)
        {
        case State::FREE:
            if (mCurrentPosition < mLengthInBytes)
            {
                auto cbValid = static_cast<uint32_t>(std::min(mPacketSize, mLengthInBytes - mCurrentPosition));

                mPackets[entry].request.Offset = static_cast<DWORD>(mOffsetBytes + mCurrentPosition);
                mPackets[entry].valid = cbValid;

                if (!ReadFile(async, mPackets[entry].buffer, mPacketSize, nullptr, &mPackets[entry].request))
                {
                    DWORD error = GetLastError();
                    if (error != ERROR_IO_PENDING)
                    {
                        return HRESULT_FROM_WIN32(error);
                    }
                }

                mCurrentPosition += cbValid;

                mCurrentDiskReadBuffer = (j + readBuffer + 1) % MAX_BUFFER_COUNT;

                mPackets[entry].state = State::PENDING;

                if ((cbValid < mPacketSize) && mLooped)
                {
                    mCurrentPosition = 0;
                }
            }
            break;

        case State::PENDING:
            {
                DWORD cb = 0;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
                BOOL result = GetOverlappedResultEx(async, &mPackets[entry].request, &cb, 0, FALSE);
#else
                BOOL result = GetOverlappedResult(async, &mPackets[entry].request, &cb, FALSE);
#endif
                if (result)
                {
                    mPackets[entry].state = State::READY;
                }
                else
                {
                    DWORD error = GetLastError();
                    if (error != ERROR_IO_INCOMPLETE)
                    {
                        ThrowIfFailed(HRESULT_FROM_WIN32(error));
                    }
                }
            }
            break;
        }
    }

    return S_OK;
}


HRESULT SoundStreamInstance::Impl::PlayBuffers() noexcept
{
    if (!mBase.voice)
        return S_FALSE;

    HANDLE async = mWaveBank->GetAsyncHandle();

    for (uint32_t j = 0; j < MAX_BUFFER_COUNT; ++j)
    {
        if (mPackets[j].state == State::PENDING)
        {
            DWORD cb = 0;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
            BOOL result = GetOverlappedResultEx(async, &mPackets[j].request, &cb, 0, FALSE);
#else
            BOOL result = GetOverlappedResult(async, &mPackets[j].request, &cb, FALSE);
#endif
            if (result)
            {
                mPackets[j].state = State::READY;
            }
            else
            {
                DWORD error = GetLastError();
                if (error != ERROR_IO_INCOMPLETE)
                {
                    ThrowIfFailed(HRESULT_FROM_WIN32(error));
                }
            }
        }
    }

    uint32_t playBuffer = mCurrentPlayBuffer;
    for (uint32_t j = 0; j < MAX_BUFFER_COUNT; ++j)
    {
        uint32_t entry = (j + playBuffer) % MAX_BUFFER_COUNT;
        switch (mPackets[entry].state)
        {
        case State::READY:
            {
                XAUDIO2_BUFFER buf = {};
                buf.AudioBytes = mPackets[entry].valid;
                buf.pAudioData = mPackets[entry].buffer;
                if (buf.AudioBytes < mPacketSize)
                    buf.Flags = XAUDIO2_END_OF_STREAM;
                buf.pContext = this;

                mBase.voice->SubmitSourceBuffer(&buf);

                mCurrentPlayBuffer = (j + playBuffer + 1) % MAX_BUFFER_COUNT;

                mPlayingBuffers += 1;
                mPackets[entry].state = State::PLAYING;
            }
            break;

        case State::PLAYING:
            if (uint32_t(mBase.GetPendingBufferCount()) < mPlayingBuffers)
            {
                mPlayingBuffers -= 1;
                mPackets[entry].state = State::FREE;
            }
            break;
        }
    }

    return S_OK;
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
    pImpl->mPlaying = true;
}


void SoundStreamInstance::Stop(bool immediate) noexcept
{
    pImpl->mBase.Stop(immediate, pImpl->mLooped);
    pImpl->mPlaying = false;
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
    return pImpl->mBase.GetState(false);
}


IVoiceNotify* SoundStreamInstance::GetVoiceNotify() const noexcept
{
    return pImpl.get();
}
