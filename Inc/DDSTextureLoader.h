//--------------------------------------------------------------------------------------
// File: DDSTextureLoader.h
//
// Functions for loading a DDS texture and creating a Direct3D runtime resource for it
//
// Note these functions are useful as a light-weight runtime loader for DDS files. For
// a full-featured DDS file reader, writer, and texture processing pipeline see
// the 'Texconv' sample and the 'DirectXTex' library.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
#endif

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


namespace DirectX
{
#ifndef DDS_ALPHA_MODE_DEFINED
#define DDS_ALPHA_MODE_DEFINED
    enum DDS_ALPHA_MODE : uint32_t
    {
        DDS_ALPHA_MODE_UNKNOWN = 0,
        DDS_ALPHA_MODE_STRAIGHT = 1,
        DDS_ALPHA_MODE_PREMULTIPLIED = 2,
        DDS_ALPHA_MODE_OPAQUE = 3,
        DDS_ALPHA_MODE_CUSTOM = 4,
    };
#endif

    inline namespace DX11
    {
        enum DDS_LOADER_FLAGS : uint32_t
        {
            DDS_LOADER_DEFAULT = 0,
            DDS_LOADER_FORCE_SRGB = 0x1,
            DDS_LOADER_IGNORE_SRGB = 0x2,
            DDS_LOADER_IGNORE_MIPS = 0x20,
        };
    }

    // Standard version
    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateDDSTextureFromMemory(
        _In_ ID3D11Device* d3dDevice,
        _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
        _In_ size_t ddsDataSize,
        _Outptr_opt_ ID3D11Resource** texture,
        _Outptr_opt_ ID3D11ShaderResourceView** textureView,
        _In_ size_t maxsize = 0,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr) noexcept;

    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateDDSTextureFromFile(
        _In_ ID3D11Device* d3dDevice,
        _In_z_ const wchar_t* szFileName,
        _Outptr_opt_ ID3D11Resource** texture,
        _Outptr_opt_ ID3D11ShaderResourceView** textureView,
        _In_ size_t maxsize = 0,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr) noexcept;

    // Standard version with optional auto-gen mipmap support
    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateDDSTextureFromMemory(
    #if defined(_XBOX_ONE) && defined(_TITLE)
        _In_ ID3D11DeviceX* d3dDevice,
        _In_opt_ ID3D11DeviceContextX* d3dContext,
    #else
        _In_ ID3D11Device* d3dDevice,
        _In_opt_ ID3D11DeviceContext* d3dContext,
    #endif
        _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
        _In_ size_t ddsDataSize,
        _Outptr_opt_ ID3D11Resource** texture,
        _Outptr_opt_ ID3D11ShaderResourceView** textureView,
        _In_ size_t maxsize = 0,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr) noexcept;

    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateDDSTextureFromFile(
    #if defined(_XBOX_ONE) && defined(_TITLE)
        _In_ ID3D11DeviceX* d3dDevice,
        _In_opt_ ID3D11DeviceContextX* d3dContext,
    #else
        _In_ ID3D11Device* d3dDevice,
        _In_opt_ ID3D11DeviceContext* d3dContext,
    #endif
        _In_z_ const wchar_t* szFileName,
        _Outptr_opt_ ID3D11Resource** texture,
        _Outptr_opt_ ID3D11ShaderResourceView** textureView,
        _In_ size_t maxsize = 0,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr) noexcept;

    // Extended version
    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateDDSTextureFromMemoryEx(
        _In_ ID3D11Device* d3dDevice,
        _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
        _In_ size_t ddsDataSize,
        _In_ size_t maxsize,
        _In_ D3D11_USAGE usage,
        _In_ unsigned int bindFlags,
        _In_ unsigned int cpuAccessFlags,
        _In_ unsigned int miscFlags,
        _In_ DDS_LOADER_FLAGS loadFlags,
        _Outptr_opt_ ID3D11Resource** texture,
        _Outptr_opt_ ID3D11ShaderResourceView** textureView,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr) noexcept;

    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateDDSTextureFromFileEx(
        _In_ ID3D11Device* d3dDevice,
        _In_z_ const wchar_t* szFileName,
        _In_ size_t maxsize,
        _In_ D3D11_USAGE usage,
        _In_ unsigned int bindFlags,
        _In_ unsigned int cpuAccessFlags,
        _In_ unsigned int miscFlags,
        _In_ DDS_LOADER_FLAGS loadFlags,
        _Outptr_opt_ ID3D11Resource** texture,
        _Outptr_opt_ ID3D11ShaderResourceView** textureView,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr) noexcept;

