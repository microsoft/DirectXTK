//--------------------------------------------------------------------------------------
// File: WAVFileReader.cpp
//
// Functions for loading WAV audio files
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "PlatformHelpers.h"
#include "WAVFileReader.h"

using namespace DirectX;


namespace
{
    //---------------------------------------------------------------------------------
    // .WAV files
    //---------------------------------------------------------------------------------
    constexpr uint32_t FOURCC_RIFF_TAG = MAKEFOURCC('R', 'I', 'F', 'F');
    constexpr uint32_t FOURCC_FORMAT_TAG = MAKEFOURCC('f', 'm', 't', ' ');
    constexpr uint32_t FOURCC_DATA_TAG = MAKEFOURCC('d', 'a', 't', 'a');
    constexpr uint32_t FOURCC_WAVE_FILE_TAG = MAKEFOURCC('W', 'A', 'V', 'E');
    constexpr uint32_t FOURCC_XWMA_FILE_TAG = MAKEFOURCC('X', 'W', 'M', 'A');
    constexpr uint32_t FOURCC_DLS_SAMPLE = MAKEFOURCC('w', 's', 'm', 'p');
    constexpr uint32_t FOURCC_MIDI_SAMPLE = MAKEFOURCC('s', 'm', 'p', 'l');
    constexpr uint32_t FOURCC_XWMA_DPDS = MAKEFOURCC('d', 'p', 'd', 's');
    constexpr uint32_t FOURCC_XMA_SEEK = MAKEFOURCC('s', 'e', 'e', 'k');

#pragma pack(push,1)
    struct RIFFChunk
    {
        uint32_t tag;
        uint32_t size;
    };

    struct RIFFChunkHeader
    {
        uint32_t tag;
        uint32_t size;
        uint32_t riff;
    };

    struct DLSLoop
    {
        static constexpr uint32_t LOOP_TYPE_FORWARD = 0x00000000;
        static constexpr uint32_t LOOP_TYPE_RELEASE = 0x00000001;

        uint32_t size;
        uint32_t loopType;
        uint32_t loopStart;
        uint32_t loopLength;
    };

    struct RIFFDLSSample
    {
        static constexpr uint32_t OPTIONS_NOTRUNCATION = 0x00000001;
        static constexpr uint32_t OPTIONS_NOCOMPRESSION = 0x00000002;

        uint32_t    size;
        uint16_t    unityNote;
        int16_t     fineTune;
        int32_t     gain;
        uint32_t    options;
        uint32_t    loopCount;
    };

    struct MIDILoop
    {
        static constexpr uint32_t LOOP_TYPE_FORWARD = 0x00000000;
        static constexpr uint32_t LOOP_TYPE_ALTERNATING = 0x00000001;
        static constexpr uint32_t LOOP_TYPE_BACKWARD = 0x00000002;

        uint32_t cuePointId;
        uint32_t type;
        uint32_t start;
        uint32_t end;
        uint32_t fraction;
        uint32_t playCount;
    };

    struct RIFFMIDISample
    {
        uint32_t        manufacturerId;
        uint32_t        productId;
        uint32_t        samplePeriod;
        uint32_t        unityNode;
        uint32_t        pitchFraction;
        uint32_t        SMPTEFormat;
        uint32_t        SMPTEOffset;
        uint32_t        loopCount;
        uint32_t        samplerData;
    };
#pragma pack(pop)

    static_assert(sizeof(RIFFChunk) == 8, "structure size mismatch");
    static_assert(sizeof(RIFFChunkHeader) == 12, "structure size mismatch");
    static_assert(sizeof(DLSLoop) == 16, "structure size mismatch");
    static_assert(sizeof(RIFFDLSSample) == 20, "structure size mismatch");
    static_assert(sizeof(MIDILoop) == 24, "structure size mismatch");
    static_assert(sizeof(RIFFMIDISample) == 36, "structure size mismatch");

    //---------------------------------------------------------------------------------
    const RIFFChunk* FindChunk(
        _In_reads_bytes_(sizeBytes) const uint8_t* data,
        _In_ size_t sizeBytes,
        _In_ uint32_t tag) noexcept
    {
        if (!data)
            return nullptr;

        const uint8_t* ptr = data;
        const uint8_t* end = data + sizeBytes;

        while (end > (ptr + sizeof(RIFFChunk)))
        {
            auto header = reinterpret_cast<const RIFFChunk*>(ptr);
            if (header->tag == tag)
                return header;

            auto offset = header->size + sizeof(RIFFChunk);
            ptr += offset;
        }

        return nullptr;
    }


