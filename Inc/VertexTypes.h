//--------------------------------------------------------------------------------------
// File: VertexTypes.h
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

#include <d3d11.h>
#include <DirectXMath.h>


namespace DirectX
{
    // Vertex struct holding position and color information.
    struct VertexPositionColor
    {
        VertexPositionColor()
        { }

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
        VertexPositionTexture()
        { }

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


    // Vertex struct holding position and normal vector.
    struct VertexPositionNormal
    {
        VertexPositionNormal()
        { }

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
        VertexPositionColorTexture()
        { }

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
        VertexPositionNormalColor()
        { }

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
        VertexPositionNormalTexture()
        { }

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
        VertexPositionNormalColorTexture()
        { }

        VertexPositionNormalColorTexture(XMFLOAT3 const& position, XMFLOAT3 const& normal, XMFLOAT4 const& color, XMFLOAT2 const& textureCoordinate)
          : position(position),
            normal(normal),
            color(color),
            textureCoordinate(textureCoordinate)
        { }

        VertexPositionNormalColorTexture(FXMVECTOR position, FXMVECTOR normal, FXMVECTOR color, GXMVECTOR textureCoordinate)
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
}
