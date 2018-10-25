//--------------------------------------------------------------------------------------
// File: xwbtool.cpp
//
// Simple command-line tool for building wave banks from 1 or more .WAV files. This
// generates binary wave banks compliant with XACT 3's Wave Bank .XWB format. The
// .WAV files are not format converted or compressed.
//
// For a more full-featured builder, see XACT 3 and the XACTBLD tool in the legacy
// DirectX SDK (June 2010) release.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <algorithm>
#include <fstream>
#include <list>
#include <memory>
#include <vector>

#include "WAVFileReader.h"

#ifdef __INTEL_COMPILER
#pragma warning(disable : 161)
// warning #161: unrecognized #pragma
#endif

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
                (static_cast<uint32_t>(static_cast<uint8_t>(ch0)) \
                | (static_cast<uint32_t>(static_cast<uint8_t>(ch1)) << 8) \
                | (static_cast<uint32_t>(static_cast<uint8_t>(ch2)) << 16) \
                | (static_cast<uint32_t>(static_cast<uint8_t>(ch3)) << 24))
#endif /* defined(MAKEFOURCC) */

#ifndef WAVE_FORMAT_XMA2
#define WAVE_FORMAT_XMA2 0x166

#pragma pack(push,1)
typedef struct XMA2WAVEFORMATEX
{
    WAVEFORMATEX wfx;
    // Meaning of the WAVEFORMATEX fields here:
    //    wFormatTag;        // Audio format type; always WAVE_FORMAT_XMA2
    //    nChannels;         // Channel count of the decoded audio
    //    nSamplesPerSec;    // Sample rate of the decoded audio
    //    nAvgBytesPerSec;   // Used internally by the XMA encoder
    //    nBlockAlign;       // Decoded sample size; channels * wBitsPerSample / 8
    //    wBitsPerSample;    // Bits per decoded mono sample; always 16 for XMA
    //    cbSize;            // Size in bytes of the rest of this structure (34)

    WORD  NumStreams;        // Number of audio streams (1 or 2 channels each)
    DWORD ChannelMask;       // Spatial positions of the channels in this file,
                             // stored as SPEAKER_xxx values (see audiodefs.h)
    DWORD SamplesEncoded;    // Total number of PCM samples per channel the file decodes to
    DWORD BytesPerBlock;     // XMA block size (but the last one may be shorter)
    DWORD PlayBegin;         // First valid sample in the decoded audio
    DWORD PlayLength;        // Length of the valid part of the decoded audio
    DWORD LoopBegin;         // Beginning of the loop region in decoded sample terms
    DWORD LoopLength;        // Length of the loop region in decoded sample terms
    BYTE  LoopCount;         // Number of loop repetitions; 255 = infinite
    BYTE  EncoderVersion;    // Version of XMA encoder that generated the file
    WORD  BlockCount;        // XMA blocks in file (and entries in its seek table)
} XMA2WAVEFORMATEX;
#pragma pack(pop)
#endif

static_assert(sizeof(XMA2WAVEFORMATEX) == 52, "Mismatch of XMA2 type");

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

