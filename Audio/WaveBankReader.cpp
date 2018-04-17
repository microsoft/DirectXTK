//--------------------------------------------------------------------------------------
// File: WaveBankReader.cpp
//
// Functions for loading audio data from Wave Banks
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "WaveBankReader.h"
#include "Audio.h"
#include "PlatformHelpers.h"

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <apu.h>
#endif


namespace
{

//--------------------------------------------------------------------------------------
#pragma pack(push, 1)

    static const size_t DVD_SECTOR_SIZE = 2048;
    static const size_t DVD_BLOCK_SIZE = DVD_SECTOR_SIZE * 16;

    static const size_t ALIGNMENT_MIN = 4;
    static const size_t ALIGNMENT_DVD = DVD_SECTOR_SIZE;

    static const size_t MAX_DATA_SEGMENT_SIZE = 0xFFFFFFFF;
    static const size_t MAX_COMPACT_DATA_SEGMENT_SIZE = 0x001FFFFF;

    struct REGION
    {
        uint32_t    dwOffset;   // Region offset, in bytes.
        uint32_t    dwLength;   // Region length, in bytes.

        void BigEndian()
        {
            dwOffset = _byteswap_ulong(dwOffset);
            dwLength = _byteswap_ulong(dwLength);
        }
    };

    struct SAMPLEREGION
    {
        uint32_t    dwStartSample;  // Start sample for the region.
        uint32_t    dwTotalSamples; // Region length in samples.

        void BigEndian()
        {
            dwStartSample = _byteswap_ulong(dwStartSample);
            dwTotalSamples = _byteswap_ulong(dwTotalSamples);
        }
    };

    struct HEADER
    {
        static const uint32_t SIGNATURE = 'DNBW';
        static const uint32_t BE_SIGNATURE = 'WBND';
        static const uint32_t VERSION = 44;

        enum SEGIDX
        {
            SEGIDX_BANKDATA = 0,       // Bank data
            SEGIDX_ENTRYMETADATA,      // Entry meta-data
            SEGIDX_SEEKTABLES,         // Storage for seek tables for the encoded waves.
            SEGIDX_ENTRYNAMES,         // Entry friendly names
            SEGIDX_ENTRYWAVEDATA,      // Entry wave data
            SEGIDX_COUNT
        };

        uint32_t    dwSignature;            // File signature
        uint32_t    dwVersion;              // Version of the tool that created the file
        uint32_t    dwHeaderVersion;        // Version of the file format
        REGION      Segments[SEGIDX_COUNT]; // Segment lookup table

        void BigEndian()
        {
            // Leave dwSignature alone as indicator of BE vs. LE

            dwVersion = _byteswap_ulong(dwVersion);
            dwHeaderVersion = _byteswap_ulong(dwHeaderVersion);
            for (size_t j = 0; j < SEGIDX_COUNT; ++j)
            {
                Segments[j].BigEndian();
            }
        }
    };

#pragma warning( disable : 4201 4203 )

    union MINIWAVEFORMAT
    {
        static const uint32_t TAG_PCM = 0x0;
        static const uint32_t TAG_XMA = 0x1;
        static const uint32_t TAG_ADPCM = 0x2;
        static const uint32_t TAG_WMA = 0x3;

        static const uint32_t BITDEPTH_8 = 0x0; // PCM only
        static const uint32_t BITDEPTH_16 = 0x1; // PCM only

        static const size_t ADPCM_BLOCKALIGN_CONVERSION_OFFSET = 22;

        struct
        {
            uint32_t       wFormatTag : 2;        // Format tag
            uint32_t       nChannels : 3;        // Channel count (1 - 6)
            uint32_t       nSamplesPerSec : 18;       // Sampling rate
            uint32_t       wBlockAlign : 8;        // Block alignment.  For WMA, lower 6 bits block alignment index, upper 2 bits bytes-per-second index.
            uint32_t       wBitsPerSample : 1;        // Bits per sample (8 vs. 16, PCM only); WMAudio2/WMAudio3 (for WMA)
        };

        uint32_t           dwValue;

        void BigEndian()
        {
            dwValue = _byteswap_ulong(dwValue);
        }

        WORD BitsPerSample() const
        {
            if (wFormatTag == TAG_XMA)
                return 16; // XMA_OUTPUT_SAMPLE_BITS == 16
            if (wFormatTag == TAG_WMA)
                return 16;
            if (wFormatTag == TAG_ADPCM)
                return 4; // MSADPCM_BITS_PER_SAMPLE == 4

            // wFormatTag must be TAG_PCM (2 bits can only represent 4 different values)
            return (wBitsPerSample == BITDEPTH_16) ? 16 : 8;
        }

        DWORD BlockAlign() const
        {
            switch (wFormatTag)
            {
                case TAG_PCM:
                    return wBlockAlign;

                case TAG_XMA:
                    return (nChannels * 16 / 8); // XMA_OUTPUT_SAMPLE_BITS = 16

                case TAG_ADPCM:
                    return (wBlockAlign + ADPCM_BLOCKALIGN_CONVERSION_OFFSET) * nChannels;

                case TAG_WMA:
                {
                    static const uint32_t aWMABlockAlign[] =
                    {
                        929,
                        1487,
                        1280,
                        2230,
                        8917,
                        8192,
                        4459,
                        5945,
                        2304,
                        1536,
                        1485,
                        1008,
                        2731,
                        4096,
                        6827,
                        5462,
                        1280
                    };

                    uint32_t dwBlockAlignIndex = wBlockAlign & 0x1F;
                    if (dwBlockAlignIndex < _countof(aWMABlockAlign))
                        return aWMABlockAlign[dwBlockAlignIndex];
                }
                break;
            }

            return 0;
        }

