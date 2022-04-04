//--------------------------------------------------------------------------------------
// File: SoundStreamInstance.cpp
//
// Copyright (c) Microsoft Corporation.
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

#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
#ifdef __clang__
#pragma clang diagnostic ignored "-Wnonportable-system-include-path"
#endif

#include <apu.h>
#include <shapexmacontext.h>
#endif

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
    constexpr size_t DVD_SECTOR_SIZE = 2048;
    constexpr size_t ADVANCED_FORMAT_SECTOR_SIZE = 4096;
    constexpr size_t MAX_BUFFER_COUNT = 3;

#ifdef DIRECTX_ENABLE_SEEK_TABLES
    constexpr size_t MAX_STREAMING_SEEK_PACKETS = 2048;
#endif

#ifdef DIRECTX_ENABLE_XMA2
    constexpr size_t XMA2_64KBLOCKINBYTES = 65536;

    struct apu_deleter { void operator()(void* p) noexcept { if (p) ApuFree(p); } };
#endif

    size_t ComputeAsyncPacketSize(_In_ const WAVEFORMATEX* wfx, uint32_t tag, uint32_t alignment)
    {
        if (!wfx)
            return 0;

        size_t buffer = size_t(wfx->nAvgBytesPerSec) * 2u;

    #ifdef DIRECTX_ENABLE_XMA2
        if (tag == WAVE_FORMAT_XMA2)
        {
            buffer = AlignUp(buffer, XMA2_64KBLOCKINBYTES);
            buffer = std::max<size_t>(XMA2_64KBLOCKINBYTES, buffer);
            return buffer;
        }
    #else
        UNREFERENCED_PARAMETER(tag);
    #endif

        buffer = AlignUp(buffer, size_t(alignment) * 2);
        buffer = std::max<size_t>(65536u, buffer);
        return buffer;
    }
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
        mPrefetch(false),
        mSitching(false),
        mPackets{},
        mCurrentDiskReadBuffer(0),
        mCurrentPlayBuffer(0),
        mBlockAlign(0),
        mAsyncAlign(DVD_SECTOR_SIZE),
        mCurrentPosition(0),
        mOffsetBytes(0),
        mLengthInBytes(0),
        mPacketSize(0),
        mTotalSize(0)
    #ifdef DIRECTX_ENABLE_SEEK_TABLES
        , mSeekCount(0),
        mSeekTable(nullptr),
        mSeekTableCopy{}
    #endif
    {
        assert(engine != nullptr);
        engine->RegisterNotify(this, true);

        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(buff);
        assert(mWaveBank != nullptr);
        mBase.Initialize(engine, mWaveBank->GetFormat(index, wfx, sizeof(buff)), flags);

        WaveBankReader::Metadata metadata = {};
        std::ignore = mWaveBank->GetPrivateData(index, &metadata, sizeof(metadata));

        mOffsetBytes = metadata.offsetBytes;
        mLengthInBytes = metadata.lengthBytes;
        mAsyncAlign = mWaveBank->IsAdvancedFormat() ? ADVANCED_FORMAT_SECTOR_SIZE : DVD_SECTOR_SIZE;

    #ifdef DIRECTX_ENABLE_SEEK_TABLES
        WaveBankSeekData seekData = {};
        std::ignore = mWaveBank->GetPrivateData(index, &seekData, sizeof(seekData));
        if (seekData.tag == WAVE_FORMAT_WMAUDIO2 || seekData.tag == WAVE_FORMAT_WMAUDIO3)
        {
            mSeekCount = seekData.seekCount;
            mSeekTable = seekData.seekTable;
        }
    #endif

        mBufferEnd.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        mBufferRead.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!mBufferEnd || !mBufferRead)
        {
            throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
        }

        ThrowIfFailed(AllocateStreamingBuffers(wfx));

    #ifdef VERBOSE_TRACE
        DebugTrace("INFO (Streaming): packet size %zu, play length %zu\n", mPacketSize, mLengthInBytes);
    #endif

        mPrefetch = true;
        ThrowIfFailed(ReadBuffers());
    }

    virtual ~Impl() override
    {
        mBase.DestroyVoice();

        if (mWaveBank && mWaveBank->GetAsyncHandle())
        {
            for (size_t j = 0; j < MAX_BUFFER_COUNT; ++j)
            {
                std::ignore = CancelIoEx(mWaveBank->GetAsyncHandle(), &mPackets[j].request);
            }
        }

        if (mBase.engine)
        {
            mBase.engine->UnregisterNotify(this, false, true);
            mBase.engine = nullptr;
        }

        for (size_t j = 0; j < MAX_BUFFER_COUNT; ++j)
        {
            mPackets[j] = {};
        }
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

        if (!mPrefetch)
        {
            mCurrentPosition = 0;
        }

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
        switch (WaitForMultipleObjectsEx(static_cast<DWORD>(std::size(events)), events, FALSE, 0, FALSE))
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
            mPrefetch = false;
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
            throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "WaitForMultipleObjectsEx");
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
    bool                            mPrefetch;
    bool                            mSitching;

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
        BufferNotify() : mParent(nullptr), mIndex(0) {}

        void Set(SoundStreamInstance::Impl* parent, size_t index) noexcept(true) { mParent = parent; mIndex = index; }

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
        uint32_t    audioBytes;
        uint32_t    startPosition;
        OVERLAPPED  request;
        BufferNotify notify;

        Packets() :
            state(State::FREE),
            buffer(nullptr),
            stitchBuffer(nullptr),
            valid(0),
            audioBytes(0),
            startPosition(0),
            request{},
            notify{} {}
    };

    Packets                         mPackets[MAX_BUFFER_COUNT];

