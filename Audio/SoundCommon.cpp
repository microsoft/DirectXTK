//--------------------------------------------------------------------------------------
// File: SoundCommon.cpp
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


namespace
{
    template <typename T> WORD ChannelsSpecifiedInMask(T x)
    {
        WORD bitCount = 0;
        while (x) { ++bitCount; x &= (x - 1); }
        return bitCount;
    }
}


//======================================================================================
// Wave format utilities
//======================================================================================

bool DirectX::IsValid(_In_ const WAVEFORMATEX* wfx)
{
    if (!wfx)
        return false;

    if (!wfx->nChannels)
    {
        DebugTrace("ERROR: Wave format must have at least 1 channel\n");
        return false;
    }

    if (wfx->nChannels > XAUDIO2_MAX_AUDIO_CHANNELS)
    {
        DebugTrace("ERROR: Wave format must have less than %u channels (%u)\n", XAUDIO2_MAX_AUDIO_CHANNELS, wfx->nChannels);
        return false;
    }

    if (!wfx->nSamplesPerSec)
    {
        DebugTrace("ERROR: Wave format cannot have a sample rate of 0\n");
        return false;
    }

    if ((wfx->nSamplesPerSec < XAUDIO2_MIN_SAMPLE_RATE)
        || (wfx->nSamplesPerSec > XAUDIO2_MAX_SAMPLE_RATE))
    {
        DebugTrace("ERROR: Wave format channel count must be in range %u..%u (%u)\n", XAUDIO2_MIN_SAMPLE_RATE, XAUDIO2_MAX_SAMPLE_RATE, wfx->nSamplesPerSec);
        return false;
    }

    switch (wfx->wFormatTag)
    {
        case WAVE_FORMAT_PCM:

            switch (wfx->wBitsPerSample)
            {
                case 8:
                case 16:
                case 24:
                case 32:
                    break;

                default:
                    DebugTrace("ERROR: Wave format integer PCM must have 8, 16, 24, or 32 bits per sample (%u)\n", wfx->wBitsPerSample);
                    return false;
            }

            if (wfx->nBlockAlign != (wfx->nChannels * wfx->wBitsPerSample / 8))
            {
                DebugTrace("ERROR: Wave format integer PCM - nBlockAlign (%u) != nChannels (%u) * wBitsPerSample (%u) / 8\n",
                           wfx->nBlockAlign, wfx->nChannels, wfx->wBitsPerSample);
                return false;
            }

            if (wfx->nAvgBytesPerSec != (wfx->nSamplesPerSec * wfx->nBlockAlign))
            {
                DebugTrace("ERROR: Wave format integer PCM - nAvgBytesPerSec (%lu) != nSamplesPerSec (%lu) * nBlockAlign (%u)\n",
                           wfx->nAvgBytesPerSec, wfx->nSamplesPerSec, wfx->nBlockAlign);
                return false;
            }

            return true;

        case WAVE_FORMAT_IEEE_FLOAT:

            if (wfx->wBitsPerSample != 32)
            {
                DebugTrace("ERROR: Wave format float PCM must have 32-bits per sample (%u)\n", wfx->wBitsPerSample);
                return false;
            }

            if (wfx->nBlockAlign != (wfx->nChannels * wfx->wBitsPerSample / 8))
            {
                DebugTrace("ERROR: Wave format float PCM - nBlockAlign (%u) != nChannels (%u) * wBitsPerSample (%u) / 8\n",
                           wfx->nBlockAlign, wfx->nChannels, wfx->wBitsPerSample);
                return false;
            }

            if (wfx->nAvgBytesPerSec != (wfx->nSamplesPerSec * wfx->nBlockAlign))
            {
                DebugTrace("ERROR: Wave format float PCM - nAvgBytesPerSec (%lu) != nSamplesPerSec (%lu) * nBlockAlign (%u)\n",
                           wfx->nAvgBytesPerSec, wfx->nSamplesPerSec, wfx->nBlockAlign);
                return false;
            }

            return true;

        case WAVE_FORMAT_ADPCM:

            if ((wfx->nChannels != 1) && (wfx->nChannels != 2))
            {
                DebugTrace("ERROR: Wave format ADPCM must have 1 or 2 channels (%u)\n", wfx->nChannels);
                return false;
            }

            if (wfx->wBitsPerSample != 4 /*MSADPCM_BITS_PER_SAMPLE*/)
            {
                DebugTrace("ERROR: Wave format ADPCM must have 4 bits per sample (%u)\n", wfx->wBitsPerSample);
                return false;
            }

            if (wfx->cbSize != 32 /*MSADPCM_FORMAT_EXTRA_BYTES*/)
            {
                DebugTrace("ERROR: Wave format ADPCM must have cbSize = 32 (%u)\n", wfx->cbSize);
                return false;
            }
            else
            {
                auto wfadpcm = reinterpret_cast<const ADPCMWAVEFORMAT*>(wfx);

                if (wfadpcm->wNumCoef != 7 /*MSADPCM_NUM_COEFFICIENTS*/)
                {
                    DebugTrace("ERROR: Wave format ADPCM must have 7 coefficients (%u)\n", wfadpcm->wNumCoef);
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
                    DebugTrace("ERROR: Wave formt ADPCM found non-standard coefficients\n");
                    return false;
                }

                if ((wfadpcm->wSamplesPerBlock < 4 /*MSADPCM_MIN_SAMPLES_PER_BLOCK*/)
                    || (wfadpcm->wSamplesPerBlock > 64000 /*MSADPCM_MAX_SAMPLES_PER_BLOCK*/))
                {
                    DebugTrace("ERROR: Wave format ADPCM wSamplesPerBlock must be 4..64000 (%u)\n", wfadpcm->wSamplesPerBlock);
                    return false;
                }

                if (wfadpcm->wfx.nChannels == 1 && (wfadpcm->wSamplesPerBlock % 2))
                {
                    DebugTrace("ERROR: Wave format ADPCM mono files must have even wSamplesPerBlock\n");
                    return false;
                }

                int nHeaderBytes = 7 /*MSADPCM_HEADER_LENGTH*/ * wfx->nChannels;
                int nBitsPerFrame = 4 /*MSADPCM_BITS_PER_SAMPLE*/ * wfx->nChannels;
                int nPcmFramesPerBlock = (wfx->nBlockAlign - nHeaderBytes) * 8 / nBitsPerFrame + 2;

                if (wfadpcm->wSamplesPerBlock != nPcmFramesPerBlock)
                {
                    DebugTrace("ERROR: Wave format ADPCM %u-channel with nBlockAlign = %u must have wSamplesPerBlock = %u (%u)\n",
                               wfx->nChannels, wfx->nBlockAlign, nPcmFramesPerBlock, wfadpcm->wSamplesPerBlock);
                    return false;
                }
            }
            return true;

        case WAVE_FORMAT_WMAUDIO2:
        case WAVE_FORMAT_WMAUDIO3:

        #if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

            if (wfx->wBitsPerSample != 16)
            {
                DebugTrace("ERROR: Wave format xWMA only supports 16-bit data\n");
                return false;
            }

            if (!wfx->nBlockAlign)
            {
                DebugTrace("ERROR: Wave format xWMA must have a non-zero nBlockAlign\n");
                return false;
            }

            if (!wfx->nAvgBytesPerSec)
            {
                DebugTrace("ERROR: Wave format xWMA must have a non-zero nAvgBytesPerSec\n");
                return false;
            }

            return true;

        #else
            DebugTrace("ERROR: Wave format xWMA not supported by this version of DirectXTK for Audio\n");
            return false;
        #endif

        case 0x166 /* WAVE_FORMAT_XMA2 */:

        #if defined(_XBOX_ONE) && defined(_TITLE)

            if (wfx->nBlockAlign != wfx->nChannels * XMA_OUTPUT_SAMPLE_BYTES)
            {
                DebugTrace("ERROR: Wave format XMA2 - nBlockAlign (%u) != nChannels(%u) * %u\n", wfx->nBlockAlign, wfx->nChannels, XMA_OUTPUT_SAMPLE_BYTES);
                return false;
            }

            if (wfx->wBitsPerSample != XMA_OUTPUT_SAMPLE_BITS)
            {
                DebugTrace("ERROR: Wave format XMA2 wBitsPerSample (%u) should be %u\n", wfx->wBitsPerSample, XMA_OUTPUT_SAMPLE_BITS);
                return false;
            }

            if (wfx->cbSize != (sizeof(XMA2WAVEFORMATEX) - sizeof(WAVEFORMATEX)))
            {
                DebugTrace("ERROR: Wave format XMA2 - cbSize must be %zu (%u)\n", (sizeof(XMA2WAVEFORMATEX) - sizeof(WAVEFORMATEX)), wfx->cbSize);
                return false;
            }
            else
            {
                auto xmaFmt = reinterpret_cast<const XMA2WAVEFORMATEX*>(wfx);

                if (xmaFmt->EncoderVersion < 3)
                {
                    DebugTrace("ERROR: Wave format XMA2 encoder version (%u) - 3 or higher is required\n", xmaFmt->EncoderVersion);
                    return false;
                }

                if (!xmaFmt->BlockCount)
                {
                    DebugTrace("ERROR: Wave format XMA2 BlockCount must be non-zero\n");
                    return false;
                }

                if (!xmaFmt->BytesPerBlock || (xmaFmt->BytesPerBlock > XMA_READBUFFER_MAX_BYTES))
                {
                    DebugTrace("ERROR: Wave format XMA2 BytesPerBlock (%u) is invalid\n", xmaFmt->BytesPerBlock);
                    return false;
                }

                if (xmaFmt->ChannelMask)
                {
                    auto channelBits = ChannelsSpecifiedInMask(xmaFmt->ChannelMask);
                    if (channelBits != wfx->nChannels)
                    {
                        DebugTrace("ERROR: Wave format XMA2 - nChannels=%u but ChannelMask (%08X) has %u bits set\n",
                                   xmaFmt->ChannelMask, wfx->nChannels, channelBits);
                        return false;
                    }
                }

                if (xmaFmt->NumStreams != ((wfx->nChannels + 1) / 2))
                {
                    DebugTrace("ERROR: Wave format XMA2 - NumStreams (%u) != ( nChannels(%u) + 1 ) / 2\n", xmaFmt->NumStreams, wfx->nChannels);
                    return false;
                }

                if ((xmaFmt->PlayBegin + xmaFmt->PlayLength) > xmaFmt->SamplesEncoded)
                {
                    DebugTrace("ERROR: Wave format XMA2 play region too large (%u + %u > %u)\n", xmaFmt->PlayBegin, xmaFmt->PlayLength, xmaFmt->SamplesEncoded);
                    return false;
                }

                if ((xmaFmt->LoopBegin + xmaFmt->LoopLength) > xmaFmt->SamplesEncoded)
                {
                    DebugTrace("ERROR: Wave format XMA2 loop region too large (%u + %u > %u)\n", xmaFmt->LoopBegin, xmaFmt->LoopLength, xmaFmt->SamplesEncoded);
                    return false;
                }
            }
            return true;

        #else
            DebugTrace("ERROR: Wave format XMA2 not supported by this version of DirectXTK for Audio\n");
            return false;
        #endif

        case WAVE_FORMAT_EXTENSIBLE:
            if (wfx->cbSize < (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)))
            {
                DebugTrace("ERROR: Wave format WAVE_FORMAT_EXTENSIBLE - cbSize must be %zu (%u)\n", (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)), wfx->cbSize);
                return false;
            }
            else
            {
                static const GUID s_wfexBase = { 0x00000000, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

                auto wfex = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wfx);

                if (memcmp(reinterpret_cast<const BYTE*>(&wfex->SubFormat) + sizeof(DWORD),
                    reinterpret_cast<const BYTE*>(&s_wfexBase) + sizeof(DWORD), sizeof(GUID) - sizeof(DWORD)) != 0)
                {
                    DebugTrace("ERROR: Wave format WAVEFORMATEXTENSIBLE encountered with unknown GUID ({%8.8lX-%4.4X-%4.4X-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X})\n",
                               wfex->SubFormat.Data1, wfex->SubFormat.Data2, wfex->SubFormat.Data3,
                               wfex->SubFormat.Data4[0], wfex->SubFormat.Data4[1], wfex->SubFormat.Data4[2], wfex->SubFormat.Data4[3],
                               wfex->SubFormat.Data4[4], wfex->SubFormat.Data4[5], wfex->SubFormat.Data4[6], wfex->SubFormat.Data4[7]);
                    return false;
                }

                switch (wfex->SubFormat.Data1)
                {
                    case WAVE_FORMAT_PCM:

                        switch (wfx->wBitsPerSample)
                        {
                            case 8:
                            case 16:
                            case 24:
                            case 32:
                                break;

                            default:
                                DebugTrace("ERROR: Wave format integer PCM must have 8, 16, 24, or 32 bits per sample (%u)\n", wfx->wBitsPerSample);
                                return false;
                        }

                        switch (wfex->Samples.wValidBitsPerSample)
                        {
                            case 0:
                            case 8:
                            case 16:
                            case 20:
                            case 24:
                            case 32:
                                break;

                            default:
                                DebugTrace("ERROR: Wave format integer PCM must have 8, 16, 20, 24, or 32 valid bits per sample (%u)\n", wfex->Samples.wValidBitsPerSample);
                                return false;
                        }

                        if (wfex->Samples.wValidBitsPerSample
                            && (wfex->Samples.wValidBitsPerSample > wfx->wBitsPerSample))
                        {
                            DebugTrace("ERROR: Wave format ingter PCM wValidBitsPerSample (%u) is greater than wBitsPerSample (%u)\n", wfex->Samples.wValidBitsPerSample, wfx->wBitsPerSample);
                            return false;
                        }

                        if (wfx->nBlockAlign != (wfx->nChannels * wfx->wBitsPerSample / 8))
                        {
                            DebugTrace("ERROR: Wave format integer PCM - nBlockAlign (%u) != nChannels (%u) * wBitsPerSample (%u) / 8\n",
                                       wfx->nBlockAlign, wfx->nChannels, wfx->wBitsPerSample);
                            return false;
                        }

                        if (wfx->nAvgBytesPerSec != (wfx->nSamplesPerSec * wfx->nBlockAlign))
                        {
                            DebugTrace("ERROR: Wave format integer PCM - nAvgBytesPerSec (%lu) != nSamplesPerSec (%lu) * nBlockAlign (%u)\n",
                                       wfx->nAvgBytesPerSec, wfx->nSamplesPerSec, wfx->nBlockAlign);
                            return false;
                        }

                        break;

                    case WAVE_FORMAT_IEEE_FLOAT:

                        if (wfx->wBitsPerSample != 32)
                        {
                            DebugTrace("ERROR: Wave format float PCM must have 32-bits per sample (%u)\n", wfx->wBitsPerSample);
                            return false;
                        }

                        switch (wfex->Samples.wValidBitsPerSample)
                        {
                            case 0:
                            case 32:
                                break;

                            default:
                                DebugTrace("ERROR: Wave format float PCM must have 32 valid bits per sample (%u)\n", wfex->Samples.wValidBitsPerSample);
                                return false;
                        }

                        if (wfx->nBlockAlign != (wfx->nChannels * wfx->wBitsPerSample / 8))
                        {
                            DebugTrace("ERROR: Wave format float PCM - nBlockAlign (%u) != nChannels (%u) * wBitsPerSample (%u) / 8\n",
                                       wfx->nBlockAlign, wfx->nChannels, wfx->wBitsPerSample);
                            return false;
                        }

                        if (wfx->nAvgBytesPerSec != (wfx->nSamplesPerSec * wfx->nBlockAlign))
                        {
                            DebugTrace("ERROR: Wave format float PCM - nAvgBytesPerSec (%lu) != nSamplesPerSec (%lu) * nBlockAlign (%u)\n",
                                       wfx->nAvgBytesPerSec, wfx->nSamplesPerSec, wfx->nBlockAlign);
                            return false;
                        }

                        break;

                    case WAVE_FORMAT_ADPCM:
                        DebugTrace("ERROR: Wave format ADPCM is not supported as a WAVEFORMATEXTENSIBLE\n");
                        return false;

                    case WAVE_FORMAT_WMAUDIO2:
                    case WAVE_FORMAT_WMAUDIO3:

                    #if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

                        if (wfx->wBitsPerSample != 16)
                        {
                            DebugTrace("ERROR: Wave format xWMA only supports 16-bit data\n");
                            return false;
                        }

                        if (!wfx->nBlockAlign)
                        {
                            DebugTrace("ERROR: Wave format xWMA must have a non-zero nBlockAlign\n");
                            return false;
                        }

                        if (!wfx->nAvgBytesPerSec)
                        {
                            DebugTrace("ERROR: Wave format xWMA must have a non-zero nAvgBytesPerSec\n");
                            return false;
                        }

                        break;

                    #else
                        DebugTrace("ERROR: Wave format xWMA not supported by this version of DirectXTK for Audio\n");
                        return false;
                    #endif

                    case 0x166 /* WAVE_FORMAT_XMA2 */:
                        DebugTrace("ERROR: Wave format XMA2 is not supported as a WAVEFORMATEXTENSIBLE\n");
                        return false;

                    default:
                        DebugTrace("ERROR: Unknown WAVEFORMATEXTENSIBLE format tag (%u)\n", wfex->SubFormat.Data1);
                        return false;
                }

                if (wfex->dwChannelMask)
                {
                    auto channelBits = ChannelsSpecifiedInMask(wfex->dwChannelMask);
                    if (channelBits != wfx->nChannels)
                    {
                        DebugTrace("ERROR: WAVEFORMATEXTENSIBLE: nChannels=%u but ChannelMask has %u bits set\n",
                                   wfx->nChannels, channelBits);
                        return false;
                    }
                }

                return true;
            }

        default:
            DebugTrace("ERROR: Unknown WAVEFORMATEX format tag (%u)\n", wfx->wFormatTag);
            return false;
    }
}


