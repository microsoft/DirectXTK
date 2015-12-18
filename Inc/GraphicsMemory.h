//--------------------------------------------------------------------------------------
// File: GraphicsMemory.h
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

#include <memory>


namespace DirectX
{
    class GraphicsMemory
    {
    public:
        #if defined(_XBOX_ONE) && defined(_TITLE)
        GraphicsMemory(_In_ ID3D11DeviceX* device, UINT backBufferCount = 2);
        #else
        GraphicsMemory(_In_ ID3D11Device* device, UINT backBufferCount = 2);
        #endif
        GraphicsMemory(GraphicsMemory&& moveFrom);
        GraphicsMemory& operator= (GraphicsMemory&& moveFrom);
        virtual ~GraphicsMemory();

        void* __cdecl Allocate(_In_opt_ ID3D11DeviceContext* context, size_t size, int alignment);

        void __cdecl Commit();

        // Singleton
        static GraphicsMemory& __cdecl Get();

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Prevent copying.
        GraphicsMemory(GraphicsMemory const&) DIRECTX_CTOR_DELETE
        GraphicsMemory& operator=(GraphicsMemory const&) DIRECTX_CTOR_DELETE
    };
}