private:
    uint32_t                        mCurrentDiskReadBuffer;
    uint32_t                        mCurrentPlayBuffer;
    uint32_t                        mBlockAlign;
    uint32_t                        mAsyncAlign;
    size_t                          mCurrentPosition;
    size_t                          mOffsetBytes;
    size_t                          mLengthInBytes;

    size_t                          mPacketSize;
    size_t                          mTotalSize;
    std::unique_ptr<uint8_t[], virtual_deleter> mStreamBuffer;

#ifdef DIRECTX_ENABLE_SEEK_TABLES
    uint32_t                        mSeekCount;
    const uint32_t*                 mSeekTable;
    uint32_t                        mSeekTableCopy[MAX_STREAMING_SEEK_PACKETS];
#endif

#ifdef DIRECTX_ENABLE_XMA2
    std::unique_ptr<uint8_t[], apu_deleter> mXMAMemory;
#endif

    HRESULT AllocateStreamingBuffers(const WAVEFORMATEX* wfx) noexcept;
    HRESULT ReadBuffers() noexcept;
    HRESULT PlayBuffers() noexcept;
};


HRESULT SoundStreamInstance::Impl::AllocateStreamingBuffers(const WAVEFORMATEX* wfx) noexcept
{
    if (!wfx)
        return E_INVALIDARG;

    const uint32_t tag = GetFormatTag(wfx);

    size_t packetSize = ComputeAsyncPacketSize(wfx, tag, mAsyncAlign);
    if (!packetSize)
        return E_UNEXPECTED;

    uint64_t totalSize = uint64_t(packetSize) * uint64_t(MAX_BUFFER_COUNT);
    if (totalSize > UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    mPacketSize = packetSize;
    mBlockAlign = wfx->nBlockAlign;
    mSitching = false;

    size_t stitchSize = 0;
    if ((packetSize % wfx->nBlockAlign) != 0)
    {
        mSitching = true;

        stitchSize = AlignUp<size_t>(wfx->nBlockAlign, mAsyncAlign);
        totalSize += uint64_t(stitchSize) * uint64_t(MAX_BUFFER_COUNT);
        if (totalSize > UINT32_MAX)
            return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
    }

#ifdef DIRECTX_ENABLE_XMA2
    if ((mTotalSize < totalSize) || (tag == WAVE_FORMAT_XMA2 && !mXMAMemory) || (tag != WAVE_FORMAT_XMA2 && !mStreamBuffer))
    #else
    if (mTotalSize < totalSize)
    #endif
    {
        mStreamBuffer.reset();
    #ifdef DIRECTX_ENABLE_XMA2
        mXMAMemory.reset();
        if (tag == WAVE_FORMAT_XMA2)
        {
            void* xmaMemory = nullptr;
            HRESULT hr = ApuAlloc(&xmaMemory, nullptr, static_cast<UINT32>(totalSize), SHAPE_XMA_INPUT_BUFFER_ALIGNMENT);
            if (FAILED(hr))
            {
                DebugTrace("ERROR: ApuAlloc failed (%llu bytes). Did you allocate a large enough heap with ApuCreateHeap for all your XMA wave data?\n", totalSize);
                return hr;
            }
            mXMAMemory.reset(static_cast<uint8_t*>(xmaMemory));
        }
        else
        #endif
        {
            mStreamBuffer.reset(reinterpret_cast<uint8_t*>(
                VirtualAlloc(nullptr, static_cast<SIZE_T>(totalSize), MEM_COMMIT, PAGE_READWRITE)
                ));

            if (!mStreamBuffer)
            {
                DebugTrace("ERROR: Failed allocating %llu bytes for SoundStreamInstance\n", totalSize);
                mPacketSize = 0;
                totalSize = 0;
                return E_OUTOFMEMORY;
            }
        }

        mTotalSize = static_cast<size_t>(totalSize);

    #ifdef DIRECTX_ENABLE_XMA2
        uint8_t* ptr = (tag == WAVE_FORMAT_XMA2) ? mXMAMemory.get() : mStreamBuffer.get();
    #else
        uint8_t* ptr = mStreamBuffer.get();
    #endif
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

    const uint32_t readBuffer = mCurrentDiskReadBuffer;
    for (uint32_t j = 0; j < MAX_BUFFER_COUNT; ++j)
    {
        uint32_t entry = (j + readBuffer) % uint32_t(MAX_BUFFER_COUNT);
        if (mPackets[entry].state == State::FREE)
        {
            if (mCurrentPosition < mLengthInBytes)
            {
                auto const cbValid = static_cast<uint32_t>(std::min(mPacketSize, mLengthInBytes - mCurrentPosition));

                mPackets[entry].valid = cbValid;
                mPackets[entry].audioBytes = 0;
                mPackets[entry].startPosition = static_cast<uint32_t>(mCurrentPosition);
                mPackets[entry].request.Offset = static_cast<DWORD>(mOffsetBytes + mCurrentPosition);

                if (!ReadFile(async, mPackets[entry].buffer, uint32_t(mPacketSize), nullptr, &mPackets[entry].request))
                {
                    const DWORD error = GetLastError();
                    if (error != ERROR_IO_PENDING)
                    {
                    #ifdef _DEBUG
                        if (error == ERROR_INVALID_PARAMETER)
                        {
                            // May be due to Advanced Format (4Kn) vs. DVD sector size. See the xwbtool -af switch.
                            OutputDebugStringA("ERROR: non-buffered async I/O failed: check disk sector size vs. streaming wave bank alignment!\n");
                        }
                    #endif
                        return HRESULT_FROM_WIN32(error);
                    }
                }

                mCurrentPosition += cbValid;

                mCurrentDiskReadBuffer = (entry + 1) % uint32_t(MAX_BUFFER_COUNT);

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
            const BOOL result = GetOverlappedResultEx(async, &mPackets[j].request, &cb, 0, FALSE);
        #else
            const BOOL result = GetOverlappedResult(async, &mPackets[j].request, &cb, FALSE);
        #endif
            if (result)
            {
                mPackets[j].state = State::READY;
            }
            else
            {
                const DWORD error = GetLastError();
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
        if (mSitching)
        {
            // Compute how many left-over bytes at the end of the previous packet (if any, they form the head of a partial block).
            uint32_t prevFrameStitch = (mPackets[mCurrentPlayBuffer].startPosition % mBlockAlign);

            if (prevFrameStitch > 0)
            {
                auto buffer = mPackets[mCurrentPlayBuffer].stitchBuffer;

                // Compute how many bytes at the start of our current packet are the tail of the partial block.
                thisFrameStitch = mBlockAlign - prevFrameStitch;

                const uint32_t k = (mCurrentPlayBuffer + MAX_BUFFER_COUNT - 1) % MAX_BUFFER_COUNT;
                if (mPackets[k].state == State::READY || mPackets[k].state == State::PLAYING)
                {
                    // Compute how many bytes at the start of the previous packet were the tail of the previous stitch block.
                    uint32_t prevFrameStitchOffset = (mPackets[k].startPosition % mBlockAlign);
                    prevFrameStitchOffset = (prevFrameStitchOffset > 0) ? (mBlockAlign - prevFrameStitchOffset) : 0u;

                    // Point to the start of the partial block's head in the previous packet.
                    const auto *prevBuffer = mPackets[k].buffer + prevFrameStitchOffset + mPackets[k].audioBytes;

                    // Merge the the head partial block in the previous packet with the tail partial block at the start of our packet.
                    memcpy(buffer, prevBuffer, prevFrameStitch);
                    memcpy(buffer + prevFrameStitch, ptr, thisFrameStitch);

                    // Submit stitch packet (only need to get notified if we aren't submitting another packet for this buffer).
                    XAUDIO2_BUFFER buf = {};
                    buf.AudioBytes = mBlockAlign;
                    buf.pAudioData = buffer;

                    if (endstream && (valid <= thisFrameStitch))
                    {
                        buf.Flags = XAUDIO2_END_OF_STREAM;
                        buf.pContext = &mPackets[mCurrentPlayBuffer].notify;
                    }
                #ifdef VERBOSE_TRACE
                    DebugTrace("INFO (Streaming): Stitch packet (%u + %u = %u)\n", prevFrameStitch, thisFrameStitch, mBlockAlign);
                #endif
                #ifdef DIRECTX_ENABLE_XWMA
                    if (mSeekCount > 0)
                    {
                        XAUDIO2_BUFFER_WMA wmaBuf = {};
                        wmaBuf.pDecodedPacketCumulativeBytes = mSeekTableCopy;
                        wmaBuf.PacketCount = 1;

                        const uint32_t seekOffset = (mPackets[k].startPosition + prevFrameStitchOffset + mPackets[k].audioBytes) / mBlockAlign;
                        assert(seekOffset > 0);
                        mSeekTableCopy[0] = mSeekTable[seekOffset] - mSeekTable[seekOffset - 1];

                        ThrowIfFailed(mBase.voice->SubmitSourceBuffer(&buf, &wmaBuf));
                    }
                    else
                    #endif // XWMA
                    {
                        ThrowIfFailed(mBase.voice->SubmitSourceBuffer(&buf));
                    }
                }

                ptr += thisFrameStitch;
            }

            // Compute valid audio bytes in our current packet.
            valid = ((valid - thisFrameStitch) / mBlockAlign) * mBlockAlign;
        }

        if (valid > 0)
        {
            // Record the audioBytes we actually submitted...
            mPackets[mCurrentPlayBuffer].audioBytes = valid;

            XAUDIO2_BUFFER buf = {};
            buf.Flags = (endstream) ? XAUDIO2_END_OF_STREAM : 0;
            buf.AudioBytes = valid;
            buf.pAudioData = ptr;
            buf.pContext = &mPackets[mCurrentPlayBuffer].notify;

        #ifdef DIRECTX_ENABLE_XWMA
            if (mSeekCount > 0)
            {
                XAUDIO2_BUFFER_WMA wmaBuf = {};

                wmaBuf.PacketCount = valid / mBlockAlign;

                const uint32_t seekOffset = mPackets[mCurrentPlayBuffer].startPosition / mBlockAlign;
                if (seekOffset > MAX_STREAMING_SEEK_PACKETS)
                {
                    DebugTrace("ERROR: xWMA packet seek count exceeds %zu\n", MAX_STREAMING_SEEK_PACKETS);
                    return E_FAIL;
                }
                else if (seekOffset > 0)
                {
                    for (uint32_t i = 0; i < wmaBuf.PacketCount; ++i)
                    {
                        mSeekTableCopy[i] = mSeekTable[i + seekOffset] - mSeekTable[seekOffset - 1];
                    }

                    wmaBuf.pDecodedPacketCumulativeBytes = mSeekTableCopy;
                }
                else
                {
                    wmaBuf.pDecodedPacketCumulativeBytes = mSeekTable;
                }

                ThrowIfFailed(mBase.voice->SubmitSourceBuffer(&buf, &wmaBuf));
            }
            else
            #endif // xWMA
            {
                ThrowIfFailed(mBase.voice->SubmitSourceBuffer(&buf));
            }
        }

        mPackets[mCurrentPlayBuffer].state = State::PLAYING;
        mCurrentPlayBuffer = (mCurrentPlayBuffer + 1) % uint32_t(MAX_BUFFER_COUNT);
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


// Move ctor/operator.
SoundStreamInstance::SoundStreamInstance(SoundStreamInstance&&) noexcept = default;
SoundStreamInstance& SoundStreamInstance::operator= (SoundStreamInstance&&) noexcept = default;


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


void SoundStreamInstance::Apply3D(const X3DAUDIO_LISTENER& listener, const X3DAUDIO_EMITTER& emitter, bool rhcoords)
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
    const SoundState state = pImpl->mBase.GetState(pImpl->mEndStream);
    if (state == STOPPED)
    {
        pImpl->mPlaying = false;
    }
    return state;
}


unsigned int SoundStreamInstance::GetChannelCount() const noexcept
{
    return pImpl->mBase.GetChannelCount();
}


IVoiceNotify* SoundStreamInstance::GetVoiceNotify() const noexcept
{
    return pImpl.get();
}
