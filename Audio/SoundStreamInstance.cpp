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

//#define VERBOSE_TRACE

#ifdef VERBOSE_TRACE
#pragma message("NOTE: Verbose tracing enabled")
#endif

namespace
{
    const size_t DVD_SECTOR_SIZE = 2048;
    const size_t MEMORY_ALLOC_SIZE = 4096;
    const size_t MAX_BUFFER_COUNT = 3;

    size_t ComputeAsyncPacketSize(_In_ const WAVEFORMATEX* wfx)
    {
        if (!wfx)
            return 0;

        size_t buffer = wfx->nAvgBytesPerSec * 2;
        buffer = AlignUp(buffer, MEMORY_ALLOC_SIZE);
        buffer = std::max<size_t>(65536u, buffer);
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
        mEndStream(false),
        mPackets{},
        mCurrentDiskReadBuffer(0),
        mCurrentPlayBuffer(0),
        mStitchBytes(0),
        mCurrentPosition(0),
        mOffsetBytes(0),
        mLengthInBytes(0),
        mPacketSize(0),
        mTotalSize(0)
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

        mBufferEnd.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        mBufferRead.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!mBufferEnd || !mBufferRead)
        {
            throw std::exception("CreateEvent");
        }

        ThrowIfFailed(AllocateStreamingBuffers(wfx));

#ifdef VERBOSE_TRACE
        DebugTrace("INFO (Streaming): packet size %zu, play length %zu\n", mPacketSize, mLengthInBytes);
#endif

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
            mBase.engine->UnregisterNotify(this, false, true);
            mBase.engine = nullptr;
        }

        memset(mPackets, 0, sizeof(mPackets));
        mPacketSize = 0;
    }

    Impl(Impl&&) = default;
    Impl& operator= (Impl&&) = default;

    Impl(Impl const&) = delete;
    Impl& operator= (Impl const&) = delete;

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
        mEndStream = false;

        ThrowIfFailed(PlayBuffers());
    }

    // IVoiceNotify
    virtual void __cdecl OnBufferEnd() override
    {
        // Not used
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

        case WAIT_OBJECT_0: // Read completed
#ifdef VERBOSE_TRACE
            DebugTrace("INFO (Streaming): Playing... (readpos %zu) [", mCurrentPosition);
            for (uint32_t k = 0; k < MAX_BUFFER_COUNT; ++k)
            {
                DebugTrace("%ls ", s_debugState[static_cast<int>(mPackets[k].state)]);
            }
            DebugTrace("]\n");
#endif
            ThrowIfFailed(PlayBuffers());
            break;

        case (WAIT_OBJECT_0 + 1): // Play completed
#ifdef VERBOSE_TRACE
            DebugTrace("INFO (Streaming): Reading... (readpos %zu) [", mCurrentPosition);
            for (uint32_t k = 0; k < MAX_BUFFER_COUNT; ++k)
            {
                DebugTrace("%ls ", s_debugState[static_cast<int>(mPackets[k].state)]);
            }
            DebugTrace("]\n");
#endif
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
    bool                            mEndStream;

    ScopedHandle                    mBufferEnd;
    ScopedHandle                    mBufferRead;

    enum class State : uint32_t
    {
        FREE = 0,
        PENDING,
        READY,
        PLAYING,
    };

#ifdef VERBOSE_TRACE
    static const wchar_t* s_debugState[4];
#endif

    struct BufferNotify : public IVoiceNotify
    {
        void Set(SoundStreamInstance::Impl* parent, size_t index) noexcept(true) { mParent = parent; mIndex = index; };

        void __cdecl OnBufferEnd() override
        {
            assert(mParent != nullptr);
            mParent->mPackets[mIndex].state = State::FREE;
            SetEvent(mParent->mBufferEnd.get());
        }

        void __cdecl OnCriticalError() override { assert(mParent != nullptr); mParent->OnCriticalError(); }
        void __cdecl OnReset() override { assert(mParent != nullptr); mParent->OnReset(); }
        void __cdecl OnUpdate() override { assert(mParent != nullptr); mParent->OnUpdate(); }
        void __cdecl OnDestroyEngine() noexcept override { assert(mParent != nullptr); mParent->OnDestroyEngine(); }
        void __cdecl OnTrim() override { assert(mParent != nullptr); mParent->OnTrim(); }
        void __cdecl GatherStatistics(AudioStatistics& stats) const override { assert(mParent != nullptr); mParent->GatherStatistics(stats); }
        void __cdecl OnDestroyParent() noexcept override { assert(mParent != nullptr); mParent->OnDestroyParent(); }

    private:
        SoundStreamInstance::Impl* mParent;
        size_t mIndex;
    };

    struct Packets
    {
        State       state;
        uint8_t*    buffer;
        uint8_t*    stitchBuffer;
        uint32_t    valid;
        uint32_t    startPosition;
        OVERLAPPED  request;
        BufferNotify notify;
    };

    Packets                         mPackets[MAX_BUFFER_COUNT];

private:
    uint32_t                        mCurrentDiskReadBuffer;
    uint32_t                        mCurrentPlayBuffer;
    uint32_t                        mStitchBytes;
    size_t                          mCurrentPosition;
    size_t                          mOffsetBytes;
    size_t                          mLengthInBytes;

    size_t                          mPacketSize;
    size_t                          mTotalSize;
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

    uint64_t totalSize = uint64_t(packetSize) * uint64_t(MAX_BUFFER_COUNT);
    if (totalSize > UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    mPacketSize = packetSize;
    mStitchBytes = 0;

    size_t stitchSize = 0;
    if ((packetSize % wfx->nBlockAlign) != 0)
    {
        mStitchBytes = wfx->nBlockAlign;

        stitchSize = AlignUp<size_t>(wfx->nBlockAlign, DVD_SECTOR_SIZE);
        totalSize += uint64_t(stitchSize) * uint64_t(MAX_BUFFER_COUNT);
        if (totalSize > UINT32_MAX)
            return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
    }

    if (mTotalSize < totalSize)
    {
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
            totalSize = 0;
            return E_OUTOFMEMORY;
        }

        mTotalSize = static_cast<size_t>(totalSize);

        uint8_t* ptr = mStreamBuffer.get();
        for (size_t j = 0; j < MAX_BUFFER_COUNT; ++j)
        {
            mPackets[j].buffer = ptr;
            mPackets[j].stitchBuffer = nullptr;
            mPackets[j].request.hEvent = mBufferRead.get();
            mPackets[j].notify.Set(this, j);
            ptr += packetSize;
        }

        if (stitchSize > 0)
        {
            for (size_t j = 0; j < MAX_BUFFER_COUNT; ++j)
            {
                mPackets[j].stitchBuffer = ptr;
                ptr += stitchSize;
            }
        }
    }

    return S_OK;
}