uint32_t DirectX::GetDefaultChannelMask(int channels)
{
    switch (channels)
    {
        case 1: return SPEAKER_MONO;
        case 2: return SPEAKER_STEREO;
        case 3: return SPEAKER_2POINT1;
        case 4: return SPEAKER_QUAD;
        case 5: return SPEAKER_4POINT1;
        case 6: return SPEAKER_5POINT1;
        case 7: return SPEAKER_5POINT1 | SPEAKER_BACK_CENTER;
        case 8: return SPEAKER_7POINT1;
        default: return 0;
    }
}


_Use_decl_annotations_
void DirectX::CreateIntegerPCM(WAVEFORMATEX* wfx, int sampleRate, int channels, int sampleBits)
{
    int blockAlign = channels * sampleBits / 8;

    wfx->wFormatTag = WAVE_FORMAT_PCM;
    wfx->nChannels = static_cast<WORD>(channels);
    wfx->nSamplesPerSec = static_cast<DWORD>(sampleRate);
    wfx->nAvgBytesPerSec = static_cast<DWORD>(blockAlign * sampleRate);
    wfx->nBlockAlign = static_cast<WORD>(blockAlign);
    wfx->wBitsPerSample = static_cast<WORD>(sampleBits);
    wfx->cbSize = 0;

    assert(IsValid(wfx));
}


