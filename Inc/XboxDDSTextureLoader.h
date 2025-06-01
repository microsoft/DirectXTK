s//--------------------------------------------------------------------------------------
// File: XboxDDSTextureLoader.h
//
// Functions for loading a DDS texture using the XBOX extended header and creating a
// Direct3D11.X runtime resource for it via the CreatePlacement APIs
//
// Note these functions will not load standard DDS files. Use the DDSTextureLoader
// module in the DirectXTex package or as part of the DirectXTK library to load
// these files which use standard Direct3D resource creation APIs.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#if !defined(_XBOX_ONE) || !defined(_TITLE)
#error This module only supports Xbox One exclusive apps
#endif

#include <d3d11_x.h>

#include <cstddef>
#include <cstdint>

#ifndef DIRECTX_TOOLKIT_API
#ifdef DIRECTX_TOOLKIT_EXPORT
#ifdef __GNUC__
#define DIRECTX_TOOLKIT_API __attribute__ ((dllexport))
#else
#define DIRECTX_TOOLKIT_API __declspec(dllexport)
#endif
#elif defined(DIRECTX_TOOLKIT_IMPORT)
#ifdef __GNUC__
#define DIRECTX_TOOLKIT_API __attribute__ ((dllimport))
#else
#define DIRECTX_TOOLKIT_API __declspec(dllimport)
#endif
#else
#define DIRECTX_TOOLKIT_API
#endif
#endif

#ifndef DDS_ALPHA_MODE_DEFINED
#define DDS_ALPHA_MODE_DEFINED
namespace DirectX
{
    enum DDS_ALPHA_MODE : uint32_t
    {
        DDS_ALPHA_MODE_UNKNOWN = 0,
        DDS_ALPHA_MODE_STRAIGHT = 1,
        DDS_ALPHA_MODE_PREMULTIPLIED = 2,
        DDS_ALPHA_MODE_OPAQUE = 3,
        DDS_ALPHA_MODE_CUSTOM = 4,
    };
}
#endif


namespace Xbox
{
    using DirectX::DDS_ALPHA_MODE;

    DIRECTX_TOOLKIT_API
        HRESULT __cdecl CreateDDSTextureFromMemory(
            _In_ ID3D11DeviceX* d3dDevice,
            _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
            _In_ size_t ddsDataSize,
            _Outptr_opt_ ID3D11Resource** texture,
            _Outptr_opt_ ID3D11ShaderResourceView** textureView,
            _Outptr_ void** grfxMemory,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _In_ bool forceSRGB = false) noexcept;

    DIRECTX_TOOLKIT_API
        HRESULT __cdecl CreateDDSTextureFromFile(
            _In_ ID3D11DeviceX* d3dDevice,
            _In_z_ const wchar_t* szFileName,
            _Outptr_opt_ ID3D11Resource** texture,
            _Outptr_opt_ ID3D11ShaderResourceView** textureView,
            _Outptr_ void** grfxMemory,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _In_ bool forceSRGB = false) noexcept;

    DIRECTX_TOOLKIT_API
        void FreeDDSTextureMemory(_In_opt_ void* grfxMemory) noexcept;

#ifdef __cpp_lib_byte
    DIRECTX_TOOLKIT_API
        inline HRESULT __cdecl CreateDDSTextureFromMemory(
            _In_ ID3D11DeviceX* d3dDevice,
            _In_reads_bytes_(ddsDataSize) const std::byte* ddsData,
            _In_ size_t ddsDataSize,
            _Outptr_opt_ ID3D11Resource** texture,
            _Outptr_opt_ ID3D11ShaderResourceView** textureView,
            _Outptr_ void** grfxMemory,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _In_ bool forceSRGB = false) noexcept
    {
        return CreateDDSTextureFromMemory(d3dDevice, reinterpret_cast<const uint8_t*>(ddsData), ddsDataSize, texture, textureView, grfxMemory, alphaMode, forceSRGB);
    }
#endif // __cpp_lib_byte
}
