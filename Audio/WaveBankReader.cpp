//--------------------------------------------------------------------------------------
// File: WaveBankReader.cpp
//
// Functions for loading audio data from Wave Banks
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "WaveBankReader.h"

//--------------------------------------------------------------------------------------
namespace
{
    
struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };

typedef public std::unique_ptr<void, handle_closer> ScopedHandle;

inline HANDLE safe_handle( HANDLE h ) { return (h == INVALID_HANDLE_VALUE) ? 0 : h; }

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
};

struct SAMPLEREGION
{
    uint32_t    dwStartSample;  // Start sample for the region.
    uint32_t    dwTotalSamples; // Region length in samples.
};

struct HEADER
{
    static const uint32_t SIGNATURE = 'DNBW';
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
    static const uint32_t TAG_PCM   = 0x0;
    static const uint32_t TAG_XMA   = 0x1;
    static const uint32_t TAG_ADPCM = 0x2;
    static const uint32_t TAG_WMA   = 0x3;

    static const uint32_t BITDEPTH_8 = 0x0; // PCM only
    static const uint32_t BITDEPTH_16 = 0x1; // PCM only

    static const size_t ADPCM_BLOCKALIGN_CONVERSION_OFFSET = 22;
    
    struct
    {
        uint32_t       wFormatTag      : 2;        // Format tag
        uint32_t       nChannels       : 3;        // Channel count (1 - 6)
        uint32_t       nSamplesPerSec  : 18;       // Sampling rate
        uint32_t       wBlockAlign     : 8;        // Block alignment.  For WMA, lower 6 bits block alignment index, upper 2 bits bytes-per-second index.
        uint32_t       wBitsPerSample  : 1;        // Bits per sample (8 vs. 16, PCM only); WMAudio2/WMAudio3 (for WMA)
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
                if ( dwBlockAlignIndex < _countof(aWMABlockAlign) )
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
        case TAG_XMA:
            return nSamplesPerSec * wBlockAlign;
            break;

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
                if ( dwBytesPerSecIndex < _countof(aWMAAvgBytesPerSec) )
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
        fmt->wNumCoef = 7; /* MSADPCM_NUM_COEFFICIENTS */

        static ADPCMCOEFSET aCoef[7] = { { 256, 0}, {512, -256}, {0,0}, {192,64}, {240,0}, {460, -208}, {392,-232} };
        memcpy( &fmt->aCoef, aCoef, sizeof(aCoef) );
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
            uint32_t                   dwFlags  :  4;

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
    uint32_t       dwOffset            : 21;       // Data offset, in multiplies of the bank alignment
    uint32_t       dwLengthDeviation   : 11;       // Data length deviation, in bytes
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

};

static_assert( sizeof(REGION)==8, "Mismatch with xact3wb.h" );
static_assert( sizeof(SAMPLEREGION)==8, "Mismatch with xact3wb.h" );
static_assert( sizeof(HEADER)==52, "Mismatch with xact3wb.h" );
static_assert( sizeof(ENTRY)==24, "Mismatch with xact3wb.h" );
static_assert( sizeof(MINIWAVEFORMAT)==4, "Mismatch with xact3wb.h" );
static_assert( sizeof(ENTRY)==24, "Mismatch with xact3wb.h" );
static_assert( sizeof(ENTRYCOMPACT)==4, "Mismatch with xact3wb.h" );
static_assert( sizeof(BANKDATA)==96, "Mismatch with xact3wb.h" );

using namespace DirectX;

//--------------------------------------------------------------------------------------
class WaveBankReader::Impl
{
public:
    Impl() :
        m_async( INVALID_HANDLE_VALUE ),
        m_event( INVALID_HANDLE_VALUE ),
        m_prepared(false)
    {
        memset( &m_header, 0, sizeof(HEADER) );
        memset( &m_data, 0, sizeof(BANKDATA) );
        memset( &m_request, 0, sizeof(OVERLAPPED) );
    }

    ~Impl() { Close(); }

    HRESULT Open( _In_z_ const wchar_t* szFileName );
    void Close();

    HRESULT GetFormat( _In_ uint32_t index, _Out_ WAVEFORMATEX* pFormat, _In_ size_t maxsize ) const;

    HRESULT GetWaveData( _In_ uint32_t index, _Outptr_ const uint8_t** pData, _Out_ uint32_t& dataSize ) const;

    HRESULT GetMetadata( _In_ uint32_t index, _Out_ Metadata& metadata ) const;

    bool UpdatePrepared();

    void Clear()
    {
        memset( &m_header, 0, sizeof(HEADER) );
        memset( &m_data, 0, sizeof(BANKDATA ) );

        m_names.clear();
        m_entries.reset();
        m_waveData.reset();
    }