        DWORD AvgBytesPerSec() const
        {
            switch (wFormatTag)
            {
                case TAG_PCM:
                    return nSamplesPerSec * wBlockAlign;

                case TAG_XMA:
                    return nSamplesPerSec * BlockAlign();

                case TAG_ADPCM:
                {
                    uint32_t blockAlign = BlockAlign();
                    uint32_t samplesPerAdpcmBlock = AdpcmSamplesPerBlock();
                    return blockAlign * nSamplesPerSec / samplesPerAdpcmBlock;
                }
                break;

                case TAG_WMA:
                {
                    static const uint32_t aWMAAvgBytesPerSec[] =
                    {
                        12000,
                        24000,
                        4000,
                        6000,
                        8000,
                        20000,
                        2500
                    };
                    // bitrate = entry * 8

                    uint32_t dwBytesPerSecIndex = wBlockAlign >> 5;
                    if (dwBytesPerSecIndex < _countof(aWMAAvgBytesPerSec))
                        return aWMAAvgBytesPerSec[dwBytesPerSecIndex];
                }
                break;
            }

            return 0;
        }

        DWORD AdpcmSamplesPerBlock() const
        {
            uint32_t nBlockAlign = (wBlockAlign + ADPCM_BLOCKALIGN_CONVERSION_OFFSET) * nChannels;
            return nBlockAlign * 2 / (uint32_t)nChannels - 12;
        }

        void AdpcmFillCoefficientTable(ADPCMWAVEFORMAT *fmt) const
        {
            // These are fixed since we are always using MS ADPCM
            fmt->wNumCoef = 7 /* MSADPCM_NUM_COEFFICIENTS */;

            static ADPCMCOEFSET aCoef[7] = { { 256, 0}, {512, -256}, {0,0}, {192,64}, {240,0}, {460, -208}, {392,-232} };
            memcpy(&fmt->aCoef, aCoef, sizeof(aCoef));
        }
    };

    struct BANKDATA
    {
        static const size_t BANKNAME_LENGTH = 64;

        static const uint32_t TYPE_BUFFER = 0x00000000;
        static const uint32_t TYPE_STREAMING = 0x00000001;
        static const uint32_t TYPE_MASK = 0x00000001;

        static const uint32_t FLAGS_ENTRYNAMES = 0x00010000;
        static const uint32_t FLAGS_COMPACT = 0x00020000;
        static const uint32_t FLAGS_SYNC_DISABLED = 0x00040000;
        static const uint32_t FLAGS_SEEKTABLES = 0x00080000;
        static const uint32_t FLAGS_MASK = 0x000F0000;

        uint32_t        dwFlags;                        // Bank flags
        uint32_t        dwEntryCount;                   // Number of entries in the bank
        char            szBankName[BANKNAME_LENGTH];    // Bank friendly name
        uint32_t        dwEntryMetaDataElementSize;     // Size of each entry meta-data element, in bytes
        uint32_t        dwEntryNameElementSize;         // Size of each entry name element, in bytes
        uint32_t        dwAlignment;                    // Entry alignment, in bytes
        MINIWAVEFORMAT  CompactFormat;                  // Format data for compact bank
        FILETIME        BuildTime;                      // Build timestamp

        void BigEndian()
        {
            dwFlags = _byteswap_ulong(dwFlags);
            dwEntryCount = _byteswap_ulong(dwEntryCount);
            dwEntryMetaDataElementSize = _byteswap_ulong(dwEntryMetaDataElementSize);
            dwEntryNameElementSize = _byteswap_ulong(dwEntryNameElementSize);
            dwAlignment = _byteswap_ulong(dwAlignment);
            CompactFormat.BigEndian();
            BuildTime.dwLowDateTime = _byteswap_ulong(BuildTime.dwLowDateTime);
            BuildTime.dwHighDateTime = _byteswap_ulong(BuildTime.dwHighDateTime);
        }
    };

    struct ENTRY
    {
        static const uint32_t FLAGS_READAHEAD = 0x00000001;     // Enable stream read-ahead
        static const uint32_t FLAGS_LOOPCACHE = 0x00000002;     // One or more looping sounds use this wave
        static const uint32_t FLAGS_REMOVELOOPTAIL = 0x00000004;// Remove data after the end of the loop region
        static const uint32_t FLAGS_IGNORELOOP = 0x00000008;    // Used internally when the loop region can't be used
        static const uint32_t FLAGS_MASK = 0x00000008;

        union
        {
            struct
            {
                // Entry flags
                uint32_t                   dwFlags : 4;

                // Duration of the wave, in units of one sample.
                // For instance, a ten second long wave sampled
                // at 48KHz would have a duration of 480,000.
                // This value is not affected by the number of
                // channels, the number of bits per sample, or the
                // compression format of the wave.
                uint32_t                   Duration : 28;
            };
            uint32_t dwFlagsAndDuration;
        };

        MINIWAVEFORMAT  Format;         // Entry format.
        REGION          PlayRegion;     // Region within the wave data segment that contains this entry.
        SAMPLEREGION    LoopRegion;     // Region within the wave data (in samples) that should loop.

        void BigEndian()
        {
            dwFlagsAndDuration = _byteswap_ulong(dwFlagsAndDuration);
            Format.BigEndian();
            PlayRegion.BigEndian();
            LoopRegion.BigEndian();
        }
    };

    struct ENTRYCOMPACT
    {
        uint32_t       dwOffset : 21;       // Data offset, in multiplies of the bank alignment
        uint32_t       dwLengthDeviation : 11;       // Data length deviation, in bytes