namespace
{
    struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };

    typedef std::unique_ptr<void, handle_closer> ScopedHandle;

    inline HANDLE safe_handle(HANDLE h) { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

    struct find_closer { void operator()(HANDLE h) { assert(h != INVALID_HANDLE_VALUE); if (h) FindClose(h); } };

    typedef std::unique_ptr<void, find_closer> ScopedFindHandle;

#define BLOCKALIGNPAD(a, b) \
    ((((a) + ((b) - 1)) / (b)) * (b))

#define XACT_CONTENT_VERSION 46 // DirectX SDK (June 2010)

#pragma pack(push, 1)

    static const size_t DVD_SECTOR_SIZE = 2048;
    static const size_t DVD_BLOCK_SIZE = DVD_SECTOR_SIZE * 16;

    static const size_t ALIGNMENT_MIN = 4;
    static const size_t ALIGNMENT_DVD = DVD_SECTOR_SIZE;

    static const size_t MAX_DATA_SEGMENT_SIZE = 0xFFFFFFFF;
    static const size_t MAX_COMPACT_DATA_SEGMENT_SIZE = 0x001FFFFF;

    static const size_t ENTRYNAME_LENGTH = 64;

    struct REGION
    {
        uint32_t    dwOffset;   // Region offset, in bytes.
        uint32_t    dwLength;   // Region length, in bytes.
    };

    struct SAMPLEREGION
    {
        uint32_t    dwStartSample;  // Start sample for the region.
        uint32_t    dwTotalSamples; // Region length in samples.
    };

    struct HEADER
    {
        static const uint32_t SIGNATURE = MAKEFOURCC('W', 'B', 'N', 'D');
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
    };

    struct ENTRYCOMPACT
    {
        uint32_t       dwOffset : 21;       // Data offset, in multiplies of the bank alignment
        uint32_t       dwLengthDeviation : 11;       // Data length deviation, in bytes
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
    };

#pragma pack(pop)

    static_assert(sizeof(REGION) == 8, "Mismatch with xact3wb.h");
    static_assert(sizeof(SAMPLEREGION) == 8, "Mismatch with xact3wb.h");
    static_assert(sizeof(HEADER) == 52, "Mismatch with xact3wb.h");
    static_assert(sizeof(ENTRY) == 24, "Mismatch with xact3wb.h");
    static_assert(sizeof(MINIWAVEFORMAT) == 4, "Mismatch with xact3wb.h");
    static_assert(sizeof(ENTRY) == 24, "Mismatch with xact3wb.h");
    static_assert(sizeof(ENTRYCOMPACT) == 4, "Mismatch with xact3wb.h");
    static_assert(sizeof(BANKDATA) == 96, "Mismatch with xact3wb.h");

    template <typename T> WORD ChannelsSpecifiedInMask(T x)
    {
        WORD bitCount = 0;
        while (x) { ++bitCount; x &= (x - 1); }
        return bitCount;
    }

    WORD AdpcmBlockSizeFromPcmFrames(WORD nPcmFrames, WORD nChannels)
    {
        // The full calculation is as follows:
        //    UINT uHeaderBytes = MSADPCM_HEADER_LENGTH * nChannels;
        //    UINT uBitsPerSample = MSADPCM_BITS_PER_SAMPLE * nChannels;
        //    UINT uBlockAlign = uHeaderBytes + (nPcmFrames - 2) * uBitsPerSample / 8;
        //    return WORD(uBlockAlign);

        assert(nChannels == 1 || nChannels == 2);

        if (nPcmFrames)
        {
            if (nChannels == 1)
            {
                assert(nPcmFrames % 2 == 0); // Mono data needs even nPcmFrames
                return WORD(nPcmFrames / 2 + 6);
            }
            else
            {
                return WORD(nPcmFrames + 12);
            }
        }
        else
        {
            return 0;
        }
    }

    DWORD EncodeWMABlockAlign(DWORD dwBlockAlign, DWORD dwAvgBytesPerSec)
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

        auto bit = std::find(std::begin(aWMABlockAlign), std::end(aWMABlockAlign), dwBlockAlign);
        if (bit == std::end(aWMABlockAlign))
            return DWORD(-1);

        DWORD blockAlignIndex = DWORD(bit - std::begin(aWMABlockAlign));

        auto ait = std::find(std::begin(aWMAAvgBytesPerSec), std::end(aWMAAvgBytesPerSec), dwAvgBytesPerSec);
        if (ait == std::end(aWMAAvgBytesPerSec))
            return DWORD(-1);

        DWORD bytesPerSecIndex = DWORD(ait - std::begin(aWMAAvgBytesPerSec));

        return DWORD(blockAlignIndex | (bytesPerSecIndex << 5));
    }

    bool ConvertToMiniFormat(const WAVEFORMATEX* wfx, bool hasSeek, MINIWAVEFORMAT& miniFmt)
    {
        if (!wfx)
            return false;

        if (!wfx->nChannels)
        {
            wprintf(L"ERROR: Wave bank entry must have at least 1 channel\n");
            return false;
        }

        if (wfx->nChannels > 7)
        {
            wprintf(L"ERROR: Wave banks only support up to 7 channels\n");
            return false;
        }

        if (!wfx->nSamplesPerSec)
        {
            wprintf(L"ERROR: Wave banks entry sample rate must be non-zero\n");
            return false;
        }

        if (wfx->nSamplesPerSec > 262143)
        {
            wprintf(L"ERROR: Wave banks only support sample rates up to 2^18 (262143)\n");
            return false;
        }

        miniFmt.dwValue = 0;
        miniFmt.nSamplesPerSec = wfx->nSamplesPerSec;
        miniFmt.nChannels = wfx->nChannels;

        switch (wfx->wFormatTag)
        {
        case WAVE_FORMAT_PCM:
            if ((wfx->wBitsPerSample != 8) && (wfx->wBitsPerSample != 16))
            {
                wprintf(L"ERROR: Wave banks only support 8-bit or 16-bit integer PCM data\n");
                return false;
            }

            if (wfx->nBlockAlign > 255)
            {
                wprintf(L"ERROR: Wave banks only support block alignments up to 255 (%u)\n", wfx->nBlockAlign);
                return false;
            }

            if (wfx->nBlockAlign != (wfx->nChannels * wfx->wBitsPerSample / 8))
            {
                wprintf(L"ERROR: nBlockAlign (%u) != nChannels (%u) * wBitsPerSample (%u) / 8\n",
                    wfx->nBlockAlign, wfx->nChannels, wfx->wBitsPerSample);
                return false;
            }

            if (wfx->nAvgBytesPerSec != (wfx->nSamplesPerSec * wfx->nBlockAlign))
            {
                wprintf(L"ERROR: nAvgBytesPerSec (%lu) != nSamplesPerSec (%lu) * nBlockAlign (%u)\n",
                    wfx->nAvgBytesPerSec, wfx->nSamplesPerSec, wfx->nBlockAlign);
                return false;
            }

            miniFmt.wFormatTag = MINIWAVEFORMAT::TAG_PCM;
            miniFmt.wBitsPerSample = (wfx->wBitsPerSample == 16) ? MINIWAVEFORMAT::BITDEPTH_16 : MINIWAVEFORMAT::BITDEPTH_8;
            miniFmt.wBlockAlign = wfx->nBlockAlign;
            return true;

        case WAVE_FORMAT_IEEE_FLOAT:
            wprintf(L"ERROR: Wave banks do not support IEEE float PCM data\n");
            return false;

        case WAVE_FORMAT_ADPCM:
            if ((wfx->nChannels != 1) && (wfx->nChannels != 2))
            {
                wprintf(L"ERROR: ADPCM wave format must have 1 or 2 channels (not %u)\n", wfx->nChannels);
                return false;
            }

            if (wfx->wBitsPerSample != 4 /*MSADPCM_BITS_PER_SAMPLE*/)
            {
                wprintf(L"ERROR: ADPCM wave format must have 4 bits per sample (not %u)\n", wfx->wBitsPerSample);
                return false;
            }

            if (wfx->cbSize != 32 /*MSADPCM_FORMAT_EXTRA_BYTES*/)
            {
                wprintf(L"ERROR: ADPCM wave format must have cbSize = 32 (not %u)\n", wfx->cbSize);
                return false;
            }
            else
            {
                auto wfadpcm = reinterpret_cast<const ADPCMWAVEFORMAT*>(wfx);

                if (wfadpcm->wNumCoef != 7 /*MSADPCM_NUM_COEFFICIENTS*/)
                {
                    wprintf(L"ERROR: ADPCM wave format must have 7 coefficients (not %u)\n", wfadpcm->wNumCoef);
                    return false;
                }

                bool valid = true;
                for (int j = 0; j < 7 /*MSADPCM_NUM_COEFFICIENTS*/; ++j)
                {
                    // Microsoft ADPCM standard encoding coefficients
                    static const short g_pAdpcmCoefficients1[] = { 256,  512, 0, 192, 240,  460,  392 };
                    static const short g_pAdpcmCoefficients2[] = { 0, -256, 0,  64,   0, -208, -232 };

                    if (wfadpcm->aCoef[j].iCoef1 != g_pAdpcmCoefficients1[j]
                        || wfadpcm->aCoef[j].iCoef2 != g_pAdpcmCoefficients2[j])
                    {
                        valid = false;
                    }
                }

                if (!valid)
                {
                    wprintf(L"ERROR: Non-standard coefficients for ADPCM found\n");
                    return false;
                }

                if ((wfadpcm->wSamplesPerBlock < 4 /*MSADPCM_MIN_SAMPLES_PER_BLOCK*/)
                    || (wfadpcm->wSamplesPerBlock > 64000 /*MSADPCM_MAX_SAMPLES_PER_BLOCK*/))
                {
                    wprintf(L"ERROR: ADPCM wave format wSamplesPerBlock must be 4..64000\n");
                    return false;
                }

                if (wfadpcm->wfx.nChannels == 1 && (wfadpcm->wSamplesPerBlock % 2))
                {
                    wprintf(L"ERROR: ADPCM wave format mono files must have even wSamplesPerBlock\n");
                    return false;
                }

                unsigned int nHeaderBytes = 7 /*MSADPCM_HEADER_LENGTH*/ * wfx->nChannels;
                unsigned int nBitsPerFrame = 4 /*MSADPCM_BITS_PER_SAMPLE*/ * wfx->nChannels;
                unsigned int nPcmFramesPerBlock = (wfx->nBlockAlign - nHeaderBytes) * 8 / nBitsPerFrame + 2;

                if (wfadpcm->wSamplesPerBlock != nPcmFramesPerBlock)
                {
                    wprintf(L"ERROR: ADPCM %u-channel format with nBlockAlign = %u must have wSamplesPerBlock = %u (not %u)\n",
                        wfx->nChannels, wfx->nBlockAlign, nPcmFramesPerBlock, wfadpcm->wSamplesPerBlock);
                    return false;
                }

                miniFmt.wFormatTag = MINIWAVEFORMAT::TAG_ADPCM;
                miniFmt.wBitsPerSample = 0;
                miniFmt.wBlockAlign = AdpcmBlockSizeFromPcmFrames(wfadpcm->wSamplesPerBlock, 1) - MINIWAVEFORMAT::ADPCM_BLOCKALIGN_CONVERSION_OFFSET;
            }
            return true;

        case WAVE_FORMAT_WMAUDIO2:
        case WAVE_FORMAT_WMAUDIO3:
            if (!hasSeek)
            {
                wprintf(L"ERROR: xWMA requires seek tables ('dpds' chunk)\n");
                return false;
            }

            if (wfx->wBitsPerSample != 16)
            {
                wprintf(L"ERROR: Wave banks only support 16-bit xWMA data\n");
                return false;
            }

            if (!wfx->nBlockAlign)
            {
                wprintf(L"ERROR: Wave bank xWMA must have a non-zero nBlockAlign\n");
                return false;
            }

            if (!wfx->nAvgBytesPerSec)
            {
                wprintf(L"ERROR: Wave bank xWMA must have a non-zero nAvgBytesPerSec\n");
                return false;
            }

            if (wfx->cbSize != 0)
            {
                wprintf(L"ERROR: Unexpected data found in xWMA header\n");
                return false;
            }

            miniFmt.wFormatTag = MINIWAVEFORMAT::TAG_WMA;
            miniFmt.wBitsPerSample = (wfx->wFormatTag == WAVE_FORMAT_WMAUDIO3) ? MINIWAVEFORMAT::BITDEPTH_16 : MINIWAVEFORMAT::BITDEPTH_8;
            {
                DWORD blockAlign = EncodeWMABlockAlign(wfx->nBlockAlign, wfx->nAvgBytesPerSec);
                if (blockAlign == DWORD(-1))
                {
                    wprintf(L"ERROR: Failed encoding nBlockAlign and nAvgBytesPerSec for xWMA\n");
                    return false;
                }
                miniFmt.wBlockAlign = blockAlign;
            }
            return true;

        case WAVE_FORMAT_XMA2:
            if (!hasSeek)
            {
                wprintf(L"ERROR: XMA2 requires seek tables ('seek' chunk)\n");
                return false;
            }

            if (wfx->nBlockAlign != wfx->nChannels * 2 /*XMA_OUTPUT_SAMPLE_BYTES*/)
            {
                wprintf(L"ERROR: XMA2 nBlockAlign (%u) != nChannels(%u) * 2\n", wfx->nBlockAlign, wfx->nChannels);
                return false;
            }

            if (wfx->wBitsPerSample != 16 /*XMA_OUTPUT_SAMPLE_BITS*/)
            {
                wprintf(L"ERROR: XMA2 wBitsPerSample (%u) should be 16\n", wfx->wBitsPerSample);
                return false;
            }

            if (wfx->cbSize != (sizeof(XMA2WAVEFORMATEX) - sizeof(WAVEFORMATEX)))
            {
                wprintf(L"ERROR: XMA2 cbSize must be %zu (%u)", (sizeof(XMA2WAVEFORMATEX) - sizeof(WAVEFORMATEX)), wfx->cbSize);
                return false;
            }
            else
            {
                auto xmaFmt = reinterpret_cast<const XMA2WAVEFORMATEX*>(wfx);

                if (xmaFmt->EncoderVersion < 3)
                {
                    wprintf(L"ERROR: XMA2 encoder version (%u) - 3 or higher is required", xmaFmt->EncoderVersion);
                    return false;
                }

                if (!xmaFmt->BlockCount)
                {
                    wprintf(L"ERROR: XMA2 BlockCount must be non-zero\n");
                    return false;
                }

                if (!xmaFmt->BytesPerBlock || (xmaFmt->BytesPerBlock > 8386560 /*XMA_READBUFFER_MAX_BYTES*/))
                {
                    wprintf(L"ERROR: XMA2 BytesPerBlock (%u) is invalid\n", xmaFmt->BytesPerBlock);
                    return false;
                }

                if (xmaFmt->ChannelMask)
                {
                    auto channelBits = ChannelsSpecifiedInMask(xmaFmt->ChannelMask);
                    if (channelBits != wfx->nChannels)
                    {
                        wprintf(L"ERROR: XMA2 nChannels=%u but ChannelMask (%08X) has %u bits set\n",
                            xmaFmt->ChannelMask, wfx->nChannels, channelBits);
                        return false;
                    }
                }

                if (xmaFmt->NumStreams != ((wfx->nChannels + 1) / 2))
                {
                    wprintf(L"ERROR: XMA2 NumStreams (%u) != ( nChannels(%u) + 1 ) / 2\n", xmaFmt->NumStreams, wfx->nChannels);
                    return false;
                }

                if (!xmaFmt->SamplesEncoded)
                {
                    wprintf(L"ERROR: XMA2 SamplesEncoded must be non-zero\n");
                    return false;
                }

                if ((xmaFmt->PlayBegin + xmaFmt->PlayLength) > xmaFmt->SamplesEncoded)
                {
                    wprintf(L"ERROR: XMA2 play region too large (%u + %u > %u)", xmaFmt->PlayBegin, xmaFmt->PlayLength, xmaFmt->SamplesEncoded);
                    return false;
                }

                if ((xmaFmt->LoopBegin + xmaFmt->LoopLength) > xmaFmt->SamplesEncoded)
                {
                    wprintf(L"ERROR: XMA2 loop region too large (%u + %u > %u)", xmaFmt->LoopBegin, xmaFmt->LoopLength, xmaFmt->SamplesEncoded);
                    return false;
                }

                miniFmt.wFormatTag = MINIWAVEFORMAT::TAG_XMA;
                miniFmt.wBlockAlign = 2 * wfx->nChannels;
                miniFmt.wBitsPerSample = MINIWAVEFORMAT::BITDEPTH_16;
            }
            return true;

        case WAVE_FORMAT_EXTENSIBLE:
            if (wfx->cbSize < (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)))
            {
                wprintf(L"ERROR: WAVEFORMATEXTENSIBLE cbSize must be at least %zu (%u)", (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)), wfx->cbSize);
                return false;
            }
            else
            {
                static const GUID s_wfexBase = { 0x00000000, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

                auto wfex = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wfx);

                if (memcmp(reinterpret_cast<const BYTE*>(&wfex->SubFormat) + sizeof(DWORD),
                    reinterpret_cast<const BYTE*>(&s_wfexBase) + sizeof(DWORD), sizeof(GUID) - sizeof(DWORD)) != 0)
                {
                    wprintf(L"ERROR: WAVEFORMATEXTENSIBLE encountered with unknown GUID ({%8.8lX-%4.4X-%4.4X-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X})\n",
                        wfex->SubFormat.Data1, wfex->SubFormat.Data2, wfex->SubFormat.Data3,
                        wfex->SubFormat.Data4[0], wfex->SubFormat.Data4[1], wfex->SubFormat.Data4[2], wfex->SubFormat.Data4[3],
                        wfex->SubFormat.Data4[4], wfex->SubFormat.Data4[5], wfex->SubFormat.Data4[6], wfex->SubFormat.Data4[7]);
                    return false;
                }

                switch (wfex->SubFormat.Data1)
                {
                case WAVE_FORMAT_PCM:
                    if ((wfx->wBitsPerSample != 8) && (wfx->wBitsPerSample != 16))
                    {
                        wprintf(L"ERROR: Wave banks only support 8-bit or 16-bit integer PCM data (%u)\n", wfx->wBitsPerSample);
                        return false;
                    }

                    if (!wfex->Samples.wValidBitsPerSample)
                    {
                        wprintf(L"WARNING: Integer PCM WAVEFORMATEXTENSIBLE format should not have wValidBitsPerSample = 0\n");
                    }
                    else if (((wfex->Samples.wValidBitsPerSample != 8) && (wfex->Samples.wValidBitsPerSample != 16))
                        || (wfex->Samples.wValidBitsPerSample > wfx->wBitsPerSample))
                    {
                        wprintf(L"ERROR: Unexpected wValidBitsPerSample value (%u)\n", wfex->Samples.wValidBitsPerSample);
                        return false;
                    }

                    if (wfx->nBlockAlign > 255)
                    {
                        wprintf(L"ERROR: Wave banks only support block alignments up to 255 (%u)\n", wfx->nBlockAlign);
                        return false;
                    }

                    if (wfx->nBlockAlign != (wfx->nChannels * wfx->wBitsPerSample / 8))
                    {
                        wprintf(L"ERROR: nBlockAlign (%u) != nChannels (%u) * wBitsPerSample (%u) / 8\n",
                            wfx->nBlockAlign, wfx->nChannels, wfx->wBitsPerSample);
                        return false;
                    }

                    if (wfx->nAvgBytesPerSec != (wfx->nSamplesPerSec * wfx->nBlockAlign))
                    {
                        wprintf(L"ERROR: nAvgBytesPerSec (%lu) != nSamplesPerSec (%lu) * nBlockAlign (%u)\n",
                            wfx->nAvgBytesPerSec, wfx->nSamplesPerSec, wfx->nBlockAlign);
                        return false;
                    }

                    miniFmt.wFormatTag = MINIWAVEFORMAT::TAG_PCM;
                    miniFmt.wBitsPerSample = (wfex->Samples.wValidBitsPerSample == 16) ? MINIWAVEFORMAT::BITDEPTH_16 : MINIWAVEFORMAT::BITDEPTH_8;
                    miniFmt.wBlockAlign = wfx->nBlockAlign;
                    break;

                case WAVE_FORMAT_IEEE_FLOAT:
                    wprintf(L"ERROR: Wave banks do not support float PCM data\n");
                    return false;

                case WAVE_FORMAT_ADPCM:
                    wprintf(L"ERROR: ADPCM is not supported as a WAVEFORMATEXTENSIBLE\n");
                    return false;

                case WAVE_FORMAT_WMAUDIO2:
                case WAVE_FORMAT_WMAUDIO3:
                    if (!hasSeek)
                    {
                        wprintf(L"ERROR: xWMA requires seek tables (dpds chunk)\n");
                        return false;
                    }

                    if (wfx->wBitsPerSample != 16)
                    {
                        wprintf(L"ERROR: Wave banks only support 16-bit xWMA data\n");
                        return false;
                    }

                    if (!wfx->nBlockAlign)
                    {
                        wprintf(L"ERROR: Wvae bank xWMA must have a non-zero nBlockAlign\n");
                        return false;
                    }

                    if (!wfx->nAvgBytesPerSec)
                    {
                        wprintf(L"ERROR: Wave bank xWMA must have a non-zero nAvgBytesPerSec\n");
                        return false;
                    }

                    miniFmt.wFormatTag = MINIWAVEFORMAT::TAG_WMA;
                    miniFmt.wBitsPerSample = (wfx->wFormatTag == WAVE_FORMAT_WMAUDIO3) ? MINIWAVEFORMAT::BITDEPTH_16 : MINIWAVEFORMAT::BITDEPTH_8;
                    {
                        DWORD blockAlign = EncodeWMABlockAlign(wfx->nBlockAlign, wfx->nAvgBytesPerSec);
                        if (blockAlign == DWORD(-1))
                        {
                            wprintf(L"ERROR: Failed encoding nBlockAlign and nAvgBytesPerSec for xWMA\n");
                            return false;
                        }
                        miniFmt.wBlockAlign = blockAlign;
                    }
                    break;

                case WAVE_FORMAT_XMA2:
                    wprintf(L"ERROR: XMA2 is not supported as a WAVEFORMATEXTENSIBLE\n");
                    return false;

                default:
                    wprintf(L"ERROR: Unknown WAVEFORMATEXTENSIBLE format tag\n");
                    return false;
                }

                if (wfex->dwChannelMask)
                {
                    auto channelBits = ChannelsSpecifiedInMask(wfex->dwChannelMask);
                    if (channelBits != wfx->nChannels)
                    {
                        wprintf(L"ERROR: WAVEFORMATEXTENSIBLE: nChannels=%u but ChannelMask has %u bits set\n",
                            wfx->nChannels, channelBits);
                        return false;
                    }
                    else
                    {
                        wprintf(L"WARNING: WAVEFORMATEXTENSIBLE ChannelMask is ignored in wave banks\n");
                    }
                }

                return true;
            }

        default:
            return false;
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