    HANDLE                              m_async;
    HANDLE                              m_event;
    OVERLAPPED                          m_request;
    bool                                m_prepared;

    HEADER                              m_header;
    BANKDATA                            m_data;
    std::map<std::string, uint32_t>     m_names;
    std::unique_ptr<uint8_t[]>          m_entries;
    std::unique_ptr<uint8_t[]>          m_waveData;
};


_Use_decl_annotations_
HRESULT WaveBankReader::Impl::Open( const wchar_t* szFileName )
{
    Close();
    Clear();

    m_prepared = false;

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
    m_event = CreateEventEx( nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE );
#else
    m_event = CreateEvent( nullptr, TRUE, FALSE, nullptr );
#endif

    if ( !m_event )
    {
        m_event = INVALID_HANDLE_VALUE;
        return HRESULT_FROM_WIN32( GetLastError() );
    }

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    CREATEFILE2_EXTENDED_PARAMETERS params = { sizeof(params), 0 };
    params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    params.dwFileFlags = FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN;
    params.dwSecurityQosFlags = SECURITY_IMPERSONATION;
    ScopedHandle hFile( safe_handle( CreateFile2( szFileName,
                                                  GENERIC_READ,
                                                  FILE_SHARE_READ,
                                                  OPEN_EXISTING,
                                                  &params ) ) );
#else
    ScopedHandle hFile( safe_handle( CreateFileW( szFileName,
                                                  GENERIC_READ,
                                                  FILE_SHARE_READ,
                                                  nullptr,
                                                  OPEN_EXISTING,
                                                  FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN,
                                                  nullptr ) ) );
#endif

    if ( !hFile )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    // Read and verify header
    OVERLAPPED request;
    memset( &request, 0, sizeof(request) );
    request.hEvent = m_event;

    if( !ReadFile( hFile.get(), &m_header, sizeof( m_header ), nullptr, &request ) )
    {
        DWORD error = GetLastError();
        if ( error != ERROR_IO_PENDING )
            return HRESULT_FROM_WIN32( error );
    }

    DWORD bytes;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    BOOL result = GetOverlappedResultEx( hFile.get(), &request, &bytes, INFINITE, FALSE );
#else
    BOOL result = GetOverlappedResult( hFile.get(), &request, &bytes, TRUE );
#endif

    if( !result || ( bytes != sizeof( m_header ) ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    if ( m_header.dwSignature != HEADER::SIGNATURE )
    {
        return E_FAIL;
    }

    if ( m_header.dwHeaderVersion != HEADER::VERSION )
    {
        return E_FAIL;
    }

    // Load bank data
    memset( &request, 0, sizeof(request) );
    request.Offset = m_header.Segments[HEADER::SEGIDX_BANKDATA].dwOffset;
    request.hEvent = m_event;

    if( !ReadFile( hFile.get(), &m_data, sizeof( m_data ), nullptr, &request ) )
    {
        DWORD error = GetLastError();
        if ( error != ERROR_IO_PENDING )
            return HRESULT_FROM_WIN32( error );
    }

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    result = GetOverlappedResultEx( hFile.get(), &request, &bytes, INFINITE, FALSE );
#else
    result = GetOverlappedResult( hFile.get(), &request, &bytes, TRUE );
#endif

    if ( !result || ( bytes != sizeof( m_data ) ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    if ( !m_data.dwEntryCount )
    {
        return HRESULT_FROM_WIN32( ERROR_NO_DATA );
    }

    if ( m_data.dwFlags & BANKDATA::FLAGS_COMPACT )
    {
        if ( m_data.dwEntryMetaDataElementSize != sizeof(ENTRYCOMPACT) )
        {
            return E_FAIL;
        }

        if (  m_header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwLength > MAX_COMPACT_DATA_SEGMENT_SIZE )
        {
            // Data segment is too large to be valid compact wavebank
            return E_FAIL;
        }
    }
    else
    {
        if ( m_data.dwEntryMetaDataElementSize != sizeof(ENTRY) )
        {
            return E_FAIL;
        }
    }

    DWORD metadataBytes = m_header.Segments[HEADER::SEGIDX_ENTRYMETADATA].dwLength;
    if ( metadataBytes != ( m_data.dwEntryCount * m_data.dwEntryMetaDataElementSize ) )
    {
        return E_FAIL;
    }

    if ( m_data.dwFlags & BANKDATA::TYPE_STREAMING )
    {
        if ( m_data.dwAlignment < ALIGNMENT_DVD )
            return E_FAIL;
    }
    else if ( m_data.dwAlignment < ALIGNMENT_MIN )
    {
        return E_FAIL;
    }

    // Load names
    DWORD namesBytes = m_header.Segments[HEADER::SEGIDX_ENTRYNAMES].dwLength;
    if ( namesBytes > 0 )
    {
        if ( namesBytes >= ( m_data.dwEntryNameElementSize * m_data.dwEntryCount ) )
        {
            std::unique_ptr<char[]> temp( new (std::nothrow) char[ namesBytes ] );
            if ( !temp )
                return E_OUTOFMEMORY;

            memset( &request, 0, sizeof(request) );
            request.Offset = m_header.Segments[HEADER::SEGIDX_ENTRYNAMES].dwOffset;
            request.hEvent = m_event;

            if ( !ReadFile( hFile.get(), temp.get(), namesBytes, nullptr, &request ) )
            {
                DWORD error = GetLastError();
                if ( error != ERROR_IO_PENDING )
                    return HRESULT_FROM_WIN32( error );
            }

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
            result = GetOverlappedResultEx( hFile.get(), &request, &bytes, INFINITE, FALSE );
#else
            result = GetOverlappedResult( hFile.get(), &request, &bytes, TRUE );
#endif

            if ( !result || ( namesBytes != bytes ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }

            for( uint32_t j = 0; j < m_data.dwEntryCount; ++j )
            {
                DWORD n = m_data.dwEntryNameElementSize * j;

                char name[ 64 ] = {0};
                strncpy_s( name, &temp.get()[ n ], 64 );

                m_names[ name ] = j;
            }
        }
    }

    // Load entries
    if ( m_data.dwFlags & BANKDATA::FLAGS_COMPACT )
    {
        m_entries.reset( reinterpret_cast<uint8_t*>( new (std::nothrow) ENTRYCOMPACT[ m_data.dwEntryCount ] ) );
    }
    else
    {
        m_entries.reset( reinterpret_cast<uint8_t*>( new (std::nothrow) ENTRY[ m_data.dwEntryCount ] ) );
    }
    if ( !m_entries )
        return E_OUTOFMEMORY;

    memset( &request, 0, sizeof(request) );
    request.Offset = m_header.Segments[HEADER::SEGIDX_ENTRYMETADATA].dwOffset;
    request.hEvent = m_event;

    if ( !ReadFile( hFile.get(), m_entries.get(), metadataBytes, nullptr, &request ) )
    {
        DWORD error = GetLastError();
        if ( error != ERROR_IO_PENDING )
            return HRESULT_FROM_WIN32( error );
    }

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    result = GetOverlappedResultEx( hFile.get(), &request, &bytes, INFINITE, FALSE );
#else
    result = GetOverlappedResult( hFile.get(), &request, &bytes, TRUE );
#endif

    if ( !result || ( metadataBytes != bytes ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    DWORD waveLen = m_header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwLength;
    if ( !waveLen )
    {
        return HRESULT_FROM_WIN32( ERROR_NO_DATA );
    }

    if ( m_data.dwFlags & BANKDATA::TYPE_STREAMING )
    {
        // If streaming, reopen without buffering
        hFile.reset();

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        CREATEFILE2_EXTENDED_PARAMETERS params = { sizeof(params), 0 };
        params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        params.dwFileFlags = FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING;
        params.dwSecurityQosFlags = SECURITY_IMPERSONATION;
        m_async = CreateFile2( szFileName,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               OPEN_EXISTING,
                               &params );
#else
        m_async = CreateFileW( szFileName,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               nullptr,
                               OPEN_EXISTING,
                               FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                               nullptr );
#endif

        if ( m_async == INVALID_HANDLE_VALUE )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        m_prepared = true;
    }
    else
    {
        // If in-memory, kick off read of wave data
        m_waveData.reset( new (std::nothrow) uint8_t[ waveLen ] );
        if ( !m_waveData )
            return E_OUTOFMEMORY;

        memset( &m_request, 0, sizeof(OVERLAPPED) );
        m_request.Offset = m_header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwOffset;
        m_request.hEvent = m_event;

        if ( !ReadFile( hFile.get(), m_waveData.get(), waveLen, nullptr, &m_request ) )
        {
            DWORD error = GetLastError();
            if ( error != ERROR_IO_PENDING )
                return HRESULT_FROM_WIN32( error );
        }

        m_async = hFile.release();
    }

    return S_OK;
}


void WaveBankReader::Impl::Close()
{
    if ( m_async != INVALID_HANDLE_VALUE )
    {
        if ( m_request.hEvent != 0 )
        {
            DWORD bytes;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
            (void)GetOverlappedResultEx( m_async, &m_request, &bytes, INFINITE, FALSE );
#else
            (void)GetOverlappedResult( m_async, &m_request, &bytes, TRUE );
#endif
        }

        CloseHandle( m_async );
        m_async = INVALID_HANDLE_VALUE;
    }
    if ( m_event != INVALID_HANDLE_VALUE )
    {
        CloseHandle( m_event );
        m_event = INVALID_HANDLE_VALUE;
    }
}


_Use_decl_annotations_
HRESULT WaveBankReader::Impl::GetFormat( uint32_t index, WAVEFORMATEX* pFormat, size_t maxsize ) const
{
    if ( !pFormat || !maxsize )
        return E_INVALIDARG;

    if ( index >= m_data.dwEntryCount || !m_entries )
    {
        return E_FAIL;
    }

    auto& miniFmt = ( m_data.dwFlags & BANKDATA::FLAGS_COMPACT ) ? m_data.CompactFormat : ( reinterpret_cast<const ENTRY*>( m_entries.get() )[ index ].Format );

    switch( miniFmt.wFormatTag )
    {
        case MINIWAVEFORMAT::TAG_PCM:
            if ( maxsize < sizeof(PCMWAVEFORMAT) )
                return HRESULT_FROM_WIN32( ERROR_MORE_DATA );

            pFormat->wFormatTag = WAVE_FORMAT_PCM;
            pFormat->cbSize = 0;
            break;

        case MINIWAVEFORMAT::TAG_ADPCM:
            if ( maxsize < ( sizeof(ADPCMWAVEFORMAT) + 30 ) )
                return HRESULT_FROM_WIN32( ERROR_MORE_DATA );

            pFormat->wFormatTag = WAVE_FORMAT_ADPCM;
            pFormat->cbSize = 32; /* MSADPCM_FORMAT_EXTRA_BYTES */
            {
                ADPCMWAVEFORMAT *adpcmFmt = reinterpret_cast<ADPCMWAVEFORMAT*>(pFormat);
                adpcmFmt->wSamplesPerBlock = (WORD) miniFmt.AdpcmSamplesPerBlock();
                miniFmt.AdpcmFillCoefficientTable( adpcmFmt );
            }
            break;

        case MINIWAVEFORMAT::TAG_WMA:
            if ( maxsize < sizeof(WAVEFORMATEX) )
                return HRESULT_FROM_WIN32( ERROR_MORE_DATA );

            pFormat->wFormatTag = (miniFmt.wBitsPerSample & 0x1) ? WAVE_FORMAT_WMAUDIO3 : WAVE_FORMAT_WMAUDIO2;
            pFormat->cbSize = 0;
            break;

        default:
            // MINIWAVEFORMAT::TAG_XMA is only valid for Xbox
            return E_FAIL;
    }

    pFormat->nChannels = miniFmt.nChannels;
    pFormat->wBitsPerSample = miniFmt.BitsPerSample();
    pFormat->nBlockAlign = (WORD) miniFmt.BlockAlign();
    pFormat->nSamplesPerSec = miniFmt.nSamplesPerSec;
    pFormat->nAvgBytesPerSec = miniFmt.AvgBytesPerSec();

    return S_OK;
}


_Use_decl_annotations_
HRESULT WaveBankReader::Impl::GetWaveData( uint32_t index, const uint8_t** pData, uint32_t& dataSize ) const
{
    if ( !pData )
        return E_INVALIDARG;

    if ( index >= m_data.dwEntryCount || !m_entries || !m_waveData )
    {
        return E_FAIL;
    }

    if ( m_data.dwFlags & BANKDATA::TYPE_STREAMING )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }

    if ( !m_prepared )
    {
        return HRESULT_FROM_WIN32( ERROR_IO_INCOMPLETE );
    }

    if ( m_data.dwFlags & BANKDATA::FLAGS_COMPACT )
    {
        auto& entry = reinterpret_cast<const ENTRYCOMPACT*>( m_entries.get() )[ index ];

        DWORD dwOffset = entry.dwOffset * m_data.dwAlignment;

        DWORD dwLength;
        if ( index < ( m_data.dwEntryCount - 1 ) )
        {
            dwLength = ( reinterpret_cast<const ENTRYCOMPACT*>( m_entries.get() )[index + 1].dwOffset * m_data.dwAlignment ) - dwOffset - entry.dwLengthDeviation;
        }
        else
        {
            dwLength = m_header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwLength - dwOffset - entry.dwLengthDeviation;
        }

        if ( ( dwOffset + dwLength ) > m_header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwLength )
        {
            return HRESULT_FROM_WIN32( ERROR_HANDLE_EOF );
        }

        *pData = &m_waveData.get()[ dwOffset ];
        dataSize = dwLength;
    }
    else
    {
        auto& entry = reinterpret_cast<const ENTRY*>( m_entries.get() )[ index ];

        if ( ( entry.PlayRegion.dwOffset + entry.PlayRegion.dwLength ) > m_header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwLength )
        {
            return HRESULT_FROM_WIN32( ERROR_HANDLE_EOF );
        }
   
        *pData = &m_waveData.get()[ entry.PlayRegion.dwOffset ];
        dataSize = entry.PlayRegion.dwLength;
    }

    return S_OK;
}


_Use_decl_annotations_
HRESULT WaveBankReader::Impl::GetMetadata( uint32_t index, Metadata& metadata ) const
{
    if ( index >= m_data.dwEntryCount || !m_entries )
    {
        return E_FAIL;
    }

    if ( m_data.dwFlags & BANKDATA::FLAGS_COMPACT )
    {
        auto& entry = reinterpret_cast<const ENTRYCOMPACT*>( m_entries.get() )[ index ];

        DWORD dwOffset = entry.dwOffset * m_data.dwAlignment;

        DWORD dwLength;
        if ( index < ( m_data.dwEntryCount - 1 ) )
        {
            dwLength = ( reinterpret_cast<const ENTRYCOMPACT*>( m_entries.get() )[index + 1].dwOffset * m_data.dwAlignment ) - dwOffset - entry.dwLengthDeviation;
        }
        else
        {
            dwLength = m_header.Segments[HEADER::SEGIDX_ENTRYWAVEDATA].dwLength - dwOffset - entry.dwLengthDeviation;
        }

        metadata.duration = uint32_t( ( uint64_t(dwLength) * 8 ) / uint64_t( m_data.CompactFormat.BitsPerSample() * m_data.CompactFormat.nChannels ) );
        metadata.loopStart = metadata.loopLength = 0;
        metadata.offsetBytes = dwOffset;
        metadata.lengthBytes = dwLength;
    }
    else
    {
        auto& entry = reinterpret_cast<const ENTRY*>( m_entries.get() )[ index ];

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
    if ( m_prepared )
        return true;

    if ( m_async == INVALID_HANDLE_VALUE )
        return false;

    if ( m_request.hEvent != 0 )
    {
        DWORD bytes;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        BOOL result = GetOverlappedResultEx( m_async, &m_request, &bytes, 0, FALSE );
#else
        BOOL result = GetOverlappedResult( m_async, &m_request, &bytes, FALSE );
#endif

        if ( result )
        {
            m_prepared = true;

            memset( &m_request, 0, sizeof(OVERLAPPED) );
        }
    }
  
    return m_prepared;
}



//--------------------------------------------------------------------------------------
WaveBankReader::WaveBankReader() :
    pImpl( new Impl )
{
}


WaveBankReader::~WaveBankReader()
{
}


_Use_decl_annotations_
HRESULT WaveBankReader::Open( const wchar_t* szFileName )
{
    return pImpl->Open( szFileName );
}


_Use_decl_annotations_
uint32_t WaveBankReader::Find( const char* name ) const
{
    auto it = pImpl->m_names.find( name );
    if ( it != pImpl->m_names.cend() )
    {
        return it->second;
    }

    return uint32_t(-1);
}


bool WaveBankReader::IsPrepared()
{
    if ( pImpl->m_prepared )
        return true;

    return pImpl->UpdatePrepared();
}


void WaveBankReader::WaitOnPrepare()
{
    if ( pImpl->m_prepared )
        return;

    if ( pImpl->m_request.hEvent != 0 )
    {
        WaitForSingleObjectEx( pImpl->m_request.hEvent, INFINITE, FALSE );

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
HRESULT WaveBankReader::GetFormat( uint32_t index, WAVEFORMATEX* pFormat, size_t maxsize ) const
{
    return pImpl->GetFormat( index, pFormat, maxsize );
}


_Use_decl_annotations_
HRESULT WaveBankReader::GetWaveData( uint32_t index, const uint8_t** pData, uint32_t& dataSize ) const
{
    return pImpl->GetWaveData( index, pData, dataSize );
}


_Use_decl_annotations_
HRESULT WaveBankReader::GetMetadata( uint32_t index, Metadata& metadata ) const
{
    return pImpl->GetMetadata( index, metadata );
}


HANDLE WaveBankReader::GetAsyncHandle() const
{
    return ( pImpl->m_data.dwFlags & BANKDATA::TYPE_STREAMING ) ? pImpl->m_async : INVALID_HANDLE_VALUE;
}