    //---------------------------------------------------------------------------------
    HRESULT WaveFindFormatAndData(
        _In_reads_bytes_(wavDataSize) const uint8_t* wavData,
        _In_ size_t wavDataSize,
        _Outptr_ const WAVEFORMATEX** pwfx,
        _Outptr_ const uint8_t** pdata,
        _Out_ uint32_t* dataSize,
        _Out_ bool& dpds,
        _Out_ bool& seek) noexcept
    {
        if (!wavData || !pwfx)
            return E_POINTER;

        dpds = seek = false;

        if (wavDataSize < (sizeof(RIFFChunk) * 2 + sizeof(uint32_t) + sizeof(WAVEFORMAT)))
        {
            return E_FAIL;
        }

        const uint8_t* wavEnd = wavData + wavDataSize;

        // Locate RIFF 'WAVE'
        auto riffChunk = FindChunk(wavData, wavDataSize, FOURCC_RIFF_TAG);
        if (!riffChunk || riffChunk->size < 4)
        {
            return E_FAIL;
        }

        auto riffHeader = reinterpret_cast<const RIFFChunkHeader*>(riffChunk);
        if (riffHeader->riff != FOURCC_WAVE_FILE_TAG && riffHeader->riff != FOURCC_XWMA_FILE_TAG)
        {
            return E_FAIL;
        }

        // Locate 'fmt '
        auto ptr = reinterpret_cast<const uint8_t*>(riffHeader) + sizeof(RIFFChunkHeader);
        if ((ptr + sizeof(RIFFChunk)) > wavEnd)
        {
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
        }

        auto fmtChunk = FindChunk(ptr, riffHeader->size, FOURCC_FORMAT_TAG);
        if (!fmtChunk || fmtChunk->size < sizeof(PCMWAVEFORMAT))
        {
            return E_FAIL;
        }

        ptr = reinterpret_cast<const uint8_t*>(fmtChunk) + sizeof(RIFFChunk);
        if (ptr + fmtChunk->size > wavEnd)
        {
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
        }

        auto wf = reinterpret_cast<const WAVEFORMAT*>(ptr);

        // Validate WAVEFORMAT (focused on chunk size and format tag, not other data that XAUDIO2 will validate)
        switch (wf->wFormatTag)
        {
            case WAVE_FORMAT_PCM:
            case WAVE_FORMAT_IEEE_FLOAT:
                // Can be a PCMWAVEFORMAT (16 bytes) or WAVEFORMATEX (18 bytes)
                // We validiated chunk as at least sizeof(PCMWAVEFORMAT) above
                break;

            default:
            {
                if (fmtChunk->size < sizeof(WAVEFORMATEX))
                {
                    return E_FAIL;
                }

                auto wfx = reinterpret_cast<const WAVEFORMATEX*>(ptr);

                if (fmtChunk->size < (sizeof(WAVEFORMATEX) + wfx->cbSize))
                {
                    return E_FAIL;
                }

                switch (wfx->wFormatTag)
                {
                    case WAVE_FORMAT_WMAUDIO2:
                    case WAVE_FORMAT_WMAUDIO3:
                        dpds = true;
                        break;

                    case  0x166 /*WAVE_FORMAT_XMA2*/: // XMA2 is supported by Xbox One
                        if ((fmtChunk->size < 52 /*sizeof(XMA2WAVEFORMATEX)*/) || (wfx->cbSize < 34 /*( sizeof(XMA2WAVEFORMATEX) - sizeof(WAVEFORMATEX) )*/))
                        {
                            return E_FAIL;
                        }
                        seek = true;
                        break;

                    case WAVE_FORMAT_ADPCM:
                        if ((fmtChunk->size < (sizeof(WAVEFORMATEX) + 32)) || (wfx->cbSize < 32 /*MSADPCM_FORMAT_EXTRA_BYTES*/))
                        {
                            return E_FAIL;
                        }
                        break;

                    case WAVE_FORMAT_EXTENSIBLE:
                        if ((fmtChunk->size < sizeof(WAVEFORMATEXTENSIBLE)) || (wfx->cbSize < (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))))
                        {
                            return E_FAIL;
                        }
                        else
                        {
                            static const GUID s_wfexBase = { 0x00000000, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

                            auto wfex = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(ptr);

                            if (memcmp(reinterpret_cast<const BYTE*>(&wfex->SubFormat) + sizeof(DWORD),
                                reinterpret_cast<const BYTE*>(&s_wfexBase) + sizeof(DWORD), sizeof(GUID) - sizeof(DWORD)) != 0)
                            {
                                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                            }

                            switch (wfex->SubFormat.Data1)
                            {
                                case WAVE_FORMAT_PCM:
                                case WAVE_FORMAT_IEEE_FLOAT:
                                    break;

                                // MS-ADPCM and XMA2 are not supported as WAVEFORMATEXTENSIBLE

                                case WAVE_FORMAT_WMAUDIO2:
                                case WAVE_FORMAT_WMAUDIO3:
                                    dpds = true;
                                    break;

                                default:
                                    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                            }

                        }
                        break;

                    default:
                        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                }
            }
        }

        // Locate 'data'
        ptr = reinterpret_cast<const uint8_t*>(riffHeader) + sizeof(RIFFChunkHeader);
        if ((ptr + sizeof(RIFFChunk)) > wavEnd)
        {
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
        }

        auto dataChunk = FindChunk(ptr, riffChunk->size, FOURCC_DATA_TAG);
        if (!dataChunk || !dataChunk->size)
        {
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        ptr = reinterpret_cast<const uint8_t*>(dataChunk) + sizeof(RIFFChunk);
        if (ptr + dataChunk->size > wavEnd)
        {
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
        }

        *pwfx = reinterpret_cast<const WAVEFORMATEX*>(wf);
        *pdata = ptr;
        *dataSize = dataChunk->size;
        return S_OK;
    }


    //---------------------------------------------------------------------------------
    HRESULT WaveFindLoopInfo(
        _In_reads_bytes_(wavDataSize) const uint8_t* wavData,
        _In_ size_t wavDataSize,
        _Out_ uint32_t* pLoopStart,
        _Out_ uint32_t* pLoopLength) noexcept
    {
        if (!wavData || !pLoopStart || !pLoopLength)
            return E_POINTER;

        if (wavDataSize < (sizeof(RIFFChunk) + sizeof(uint32_t)))
        {
            return E_FAIL;
        }

        *pLoopStart = 0;
        *pLoopLength = 0;

        const uint8_t* wavEnd = wavData + wavDataSize;

        // Locate RIFF 'WAVE'
        auto riffChunk = FindChunk(wavData, wavDataSize, FOURCC_RIFF_TAG);
        if (!riffChunk || riffChunk->size < 4)
        {
            return E_FAIL;
        }

        auto riffHeader = reinterpret_cast<const RIFFChunkHeader*>(riffChunk);
        if (riffHeader->riff == FOURCC_XWMA_FILE_TAG)
        {
            // xWMA files do not contain loop information
            return S_OK;
        }

        if (riffHeader->riff != FOURCC_WAVE_FILE_TAG)
        {
            return E_FAIL;
        }

        // Locate 'wsmp' (DLS Chunk)
        auto ptr = reinterpret_cast<const uint8_t*>(riffHeader) + sizeof(RIFFChunkHeader);
        if ((ptr + sizeof(RIFFChunk)) > wavEnd)
        {
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
        }

        auto dlsChunk = FindChunk(ptr, riffChunk->size, FOURCC_DLS_SAMPLE);
        if (dlsChunk)
        {
            ptr = reinterpret_cast<const uint8_t*>(dlsChunk) + sizeof(RIFFChunk);
            if (ptr + dlsChunk->size > wavEnd)
            {
                return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
            }

            if (dlsChunk->size >= sizeof(RIFFDLSSample))
            {
                auto dlsSample = reinterpret_cast<const RIFFDLSSample*>(ptr);

                if (dlsChunk->size >= (dlsSample->size + dlsSample->loopCount * sizeof(DLSLoop)))
                {
                    auto loops = reinterpret_cast<const DLSLoop*>(ptr + dlsSample->size);
                    for (uint32_t j = 0; j < dlsSample->loopCount; ++j)
                    {
                        if ((loops[j].loopType == DLSLoop::LOOP_TYPE_FORWARD || loops[j].loopType == DLSLoop::LOOP_TYPE_RELEASE))
                        {
                            // Return 'forward' loop
                            *pLoopStart = loops[j].loopStart;
                            *pLoopLength = loops[j].loopLength;
                            return S_OK;
                        }
                    }
                }
            }
        }

        // Locate 'smpl' (Sample Chunk)
        auto midiChunk = FindChunk(ptr, riffChunk->size, FOURCC_MIDI_SAMPLE);
        if (midiChunk)
        {
            ptr = reinterpret_cast<const uint8_t*>(midiChunk) + sizeof(RIFFChunk);
            if (ptr + midiChunk->size > wavEnd)
            {
                return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
            }

            if (midiChunk->size >= sizeof(RIFFMIDISample))
            {
                auto midiSample = reinterpret_cast<const RIFFMIDISample*>(ptr);

                if (midiChunk->size >= (sizeof(RIFFMIDISample) + midiSample->loopCount * sizeof(MIDILoop)))
                {
                    auto loops = reinterpret_cast<const MIDILoop*>(ptr + sizeof(RIFFMIDISample));
                    for (uint32_t j = 0; j < midiSample->loopCount; ++j)
                    {
                        if (loops[j].type == MIDILoop::LOOP_TYPE_FORWARD)
                        {
                            // Return 'forward' loop
                            *pLoopStart = loops[j].start;
                            *pLoopLength = loops[j].end - loops[j].start + 1;
                            return S_OK;
                        }
                    }
                }
            }
        }

        return S_OK;
    }


    //---------------------------------------------------------------------------------
    HRESULT WaveFindTable(
        _In_reads_bytes_(wavDataSize) const uint8_t* wavData,
        _In_ size_t wavDataSize,
        _In_ uint32_t tag,
        _Outptr_result_maybenull_ const uint32_t** pData,
        _Out_ uint32_t* dataCount) noexcept
    {
        if (!wavData || !pData || !dataCount)
            return E_POINTER;

        if (wavDataSize < (sizeof(RIFFChunk) + sizeof(uint32_t)))
        {
            return E_FAIL;
        }

        *pData = nullptr;
        *dataCount = 0;

        const uint8_t* wavEnd = wavData + wavDataSize;

        // Locate RIFF 'WAVE'
        auto riffChunk = FindChunk(wavData, wavDataSize, FOURCC_RIFF_TAG);
        if (!riffChunk || riffChunk->size < 4)
        {
            return E_FAIL;
        }

        auto riffHeader = reinterpret_cast<const RIFFChunkHeader*>(riffChunk);
        if (riffHeader->riff != FOURCC_WAVE_FILE_TAG && riffHeader->riff != FOURCC_XWMA_FILE_TAG)
        {
            return E_FAIL;
        }

        // Locate tag
        auto ptr = reinterpret_cast<const uint8_t*>(riffHeader) + sizeof(RIFFChunkHeader);
        if ((ptr + sizeof(RIFFChunk)) > wavEnd)
        {
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
        }

        auto tableChunk = FindChunk(ptr, riffChunk->size, tag);
        if (tableChunk)
        {
            ptr = reinterpret_cast<const uint8_t*>(tableChunk) + sizeof(RIFFChunk);
            if (ptr + tableChunk->size > wavEnd)
            {
                return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
            }

            if ((tableChunk->size % sizeof(uint32_t)) != 0)
            {
                return E_FAIL;
            }

            *pData = reinterpret_cast<const uint32_t*>(ptr);
            *dataCount = tableChunk->size / 4;
        }

        return S_OK;
    }


    //---------------------------------------------------------------------------------
    HRESULT LoadAudioFromFile(
        _In_z_ const wchar_t* szFileName,
        _Inout_ std::unique_ptr<uint8_t[]>& wavData,
        _Out_ DWORD* bytesRead) noexcept
    {
        if (!szFileName)
            return E_INVALIDARG;

        // open the file
    #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        ScopedHandle hFile(safe_handle(CreateFile2(
            szFileName,
            GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
            nullptr)));
    #else
        ScopedHandle hFile(safe_handle(CreateFileW(
            szFileName,
            GENERIC_READ, FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
            nullptr)));
    #endif

        if (!hFile)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        // Get the file size
        FILE_STANDARD_INFO fileInfo;
        if (!GetFileInformationByHandleEx(hFile.get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        // File is too big for 32-bit allocation, so reject read
        if (fileInfo.EndOfFile.HighPart > 0)
        {
            return E_FAIL;
        }

        // Need at least enough data to have a valid minimal WAV file
        if (fileInfo.EndOfFile.LowPart < (sizeof(RIFFChunk) * 2 + sizeof(DWORD) + sizeof(WAVEFORMAT)))
        {
            return E_FAIL;
        }

        // create enough space for the file data
        wavData.reset(new (std::nothrow) uint8_t[fileInfo.EndOfFile.LowPart]);
        if (!wavData)
        {
            return E_OUTOFMEMORY;
        }

        // read the data in
        if (!ReadFile(hFile.get(),
            wavData.get(),
            fileInfo.EndOfFile.LowPart,
            bytesRead,
            nullptr
            ))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        return (*bytesRead < fileInfo.EndOfFile.LowPart) ? E_FAIL : S_OK;
    }
}

