//--------------------------------------------------------------------------------------
// File: DirectXHelpers.h
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
#endif

#if !defined(NO_D3D11_DEBUG_NAME) && ( defined(_DEBUG) || defined(PROFILE) )
#if !defined(_XBOX_ONE) || !defined(_TITLE)
#pragma comment(lib,"dxguid.lib")
#endif
#endif

#ifndef IID_GRAPHICS_PPV_ARGS
#define IID_GRAPHICS_PPV_ARGS(x) IID_PPV_ARGS(x)
#endif

#include <exception>

#include <assert.h>
#include <stdint.h>

//
// The core Direct3D headers provide the following helper C++ classes
//  CD3D11_RECT
//  CD3D11_BOX
//  CD3D11_DEPTH_STENCIL_DESC
//  CD3D11_BLEND_DESC, CD3D11_BLEND_DESC1
//  CD3D11_RASTERIZER_DESC, CD3D11_RASTERIZER_DESC1
//  CD3D11_BUFFER_DESC
//  CD3D11_TEXTURE1D_DESC
//  CD3D11_TEXTURE2D_DESC
//  CD3D11_TEXTURE3D_DESC
//  CD3D11_SHADER_RESOURCE_VIEW_DESC
//  CD3D11_RENDER_TARGET_VIEW_DESC
//  CD3D11_VIEWPORT
//  CD3D11_DEPTH_STENCIL_VIEW_DESC
//  CD3D11_UNORDERED_ACCESS_VIEW_DESC
//  CD3D11_SAMPLER_DESC
//  CD3D11_QUERY_DESC
//  CD3D11_COUNTER_DESC
//


namespace DirectX
{
    // simliar to std::lock_guard for exception-safe Direct3D resource locking
    class MapGuard : public D3D11_MAPPED_SUBRESOURCE
    {
    public:
        MapGuard(_In_ ID3D11DeviceContext* context,
                 _In_ ID3D11Resource *resource,
                 _In_ UINT subresource,
                 _In_ D3D11_MAP mapType,
                 _In_ UINT mapFlags) noexcept(false)
            : mContext(context), mResource(resource), mSubresource(subresource)
        {
            HRESULT hr = mContext->Map(resource, subresource, mapType, mapFlags, this);
            if (FAILED(hr))
            {
                throw std::exception();
            }
        }

        ~MapGuard()
        {
            mContext->Unmap(mResource, mSubresource);
        }

        uint8_t* get() const noexcept
        {
            return static_cast<uint8_t*>(pData);
        }
        uint8_t* get(size_t slice) const noexcept
        {
            return static_cast<uint8_t*>(pData) + (slice * DepthPitch);
        }

        uint8_t* scanline(size_t row) const noexcept
        {
            return static_cast<uint8_t*>(pData) + (row * RowPitch);
        }
        uint8_t* scanline(size_t slice, size_t row) const noexcept
        {
            return static_cast<uint8_t*>(pData) + (slice * DepthPitch) + (row * RowPitch);
        }

    private:
        ID3D11DeviceContext*    mContext;
        ID3D11Resource*         mResource;
        UINT                    mSubresource;

        MapGuard(MapGuard const&);
        MapGuard& operator= (MapGuard const&);
    };


    // Helper sets a D3D resource name string (used by PIX and debug layer leak reporting).
    template<UINT TNameLength>
    inline void SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_z_ const char (&name)[TNameLength]) noexcept
    {
        #if !defined(NO_D3D11_DEBUG_NAME) && ( defined(_DEBUG) || defined(PROFILE) )
            #if defined(_XBOX_ONE) && defined(_TITLE)
                wchar_t wname[MAX_PATH];
                int result = MultiByteToWideChar(CP_UTF8, 0, name, TNameLength, wname, MAX_PATH);
                if (result > 0)
                {
                    resource->SetName(wname);
                }
            #else
                resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
            #endif
        #else
            UNREFERENCED_PARAMETER(resource);
            UNREFERENCED_PARAMETER(name);
        #endif
    }

    template<UINT TNameLength>
    inline void SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_z_ const wchar_t (&name)[TNameLength])
    {
        #if !defined(NO_D3D11_DEBUG_NAME) && ( defined(_DEBUG) || defined(PROFILE) )
            #if defined(_XBOX_ONE) && defined(_TITLE)
                resource->SetName( name );
            #else
                char aname[MAX_PATH];
                int result = WideCharToMultiByte(CP_UTF8, 0, name, TNameLength, aname, MAX_PATH, nullptr, nullptr);
                if (result > 0)
                {
                    resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, aname);
                }
            #endif
        #else
            UNREFERENCED_PARAMETER(resource);
            UNREFERENCED_PARAMETER(name);
        #endif
    }

    // Helpers for aligning values by a power of 2
    template<typename T>
    inline T AlignDown(T size, size_t alignment) noexcept
    {
        if (alignment > 0)
        {
            assert(((alignment - 1) & alignment) == 0);
            T mask = static_cast<T>(alignment - 1);
            return size & ~mask;
        }
        return size;
    }

    template<typename T>
    inline T AlignUp(T size, size_t alignment) noexcept
    {
        if (alignment > 0)
        {
            assert(((alignment - 1) & alignment) == 0);
            T mask = static_cast<T>(alignment - 1);
            return (size + mask) & ~mask;
        }
        return size;
    }
}