_Use_decl_annotations_
void DirectX::CreateFloatPCM(WAVEFORMATEX* wfx, int sampleRate, int channels)
{
    int blockAlign = channels * 4;

    wfx->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    wfx->nChannels = static_cast<WORD>(channels);
    wfx->nSamplesPerSec = static_cast<DWORD>(sampleRate);
    wfx->nAvgBytesPerSec = static_cast<DWORD>(blockAlign * sampleRate);
    wfx->nBlockAlign = static_cast<WORD>(blockAlign);
    wfx->wBitsPerSample = 32;
    wfx->cbSize = 0;

    assert(IsValid(wfx));
}


_Use_decl_annotations_
void DirectX::CreateADPCM(WAVEFORMATEX* wfx, size_t wfxSize, int sampleRate, int channels, int samplesPerBlock)
{
    if (wfxSize < (sizeof(WAVEFORMATEX) + 32 /*MSADPCM_FORMAT_EXTRA_BYTES*/))
    {
        DebugTrace("CreateADPCM needs at least %zu bytes for the result\n", (sizeof(WAVEFORMATEX) + 32 /*MSADPCM_FORMAT_EXTRA_BYTES*/));
        throw std::invalid_argument("ADPCMWAVEFORMAT");
    }

    if (!samplesPerBlock)
    {
        DebugTrace("CreateADPCM needs a non-zero samples per block count\n");
        throw std::invalid_argument("ADPCMWAVEFORMAT");
    }

    int blockAlign = (7 /*MSADPCM_HEADER_LENGTH*/) * channels
        + (samplesPerBlock - 2) * (4 /* MSADPCM_BITS_PER_SAMPLE */) * channels / 8;

    wfx->wFormatTag = WAVE_FORMAT_ADPCM;
    wfx->nChannels = static_cast<WORD>(channels);
    wfx->nSamplesPerSec = static_cast<DWORD>(sampleRate);
    wfx->nAvgBytesPerSec = static_cast<DWORD>(blockAlign * sampleRate / samplesPerBlock);
    wfx->nBlockAlign = static_cast<WORD>(blockAlign);
    wfx->wBitsPerSample = 4 /* MSADPCM_BITS_PER_SAMPLE */;
    wfx->cbSize = 32 /*MSADPCM_FORMAT_EXTRA_BYTES*/;

    auto adpcm = reinterpret_cast<ADPCMWAVEFORMAT*>(wfx);
    adpcm->wSamplesPerBlock = static_cast<WORD>(samplesPerBlock);
    adpcm->wNumCoef = 7 /* MSADPCM_NUM_COEFFICIENTS */;

    static ADPCMCOEFSET aCoef[7] = { { 256, 0}, {512, -256}, {0,0}, {192,64}, {240,0}, {460, -208}, {392,-232} };
    memcpy(&adpcm->aCoef, aCoef, sizeof(aCoef));

    assert(IsValid(wfx));
}


