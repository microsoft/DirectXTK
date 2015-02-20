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

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
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


namespace DirectX
{
    #if (DIRECTX_MATH_VERSION < 305) && !defined(XM_CALLCONV)
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
        static std::unique_ptr<GeometricPrimitive> __cdecl CreateCube         (_In_ ID3D11DeviceContext* deviceContext, float size = 1, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> __cdecl CreateSphere       (_In_ ID3D11DeviceContext* deviceContext, float diameter = 1, size_t tessellation = 16, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> __cdecl CreateGeoSphere    (_In_ ID3D11DeviceContext* deviceContext, float diameter = 1, size_t tessellation = 3, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> __cdecl CreateCylinder     (_In_ ID3D11DeviceContext* deviceContext, float height = 1, float diameter = 1, size_t tessellation = 32, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> __cdecl CreateCone         (_In_ ID3D11DeviceContext* deviceContext, float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> __cdecl CreateTorus        (_In_ ID3D11DeviceContext* deviceContext, float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> __cdecl CreateTetrahedron  (_In_ ID3D11DeviceContext* deviceContext, float size = 1, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> __cdecl CreateOctahedron   (_In_ ID3D11DeviceContext* deviceContext, float size = 1, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> __cdecl CreateDodecahedron (_In_ ID3D11DeviceContext* deviceContext, float size = 1, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> __cdecl CreateIcosahedron  (_In_ ID3D11DeviceContext* deviceContext, float size = 1, bool rhcoords = true);
        static std::unique_ptr<GeometricPrimitive> __cdecl CreateTeapot       (_In_ ID3D11DeviceContext* deviceContext, float size = 1, size_t tessellation = 8, bool rhcoords = true);

        // Draw the primitive.
        void XM_CALLCONV Draw(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color = Colors::White, _In_opt_ ID3D11ShaderResourceView* texture = nullptr, bool wireframe = false,
                              _In_opt_ std::function<void DIRECTX_STD_CALLCONV()> setCustomState = nullptr );

        // Draw the primitive using a custom effect.
        void __cdecl Draw( _In_ IEffect* effect, _In_ ID3D11InputLayout* inputLayout, bool alpha = false, bool wireframe = false,
                           _In_opt_ std::function<void DIRECTX_STD_CALLCONV()> setCustomState = nullptr );

        // Create input layout for drawing with a custom effect.
        void __cdecl CreateInputLayout( _In_ IEffect* effect, _Outptr_ ID3D11InputLayout** inputLayout );
        
    private:
        GeometricPrimitive();

        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Prevent copying.
        GeometricPrimitive(GeometricPrimitive const&) DIRECTX_CTOR_DELETE
        GeometricPrimitive& operator= (GeometricPrimitive const&) DIRECTX_CTOR_DELETE
    };
}