        void BigEndian()
        {
            *reinterpret_cast<uint32_t*>(this) = _byteswap_ulong(*reinterpret_cast<const uint32_t*>(this));
        }

        void ComputeLocations(DWORD& offset, DWORD& length, uint32_t index, const HEADER& header, const BANKDATA& data, const ENTRYCOMPACT* entries) const
        {
            offset = dwOffset * data.dwAlignment;

            if (index < (data.dwEntryCount - 1))
            {
                length = (entries[index + 1].dwOffset * data.dwAlignment) - offset - dwLengthDeviation;
            }
            else
            {
                length = header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwLength - offset - dwLengthDeviation;
            }
        }

        static uint32_t GetDuration(DWORD length, const BANKDATA& data, const uint32_t* seekTable)
        {
            switch (data.CompactFormat.wFormatTag)
            {
                case MINIWAVEFORMAT::TAG_ADPCM:
                {
                    uint32_t duration = (length / data.CompactFormat.BlockAlign()) * data.CompactFormat.AdpcmSamplesPerBlock();
                    uint32_t partial = length % data.CompactFormat.BlockAlign();
                    if (partial)
                    {
                        if (partial >= (7 * data.CompactFormat.nChannels))
                            duration += (partial * 2 / data.CompactFormat.nChannels - 12);
                    }
                    return duration;
                }

                case MINIWAVEFORMAT::TAG_WMA:
                    if (seekTable)
                    {
                        uint32_t seekCount = *seekTable;
                        if (seekCount > 0)
                        {
                            return seekTable[seekCount] / uint32_t(2 * data.CompactFormat.nChannels);
                        }
                    }
                    return 0;

                case MINIWAVEFORMAT::TAG_XMA:
                    if (seekTable)
                    {
                        uint32_t seekCount = *seekTable;
                        if (seekCount > 0)
                        {
                            return seekTable[seekCount];
                        }
                    }
                    return 0;

                default:
                    return uint32_t((uint64_t(length) * 8)
                                    / (uint64_t(data.CompactFormat.BitsPerSample()) * uint64_t(data.CompactFormat.nChannels)));
            }
        }
    };

#pragma pack(pop)

    inline const uint32_t* FindSeekTable(uint32_t index, const uint8_t* seekTable, const HEADER& header, const BANKDATA& data)
    {
        if (!seekTable || index >= data.dwEntryCount)
            return nullptr;

        uint32_t seekSize = header.Segments[HEADER::SEGIDX_SEEKTABLES].dwLength;

        if ((index * sizeof(uint32_t)) > seekSize)
            return nullptr;

        auto table = reinterpret_cast<const uint32_t*>(seekTable);
        uint32_t offset = table[index];
        if (offset == uint32_t(-1))
            return nullptr;

        offset += sizeof(uint32_t) * data.dwEntryCount;

        if (offset > seekSize)
            return nullptr;

        return reinterpret_cast<const uint32_t*>(seekTable + offset);
    }

};

static_assert(sizeof(REGION) == 8, "Mismatch with xact3wb.h");
static_assert(sizeof(SAMPLEREGION) == 8, "Mismatch with xact3wb.h");
static_assert(sizeof(HEADER) == 52, "Mismatch with xact3wb.h");
static_assert(sizeof(ENTRY) == 24, "Mismatch with xact3wb.h");
static_assert(sizeof(MINIWAVEFORMAT) == 4, "Mismatch with xact3wb.h");
static_assert(sizeof(ENTRY) == 24, "Mismatch with xact3wb.h");
static_assert(sizeof(ENTRYCOMPACT) == 4, "Mismatch with xact3wb.h");
static_assert(sizeof(BANKDATA) == 96, "Mismatch with xact3wb.h");

using namespace DirectX;

//--------------------------------------------------------------------------------------
class WaveBankReader::Impl
{
public:
    Impl() throw() :
        m_async(INVALID_HANDLE_VALUE),
        m_prepared(false)
    #if defined(_XBOX_ONE) && defined(_TITLE)
        , m_xmaMemory(nullptr)
    #endif
    {
        memset(&m_header, 0, sizeof(HEADER));
        memset(&m_data, 0, sizeof(BANKDATA));
        memset(&m_request, 0, sizeof(OVERLAPPED));
    }

    ~Impl() { Close(); }

    HRESULT Open(_In_z_ const wchar_t* szFileName);
    void Close();

    HRESULT GetFormat(_In_ uint32_t index, _Out_writes_bytes_(maxsize) WAVEFORMATEX* pFormat, _In_ size_t maxsize) const;

    HRESULT GetWaveData(_In_ uint32_t index, _Outptr_ const uint8_t** pData, _Out_ uint32_t& dataSize) const;

    HRESULT GetSeekTable(_In_ uint32_t index, _Out_ const uint32_t** pData, _Out_ uint32_t& dataCount, _Out_ uint32_t& tag) const;

    HRESULT GetMetadata(_In_ uint32_t index, _Out_ Metadata& metadata) const;

    bool UpdatePrepared();

    void Clear()
    {
        memset(&m_header, 0, sizeof(HEADER));
        memset(&m_data, 0, sizeof(BANKDATA));

        m_names.clear();
        m_entries.reset();
        m_seekData.reset();
        m_waveData.reset();

    #if defined(_XBOX_ONE) && defined(_TITLE)
        if (m_xmaMemory)
        {
            ApuFree(m_xmaMemory);
            m_xmaMemory = nullptr;
        }
    #endif
    }

