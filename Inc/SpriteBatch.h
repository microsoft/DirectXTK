//--------------------------------------------------------------------------------------
// File: SpriteBatch.h
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

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
#endif

// VS 2010/2012 do not support =default =delete
#ifndef DIRECTX_CTOR_DEFAULT
#if defined(_MSC_VER) && (_MSC_VER < 1800)
#define DIRECTX_CTOR_DEFAULT {}
#define DIRECTX_CTOR_DELETE ;
#else
#define DIRECTX_CTOR_DEFAULT =default;
#define DIRECTX_CTOR_DELETE =delete;
#endif
#endif

#include <DirectXMath.h>
#include <DirectXColors.h>
#include <functional>
#include <memory>

// VS 2010 doesn't support explicit calling convention for std::function
#ifndef DIRECTX_STD_CALLCONV
#if defined(_MSC_VER) && (_MSC_VER < 1700)
#define DIRECTX_STD_CALLCONV
#else
#define DIRECTX_STD_CALLCONV __cdecl
#endif
#endif

namespace DirectX
{
    #if (DIRECTX_MATH_VERSION < 305) && !defined(XM_CALLCONV)
    #define XM_CALLCONV __fastcall
    typedef const XMVECTOR& HXMVECTOR;
    typedef const XMMATRIX& FXMMATRIX;
    #endif

    enum SpriteSortMode
    {
        SpriteSortMode_Deferred,
        SpriteSortMode_Immediate,
        SpriteSortMode_Texture,
        SpriteSortMode_BackToFront,
        SpriteSortMode_FrontToBack,
    };
    
    
    enum SpriteEffects
    {
        SpriteEffects_None = 0,
        SpriteEffects_FlipHorizontally = 1,
        SpriteEffects_FlipVertically = 2,
        SpriteEffects_FlipBoth = SpriteEffects_FlipHorizontally | SpriteEffects_FlipVertically,
    };

    
    class SpriteBatch
    {
    public:
        explicit SpriteBatch(_In_ ID3D11DeviceContext* deviceContext);
        SpriteBatch(SpriteBatch&& moveFrom);
        SpriteBatch& operator= (SpriteBatch&& moveFrom);
        virtual ~SpriteBatch();

        // Begin/End a batch of sprite drawing operations.
        void XM_CALLCONV Begin(SpriteSortMode sortMode = SpriteSortMode_Deferred, _In_opt_ ID3D11BlendState* blendState = nullptr, _In_opt_ ID3D11SamplerState* samplerState = nullptr, _In_opt_ ID3D11DepthStencilState* depthStencilState = nullptr, _In_opt_ ID3D11RasterizerState* rasterizerState = nullptr,
                               _In_opt_ std::function<void DIRECTX_STD_CALLCONV()> setCustomShaders = nullptr, FXMMATRIX transformMatrix = MatrixIdentity);
        void __cdecl End();

        // Draw overloads specifying position, origin and scale as XMFLOAT2.
        void XM_CALLCONV Draw(_In_ ID3D11ShaderResourceView* texture, XMFLOAT2 const& position, FXMVECTOR color = Colors::White);
        void XM_CALLCONV Draw(_In_ ID3D11ShaderResourceView* texture, XMFLOAT2 const& position, _In_opt_ RECT const* sourceRectangle, FXMVECTOR color = Colors::White, float rotation = 0, XMFLOAT2 const& origin = Float2Zero, float scale = 1, SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);
        void XM_CALLCONV Draw(_In_ ID3D11ShaderResourceView* texture, XMFLOAT2 const& position, _In_opt_ RECT const* sourceRectangle, FXMVECTOR color, float rotation, XMFLOAT2 const& origin, XMFLOAT2 const& scale, SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);

        // Draw overloads specifying position, origin and scale via the first two components of an XMVECTOR.
        void XM_CALLCONV Draw(_In_ ID3D11ShaderResourceView* texture, FXMVECTOR position, FXMVECTOR color = Colors::White);
        void XM_CALLCONV Draw(_In_ ID3D11ShaderResourceView* texture, FXMVECTOR position, _In_opt_ RECT const* sourceRectangle, FXMVECTOR color = Colors::White, float rotation = 0, FXMVECTOR origin = g_XMZero, float scale = 1, SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);
        void XM_CALLCONV Draw(_In_ ID3D11ShaderResourceView* texture, FXMVECTOR position, _In_opt_ RECT const* sourceRectangle, FXMVECTOR color, float rotation, FXMVECTOR origin, GXMVECTOR scale, SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);

        // Draw overloads specifying position as a RECT.
        void XM_CALLCONV Draw(_In_ ID3D11ShaderResourceView* texture, RECT const& destinationRectangle, FXMVECTOR color = Colors::White);
        void XM_CALLCONV Draw(_In_ ID3D11ShaderResourceView* texture, RECT const& destinationRectangle, _In_opt_ RECT const* sourceRectangle, FXMVECTOR color = Colors::White, float rotation = 0, XMFLOAT2 const& origin = Float2Zero, SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);

        // Rotation mode to be applied to the sprite transformation
        void __cdecl SetRotation( DXGI_MODE_ROTATION mode );
        DXGI_MODE_ROTATION __cdecl GetRotation() const;

        // Set viewport for sprite transformation
        void __cdecl SetViewport( const D3D11_VIEWPORT& viewPort );

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        static const XMMATRIX MatrixIdentity;
        static const XMFLOAT2 Float2Zero;

        // Prevent copying.
        SpriteBatch(SpriteBatch const&) DIRECTX_CTOR_DELETE
        SpriteBatch& operator= (SpriteBatch const&) DIRECTX_CTOR_DELETE
    };
}
