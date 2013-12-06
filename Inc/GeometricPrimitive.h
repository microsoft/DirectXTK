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

#if defined(_XBOX_ONE) && defined(_TITLE) && MONOLITHIC
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
#endif

#include <DirectXMath.h>
#include <DirectXColors.h>
#include <functional>
#include <memory>


namespace DirectX
{
    #if (DIRECTXMATH_VERSION < 305) && !defined(XM_CALLCONV)
    #define XM_CALLCONV __fastcall
    typedef const XMVECTOR& HXMVECTOR;
    typedef const XMMATRIX& FXMMATRIX;
    #endif

    class IEffect;

    class GeometricPrimitive
    {
    public:
        ~GeometricPrimitive();
        
        // Factory methods.
        static std::unique_ptr<GeometricPrimitive> CreateCube         (_In_ ID3D11DeviceContext* deviceContext, float size = 1, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> CreateSphere       (_In_ ID3D11DeviceContext* deviceContext, float diameter = 1, size_t tessellation = 16, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> CreateGeoSphere    (_In_ ID3D11DeviceContext* deviceContext, float diameter = 1, size_t tessellation = 3, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> CreateCylinder     (_In_ ID3D11DeviceContext* deviceContext, float height = 1, float diameter = 1, size_t tessellation = 32, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> CreateCone         (_In_ ID3D11DeviceContext* deviceContext, float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> CreateTorus        (_In_ ID3D11DeviceContext* deviceContext, float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> CreateTetrahedron  (_In_ ID3D11DeviceContext* deviceContext, float size = 1, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> CreateOctahedron   (_In_ ID3D11DeviceContext* deviceContext, float size = 1, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> CreateDodecahedron (_In_ ID3D11DeviceContext* deviceContext, float size = 1, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> CreateIcosahedron  (_In_ ID3D11DeviceContext* deviceContext, float size = 1, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> CreateTeapot       (_In_ ID3D11DeviceContext* deviceContext, float size = 1, size_t tessellation = 8, bool rhcoords = true);

        // Draw the primitive.
        void XM_CALLCONV Draw(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color = Colors::White, _In_opt_ ID3D11ShaderResourceView* texture = nullptr, bool wireframe = false, _In_opt_ std::function<void()> setCustomState = nullptr);

        // Draw the primitive using a custom effect.
        void Draw( _In_ IEffect* effect, _In_ ID3D11InputLayout* inputLayout, bool alpha = false, bool wireframe = false, _In_opt_ std::function<void()> setCustomState = nullptr );

        // Create input layout for drawing with a custom effect.
        void CreateInputLayout( _In_ IEffect* effect, _Outptr_ ID3D11InputLayout** inputLayout );
        
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