    HANDLE                              m_async;
    ScopedHandle                        m_event;
    OVERLAPPED                          m_request;
    bool                                m_prepared;

    HEADER                              m_header;
    BANKDATA                            m_data;
    std::map<std::string, uint32_t>     m_names;

private:
    std::unique_ptr<uint8_t[]>          m_entries;
    std::unique_ptr<uint8_t[]>          m_seekData;
    std::unique_ptr<uint8_t[]>          m_waveData;

#if defined(_XBOX_ONE) && defined(_TITLE)
public:
    void*                               m_xmaMemory;
#endif
};


_Use_decl_annotations_
HRESULT WaveBankReader::Impl::Open(const wchar_t* szFileName)
{
    Close();
    Clear();

    m_prepared = false;

    m_event.reset(CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!m_event)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    CREATEFILE2_EXTENDED_PARAMETERS params = { sizeof(CREATEFILE2_EXTENDED_PARAMETERS), 0 };
    params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    params.dwFileFlags = FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN;
    ScopedHandle hFile(safe_handle(CreateFile2(szFileName,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       OPEN_EXISTING,
                       &params)));
#else
    ScopedHandle hFile(safe_handle(CreateFileW(szFileName,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       nullptr,
                       OPEN_EXISTING,
                       FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN,
                       nullptr)));
#endif

    if (!hFile)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Read and verify header
    OVERLAPPED request = {};
    request.hEvent = m_event.get();

    bool wait = false;
    if (!ReadFile(hFile.get(), &m_header, sizeof(m_header), nullptr, &request))
    {
        DWORD error = GetLastError();
        if (error != ERROR_IO_PENDING)
            return HRESULT_FROM_WIN32(error);
        wait = true;
    }

    DWORD bytes;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    BOOL result = GetOverlappedResultEx(hFile.get(), &request, &bytes, INFINITE, FALSE);
#else
    if (wait)
        (void)WaitForSingleObject(m_event.get(), INFINITE);

    BOOL result = GetOverlappedResult(hFile.get(), &request, &bytes, FALSE);
#endif

    if (!result || (bytes != sizeof(m_header)))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (m_header.dwSignature != HEADER::SIGNATURE && m_header.dwSignature != HEADER::BE_SIGNATURE)
    {
        return E_FAIL;
    }

    bool be = (m_header.dwSignature == HEADER::BE_SIGNATURE);
    if (be)
    {
        DebugTrace("INFO: \"%ls\" is a big-endian (Xbox 360) wave bank\n", szFileName);
        m_header.BigEndian();
    }

    if (m_header.dwHeaderVersion != HEADER::VERSION)
    {
        return E_FAIL;
    }

    // Load bank data
    memset(&request, 0, sizeof(request));
    request.Offset = m_header.Segments[HEADER::SEGIDX_BANKDATA].dwOffset;
    request.hEvent = m_event.get();

    wait = false;
    if (!ReadFile(hFile.get(), &m_data, sizeof(m_data), nullptr, &request))
    {
        DWORD error = GetLastError();
        if (error != ERROR_IO_PENDING)
            return HRESULT_FROM_WIN32(error);
        wait = true;
    }

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    result = GetOverlappedResultEx(hFile.get(), &request, &bytes, INFINITE, FALSE);
#else
    if (wait)
        (void)WaitForSingleObject(m_event.get(), INFINITE);

    result = GetOverlappedResult(hFile.get(), &request, &bytes, FALSE);
#endif

    if (!result || (bytes != sizeof(m_data)))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (be)
        m_data.BigEndian();

    if (!m_data.dwEntryCount)
    {
        return HRESULT_FROM_WIN32(ERROR_NO_DATA);
    }

    if (m_data.dwFlags & BANKDATA::TYPE_STREAMING)
    {
        if (m_data.dwAlignment < ALIGNMENT_DVD)
            return E_FAIL;
        if (m_data.dwAlignment % DVD_SECTOR_SIZE)
            return E_FAIL;
    }
    else if (m_data.dwAlignment < ALIGNMENT_MIN)
    {
        return E_FAIL;
    }

    if (m_data.dwFlags & BANKDATA::FLAGS_COMPACT)
    {
        if (m_data.dwEntryMetaDataElementSize != sizeof(ENTRYCOMPACT))
        {
            return E_FAIL;
        }

        if (m_header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwLength > (MAX_COMPACT_DATA_SEGMENT_SIZE * m_data.dwAlignment))
        {
            // Data segment is too large to be valid compact wavebank
            return E_FAIL;
        }
    }
    else
    {
        if (m_data.dwEntryMetaDataElementSize != sizeof(ENTRY))
        {
            return E_FAIL;
        }
    }

    DWORD metadataBytes = m_header.Segments[HEADER::SEGIDX_ENTRYMETADATA].dwLength;
    if (metadataBytes != (m_data.dwEntryCount * m_data.dwEntryMetaDataElementSize))
    {
        return E_FAIL;
    }

    // Load names
    DWORD namesBytes = m_header.Segments[HEADER::SEGIDX_ENTRYNAMES].dwLength;
    if (namesBytes > 0)
    {
        if (namesBytes >= (m_data.dwEntryNameElementSize * m_data.dwEntryCount))
        {
            std::unique_ptr<char[]> temp(new (std::nothrow) char[namesBytes]);
            if (!temp)
                return E_OUTOFMEMORY;

            memset(&request, 0, sizeof(request));
            request.Offset = m_header.Segments[HEADER::SEGIDX_ENTRYNAMES].dwOffset;
            request.hEvent = m_event.get();

            wait = false;
            if (!ReadFile(hFile.get(), temp.get(), namesBytes, nullptr, &request))
            {
                DWORD error = GetLastError();
                if (error != ERROR_IO_PENDING)
                    return HRESULT_FROM_WIN32(error);
                wait = true;
            }

        #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
            result = GetOverlappedResultEx(hFile.get(), &request, &bytes, INFINITE, FALSE);
        #else
            if (wait)
                (void)WaitForSingleObject(m_event.get(), INFINITE);

            result = GetOverlappedResult(hFile.get(), &request, &bytes, FALSE);
        #endif

            if (!result || (namesBytes != bytes))
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }

            for (uint32_t j = 0; j < m_data.dwEntryCount; ++j)
            {
                DWORD n = m_data.dwEntryNameElementSize * j;

                char name[64] = {};
                strncpy_s(name, &temp[n], sizeof(name));

                m_names[name] = j;
            }
        }
    }

    // Load entries
    if (m_data.dwFlags & BANKDATA::FLAGS_COMPACT)
    {
        m_entries.reset(reinterpret_cast<uint8_t*>(new (std::nothrow) ENTRYCOMPACT[m_data.dwEntryCount]));
    }
    else
    {
        m_entries.reset(reinterpret_cast<uint8_t*>(new (std::nothrow) ENTRY[m_data.dwEntryCount]));
    }
    if (!m_entries)
        return E_OUTOFMEMORY;

    memset(&request, 0, sizeof(request));
    request.Offset = m_header.Segments[HEADER::SEGIDX_ENTRYMETADATA].dwOffset;
    request.hEvent = m_event.get();

    wait = false;
    if (!ReadFile(hFile.get(), m_entries.get(), metadataBytes, nullptr, &request))
    {
        DWORD error = GetLastError();
        if (error != ERROR_IO_PENDING)
            return HRESULT_FROM_WIN32(error);
        wait = true;
    }

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    result = GetOverlappedResultEx(hFile.get(), &request, &bytes, INFINITE, FALSE);
#else
    if (wait)
        (void)WaitForSingleObject(m_event.get(), INFINITE);

    result = GetOverlappedResult(hFile.get(), &request, &bytes, FALSE);
