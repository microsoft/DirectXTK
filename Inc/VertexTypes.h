//--------------------------------------------------------------------------------------
// File: VertexTypes.h
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

#include <DirectXMath.h>


namespace DirectX
{
    // Vertex struct holding position information.
    struct VertexPosition
    {
        VertexPosition() = default;

        VertexPosition(const VertexPosition&) = default;
        VertexPosition& operator=(const VertexPosition&) = default;

    #if !defined(_MSC_VER) || _MSC_VER >= 1900
        VertexPosition(VertexPosition&&) = default;
        VertexPosition& operator=(VertexPosition&&) = default;
    #endif

        VertexPosition(XMFLOAT3 const& position)
            : position(position)
        { }

        VertexPosition(FXMVECTOR position)
        {
            XMStoreFloat3(&this->position, position);
        }

        XMFLOAT3 position;

        static const int InputElementCount = 1;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };


    // Vertex struct holding position and color information.
    struct VertexPositionColor
    {
        VertexPositionColor() = default;

        VertexPositionColor(const VertexPositionColor&) = default;
        VertexPositionColor& operator=(const VertexPositionColor&) = default;

#if !defined(_MSC_VER) || _MSC_VER >= 1900
        VertexPositionColor(VertexPositionColor&&) = default;
        VertexPositionColor& operator=(VertexPositionColor&&) = default;
#endif

        VertexPositionColor(XMFLOAT3 const& position, XMFLOAT4 const& color)
            : position(position),
            color(color)
        { }

        VertexPositionColor(FXMVECTOR position, FXMVECTOR color)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat4(&this->color, color);
        }

        XMFLOAT3 position;
        XMFLOAT4 color;