enum OPTIONS
{
    OPT_RECURSIVE = 1,
    OPT_STREAMING,
    OPT_OUTPUTFILE,
    OPT_OUTPUTHEADER,
    OPT_NOOVERWRITE,
    OPT_COMPACT,
    OPT_NOCOMPACT,
    OPT_FRIENDLY_NAMES,
    OPT_NOLOGO,
    OPT_FILELIST,
    OPT_MAX
};

static_assert(OPT_MAX <= 32, "dwOptions is a DWORD bitfield");

struct SConversion
{
    wchar_t szSrc[MAX_PATH];
};

struct SValue
{
    LPCWSTR pName;
    DWORD dwValue;
};

struct WaveFile
{
    DirectX::WAVData data;
    size_t conv;
    MINIWAVEFORMAT miniFmt;
    std::unique_ptr<uint8_t[]> waveData;

    WaveFile() noexcept :
        data{},
        conv(0),
        miniFmt{}
        {}

    WaveFile(WaveFile&&) = default;
    WaveFile& operator= (WaveFile&&) = default;
};

namespace
{
    void FileNameToIdentifier(_Inout_updates_all_(count) wchar_t* str, size_t count)
    {
        size_t j = 0;
        for (wchar_t* c = str; j < count && *c != 0; ++c, ++j)
        {
            wchar_t t = towupper(*c);
            if (!iswdigit(t) && !iswalpha(t))
                t = '_';
            *c = t;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

const SValue g_pOptions [] =
{
    { L"r",         OPT_RECURSIVE },
    { L"s",         OPT_STREAMING },
    { L"o",         OPT_OUTPUTFILE },
    { L"h",         OPT_OUTPUTHEADER },
    { L"n",         OPT_NOOVERWRITE },
    { L"c",         OPT_COMPACT },
    { L"nc",        OPT_NOCOMPACT },
    { L"f",         OPT_FRIENDLY_NAMES },
    { L"nologo",    OPT_NOLOGO },
    { L"flist",     OPT_FILELIST },
    { nullptr,      0 }
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

namespace
{
#pragma prefast(disable : 26018, "Only used with static internal arrays")

    DWORD LookupByName(const wchar_t *pName, const SValue *pArray)
    {
        while (pArray->pName)
        {
            if (!_wcsicmp(pName, pArray->pName))
                return pArray->dwValue;

            pArray++;
        }

        return 0;
    }

    const wchar_t* LookupByValue(DWORD pValue, const SValue *pArray)
    {
        while (pArray->pName)
        {
            if (pValue == pArray->dwValue)
                return pArray->pName;

            pArray++;
        }

        return L"";
    }

    void SearchForFiles(const wchar_t* path, std::list<SConversion>& files, bool recursive)
    {
        // Process files
        WIN32_FIND_DATA findData = {};
        ScopedFindHandle hFile(safe_handle(FindFirstFileExW(path,
            FindExInfoBasic, &findData,
            FindExSearchNameMatch, nullptr,
            FIND_FIRST_EX_LARGE_FETCH)));
        if (hFile)
        {
            for (;;)
            {
                if (!(findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY)))
                {
                    wchar_t drive[_MAX_DRIVE] = {};
                    wchar_t dir[_MAX_DIR] = {};
                    _wsplitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);

                    SConversion conv;
                    _wmakepath_s(conv.szSrc, drive, dir, findData.cFileName, nullptr);
                    files.push_back(conv);
                }

                if (!FindNextFile(hFile.get(), &findData))
                    break;
            }
        }

        // Process directories
        if (recursive)
        {
            wchar_t searchDir[MAX_PATH] = {};
            {
                wchar_t drive[_MAX_DRIVE] = {};
                wchar_t dir[_MAX_DIR] = {};
                _wsplitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
                _wmakepath_s(searchDir, drive, dir, L"*", nullptr);
            }

            hFile.reset(safe_handle(FindFirstFileExW(searchDir,
                FindExInfoBasic, &findData,
                FindExSearchLimitToDirectories, nullptr,
                FIND_FIRST_EX_LARGE_FETCH)));
            if (!hFile)
                return;

            for (;;)
            {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (findData.cFileName[0] != L'.')
                    {
                        wchar_t subdir[MAX_PATH] = {};

                        {
                            wchar_t drive[_MAX_DRIVE] = {};
                            wchar_t dir[_MAX_DIR] = {};
                            wchar_t fname[_MAX_FNAME] = {};
                            wchar_t ext[_MAX_FNAME] = {};
                            _wsplitpath_s(path, drive, dir, fname, ext);
                            wcscat_s(dir, findData.cFileName);
                            _wmakepath_s(subdir, drive, dir, fname, ext);
                        }

                        SearchForFiles(subdir, files, recursive);
                    }
                }

                if (!FindNextFile(hFile.get(), &findData))
                    break;
            }
        }
    }

    void PrintLogo()
    {
        wprintf(L"Microsoft (R) XACT-style Wave Bank Tool \n");
        wprintf(L"Copyright (C) Microsoft Corp. All rights reserved.\n");
#ifdef _DEBUG
        wprintf(L"*** Debug build ***\n");
#endif
        wprintf(L"\n");
    }

    void PrintUsage()
    {
        PrintLogo();

        wprintf(L"Usage: xwbtool <options> <wav-files>\n");
        wprintf(L"\n");
        wprintf(L"   -r                  wildcard filename search is recursive\n");
        wprintf(L"   -s                  creates a streaming wave bank,\n");
        wprintf(L"                       otherwise an in-memory bank is created\n");
        wprintf(L"   -o <filename>       output filename\n");
        wprintf(L"   -h <h-filename>     output C/C++ header\n");
        wprintf(L"   -n                  do not overwrite output\n");
        wprintf(L"   -c                  force creation of compact wavebank\n");
        wprintf(L"   -nc                 force creation of non-compact wavebank\n");
        wprintf(L"   -f                  include entry friendly names\n");
        wprintf(L"   -nologo             suppress copyright message\n");
        wprintf(L"   -flist <filename>   use text file with a list of input files (one per line)\n");
    }

    const char* GetFormatTagName(WORD wFormatTag)
    {
        switch (wFormatTag)
        {
        case WAVE_FORMAT_PCM: return "PCM";
        case WAVE_FORMAT_ADPCM: return "MS ADPCM";
        case WAVE_FORMAT_EXTENSIBLE: return "EXTENSIBLE";
        case WAVE_FORMAT_IEEE_FLOAT: return "IEEE float";
        case WAVE_FORMAT_MPEGLAYER3: return "ISO/MPEG Layer3";
        case WAVE_FORMAT_DOLBY_AC3_SPDIF: return "Dolby Audio Codec 3 over S/PDIF";
        case WAVE_FORMAT_WMAUDIO2: return "Windows Media Audio";
        case WAVE_FORMAT_WMAUDIO3: return "Windows Media Audio Pro";
        case WAVE_FORMAT_WMASPDIF: return "Windows Media Audio over S/PDIF";
        case 0x165: /*WAVE_FORMAT_XMA*/ return "Xbox XMA";
        case 0x166: /*WAVE_FORMAT_XMA2*/ return "Xbox XMA2";
        default: return "*UNKNOWN*";
        }
    }

    const char *ChannelDesc(DWORD dwChannelMask)
    {
        switch (dwChannelMask)
        {
        case 0x00000004 /*SPEAKER_MONO*/: return "Mono"; // 
        case 0x00000003 /* SPEAKER_STEREO */: return "Stereo";
        case 0x0000000B /* SPEAKER_2POINT1 */: return "2.1";
        case 0x00000107 /* SPEAKER_SURROUND */: return "Surround";
        case 0x00000033 /* SPEAKER_QUAD */: return "Quad";
        case 0x0000003B /* SPEAKER_4POINT1 */: return "4.1";
        case 0x0000003F /* SPEAKER_5POINT1 */: return "5.1";
        case 0x000000FF /* SPEAKER_7POINT1 */: return "7.1";
        case 0x0000060F /* SPEAKER_5POINT1_SURROUND */: return "Surround5.1";
        case 0x0000063F /* SPEAKER_7POINT1_SURROUND */: return "Surround7.1";
        default: return "Custom";
        }
    }

    void PrintInfo(const WaveFile& wave)
    {
        wprintf(L" (%hs %u channels, %u-bit, %u Hz)", GetFormatTagName(wave.data.wfx->wFormatTag), wave.data.wfx->nChannels, wave.data.wfx->wBitsPerSample, wave.data.wfx->nSamplesPerSec);
    }

    bool FileExists(const wchar_t* pszFilename)
    {
        FILE *f = nullptr;
        if (!_wfopen_s(&f, pszFilename, L"rb"))
        {
            if (f)
                fclose(f);

            return true;
        }

        return false;
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------
// Entry-point
//--------------------------------------------------------------------------------------
#pragma prefast(disable : 28198, "Command-line tool, frees all memory on exit")

int __cdecl wmain(_In_ int argc, _In_z_count_(argc) wchar_t* argv[])
{
    // Parameters and defaults
    wchar_t szOutputFile[MAX_PATH] = {};
    wchar_t szHeaderFile[MAX_PATH] = {};

    ScopedHandle hFile;

    // Process command line
    DWORD dwOptions = 0;
    std::list<SConversion> conversion;

    for (int iArg = 1; iArg < argc; iArg++)
    {
        PWSTR pArg = argv[iArg];

        if (('-' == pArg[0]) || ('/' == pArg[0]))
        {
            pArg++;
            PWSTR pValue;

            for (pValue = pArg; *pValue && (':' != *pValue); pValue++);

            if (*pValue)
                *pValue++ = 0;

            DWORD dwOption = LookupByName(pArg, g_pOptions);

            if (!dwOption || (dwOptions & (1 << dwOption)))
            {
                PrintUsage();
                return 1;
            }

            dwOptions |= 1 << dwOption;

            // Handle options with additional value parameter
            switch (dwOption)
            {
            case OPT_OUTPUTFILE:
            case OPT_OUTPUTHEADER:
            case OPT_FILELIST:
                if (!*pValue)
                {
                    if ((iArg + 1 >= argc))
                    {
                        PrintUsage();
                        return 1;
                    }

                    iArg++;
                    pValue = argv[iArg];
                }
                break;
            }

            switch (dwOption)
            {
            case OPT_OUTPUTFILE:
                wcscpy_s(szOutputFile, MAX_PATH, pValue);
                break;

            case OPT_OUTPUTHEADER:
                wcscpy_s(szHeaderFile, MAX_PATH, pValue);
                break;

            case OPT_COMPACT:
                if (dwOptions & (1 << OPT_NOCOMPACT))
                {
                    wprintf(L"-c and -nc are mutually exclusive options\n");
                    return 1;
                }
                break;

            case OPT_NOCOMPACT:
                if (dwOptions & (1 << OPT_COMPACT))
                {
                    wprintf(L"-c and -nc are mutually exclusive options\n");
                    return 1;
                }
                break;

            case OPT_FILELIST:
                {
                    std::wifstream inFile(pValue);
                    if (!inFile)
                    {
                        wprintf(L"Error opening -flist file %ls\n", pValue);
                        return 1;
                    }
                    wchar_t fname[1024] = {};
                    for (;;)
                    {
                        inFile >> fname;
                        if (!inFile)
                            break;

                        if (*fname == L'#')
                        {
                            // Comment
                        }
                        else if (*fname == L'-')
                        {
                            wprintf(L"Command-line arguments not supported in -flist file\n");
                            return 1;
                        }
                        else if (wcspbrk(fname, L"?*") != nullptr)
                        {
                            wprintf(L"Wildcards not supported in -flist file\n");
                            return 1;
                        }
                        else
                        {
                            SConversion conv;
                            wcscpy_s(conv.szSrc, MAX_PATH, fname);
                            conversion.push_back(conv);
                        }

                        inFile.ignore(1000, '\n');
                    }
                    inFile.close();
                }
                break;
            }
        }
        else if (wcspbrk(pArg, L"?*") != nullptr)
        {
            size_t count = conversion.size();
            SearchForFiles(pArg, conversion, (dwOptions & (1 << OPT_RECURSIVE)) != 0);
            if (conversion.size() <= count)
            {
                wprintf(L"No matching files found for %ls\n", pArg);
                return 1;
            }
        }
        else
        {
            SConversion conv;
            wcscpy_s(conv.szSrc, MAX_PATH, pArg);

            conversion.push_back(conv);
        }
    }

    if (conversion.empty())
    {
        wprintf(L"ERROR: Need at least 1 wave file to build wave bank\n\n");
        PrintUsage();
        return 0;
    }

    if (~dwOptions & (1 << OPT_NOLOGO))
        PrintLogo();

    // Gather wave files
    std::unique_ptr<uint8_t[]> entries;
    std::unique_ptr<char[]> entryNames;
    std::vector<WaveFile> waves;
    MINIWAVEFORMAT compactFormat = {};

    bool xma = false;

    size_t index = 0;
    for (auto pConv = conversion.begin(); pConv != conversion.end(); ++pConv, ++index)
    {
        wchar_t ext[_MAX_EXT];
        wchar_t fname[_MAX_FNAME];
        _wsplitpath_s(pConv->szSrc, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, ext, _MAX_EXT);

        // Load source image
        if (pConv != conversion.begin())
            wprintf(L"\n");
        else if (!*szOutputFile)
        {
            if (_wcsicmp(ext, L".xwb") == 0)
            {
                wprintf(L"ERROR: Need to specify output file via -o\n");
                return 1;
            }

            _wmakepath_s(szOutputFile, nullptr, nullptr, fname, L".xwb");
        }

        wprintf(L"reading %ls", pConv->szSrc);
        fflush(stdout);

        WaveFile wave;
        wave.conv = index;
        std::unique_ptr<uint8_t[]> waveData;

        HRESULT hr = DirectX::LoadWAVAudioFromFileEx(pConv->szSrc, waveData, wave.data);
        if (FAILED(hr))
        {
            wprintf(L"\nERROR: Failed to load file (%08X)\n", hr);
            return 1;
        }

        wave.waveData = std::move(waveData);

        PrintInfo(wave);

        if (wave.data.wfx->wFormatTag == WAVE_FORMAT_XMA2)
            xma = true;

        waves.emplace_back(std::move(wave));
    }

    wprintf(L"\n");

    DWORD dwAlignment = ALIGNMENT_MIN;
    if (dwOptions & (1 << OPT_STREAMING))
        dwAlignment = ALIGNMENT_DVD;
    else if (xma)
        dwAlignment = 2048;

    // Convert wave format to miniformat, failing if any won't map
    // Check to see if we can use the compact wave bank format
    bool compact = (dwOptions & (1 << OPT_NOCOMPACT)) ? false : true;
    int reason = 0;
    uint64_t waveOffset = 0;

    for (auto it = waves.begin(); it != waves.end(); ++it)
    {
        if (!ConvertToMiniFormat(it->data.wfx, it->data.seek != 0, it->miniFmt))
        {
            auto cit = conversion.cbegin();
            advance(cit, it->conv);
            wprintf(L"ERROR: Failed encoding %ls\n", cit->szSrc);
            return 1;
        }

        if (it == waves.begin())
        {
            memcpy(&compactFormat, &it->miniFmt, sizeof(MINIWAVEFORMAT));
        }
        else if (memcmp(&compactFormat, &it->miniFmt, sizeof(MINIWAVEFORMAT)) != 0)
        {
            compact = false;
            reason |= 0x1;
        }

        if (it->data.loopLength > 0)
        {
            compact = false;
            reason |= 0x2;
        }

        DWORD alignedSize = BLOCKALIGNPAD(it->data.audioBytes, dwAlignment);
        waveOffset += alignedSize;
    }

    if (waveOffset > UINT32_MAX)
    {
        wprintf(L"ERROR: Audio wave data is too large to encode into wavebank (offset %llu)", waveOffset);
        return 1;
    }
    else if (waveOffset > (MAX_COMPACT_DATA_SEGMENT_SIZE * uint64_t(dwAlignment)))
    {
        compact = false;
        reason |= 0x4;
    }

    if ((dwOptions & (1 << OPT_COMPACT)) && !compact)
    {
        wprintf(L"ERROR: Cannot create compact wave bank:\n");
        if (reason & 0x1)
        {
            wprintf(L"- Mismatched formats. All formats must be identical for a compact wavebank.\n");
        }
        if (reason & 0x2)
        {
            wprintf(L"- Found loop points. Compact wavebanks do not support loop points.\n");
        }
        if (reason & 0x4)
        {
            wprintf(L"- Audio wave data is too large to encode in compact wavebank (%llu > %llu).\n", waveOffset, (MAX_COMPACT_DATA_SEGMENT_SIZE * uint64_t(dwAlignment)));
        }
        return 1;
    }

    // Build entry metadata (and assign wave offset within data segment)
    // Build entry friendly names if requested
    entries.reset(new uint8_t[(compact ? sizeof(ENTRYCOMPACT) : sizeof(ENTRY)) * waves.size()]);

    if (dwOptions & (1 << OPT_FRIENDLY_NAMES))
    {
        entryNames.reset(new char[waves.size() * ENTRYNAME_LENGTH]);
        memset(entryNames.get(), 0, sizeof(char) * waves.size() * ENTRYNAME_LENGTH);
    }

    waveOffset = 0;
    size_t count = 0;
    size_t seekEntries = 0;
    for (auto it = waves.begin(); it != waves.end(); ++it, ++count)
    {
        DWORD alignedSize = BLOCKALIGNPAD(it->data.audioBytes, dwAlignment);

        auto wfx = it->data.wfx;

        uint64_t duration = 0;

        switch (it->miniFmt.wFormatTag)
        {
        case MINIWAVEFORMAT::TAG_XMA:
            if (it->data.seekCount > 0)
                seekEntries += it->data.seekCount + 1;

            duration = reinterpret_cast<const XMA2WAVEFORMATEX*>(wfx)->SamplesEncoded;
            break;

        case MINIWAVEFORMAT::TAG_ADPCM:
        {
            auto adpcmFmt = reinterpret_cast<const ADPCMEWAVEFORMAT*>(wfx);
            duration = (uint64_t(it->data.audioBytes) / uint64_t(wfx->nBlockAlign)) * uint64_t(adpcmFmt->wSamplesPerBlock);
            int partial = it->data.audioBytes % wfx->nBlockAlign;
            if (partial)
            {
                if (partial >= (7 * wfx->nChannels))
                    duration += (uint64_t(partial) * 2 / uint64_t(wfx->nChannels - 12));
            }
        }
        break;

        case MINIWAVEFORMAT::TAG_WMA:
            if (it->data.seekCount > 0)
            {
                seekEntries += it->data.seekCount + 1;
                duration = it->data.seek[it->data.seekCount - 1] / uint32_t(2 * wfx->nChannels);
            }
            break;

        default: // MINIWAVEFORMAT::TAG_PCM
            duration = (uint64_t(it->data.audioBytes) * 8) / (uint64_t(wfx->wBitsPerSample) * uint64_t(wfx->nChannels));
            break;
        }

        if (compact)
        {
            auto entry = reinterpret_cast<ENTRYCOMPACT*>(entries.get() + count * sizeof(ENTRYCOMPACT));
            memset(entry, 0, sizeof(ENTRYCOMPACT));

            assert(waveOffset <= (MAX_COMPACT_DATA_SEGMENT_SIZE * uint64_t(dwAlignment)));
            entry->dwOffset = uint32_t(waveOffset / dwAlignment);

            assert(dwAlignment <= 2048);
            entry->dwLengthDeviation = alignedSize - it->data.audioBytes;
        }
        else
        {
            auto entry = reinterpret_cast<ENTRY*>(entries.get() + count * sizeof(ENTRY));
            memset(entry, 0, sizeof(ENTRY));

            if (duration > 268435455)
            {
                wprintf(L"ERROR: Duration of audio too long to encode into wavebank (%llu > 2^28))\n", duration);
                return 1;
            }

            entry->Duration = uint32_t(duration);
            memcpy(&entry->Format, &it->miniFmt, sizeof(MINIWAVEFORMAT));
            entry->PlayRegion.dwOffset = uint32_t(waveOffset);
            entry->PlayRegion.dwLength = it->data.audioBytes;

            if (it->data.loopLength > 0)
            {
                entry->LoopRegion.dwStartSample = it->data.loopStart;
                entry->LoopRegion.dwTotalSamples = it->data.loopLength;
            }
        }

        if (dwOptions & (1 << OPT_FRIENDLY_NAMES))
        {
            auto cit = conversion.cbegin();
            advance(cit, it->conv);

            wchar_t wEntryName[_MAX_FNAME];
            _wsplitpath_s(cit->szSrc, nullptr, 0, nullptr, 0, wEntryName, _MAX_FNAME, nullptr, 0);

            int result = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, wEntryName, -1, &entryNames[count * ENTRYNAME_LENGTH], ENTRYNAME_LENGTH, nullptr, nullptr);
            if (result <= 0)
            {
                memset(&entryNames[count * ENTRYNAME_LENGTH], 0, ENTRYNAME_LENGTH);
            }
        }

        waveOffset += alignedSize;
    }

    assert(count > 0 && count == waves.size());

    // Create wave bank
    assert(*szOutputFile != 0);

    wprintf(L"writing %ls%ls wavebank %ls w/ %zu entries\n", (compact) ? L"compact " : L"", (dwOptions & (1 << OPT_STREAMING)) ? L"streaming" : L"in-memory", szOutputFile, waves.size());
    fflush(stdout);

    if (dwOptions & (1 << OPT_NOOVERWRITE))
    {
        if (FileExists(szOutputFile))
        {
            wprintf(L"ERROR: Output file %ls already exists!\n", szOutputFile);
            return 1;
        }

        if (*szHeaderFile)
        {
            if (FileExists(szHeaderFile))
            {
                wprintf(L"ERROR: Output header file %ls already exists!\n", szHeaderFile);
                return 1;
            }
        }
    }

    hFile.reset(safe_handle(CreateFileW(szOutputFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)));
    if (!hFile)
    {
        wprintf(L"ERROR: Failed opening output file %ls, %u\n", szOutputFile, GetLastError());
        return 1;
    }

    // Setup wave bank header
    HEADER header = {};
    header.dwSignature = HEADER::SIGNATURE;
    header.dwHeaderVersion = HEADER::VERSION;
    header.dwVersion = XACT_CONTENT_VERSION;

    DWORD segmentOffset = sizeof(HEADER);

    // Write bank metadata
    assert((segmentOffset % 4) == 0);

    BANKDATA data = {};

    data.dwEntryCount = uint32_t(waves.size());
    data.dwAlignment = dwAlignment;

    GetSystemTimeAsFileTime(&data.BuildTime);

    data.dwFlags = (dwOptions & (1 << OPT_STREAMING)) ? BANKDATA::TYPE_STREAMING : BANKDATA::TYPE_BUFFER;

    data.dwFlags |= BANKDATA::FLAGS_SEEKTABLES;

    if (dwOptions & (1 << OPT_FRIENDLY_NAMES))
    {
        data.dwFlags |= BANKDATA::FLAGS_ENTRYNAMES;
        data.dwEntryNameElementSize = ENTRYNAME_LENGTH;
    }

    if (compact)
    {
        data.dwFlags |= BANKDATA::FLAGS_COMPACT;
        data.dwEntryMetaDataElementSize = sizeof(ENTRYCOMPACT);
        memcpy(&data.CompactFormat, &compactFormat, sizeof(MINIWAVEFORMAT));
    }
    else
    {
        data.dwEntryMetaDataElementSize = sizeof(ENTRY);
    }

    {
        wchar_t wBankName[_MAX_FNAME];
        _wsplitpath_s(szOutputFile, nullptr, 0, nullptr, 0, wBankName, _MAX_FNAME, nullptr, 0);

        int result = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, wBankName, -1, data.szBankName, BANKDATA::BANKNAME_LENGTH, nullptr, nullptr);
        if (result <= 0)
        {
            memset(data.szBankName, 0, BANKDATA::BANKNAME_LENGTH);
        }
    }

    if (SetFilePointer(hFile.get(), segmentOffset, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        wprintf(L"ERROR: Failed writing bank data to %ls, SFP %u\n", szOutputFile, GetLastError());
        return 1;
    }

    DWORD bytesWritten;
    if (!WriteFile(hFile.get(), &data, sizeof(data), &bytesWritten, nullptr)
        || bytesWritten != sizeof(data))
    {
        wprintf(L"ERROR: Failed writing bank data to %ls, %u\n", szOutputFile, GetLastError());
        return 1;
    }

    header.Segments[HEADER::SEGIDX_BANKDATA].dwOffset = segmentOffset;
    header.Segments[HEADER::SEGIDX_BANKDATA].dwLength = sizeof(BANKDATA);
    segmentOffset += sizeof(BANKDATA);

    // Write entry metadata
    assert((segmentOffset % 4) == 0);

    if (SetFilePointer(hFile.get(), segmentOffset, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        wprintf(L"ERROR: Failed writing entry metadata to %ls, SFP %u\n", szOutputFile, GetLastError());
        return 1;
    }

    uint32_t entryBytes = uint32_t(waves.size() * data.dwEntryMetaDataElementSize);
    if (!WriteFile(hFile.get(), entries.get(), entryBytes, &bytesWritten, nullptr)
        || bytesWritten != entryBytes)
    {
        wprintf(L"ERROR: Failed writing entry metadata to %ls, %u\n", szOutputFile, GetLastError());
        return 1;
    }

    header.Segments[HEADER::SEGIDX_ENTRYMETADATA].dwOffset = segmentOffset;
    header.Segments[HEADER::SEGIDX_ENTRYMETADATA].dwLength = entryBytes;
    segmentOffset += entryBytes;

    // Write seek tables
    assert((segmentOffset % 4) == 0);

    header.Segments[HEADER::SEGIDX_SEEKTABLES].dwOffset = segmentOffset;

    if (seekEntries > 0)
    {
        seekEntries += waves.size(); // Room for an offset per entry

        std::unique_ptr<uint32_t[]> seekTables(new uint32_t[seekEntries]);

        if (SetFilePointer(hFile.get(), segmentOffset, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            wprintf(L"ERROR: Failed writing seek tables to %ls, SFP %u\n", szOutputFile, GetLastError());
            return 1;
        }

        uint32_t seekoffset = 0;
        uint32_t windex = 0;
        for (auto it = waves.begin(); it != waves.end(); ++it, ++windex)
        {
            if (it->miniFmt.wFormatTag == MINIWAVEFORMAT::TAG_WMA)
            {
                seekTables[windex] = seekoffset * sizeof(uint32_t);

                uint32_t baseoffset = uint32_t(waves.size() + seekoffset);
                seekTables[baseoffset] = it->data.seekCount;

                for (uint32_t j = 0; j < it->data.seekCount; ++j)
                {
                    seekTables[baseoffset + j + 1] = it->data.seek[j];
                }

                seekoffset += it->data.seekCount + 1;
            }
            else if (it->miniFmt.wFormatTag == MINIWAVEFORMAT::TAG_XMA)
            {
                seekTables[windex] = seekoffset * sizeof(uint32_t);

                uint32_t baseoffset = uint32_t(waves.size() + seekoffset);
                seekTables[baseoffset] = it->data.seekCount;

                for (uint32_t j = 0; j < it->data.seekCount; ++j)
                {
                    seekTables[baseoffset + j + 1] = _byteswap_ulong(it->data.seek[j]);
                }

                seekoffset += it->data.seekCount + 1;
            }
            else
            {
                seekTables[windex] = uint32_t(-1);
            }
        }

        uint32_t seekLen = uint32_t(sizeof(uint32_t) * seekEntries);

        if (!WriteFile(hFile.get(), seekTables.get(), seekLen, &bytesWritten, nullptr)
            || bytesWritten != seekLen)
        {
            wprintf(L"ERROR: Failed writing seek tables to %ls, %u\n", szOutputFile, GetLastError());
            return 1;
        }

        segmentOffset += seekLen;

        header.Segments[HEADER::SEGIDX_SEEKTABLES].dwLength = seekLen;
    }
    else
    {
        header.Segments[HEADER::SEGIDX_SEEKTABLES].dwLength = 0;
    }

    // Write entry names
    if (dwOptions & (1 << OPT_FRIENDLY_NAMES))
    {
        assert((segmentOffset % 4) == 0);

        if (SetFilePointer(hFile.get(), segmentOffset, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            wprintf(L"ERROR: Failed writing friendly entry names to %ls, SFP %u\n", szOutputFile, GetLastError());
            return 1;
        }

        uint32_t entryNamesBytes = uint32_t(count * data.dwEntryNameElementSize);
        if (!WriteFile(hFile.get(), entryNames.get(), entryNamesBytes, &bytesWritten, nullptr)
            || bytesWritten != entryNamesBytes)
        {
            wprintf(L"ERROR: Failed writing friendly entry names to %ls, %u\n", szOutputFile, GetLastError());
            return 1;
        }

        header.Segments[HEADER::SEGIDX_ENTRYNAMES].dwOffset = segmentOffset;
        header.Segments[HEADER::SEGIDX_ENTRYNAMES].dwLength = entryNamesBytes;
        segmentOffset += entryNamesBytes;
    }

    // Write wave data
    segmentOffset = BLOCKALIGNPAD(segmentOffset, dwAlignment);

    header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwOffset = segmentOffset;
    header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwLength = uint32_t(waveOffset);

    for (auto it = waves.begin(); it != waves.end(); ++it)
    {
        if (SetFilePointer(hFile.get(), segmentOffset, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            wprintf(L"ERROR: Failed writing audio data to %ls, SFP %u\n", szOutputFile, GetLastError());
            return 1;
        }

        if (!WriteFile(hFile.get(), it->data.startAudio, it->data.audioBytes, &bytesWritten, nullptr)
            || bytesWritten != it->data.audioBytes)
        {
            wprintf(L"ERROR: Failed writing audio data to %ls, %u\n", szOutputFile, GetLastError());
            return 1;
        }

        DWORD alignedSize = BLOCKALIGNPAD(it->data.audioBytes, dwAlignment);

        if ((uint64_t(segmentOffset) + alignedSize) > UINT32_MAX)
        {
            wprintf(L"ERROR: Data exceeds maximum size for wavebank\n");
            return 1;
        }

        segmentOffset += alignedSize;
    }

    assert(segmentOffset == (header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwOffset + waveOffset));

    // Commit wave bank
    if (SetFilePointer(hFile.get(), segmentOffset, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        wprintf(L"ERROR: Failed committing output file %ls, EOF %u\n", szOutputFile, GetLastError());
        return 1;
    }

    if (!SetEndOfFile(hFile.get()))
    {
        wprintf(L"ERROR: Failed committing output file %ls, EOF %u\n", szOutputFile, GetLastError());
        return 1;
    }

    if (SetFilePointer(hFile.get(), 0, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        wprintf(L"ERROR: Failed committing output file %ls, HDR %u\n", szOutputFile, GetLastError());
        return 1;
    }

    if (!WriteFile(hFile.get(), &header, sizeof(header), &bytesWritten, nullptr)
        || bytesWritten != sizeof(header))
    {
        wprintf(L"ERROR: Failed committing output file %ls, HDR %u\n", szOutputFile, GetLastError());
        return 1;
    }

    // Write C header if requested
    if (*szHeaderFile)
    {
        wprintf(L"writing C header %ls\n", szHeaderFile);
        fflush(stdout);

        FILE* file = nullptr;
        if (!_wfopen_s(&file, szHeaderFile, L"wt"))
        {
            wchar_t wBankName[_MAX_FNAME];
            _wsplitpath_s(szOutputFile, nullptr, 0, nullptr, 0, wBankName, _MAX_FNAME, nullptr, 0);

            FileNameToIdentifier(wBankName, _MAX_FNAME);

            fprintf_s(file, "#pragma once\n\nenum XACT_WAVEBANK_%ls\n{\n", wBankName);

            size_t windex = 0;
            for (auto it = waves.begin(); it != waves.end(); ++it, ++windex)
            {
                auto cit = conversion.cbegin();
                advance(cit, it->conv);

                wchar_t wEntryName[_MAX_FNAME];
                _wsplitpath_s(cit->szSrc, nullptr, 0, nullptr, 0, wEntryName, _MAX_FNAME, nullptr, 0);

                FileNameToIdentifier(wEntryName, _MAX_FNAME);

                fprintf_s(file, "    XACT_WAVEBANK_%ls_%ls = %zu,\n", wBankName, wEntryName, windex);
            }

            fprintf_s(file, "};\n\n#define XACT_WAVEBANK_%ls_ENTRY_COUNT %zu\n", wBankName, count);

            fclose(file);
        }
        else
        {
            wprintf(L"ERROR: Failed writing wave bank C header %ls\n", szHeaderFile);
            return 1;
        }
    }

    return 0;
}