#endif

    if (!result || (metadataBytes != bytes))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (be)
    {
        if (m_data.dwFlags & BANKDATA::FLAGS_COMPACT)
        {
            auto ptr = reinterpret_cast<ENTRYCOMPACT*>(m_entries.get());
            for (size_t j = 0; j < m_data.dwEntryCount; ++j, ++ptr)
                ptr->BigEndian();
        }
        else
        {
            auto ptr = reinterpret_cast<ENTRY*>(m_entries.get());
            for (size_t j = 0; j < m_data.dwEntryCount; ++j, ++ptr)
                ptr->BigEndian();
        }
    }

    // Load seek tables (XMA2 / xWMA)
    DWORD seekLen = m_header.Segments[HEADER::SEGIDX_SEEKTABLES].dwLength;
    if (seekLen > 0)
    {
        m_seekData.reset(new (std::nothrow) uint8_t[seekLen]);
        if (!m_seekData)
            return E_OUTOFMEMORY;

        memset(&request, 0, sizeof(OVERLAPPED));
        request.Offset = m_header.Segments[HEADER::SEGIDX_SEEKTABLES].dwOffset;
        request.hEvent = m_event.get();

        wait = false;
        if (!ReadFile(hFile.get(), m_seekData.get(), seekLen, nullptr, &request))
        {
            DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING)
                return HRESULT_FROM_WIN32(error);
            wait = true;
        }

    #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        result = GetOverlappedResultEx(hFile.get(), &request, &bytes, INFINITE, FALSE);
    #else
        if (wait)
            (void)WaitForSingleObject(m_event.get(), INFINITE);

        result = GetOverlappedResult(hFile.get(), &request, &bytes, FALSE);
    #endif

        if (!result || (seekLen != bytes))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (be)
        {
            auto ptr = reinterpret_cast<uint32_t*>(m_seekData.get());
            for (size_t j = 0; j < seekLen; j += 4, ++ptr)
            {
                *ptr = _byteswap_ulong(*ptr);
            }
        }
    }

    DWORD waveLen = m_header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwLength;
    if (!waveLen)
    {
        return HRESULT_FROM_WIN32(ERROR_NO_DATA);
    }

    if (m_data.dwFlags & BANKDATA::TYPE_STREAMING)
    {
        // If streaming, reopen without buffering
        hFile.reset();

    #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        CREATEFILE2_EXTENDED_PARAMETERS params2 = { sizeof(CREATEFILE2_EXTENDED_PARAMETERS), 0 };
        params2.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        params2.dwFileFlags = FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING;
        m_async = CreateFile2(szFileName,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              OPEN_EXISTING,
                              &params2);
    #else
        m_async = CreateFileW(szFileName,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              nullptr,
                              OPEN_EXISTING,
                              FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                              nullptr);
    #endif

        if (m_async == INVALID_HANDLE_VALUE)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        m_prepared = true;
    }
    else
    {
        // If in-memory, kick off read of wave data
        void *dest;

    #if defined(_XBOX_ONE) && defined(_TITLE)
        bool xma = false;
        if (m_data.dwFlags & BANKDATA::FLAGS_COMPACT)
        {
            if (m_data.CompactFormat.wFormatTag == MINIWAVEFORMAT::TAG_XMA)
                xma = true;
        }
        else
        {
            for (uint32_t j = 0; j < m_data.dwEntryCount; ++j)
            {
                auto& entry = reinterpret_cast<const ENTRY*>(m_entries.get())[j];
                if (entry.Format.wFormatTag == MINIWAVEFORMAT::TAG_XMA)
                {
                    xma = true;
                    break;
                }
            }
        }

        if (xma)
        {
            HRESULT hr = ApuAlloc(&m_xmaMemory, nullptr, waveLen, SHAPE_XMA_INPUT_BUFFER_ALIGNMENT);
            if (FAILED(hr))
            {
                DebugTrace("ERROR: ApuAlloc failed. Did you allocate a large enough heap with ApuCreateHeap for all your XMA wave data?\n");
                return hr;
            }

            dest = m_xmaMemory;
        }
        else
        #endif // _XBOX_ONE && _TITLE
        {
            m_waveData.reset(new (std::nothrow) uint8_t[waveLen]);
            if (!m_waveData)
                return E_OUTOFMEMORY;

            dest = m_waveData.get();
        }

        memset(&m_request, 0, sizeof(OVERLAPPED));
        m_request.Offset = m_header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwOffset;
        m_request.hEvent = m_event.get();

        if (!ReadFile(hFile.get(), dest, waveLen, nullptr, &m_request))
        {
            DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING)
                return HRESULT_FROM_WIN32(error);
        }
        else
        {
            m_prepared = true;
            memset(&m_request, 0, sizeof(OVERLAPPED));
        }

        m_async = hFile.release();
    }

    return S_OK;
}


