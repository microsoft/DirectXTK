//--------------------------------------------------------------------------------------
// File: PrimitiveBatch.h
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

#include <memory.h>
#include <memory>

#pragma warning(push)
#pragma warning(disable: 4005)
#include <stdint.h>
#pragma warning(pop)


namespace DirectX
{
    namespace Internal
    {
        // Base class, not to be used directly: clients should access this via the derived PrimitiveBatch<T>.
        class PrimitiveBatchBase
        {
        protected:
            PrimitiveBatchBase(_In_ ID3D11DeviceContext* deviceContext, size_t maxIndices, size_t maxVertices, size_t vertexSize);
            PrimitiveBatchBase(PrimitiveBatchBase&& moveFrom);
            PrimitiveBatchBase& operator= (PrimitiveBatchBase&& moveFrom);
            virtual ~PrimitiveBatchBase();

        public:
            // Begin/End a batch of primitive drawing operations.
            void __cdecl Begin();
            void __cdecl End();

        protected:
            // Internal, untyped drawing method.
            void __cdecl Draw(D3D11_PRIMITIVE_TOPOLOGY topology, bool isIndexed, _In_opt_count_(indexCount) uint16_t const* indices, size_t indexCount, size_t vertexCount, _Out_ void** pMappedVertices);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;

            // Prevent copying.
            PrimitiveBatchBase(PrimitiveBatchBase const&) DIRECTX_CTOR_DELETE
            PrimitiveBatchBase& operator= (PrimitiveBatchBase const&) DIRECTX_CTOR_DELETE
        };
    }


    // Template makes the API typesafe, eg. PrimitiveBatch<VertexPositionColor>.
    template<typename TVertex>
    class PrimitiveBatch : public Internal::PrimitiveBatchBase
    {
        static const size_t DefaultBatchSize = 2048;

    public:
        PrimitiveBatch(_In_ ID3D11DeviceContext* deviceContext, size_t maxIndices = DefaultBatchSize * 3, size_t maxVertices = DefaultBatchSize)
          : PrimitiveBatchBase(deviceContext, maxIndices, maxVertices, sizeof(TVertex))
        { }

        PrimitiveBatch(PrimitiveBatch&& moveFrom)
          : PrimitiveBatchBase(std::move(moveFrom))
        { }

        PrimitiveBatch& __cdecl operator= (PrimitiveBatch&& moveFrom)
        {
            PrimitiveBatchBase::operator=(std::move(moveFrom));
            return *this;
        }


        // Similar to the D3D9 API DrawPrimitiveUP.
        void __cdecl Draw(D3D11_PRIMITIVE_TOPOLOGY topology, _In_reads_(vertexCount) TVertex const* vertices, size_t vertexCount)
        {
            void* mappedVertices;

            PrimitiveBatchBase::Draw(topology, false, nullptr, 0, vertexCount, &mappedVertices);

            memcpy(mappedVertices, vertices, vertexCount * sizeof(TVertex));
        }


        // Similar to the D3D9 API DrawIndexedPrimitiveUP.
        void __cdecl DrawIndexed(D3D11_PRIMITIVE_TOPOLOGY topology, _In_reads_(indexCount) uint16_t const* indices, size_t indexCount, _In_reads_(vertexCount) TVertex const* vertices, size_t vertexCount)
        {
            void* mappedVertices;

            PrimitiveBatchBase::Draw(topology, true, indices, indexCount, vertexCount, &mappedVertices);

            memcpy(mappedVertices, vertices, vertexCount * sizeof(TVertex));
        }


        void __cdecl DrawLine(TVertex const& v1, TVertex const& v2)
        {
            TVertex* mappedVertices;

            PrimitiveBatchBase::Draw(D3D11_PRIMITIVE_TOPOLOGY_LINELIST, false, nullptr, 0, 2, reinterpret_cast<void**>(&mappedVertices));

            mappedVertices[0] = v1;
            mappedVertices[1] = v2;
        }


        void __cdecl DrawTriangle(TVertex const& v1, TVertex const& v2, TVertex const& v3)
        {
            TVertex* mappedVertices;

            PrimitiveBatchBase::Draw(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, false, nullptr, 0, 3, reinterpret_cast<void**>(&mappedVertices));

            mappedVertices[0] = v1;
            mappedVertices[1] = v2;
            mappedVertices[2] = v3;
        }


        void __cdecl DrawQuad(TVertex const& v1, TVertex const& v2, TVertex const& v3, TVertex const& v4)
        {
            static const uint16_t quadIndices[] = { 0, 1, 2, 0, 2, 3 };

            TVertex* mappedVertices;

            PrimitiveBatchBase::Draw(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, true, quadIndices, 6, 4, reinterpret_cast<void**>(&mappedVertices));

            mappedVertices[0] = v1;
            mappedVertices[1] = v2;
            mappedVertices[2] = v3;
            mappedVertices[3] = v4;
        }
    };
}