#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
_Use_decl_annotations_
void DirectX::CreateXWMA(WAVEFORMATEX* wfx, int sampleRate, int channels, int blockAlign, int avgBytes, bool wma3)
{
    wfx->wFormatTag = static_cast<WORD>((wma3) ? WAVE_FORMAT_WMAUDIO3 : WAVE_FORMAT_WMAUDIO2);
    wfx->nChannels = static_cast<WORD>(channels);
    wfx->nSamplesPerSec = static_cast<DWORD>(sampleRate);
    wfx->nAvgBytesPerSec = static_cast<DWORD>(avgBytes);
    wfx->nBlockAlign = static_cast<WORD>(blockAlign);
    wfx->wBitsPerSample = 16;
    wfx->cbSize = 0;

    assert(IsValid(wfx));
}
#endif


#if defined(_XBOX_ONE) && defined(_TITLE)
_Use_decl_annotations_
void DirectX::CreateXMA2(WAVEFORMATEX* wfx, size_t wfxSize, int sampleRate, int channels, int bytesPerBlock, int blockCount, int samplesEncoded)
{
    if (wfxSize < sizeof(XMA2WAVEFORMATEX))
    {
        DebugTrace("XMA2 needs at least %zu bytes for the result\n", sizeof(XMA2WAVEFORMATEX));
        throw std::invalid_argument("XMA2WAVEFORMATEX");
    }

    if ((bytesPerBlock < 1) || (bytesPerBlock > int(XMA_READBUFFER_MAX_BYTES)))
    {
        DebugTrace("XMA2 needs a valid bytes per block\n");
        throw std::invalid_argument("XMA2WAVEFORMATEX");
    }

    int blockAlign = (channels * (16 /*XMA_OUTPUT_SAMPLE_BITS*/) / 8);

    wfx->wFormatTag = WAVE_FORMAT_XMA2;
    wfx->nChannels = static_cast<WORD>(channels);
    wfx->nSamplesPerSec = static_cast<WORD>(sampleRate);
    wfx->nAvgBytesPerSec = static_cast<DWORD>(blockAlign * sampleRate);
    wfx->nBlockAlign = static_cast<WORD>(blockAlign);
    wfx->wBitsPerSample = 16 /* XMA_OUTPUT_SAMPLE_BITS */;
    wfx->cbSize = sizeof(XMA2WAVEFORMATEX) - sizeof(WAVEFORMATEX);

    auto xmaFmt = reinterpret_cast<XMA2WAVEFORMATEX*>(wfx);

    xmaFmt->NumStreams = static_cast<WORD>((channels + 1) / 2);

    xmaFmt->ChannelMask = GetDefaultChannelMask(channels);

    xmaFmt->SamplesEncoded = static_cast<DWORD>(samplesEncoded);
    xmaFmt->BytesPerBlock = static_cast<DWORD>(bytesPerBlock);
    xmaFmt->PlayBegin = xmaFmt->PlayLength =
        xmaFmt->LoopBegin = xmaFmt->LoopLength = xmaFmt->LoopCount = 0;
    xmaFmt->EncoderVersion = 4 /* XMAENCODER_VERSION_XMA2 */;
    xmaFmt->BlockCount = static_cast<WORD>(blockCount);

    assert(IsValid(wfx));
}
#endif // _XBOX_ONE && _TITLE