    // Extended version with optional auto-gen mipmap support
    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateDDSTextureFromMemoryEx(
    #if defined(_XBOX_ONE) && defined(_TITLE)
        _In_ ID3D11DeviceX* d3dDevice,
        _In_opt_ ID3D11DeviceContextX* d3dContext,
    #else
        _In_ ID3D11Device* d3dDevice,
        _In_opt_ ID3D11DeviceContext* d3dContext,
    #endif
        _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
        _In_ size_t ddsDataSize,
        _In_ size_t maxsize,
        _In_ D3D11_USAGE usage,
        _In_ unsigned int bindFlags,
        _In_ unsigned int cpuAccessFlags,
        _In_ unsigned int miscFlags,
        _In_ DDS_LOADER_FLAGS loadFlags,
        _Outptr_opt_ ID3D11Resource** texture,
        _Outptr_opt_ ID3D11ShaderResourceView** textureView,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr) noexcept;

    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateDDSTextureFromFileEx(
    #if defined(_XBOX_ONE) && defined(_TITLE)
        _In_ ID3D11DeviceX* d3dDevice,
        _In_opt_ ID3D11DeviceContextX* d3dContext,
    #else
        _In_ ID3D11Device* d3dDevice,
        _In_opt_ ID3D11DeviceContext* d3dContext,
    #endif
        _In_z_ const wchar_t* szFileName,
        _In_ size_t maxsize,
        _In_ D3D11_USAGE usage,
        _In_ unsigned int bindFlags,
        _In_ unsigned int cpuAccessFlags,
        _In_ unsigned int miscFlags,
        _In_ DDS_LOADER_FLAGS loadFlags,
        _Outptr_opt_ ID3D11Resource** texture,
        _Outptr_opt_ ID3D11ShaderResourceView** textureView,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr) noexcept;

#ifdef __cpp_lib_byte
    DIRECTX_TOOLKIT_API
    inline HRESULT __cdecl CreateDDSTextureFromMemory(
        _In_ ID3D11Device* d3dDevice,
        _In_reads_bytes_(ddsDataSize) const std::byte* ddsData,
        _In_ size_t ddsDataSize,
        _Outptr_opt_ ID3D11Resource** texture,
        _Outptr_opt_ ID3D11ShaderResourceView** textureView,
        _In_ size_t maxsize = 0,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr) noexcept
    {
        return CreateDDSTextureFromMemory(d3dDevice, reinterpret_cast<const uint8_t*>(ddsData), ddsDataSize, texture, textureView, maxsize, alphaMode);
    }

    DIRECTX_TOOLKIT_API
    inline HRESULT __cdecl CreateDDSTextureFromMemory(
    #if defined(_XBOX_ONE) && defined(_TITLE)
        _In_ ID3D11DeviceX* d3dDevice,
        _In_opt_ ID3D11DeviceContextX* d3dContext,
    #else
        _In_ ID3D11Device* d3dDevice,
        _In_opt_ ID3D11DeviceContext* d3dContext,
    #endif
        _In_reads_bytes_(ddsDataSize) const std::byte* ddsData,
        _In_ size_t ddsDataSize,
        _Outptr_opt_ ID3D11Resource** texture,
        _Outptr_opt_ ID3D11ShaderResourceView** textureView,
        _In_ size_t maxsize = 0,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr) noexcept
    {
        return CreateDDSTextureFromMemory(d3dDevice, d3dContext, reinterpret_cast<const uint8_t*>(ddsData), ddsDataSize, texture, textureView, maxsize, alphaMode);
    }

    DIRECTX_TOOLKIT_API
    inline HRESULT __cdecl CreateDDSTextureFromMemoryEx(
        _In_ ID3D11Device* d3dDevice,
        _In_reads_bytes_(ddsDataSize) const std::byte* ddsData,
        _In_ size_t ddsDataSize,
        _In_ size_t maxsize,
        _In_ D3D11_USAGE usage,
        _In_ unsigned int bindFlags,
        _In_ unsigned int cpuAccessFlags,
        _In_ unsigned int miscFlags,
        _In_ DDS_LOADER_FLAGS loadFlags,
        _Outptr_opt_ ID3D11Resource** texture,
        _Outptr_opt_ ID3D11ShaderResourceView** textureView,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr) noexcept
    {
        return CreateDDSTextureFromMemoryEx(d3dDevice, reinterpret_cast<const uint8_t*>(ddsData), ddsDataSize, maxsize, usage, bindFlags, cpuAccessFlags, miscFlags, loadFlags, texture, textureView, alphaMode);
    }

    DIRECTX_TOOLKIT_API
    inline HRESULT __cdecl CreateDDSTextureFromMemoryEx(
    #if defined(_XBOX_ONE) && defined(_TITLE)
        _In_ ID3D11DeviceX* d3dDevice,
        _In_opt_ ID3D11DeviceContextX* d3dContext,
    #else
        _In_ ID3D11Device* d3dDevice,
        _In_opt_ ID3D11DeviceContext* d3dContext,
    #endif
        _In_reads_bytes_(ddsDataSize) const std::byte* ddsData,
        _In_ size_t ddsDataSize,
        _In_ size_t maxsize,
        _In_ D3D11_USAGE usage,
        _In_ unsigned int bindFlags,
        _In_ unsigned int cpuAccessFlags,
        _In_ unsigned int miscFlags,
        _In_ DDS_LOADER_FLAGS loadFlags,
        _Outptr_opt_ ID3D11Resource** texture,
        _Outptr_opt_ ID3D11ShaderResourceView** textureView,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr) noexcept
    {
        return CreateDDSTextureFromMemoryEx(d3dDevice, d3dContext, reinterpret_cast<const uint8_t*>(ddsData), ddsDataSize, maxsize, usage, bindFlags, cpuAccessFlags, miscFlags, loadFlags, texture, textureView, alphaMode);
    }
#endif // __cpp_lib_byte

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
#endif

    inline namespace DX11
    {
        DEFINE_ENUM_FLAG_OPERATORS(DDS_LOADER_FLAGS)
    }

#ifdef __clang__
#pragma clang diagnostic pop
#endif
}