HRESULT SoundStreamInstance::Impl::ReadBuffers() noexcept
{
    if (mCurrentPosition >= mLengthInBytes)
    {
        if (!mLooped)
        {
            mEndStream = true;
            return S_FALSE;
        }

#ifdef VERBOSE_TRACE
        DebugTrace("INFO (Streaming): Loop restart\n");
#endif

        mCurrentPosition = 0;
    }

    HANDLE async = mWaveBank->GetAsyncHandle();

    uint32_t readBuffer = mCurrentDiskReadBuffer;
    for (uint32_t j = 0; j < MAX_BUFFER_COUNT; ++j)
    {
        uint32_t entry = (j + readBuffer) % MAX_BUFFER_COUNT;
        if (mPackets[entry].state == State::FREE)
        {
            if (mCurrentPosition < mLengthInBytes)
            {
                auto cbValid = static_cast<uint32_t>(std::min(mPacketSize, mLengthInBytes - mCurrentPosition));

                mPackets[entry].startPosition = static_cast<uint32_t>(mCurrentPosition);
                mPackets[entry].request.Offset = static_cast<DWORD>(mOffsetBytes + mCurrentPosition);
                mPackets[entry].valid = cbValid;

                if (!ReadFile(async, mPackets[entry].buffer, uint32_t(mPacketSize), nullptr, &mPackets[entry].request))
                {
                    DWORD error = GetLastError();
                    if (error != ERROR_IO_PENDING)
                    {
                        return HRESULT_FROM_WIN32(error);
                    }
                }

                mCurrentPosition += cbValid;

                mCurrentDiskReadBuffer = (entry + 1) % MAX_BUFFER_COUNT;

                mPackets[entry].state = State::PENDING;

                if ((cbValid < mPacketSize) && mLooped)
                {
#ifdef VERBOSE_TRACE
                    DebugTrace("INFO (Streaming): Loop restart\n");
#endif
                    mCurrentPosition = 0;
                }
            }
        }
    }

    return S_OK;
}