_Use_decl_annotations_
bool DirectX::ComputePan(float pan, unsigned int channels, float* matrix)
{
    memset(matrix, 0, sizeof(float) * 16);

    if (channels == 1)
    {
        // Mono panning
        float left = (pan >= 0) ? (1.f - pan) : 1.f;
        left = std::min<float>(1.f, left);
        left = std::max<float>(-1.f, left);

        float right = (pan <= 0) ? (-pan - 1.f) : 1.f;
        right = std::min<float>(1.f, right);
        right = std::max<float>(-1.f, right);

        matrix[0] = left;
        matrix[1] = right;
    }
    else if (channels == 2)
    {
        // Stereo panning
        if (-1.f <= pan && pan <= 0.f)
        {
            matrix[0] = .5f * pan + 1.f;    // .5 when pan is -1, 1 when pan is 0
            matrix[1] = .5f * -pan;         // .5 when pan is -1, 0 when pan is 0
            matrix[2] = 0.f;                //  0 when pan is -1, 0 when pan is 0
            matrix[3] = pan + 1.f;          //  0 when pan is -1, 1 when pan is 0
        }
        else
        {
            matrix[0] = -pan + 1.f;         //  1 when pan is 0,   0 when pan is 1
            matrix[1] = 0.f;                //  0 when pan is 0,   0 when pan is 1
            matrix[2] = .5f * pan;          //  0 when pan is 0, .5f when pan is 1
            matrix[3] = .5f * -pan + 1.f;   //  1 when pan is 0. .5f when pan is 1
        }
    }
    else
    {
        if (pan != 0.f)
        {
            DebugTrace("WARNING: Only supports panning on mono or stereo source data, ignored\n");
        }
        return false;
    }

    return true;
}