void WaveBankReader::Impl::Close()
{
    if (m_async != INVALID_HANDLE_VALUE)
    {
        if (m_request.hEvent != 0)
        {
            DWORD bytes;
        #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
            (void)GetOverlappedResultEx(m_async, &m_request, &bytes, INFINITE, FALSE);
        #else
            (void)WaitForSingleObject(m_request.hEvent, INFINITE);

            (void)GetOverlappedResult(m_async, &m_request, &bytes, FALSE);
        #endif
        }

        CloseHandle(m_async);
        m_async = INVALID_HANDLE_VALUE;
    }
    m_event.reset();

#if defined(_XBOX_ONE) && defined(_TITLE)
    if (m_xmaMemory)
    {
        ApuFree(m_xmaMemory);
        m_xmaMemory = nullptr;
    }
#endif
}


_Use_decl_annotations_
HRESULT WaveBankReader::Impl::GetFormat(uint32_t index, WAVEFORMATEX* pFormat, size_t maxsize) const
{
    if (!pFormat || !maxsize)
        return E_INVALIDARG;

    if (index >= m_data.dwEntryCount || !m_entries)
    {
        return E_FAIL;
    }

    auto& miniFmt = (m_data.dwFlags & BANKDATA::FLAGS_COMPACT) ? m_data.CompactFormat : (reinterpret_cast<const ENTRY*>(m_entries.get())[index].Format);

    switch (miniFmt.wFormatTag)
    {
        case MINIWAVEFORMAT::TAG_PCM:
            if (maxsize < sizeof(PCMWAVEFORMAT))
                return HRESULT_FROM_WIN32(ERROR_MORE_DATA);

            pFormat->wFormatTag = WAVE_FORMAT_PCM;

            if (maxsize >= sizeof(WAVEFORMATEX))
            {
                pFormat->cbSize = 0;
            }
            break;

        case MINIWAVEFORMAT::TAG_ADPCM:
            if (maxsize < (sizeof(WAVEFORMATEX) + 32 /*MSADPCM_FORMAT_EXTRA_BYTES*/))
                return HRESULT_FROM_WIN32(ERROR_MORE_DATA);

            pFormat->wFormatTag = WAVE_FORMAT_ADPCM;
            pFormat->cbSize = 32 /*MSADPCM_FORMAT_EXTRA_BYTES*/;
            {
                auto adpcmFmt = reinterpret_cast<ADPCMWAVEFORMAT*>(pFormat);
                adpcmFmt->wSamplesPerBlock = (WORD)miniFmt.AdpcmSamplesPerBlock();
                miniFmt.AdpcmFillCoefficientTable(adpcmFmt);
            }
            break;

        case MINIWAVEFORMAT::TAG_WMA:
            if (maxsize < sizeof(WAVEFORMATEX))
                return HRESULT_FROM_WIN32(ERROR_MORE_DATA);

            pFormat->wFormatTag = (miniFmt.wBitsPerSample & 0x1) ? WAVE_FORMAT_WMAUDIO3 : WAVE_FORMAT_WMAUDIO2;
            pFormat->cbSize = 0;
            break;

        case MINIWAVEFORMAT::TAG_XMA: // XMA2 is supported by Xbox One
        #if defined(_XBOX_ONE) && defined(_TITLE)
            if (maxsize < sizeof(XMA2WAVEFORMATEX))
                return HRESULT_FROM_WIN32(ERROR_MORE_DATA);

            pFormat->wFormatTag = WAVE_FORMAT_XMA2;
            pFormat->cbSize = sizeof(XMA2WAVEFORMATEX) - sizeof(WAVEFORMATEX);
            {
                auto xmaFmt = reinterpret_cast<XMA2WAVEFORMATEX*>(pFormat);

                xmaFmt->NumStreams = static_cast<WORD>((miniFmt.nChannels + 1) / 2);
                xmaFmt->BytesPerBlock = 65536 /* XACT_FIXED_XMA_BLOCK_SIZE */;
                xmaFmt->EncoderVersion = 4 /* XMAENCODER_VERSION_XMA2 */;

                auto seekTable = FindSeekTable(index, m_seekData.get(), m_header, m_data);
                if (seekTable)
                {
                    xmaFmt->BlockCount = static_cast<WORD>(*seekTable);
                }
                else
                {
                    xmaFmt->BlockCount = 0;
                }

                switch (miniFmt.nChannels)
                {
                    case 1: xmaFmt->ChannelMask = SPEAKER_MONO; break;
                    case 2: xmaFmt->ChannelMask = SPEAKER_STEREO; break;
                    case 3: xmaFmt->ChannelMask = SPEAKER_2POINT1; break;
                    case 4: xmaFmt->ChannelMask = SPEAKER_QUAD; break;
                    case 5: xmaFmt->ChannelMask = SPEAKER_4POINT1; break;
                    case 6: xmaFmt->ChannelMask = SPEAKER_5POINT1; break;
                    case 7: xmaFmt->ChannelMask = SPEAKER_5POINT1 | SPEAKER_BACK_CENTER; break;
                    case 8: xmaFmt->ChannelMask = SPEAKER_7POINT1; break;
                    default: xmaFmt->ChannelMask = DWORD(-1); break;
                }

                if (m_data.dwFlags & BANKDATA::FLAGS_COMPACT)
                {
                    auto& entry = reinterpret_cast<const ENTRYCOMPACT*>(m_entries.get())[index];

                    DWORD dwOffset, dwLength;
                    entry.ComputeLocations(dwOffset, dwLength, index, m_header, m_data, reinterpret_cast<const ENTRYCOMPACT*>(m_entries.get()));

                    xmaFmt->SamplesEncoded = entry.GetDuration(dwLength, m_data, seekTable);

                    xmaFmt->PlayBegin = xmaFmt->PlayLength =
                        xmaFmt->LoopBegin = xmaFmt->LoopLength = xmaFmt->LoopCount = 0;
                }
                else
                {
                    auto& entry = reinterpret_cast<const ENTRY*>(m_entries.get())[index];

                    xmaFmt->SamplesEncoded = entry.Duration;
                    xmaFmt->PlayBegin = 0;
                    xmaFmt->PlayLength = entry.PlayRegion.dwLength;

                    if (entry.LoopRegion.dwTotalSamples > 0)
                    {
                        xmaFmt->LoopBegin = entry.LoopRegion.dwStartSample;
                        xmaFmt->LoopLength = entry.LoopRegion.dwTotalSamples;
                        xmaFmt->LoopCount = 0xff /* XACTLOOPCOUNT_INFINITE */;
                    }
                    else
                    {
                        xmaFmt->LoopBegin = xmaFmt->LoopLength = xmaFmt->LoopCount = 0;
                    }
                }
            }
            break;
        #else
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        #endif

        default:
            return E_FAIL;
    }

    pFormat->nChannels = miniFmt.nChannels;
    pFormat->wBitsPerSample = miniFmt.BitsPerSample();
    pFormat->nBlockAlign = (WORD)miniFmt.BlockAlign();
    pFormat->nSamplesPerSec = miniFmt.nSamplesPerSec;
    pFormat->nAvgBytesPerSec = miniFmt.AvgBytesPerSec();

    return S_OK;
}