HRESULT SoundStreamInstance::Impl::PlayBuffers() noexcept
{
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

    if (!mBase.voice || !mPlaying)
        return S_FALSE;

    for (uint32_t j = 0; j < MAX_BUFFER_COUNT; ++j)
    {
        if (mPackets[mCurrentPlayBuffer].state != State::READY)
            break;

        const uint8_t* ptr = mPackets[mCurrentPlayBuffer].buffer;
        uint32_t valid = mPackets[mCurrentPlayBuffer].valid;

        bool endstream = false;
        if (valid < mPacketSize)
        {
            endstream = true;
#ifdef VERBOSE_TRACE
            DebugTrace("INFO (Streaming): End of stream (%u of %zu bytes)\n", mPackets[mCurrentPlayBuffer].valid, mPacketSize);
#endif
        }

        uint32_t thisFrameStitch = 0;
        if (mStitchBytes > 0)
        {
            uint32_t prevFrameStitch = (mPackets[mCurrentPlayBuffer].startPosition % mStitchBytes);

            if (prevFrameStitch > 0)
            {
                auto buffer = mPackets[mCurrentPlayBuffer].stitchBuffer;

                thisFrameStitch = mStitchBytes - prevFrameStitch;

                uint32_t k = (mCurrentPlayBuffer + MAX_BUFFER_COUNT) % MAX_BUFFER_COUNT;
                if (mPackets[k].state == State::READY || mPackets[k].state == State::PLAYING)
                {
                    auto prevBuffer = mPackets[k].buffer + mPackets[k].valid;

                    memcpy(buffer, prevBuffer, prevFrameStitch);
                    memcpy(buffer + prevFrameStitch, ptr, thisFrameStitch);

                    XAUDIO2_BUFFER buf = {};
                    buf.AudioBytes = mStitchBytes;
                    buf.pAudioData = buffer;

                    if (endstream && (valid <= thisFrameStitch))
                    {
                        buf.Flags = XAUDIO2_END_OF_STREAM;
                        buf.pContext = &mPackets[mCurrentPlayBuffer].notify;
                    }
#ifdef VERBOSE_TRACE
                    DebugTrace("INFO (Streaming): Stitch packet (%u + %u = %u)\n", prevFrameStitch, thisFrameStitch, mStitchBytes);
#endif
                    ThrowIfFailed(mBase.voice->SubmitSourceBuffer(&buf));
                }

                ptr += thisFrameStitch;
            }

            valid = (valid / mStitchBytes) * mStitchBytes;
        }

        if (mPackets[mCurrentPlayBuffer].valid > thisFrameStitch)
        {
            XAUDIO2_BUFFER buf = {};
            buf.Flags = (endstream) ? XAUDIO2_END_OF_STREAM : 0;
            buf.AudioBytes = valid;
            buf.pAudioData = ptr;
            buf.pContext = &mPackets[mCurrentPlayBuffer].notify;

            ThrowIfFailed(mBase.voice->SubmitSourceBuffer(&buf));
        }

        mPackets[mCurrentPlayBuffer].state = State::PLAYING;
        mCurrentPlayBuffer = (mCurrentPlayBuffer + 1) % MAX_BUFFER_COUNT;
    }

    return S_OK;
}

#ifdef VERBOSE_TRACE
const wchar_t* SoundStreamInstance::Impl::s_debugState[4] =
{
    L"FREE",
    L"PENDING",
    L"READY",
    L"PLAYING"
};
#endif


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
    pImpl->mPlaying = !immediate;
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
    SoundState state = pImpl->mBase.GetState(pImpl->mEndStream);
    if (state == STOPPED)
    {
        pImpl->mPlaying = false;
    }
    return state;
}


IVoiceNotify* SoundStreamInstance::GetVoiceNotify() const noexcept
{
    return pImpl.get();
}