//======================================================================================
// SoundEffectInstanceBase
//======================================================================================

void SoundEffectInstanceBase::SetPan(float pan)
{
    assert(pan >= -1.f && pan <= 1.f);

    mPan = pan;

    if (!voice)
        return;

    float matrix[16];
    if (ComputePan(pan, mDSPSettings.SrcChannelCount, matrix))
    {
        HRESULT hr = voice->SetOutputMatrix(nullptr, mDSPSettings.SrcChannelCount, mDSPSettings.DstChannelCount, matrix);
        ThrowIfFailed(hr);
    }
}


void SoundEffectInstanceBase::Apply3D(const AudioListener& listener, const AudioEmitter& emitter, bool rhcoords)
{
    if (!voice)
        return;

    if (!(mFlags & SoundEffectInstance_Use3D))
    {
        DebugTrace("ERROR: Apply3D called for an instance created without SoundEffectInstance_Use3D set\n");
        throw std::exception("Apply3D");
    }

    DWORD dwCalcFlags = X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER | X3DAUDIO_CALCULATE_LPF_DIRECT;

    if (mFlags & SoundEffectInstance_UseRedirectLFE)
    {
        // On devices with an LFE channel, allow the mono source data to be routed to the LFE destination channel.
        dwCalcFlags |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE;
    }

    auto reverb = mReverbVoice;
    if (reverb)
    {
        dwCalcFlags |= X3DAUDIO_CALCULATE_LPF_REVERB | X3DAUDIO_CALCULATE_REVERB;
    }

    float matrix[XAUDIO2_MAX_AUDIO_CHANNELS * 8] = {};
    assert(mDSPSettings.SrcChannelCount <= XAUDIO2_MAX_AUDIO_CHANNELS);
    assert(mDSPSettings.DstChannelCount <= 8);
    mDSPSettings.pMatrixCoefficients = matrix;

    assert(engine != nullptr);
    if (rhcoords)
    {
        X3DAUDIO_EMITTER lhEmitter;
        memcpy(&lhEmitter, &emitter, sizeof(X3DAUDIO_EMITTER));
        lhEmitter.OrientFront.z = -emitter.OrientFront.z;
        lhEmitter.OrientTop.z = -emitter.OrientTop.z;
        lhEmitter.Position.z = -emitter.Position.z;
        lhEmitter.Velocity.z = -emitter.Velocity.z;

        X3DAUDIO_LISTENER lhListener;
        memcpy(&lhListener, &listener, sizeof(X3DAUDIO_LISTENER));
        lhListener.OrientFront.z = -listener.OrientFront.z;
        lhListener.OrientTop.z = -listener.OrientTop.z;
        lhListener.Position.z = -listener.Position.z;
        lhListener.Velocity.z = -listener.Velocity.z;

        X3DAudioCalculate(engine->Get3DHandle(), &lhListener, &lhEmitter, dwCalcFlags, &mDSPSettings);
    }
    else
    {
        X3DAudioCalculate(engine->Get3DHandle(), &listener, &emitter, dwCalcFlags, &mDSPSettings);
    }

    mDSPSettings.pMatrixCoefficients = nullptr;

    (void)voice->SetFrequencyRatio(mFreqRatio * mDSPSettings.DopplerFactor);

    auto direct = mDirectVoice;
    assert(direct != nullptr);
    (void)voice->SetOutputMatrix(direct, mDSPSettings.SrcChannelCount, mDSPSettings.DstChannelCount, matrix);

    if (reverb)
    {
        for (size_t j = 0; (j < mDSPSettings.SrcChannelCount) && (j < XAUDIO2_MAX_AUDIO_CHANNELS); ++j)
        {
            matrix[j] = mDSPSettings.ReverbLevel;
        }
        (void)voice->SetOutputMatrix(reverb, mDSPSettings.SrcChannelCount, 1, matrix);
    }

    if (mFlags & SoundEffectInstance_ReverbUseFilters)
    {
        XAUDIO2_FILTER_PARAMETERS filterDirect = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * mDSPSettings.LPFDirectCoefficient), 1.0f };
        // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
        (void)voice->SetOutputFilterParameters(direct, &filterDirect);

        if (reverb)
        {
            XAUDIO2_FILTER_PARAMETERS filterReverb = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * mDSPSettings.LPFReverbCoefficient), 1.0f };
            // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
            (void)voice->SetOutputFilterParameters(reverb, &filterReverb);
        }
    }
}