        static const int InputElementCount = 2;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };


    // Vertex struct holding position and texture mapping information.
    struct VertexPositionTexture
    {
        VertexPositionTexture() = default;

        VertexPositionTexture(const VertexPositionTexture&) = default;
        VertexPositionTexture& operator=(const VertexPositionTexture&) = default;

#if !defined(_MSC_VER) || _MSC_VER >= 1900
        VertexPositionTexture(VertexPositionTexture&&) = default;
        VertexPositionTexture& operator=(VertexPositionTexture&&) = default;
#endif

        VertexPositionTexture(XMFLOAT3 const& position, XMFLOAT2 const& textureCoordinate)
            : position(position),
            textureCoordinate(textureCoordinate)
        { }

        VertexPositionTexture(FXMVECTOR position, FXMVECTOR textureCoordinate)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
        }

        XMFLOAT3 position;
        XMFLOAT2 textureCoordinate;

        static const int InputElementCount = 2;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };


    // Vertex struct holding position and dual texture mapping information.
    struct VertexPositionDualTexture
    {
        VertexPositionDualTexture() = default;

        VertexPositionDualTexture(const VertexPositionDualTexture&) = default;
        VertexPositionDualTexture& operator=(const VertexPositionDualTexture&) = default;

#if !defined(_MSC_VER) || _MSC_VER >= 1900
        VertexPositionDualTexture(VertexPositionDualTexture&&) = default;
        VertexPositionDualTexture& operator=(VertexPositionDualTexture&&) = default;
#endif

        VertexPositionDualTexture(XMFLOAT3 const& position, XMFLOAT2 const& textureCoordinate0, XMFLOAT2 const& textureCoordinate1)
            : position(position),
            textureCoordinate0(textureCoordinate0),
            textureCoordinate1(textureCoordinate1)
        { }

        VertexPositionDualTexture(FXMVECTOR position,
                                  FXMVECTOR textureCoordinate0,
                                  FXMVECTOR textureCoordinate1)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat2(&this->textureCoordinate0, textureCoordinate0);
            XMStoreFloat2(&this->textureCoordinate1, textureCoordinate1);
        }

        XMFLOAT3 position;
        XMFLOAT2 textureCoordinate0;
        XMFLOAT2 textureCoordinate1;

        static const int InputElementCount = 3;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };


    // Vertex struct holding position and normal vector.
    struct VertexPositionNormal
    {
        VertexPositionNormal() = default;

        VertexPositionNormal(const VertexPositionNormal&) = default;
        VertexPositionNormal& operator=(const VertexPositionNormal&) = default;

#if !defined(_MSC_VER) || _MSC_VER >= 1900
        VertexPositionNormal(VertexPositionNormal&&) = default;
        VertexPositionNormal& operator=(VertexPositionNormal&&) = default;
#endif

        VertexPositionNormal(XMFLOAT3 const& position, XMFLOAT3 const& normal)
            : position(position),
            normal(normal)
        { }

        VertexPositionNormal(FXMVECTOR position, FXMVECTOR normal)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat3(&this->normal, normal);
        }

        XMFLOAT3 position;
        XMFLOAT3 normal;

        static const int InputElementCount = 2;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };


    // Vertex struct holding position, color, and texture mapping information.
    struct VertexPositionColorTexture
    {
        VertexPositionColorTexture() = default;

        VertexPositionColorTexture(const VertexPositionColorTexture&) = default;
        VertexPositionColorTexture& operator=(const VertexPositionColorTexture&) = default;

#if !defined(_MSC_VER) || _MSC_VER >= 1900
        VertexPositionColorTexture(VertexPositionColorTexture&&) = default;
        VertexPositionColorTexture& operator=(VertexPositionColorTexture&&) = default;
#endif

        VertexPositionColorTexture(XMFLOAT3 const& position, XMFLOAT4 const& color, XMFLOAT2 const& textureCoordinate)
            : position(position),
            color(color),
            textureCoordinate(textureCoordinate)
        { }

        VertexPositionColorTexture(FXMVECTOR position, FXMVECTOR color, FXMVECTOR textureCoordinate)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat4(&this->color, color);
            XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
        }

        XMFLOAT3 position;
        XMFLOAT4 color;
        XMFLOAT2 textureCoordinate;

        static const int InputElementCount = 3;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };


    // Vertex struct holding position, normal vector, and color information.
    struct VertexPositionNormalColor
    {
        VertexPositionNormalColor() = default;

        VertexPositionNormalColor(const VertexPositionNormalColor&) = default;
        VertexPositionNormalColor& operator=(const VertexPositionNormalColor&) = default;

#if !defined(_MSC_VER) || _MSC_VER >= 1900
        VertexPositionNormalColor(VertexPositionNormalColor&&) = default;
        VertexPositionNormalColor& operator=(VertexPositionNormalColor&&) = default;
#endif

        VertexPositionNormalColor(XMFLOAT3 const& position, XMFLOAT3 const& normal, XMFLOAT4 const& color)
            : position(position),
            normal(normal),
            color(color)
        { }

        VertexPositionNormalColor(FXMVECTOR position, FXMVECTOR normal, FXMVECTOR color)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat3(&this->normal, normal);
            XMStoreFloat4(&this->color, color);
        }

        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT4 color;

        static const int InputElementCount = 3;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };


    // Vertex struct holding position, normal vector, and texture mapping information.
    struct VertexPositionNormalTexture
    {
        VertexPositionNormalTexture() = default;

        VertexPositionNormalTexture(const VertexPositionNormalTexture&) = default;
        VertexPositionNormalTexture& operator=(const VertexPositionNormalTexture&) = default;

#if !defined(_MSC_VER) || _MSC_VER >= 1900
        VertexPositionNormalTexture(VertexPositionNormalTexture&&) = default;
        VertexPositionNormalTexture& operator=(VertexPositionNormalTexture&&) = default;
#endif

        VertexPositionNormalTexture(XMFLOAT3 const& position, XMFLOAT3 const& normal, XMFLOAT2 const& textureCoordinate)
            : position(position),
            normal(normal),
            textureCoordinate(textureCoordinate)
        { }

        VertexPositionNormalTexture(FXMVECTOR position, FXMVECTOR normal, FXMVECTOR textureCoordinate)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat3(&this->normal, normal);
            XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
        }

        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT2 textureCoordinate;

        static const int InputElementCount = 3;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };


    // Vertex struct holding position, normal vector, color, and texture mapping information.
    struct VertexPositionNormalColorTexture
    {
        VertexPositionNormalColorTexture() = default;

        VertexPositionNormalColorTexture(const VertexPositionNormalColorTexture&) = default;
        VertexPositionNormalColorTexture& operator=(const VertexPositionNormalColorTexture&) = default;

#if !defined(_MSC_VER) || _MSC_VER >= 1900
        VertexPositionNormalColorTexture(VertexPositionNormalColorTexture&&) = default;
        VertexPositionNormalColorTexture& operator=(VertexPositionNormalColorTexture&&) = default;
#endif

        VertexPositionNormalColorTexture(XMFLOAT3 const& position, XMFLOAT3 const& normal, XMFLOAT4 const& color, XMFLOAT2 const& textureCoordinate)
            : position(position),
            normal(normal),
            color(color),
            textureCoordinate(textureCoordinate)
        { }

        VertexPositionNormalColorTexture(FXMVECTOR position, FXMVECTOR normal, FXMVECTOR color, CXMVECTOR textureCoordinate)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat3(&this->normal, normal);
            XMStoreFloat4(&this->color, color);
            XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
        }

        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT4 color;
        XMFLOAT2 textureCoordinate;

        static const int InputElementCount = 4;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };


    // Vertex struct for Visual Studio Shader Designer (DGSL) holding position, normal,
    // tangent, color (RGBA), and texture mapping information
    struct VertexPositionNormalTangentColorTexture
    {
        VertexPositionNormalTangentColorTexture() = default;

        VertexPositionNormalTangentColorTexture(const VertexPositionNormalTangentColorTexture&) = default;
        VertexPositionNormalTangentColorTexture& operator=(const VertexPositionNormalTangentColorTexture&) = default;

#if !defined(_MSC_VER) || _MSC_VER >= 1900
        VertexPositionNormalTangentColorTexture(VertexPositionNormalTangentColorTexture&&) = default;
        VertexPositionNormalTangentColorTexture& operator=(VertexPositionNormalTangentColorTexture&&) = default;
#endif

        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT4 tangent;
        uint32_t color;
        XMFLOAT2 textureCoordinate;

        VertexPositionNormalTangentColorTexture(XMFLOAT3 const& position, XMFLOAT3 const& normal, XMFLOAT4 const& tangent, uint32_t rgba, XMFLOAT2 const& textureCoordinate)
            : position(position),
            normal(normal),
            tangent(tangent),
            color(rgba),
            textureCoordinate(textureCoordinate)
        {
        }

        VertexPositionNormalTangentColorTexture(FXMVECTOR position, FXMVECTOR normal, FXMVECTOR tangent, uint32_t rgba, CXMVECTOR textureCoordinate)
            : color(rgba)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat3(&this->normal, normal);
            XMStoreFloat4(&this->tangent, tangent);
            XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
        }

        VertexPositionNormalTangentColorTexture(XMFLOAT3 const& position, XMFLOAT3 const& normal, XMFLOAT4 const& tangent, XMFLOAT4 const& color, XMFLOAT2 const& textureCoordinate)
            : position(position),
            normal(normal),
            tangent(tangent),
            textureCoordinate(textureCoordinate)
        {
            SetColor(color);
        }

        VertexPositionNormalTangentColorTexture(FXMVECTOR position, FXMVECTOR normal, FXMVECTOR tangent, CXMVECTOR color, CXMVECTOR textureCoordinate)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat3(&this->normal, normal);
            XMStoreFloat4(&this->tangent, tangent);
            XMStoreFloat2(&this->textureCoordinate, textureCoordinate);

            SetColor(color);
        }

        void __cdecl SetColor(XMFLOAT4 const& icolor) { SetColor(XMLoadFloat4(&icolor)); }
        void XM_CALLCONV SetColor(FXMVECTOR icolor);

        static const int InputElementCount = 5;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };


    // Vertex struct for Visual Studio Shader Designer (DGSL) holding position, normal,
    // tangent, color (RGBA), texture mapping information, and skinning weights
    struct VertexPositionNormalTangentColorTextureSkinning : public VertexPositionNormalTangentColorTexture
    {
        VertexPositionNormalTangentColorTextureSkinning() = default;

        VertexPositionNormalTangentColorTextureSkinning(const VertexPositionNormalTangentColorTextureSkinning&) = default;
        VertexPositionNormalTangentColorTextureSkinning& operator=(const VertexPositionNormalTangentColorTextureSkinning&) = default;

#if !defined(_MSC_VER) || _MSC_VER >= 1900
        VertexPositionNormalTangentColorTextureSkinning(VertexPositionNormalTangentColorTextureSkinning&&) = default;
        VertexPositionNormalTangentColorTextureSkinning& operator=(VertexPositionNormalTangentColorTextureSkinning&&) = default;
#endif

        uint32_t indices;
        uint32_t weights;

        VertexPositionNormalTangentColorTextureSkinning(XMFLOAT3 const& position, XMFLOAT3 const& normal, XMFLOAT4 const& tangent, uint32_t rgba,
                                                        XMFLOAT2 const& textureCoordinate, XMUINT4 const& indices, XMFLOAT4 const& weights)
            : VertexPositionNormalTangentColorTexture(position, normal, tangent, rgba, textureCoordinate)
        {
            SetBlendIndices(indices);
            SetBlendWeights(weights);
        }

        VertexPositionNormalTangentColorTextureSkinning(FXMVECTOR position, FXMVECTOR normal, FXMVECTOR tangent, uint32_t rgba, CXMVECTOR textureCoordinate,
                                                        XMUINT4 const& indices, CXMVECTOR weights)
            : VertexPositionNormalTangentColorTexture(position, normal, tangent, rgba, textureCoordinate)
        {
            SetBlendIndices(indices);
            SetBlendWeights(weights);
        }

        VertexPositionNormalTangentColorTextureSkinning(XMFLOAT3 const& position, XMFLOAT3 const& normal, XMFLOAT4 const& tangent, XMFLOAT4 const& color,
                                                        XMFLOAT2 const& textureCoordinate, XMUINT4 const& indices, XMFLOAT4 const& weights)
            : VertexPositionNormalTangentColorTexture(position, normal, tangent, color, textureCoordinate)
        {
            SetBlendIndices(indices);
            SetBlendWeights(weights);
        }

        VertexPositionNormalTangentColorTextureSkinning(FXMVECTOR position, FXMVECTOR normal, FXMVECTOR tangent, CXMVECTOR color, CXMVECTOR textureCoordinate,
                                                        XMUINT4 const& indices, CXMVECTOR weights)
            : VertexPositionNormalTangentColorTexture(position, normal, tangent, color, textureCoordinate)
        {
            SetBlendIndices(indices);
            SetBlendWeights(weights);
        }

        void __cdecl SetBlendIndices(XMUINT4 const& iindices);

        void __cdecl SetBlendWeights(XMFLOAT4 const& iweights) { SetBlendWeights(XMLoadFloat4(&iweights)); }
        void XM_CALLCONV SetBlendWeights(FXMVECTOR iweights);

        static const int InputElementCount = 7;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };
}