_Use_decl_annotations_
HRESULT WaveBankReader::Impl::GetWaveData(uint32_t index, const uint8_t** pData, uint32_t& dataSize) const
{
    if (!pData)
        return E_INVALIDARG;

    if (index >= m_data.dwEntryCount || !m_entries)
    {
        return E_FAIL;
    }

#if defined(_XBOX_ONE) && defined(_TITLE)
    const uint8_t* waveData = (m_xmaMemory) ? reinterpret_cast<uint8_t*>(m_xmaMemory) : m_waveData.get();
#else
    const uint8_t* waveData = m_waveData.get();
#endif

    if (!waveData)
        return E_FAIL;

    if (m_data.dwFlags & BANKDATA::TYPE_STREAMING)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }

    if (!m_prepared)
    {
        return HRESULT_FROM_WIN32(ERROR_IO_INCOMPLETE);
    }

    if (m_data.dwFlags & BANKDATA::FLAGS_COMPACT)
    {
        auto& entry = reinterpret_cast<const ENTRYCOMPACT*>(m_entries.get())[index];

        DWORD dwOffset, dwLength;
        entry.ComputeLocations(dwOffset, dwLength, index, m_header, m_data, reinterpret_cast<const ENTRYCOMPACT*>(m_entries.get()));

        if ((dwOffset + dwLength) > m_header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwLength)
        {
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
        }

        *pData = &waveData[dwOffset];
        dataSize = dwLength;
    }
    else
    {
        auto& entry = reinterpret_cast<const ENTRY*>(m_entries.get())[index];

        if ((entry.PlayRegion.dwOffset + entry.PlayRegion.dwLength) > m_header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwLength)
        {
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
        }

        *pData = &waveData[entry.PlayRegion.dwOffset];
        dataSize = entry.PlayRegion.dwLength;
    }

    return S_OK;
}


_Use_decl_annotations_
HRESULT WaveBankReader::Impl::GetSeekTable(uint32_t index, const uint32_t** pData, uint32_t& dataCount, uint32_t& tag) const
{
    if (!pData)
        return E_INVALIDARG;

    *pData = nullptr;
    dataCount = 0;
    tag = 0;

    if (index >= m_data.dwEntryCount || !m_entries)
    {
        return E_FAIL;
    }

    if (!m_seekData)
        return S_OK;

    auto& miniFmt = (m_data.dwFlags & BANKDATA::FLAGS_COMPACT) ? m_data.CompactFormat : (reinterpret_cast<const ENTRY*>(m_entries.get())[index].Format);

    switch (miniFmt.wFormatTag)
    {
        case MINIWAVEFORMAT::TAG_WMA:
            tag = (miniFmt.wBitsPerSample & 0x1) ? WAVE_FORMAT_WMAUDIO3 : WAVE_FORMAT_WMAUDIO2;
            break;

        case MINIWAVEFORMAT::TAG_XMA:
            tag = 0x166 /* WAVE_FORMAT_XMA2 */;
            break;

        default:
            return S_OK;
    }

    auto seekTable = FindSeekTable(index, m_seekData.get(), m_header, m_data);
    if (!seekTable)
        return S_OK;

    dataCount = *seekTable;
    *pData = seekTable + 1;

    return S_OK;
}


