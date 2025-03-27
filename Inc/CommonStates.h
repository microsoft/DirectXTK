//--------------------------------------------------------------------------------------
// File: CommonStates.h
//
// Copyright (c) Microsoft Corporation.
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

#include <memory>

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
    inline namespace DX11
    {
        class CommonStates
        {
        public:
            DIRECTX_TOOLKIT_API explicit CommonStates(_In_ ID3D11Device* device);

            DIRECTX_TOOLKIT_API CommonStates(CommonStates&&) noexcept;
            DIRECTX_TOOLKIT_API CommonStates& operator= (CommonStates&&) noexcept;

            CommonStates(CommonStates const&) = delete;
            CommonStates& operator= (CommonStates const&) = delete;

            DIRECTX_TOOLKIT_API virtual ~CommonStates();

            // Blend states.
            DIRECTX_TOOLKIT_API ID3D11BlendState* __cdecl Opaque() const;
            DIRECTX_TOOLKIT_API ID3D11BlendState* __cdecl AlphaBlend() const;
            DIRECTX_TOOLKIT_API ID3D11BlendState* __cdecl Additive() const;
            DIRECTX_TOOLKIT_API ID3D11BlendState* __cdecl NonPremultiplied() const;

            // Depth stencil states.
            DIRECTX_TOOLKIT_API ID3D11DepthStencilState* __cdecl DepthNone() const;
            DIRECTX_TOOLKIT_API ID3D11DepthStencilState* __cdecl DepthDefault() const;
            DIRECTX_TOOLKIT_API ID3D11DepthStencilState* __cdecl DepthRead() const;
            DIRECTX_TOOLKIT_API ID3D11DepthStencilState* __cdecl DepthReverseZ() const;
            DIRECTX_TOOLKIT_API ID3D11DepthStencilState* __cdecl DepthReadReverseZ() const;

            // Rasterizer states.
            DIRECTX_TOOLKIT_API ID3D11RasterizerState* __cdecl CullNone() const;
            DIRECTX_TOOLKIT_API ID3D11RasterizerState* __cdecl CullClockwise() const;
            DIRECTX_TOOLKIT_API ID3D11RasterizerState* __cdecl CullCounterClockwise() const;
            DIRECTX_TOOLKIT_API ID3D11RasterizerState* __cdecl Wireframe() const;

            // Sampler states.
            DIRECTX_TOOLKIT_API ID3D11SamplerState* __cdecl PointWrap() const;
            DIRECTX_TOOLKIT_API ID3D11SamplerState* __cdecl PointClamp() const;
            DIRECTX_TOOLKIT_API ID3D11SamplerState* __cdecl LinearWrap() const;
            DIRECTX_TOOLKIT_API ID3D11SamplerState* __cdecl LinearClamp() const;
            DIRECTX_TOOLKIT_API ID3D11SamplerState* __cdecl AnisotropicWrap() const;
            DIRECTX_TOOLKIT_API ID3D11SamplerState* __cdecl AnisotropicClamp() const;

        private:
            // Private implementation.
            class Impl;

            std::shared_ptr<Impl> pImpl;
        };
    }
}
