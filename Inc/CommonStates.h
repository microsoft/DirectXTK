//--------------------------------------------------------------------------------------
// File: CommonStates.h
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE) && MONOLITHIC
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
#endif

#include <memory>


namespace DirectX
{
    class CommonStates
    {
    public:
        explicit CommonStates(_In_ ID3D11Device* device);
        CommonStates(CommonStates&& moveFrom);
        CommonStates& operator= (CommonStates&& moveFrom);
        virtual ~CommonStates();

        // Blend states.
        ID3D11BlendState* Opaque() const;
        ID3D11BlendState* AlphaBlend() const;
        ID3D11BlendState* Additive() const;
        ID3D11BlendState* NonPremultiplied() const;

        // Depth stencil states.
        ID3D11DepthStencilState* DepthNone() const;
        ID3D11DepthStencilState* DepthDefault() const;
        ID3D11DepthStencilState* DepthRead() const;

        // Rasterizer states.
        ID3D11RasterizerState* CullNone() const;
        ID3D11RasterizerState* CullClockwise() const;
        ID3D11RasterizerState* CullCounterClockwise() const;
        ID3D11RasterizerState* Wireframe() const;

        // Sampler states.
        ID3D11SamplerState* PointWrap() const;
        ID3D11SamplerState* PointClamp() const;
        ID3D11SamplerState* LinearWrap() const;
        ID3D11SamplerState* LinearClamp() const;
        ID3D11SamplerState* AnisotropicWrap() const;
        ID3D11SamplerState* AnisotropicClamp() const;

    private:
        // Private implementation.
        class Impl;

        std::shared_ptr<Impl> pImpl;

        // Prevent copying.
        CommonStates(CommonStates const&);
        CommonStates& operator= (CommonStates const&);
    };
}