_Use_decl_annotations_
HRESULT WaveBankReader::Impl::GetMetadata(uint32_t index, Metadata& metadata) const
{
    if (index >= m_data.dwEntryCount || !m_entries)
    {
        return E_FAIL;
    }

    if (m_data.dwFlags & BANKDATA::FLAGS_COMPACT)
    {
        auto& entry = reinterpret_cast<const ENTRYCOMPACT*>(m_entries.get())[index];

        DWORD dwOffset, dwLength;
        entry.ComputeLocations(dwOffset, dwLength, index, m_header, m_data, reinterpret_cast<const ENTRYCOMPACT*>(m_entries.get()));

        auto seekTable = FindSeekTable(index, m_seekData.get(), m_header, m_data);
        metadata.duration = entry.GetDuration(dwLength, m_data, seekTable);
        metadata.loopStart = metadata.loopLength = 0;
        metadata.offsetBytes = dwOffset;
        metadata.lengthBytes = dwLength;
    }
    else
    {
        auto& entry = reinterpret_cast<const ENTRY*>(m_entries.get())[index];

        metadata.duration = entry.Duration;
        metadata.loopStart = entry.LoopRegion.dwStartSample;
        metadata.loopLength = entry.LoopRegion.dwTotalSamples;
        metadata.offsetBytes = entry.PlayRegion.dwOffset;
        metadata.lengthBytes = entry.PlayRegion.dwLength;
    }

    return S_OK;
}


bool WaveBankReader::Impl::UpdatePrepared()
{
    if (m_prepared)
        return true;

    if (m_async == INVALID_HANDLE_VALUE)
        return false;

    if (m_request.hEvent != 0)
    {

    #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        DWORD bytes;
        BOOL result = GetOverlappedResultEx(m_async, &m_request, &bytes, 0, FALSE);
    #else
        bool result = HasOverlappedIoCompleted(&m_request);
    #endif
        if (result)
        {
            m_prepared = true;

            memset(&m_request, 0, sizeof(OVERLAPPED));
        }
    }

    return m_prepared;
}



//--------------------------------------------------------------------------------------
WaveBankReader::WaveBankReader() :
    pImpl(std::make_unique<Impl>())
{
}


WaveBankReader::~WaveBankReader()
{
}


_Use_decl_annotations_
HRESULT WaveBankReader::Open(const wchar_t* szFileName)
{
    return pImpl->Open(szFileName);
}


_Use_decl_annotations_
uint32_t WaveBankReader::Find(const char* name) const
{
    auto it = pImpl->m_names.find(name);
    if (it != pImpl->m_names.cend())
    {
        return it->second;
    }

    return uint32_t(-1);
}


bool WaveBankReader::IsPrepared()
{
    if (pImpl->m_prepared)
        return true;

    return pImpl->UpdatePrepared();
}


void WaveBankReader::WaitOnPrepare()
{
    if (pImpl->m_prepared)
        return;

    if (pImpl->m_request.hEvent != 0)
    {
        WaitForSingleObjectEx(pImpl->m_request.hEvent, INFINITE, FALSE);

        pImpl->UpdatePrepared();
    }
}


bool WaveBankReader::HasNames() const
{
    return !pImpl->m_names.empty();
}


bool WaveBankReader::IsStreamingBank() const
{
    return (pImpl->m_data.dwFlags  & BANKDATA::TYPE_STREAMING) != 0;
}


#if defined(_XBOX_ONE) && defined(_TITLE)
bool WaveBankReader::HasXMA() const
{
    return (pImpl->m_xmaMemory != 0);
}
#endif


const char* WaveBankReader::BankName() const
{
    return pImpl->m_data.szBankName;
}


uint32_t WaveBankReader::Count() const
{
    return pImpl->m_data.dwEntryCount;
}


uint32_t WaveBankReader::BankAudioSize() const
{
    return pImpl->m_header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwLength;
}


_Use_decl_annotations_
HRESULT WaveBankReader::GetFormat(uint32_t index, WAVEFORMATEX* pFormat, size_t maxsize) const
{
    return pImpl->GetFormat(index, pFormat, maxsize);
}


_Use_decl_annotations_
HRESULT WaveBankReader::GetWaveData(uint32_t index, const uint8_t** pData, uint32_t& dataSize) const
{
    return pImpl->GetWaveData(index, pData, dataSize);
}


_Use_decl_annotations_
HRESULT WaveBankReader::GetSeekTable(uint32_t index, const uint32_t** pData, uint32_t& dataCount, uint32_t& tag) const
{
    return pImpl->GetSeekTable(index, pData, dataCount, tag);
}


_Use_decl_annotations_
HRESULT WaveBankReader::GetMetadata(uint32_t index, Metadata& metadata) const
{
    return pImpl->GetMetadata(index, metadata);
}


HANDLE WaveBankReader::GetAsyncHandle() const
{
    return (pImpl->m_data.dwFlags & BANKDATA::TYPE_STREAMING) ? pImpl->m_async : INVALID_HANDLE_VALUE;
}
