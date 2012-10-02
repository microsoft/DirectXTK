//--------------------------------------------------------------------------------------
// File: GeometricPrimitive.h
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
#include <DirectXColors.h>
#include <functional>
#include <memory>


namespace DirectX
{
    class GeometricPrimitive
    {
    public:
        ~GeometricPrimitive();
        
        // Factory methods.
        static std::unique_ptr<GeometricPrimitive> CreateCube     (_In_ ID3D11DeviceContext* deviceContext, float size = 1);
        static std::unique_ptr<GeometricPrimitive> CreateSphere   (_In_ ID3D11DeviceContext* deviceContext, float diameter = 1, size_t tessellation = 16);
        static std::unique_ptr<GeometricPrimitive> CreateGeoSphere(_In_ ID3D11DeviceContext* deviceContext, float diameter = 1, size_t tessellation = 3);
        static std::unique_ptr<GeometricPrimitive> CreateCylinder (_In_ ID3D11DeviceContext* deviceContext, float height = 1, float diameter = 1, size_t tessellation = 32);
        static std::unique_ptr<GeometricPrimitive> CreateTorus    (_In_ ID3D11DeviceContext* deviceContext, float diameter = 1, float thickness = 0.333f, size_t tessellation = 32);
        static std::unique_ptr<GeometricPrimitive> CreateTeapot   (_In_ ID3D11DeviceContext* deviceContext, float size = 1, size_t tessellation = 8);

        // Draw the primitive.
        void Draw(CXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color = Colors::White, _In_opt_ ID3D11ShaderResourceView* texture = nullptr, bool wireframe = false, _In_opt_ std::function<void()> setCustomState = nullptr);

    private:
        GeometricPrimitive();

        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Prevent copying.
        GeometricPrimitive(GeometricPrimitive const&);
        GeometricPrimitive& operator= (GeometricPrimitive const&);
    };
}