//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::LoadWAVAudioInMemory(
    const uint8_t* wavData,
    size_t wavDataSize,
    const WAVEFORMATEX** wfx,
    const uint8_t** startAudio,
    uint32_t* audioBytes) noexcept
{
    if (!wavData || !wfx || !startAudio || !audioBytes)
        return E_INVALIDARG;

    *wfx = nullptr;
    *startAudio = nullptr;
    *audioBytes = 0;

    // Need at least enough data to have a valid minimal WAV file
    if (wavDataSize < (sizeof(RIFFChunk) * 2 + sizeof(DWORD) + sizeof(WAVEFORMAT)))
    {
        return E_FAIL;
    }

    bool dpds, seek;
    HRESULT hr = WaveFindFormatAndData(wavData, wavDataSize, wfx, startAudio, audioBytes, dpds, seek);
    if (FAILED(hr))
        return hr;

    return (dpds || seek) ? E_FAIL : S_OK;
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::LoadWAVAudioFromFile(
    const wchar_t* szFileName,
    std::unique_ptr<uint8_t[]>& wavData,
    const WAVEFORMATEX** wfx,
    const uint8_t** startAudio,
    uint32_t* audioBytes) noexcept
{
    if (!szFileName || !wfx || !startAudio || !audioBytes)
        return E_INVALIDARG;

    *wfx = nullptr;
    *startAudio = nullptr;
    *audioBytes = 0;

    DWORD bytesRead = 0;
    HRESULT hr = LoadAudioFromFile(szFileName, wavData, &bytesRead);
    if (FAILED(hr))
    {
        return hr;
    }

    bool dpds, seek;
    hr = WaveFindFormatAndData(wavData.get(), bytesRead, wfx, startAudio, audioBytes, dpds, seek);
    if (FAILED(hr))
        return hr;

    return (dpds || seek) ? E_FAIL : S_OK;
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::LoadWAVAudioInMemoryEx(
    const uint8_t* wavData,
    size_t wavDataSize,
    DirectX::WAVData& result) noexcept
{
    if (!wavData)
        return E_INVALIDARG;

    memset(&result, 0, sizeof(result));

    // Need at least enough data to have a valid minimal WAV file
    if (wavDataSize < (sizeof(RIFFChunk) * 2 + sizeof(DWORD) + sizeof(WAVEFORMAT)))
    {
        return E_FAIL;
    }

    bool dpds, seek;
    HRESULT hr = WaveFindFormatAndData(wavData, wavDataSize, &result.wfx, &result.startAudio, &result.audioBytes, dpds, seek);
    if (FAILED(hr))
        return hr;

    hr = WaveFindLoopInfo(wavData, wavDataSize, &result.loopStart, &result.loopLength);
    if (FAILED(hr))
        return hr;

    if (dpds)
    {
        hr = WaveFindTable(wavData, wavDataSize, FOURCC_XWMA_DPDS, &result.seek, &result.seekCount);
        if (FAILED(hr))
            return hr;
    }
    else if (seek)
    {
        hr = WaveFindTable(wavData, wavDataSize, FOURCC_XMA_SEEK, &result.seek, &result.seekCount);
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::LoadWAVAudioFromFileEx(
    const wchar_t* szFileName,
    std::unique_ptr<uint8_t[]>& wavData,
    DirectX::WAVData& result) noexcept
{
    if (!szFileName)
        return E_INVALIDARG;

    memset(&result, 0, sizeof(result));

    DWORD bytesRead = 0;
    HRESULT hr = LoadAudioFromFile(szFileName, wavData, &bytesRead);
    if (FAILED(hr))
    {
        return hr;
    }

    bool dpds, seek;
    hr = WaveFindFormatAndData(wavData.get(), bytesRead, &result.wfx, &result.startAudio, &result.audioBytes, dpds, seek);
    if (FAILED(hr))
        return hr;

    hr = WaveFindLoopInfo(wavData.get(), bytesRead, &result.loopStart, &result.loopLength);
    if (FAILED(hr))
        return hr;

    if (dpds)
    {
        hr = WaveFindTable(wavData.get(), bytesRead, FOURCC_XWMA_DPDS, &result.seek, &result.seekCount);
        if (FAILED(hr))
            return hr;
    }
    else if (seek)
    {
        hr = WaveFindTable(wavData.get(), bytesRead, FOURCC_XMA_SEEK, &result.seek, &result.seekCount);
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}
