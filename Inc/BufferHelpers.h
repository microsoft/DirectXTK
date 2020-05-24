//--------------------------------------------------------------------------------------
// File: BufferHelpers.h
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#include "GraphicsMemory.h"
#else
#include <d3d11_1.h>
#endif

#include <assert.h>


namespace DirectX
{
    class IEffect;

    // Helpers for creating initialized Direct3D resources.
    HRESULT __cdecl CreateStaticBuffer(_In_ ID3D11Device* device,
        _In_reads_bytes_(count* stride) const void* ptr,
        size_t count,
        size_t stride,
        D3D11_BIND_FLAG bindFlags,
        _Outptr_ ID3D11Buffer** pBuffer) noexcept;

    template<typename T>
    HRESULT CreateStaticBuffer(_In_ ID3D11Device* device,
        _In_reads_(count) T const* data,
        size_t count,
        D3D11_BIND_FLAG bindFlags,
        _Outptr_ ID3D11Buffer** pBuffer) noexcept
    {
        return CreateStaticBuffer(device, data, count, sizeof(T), bindFlags, pBuffer);
    }

    template<typename T>
    HRESULT CreateStaticBuffer(_In_ ID3D11Device* device,
        T const& data,
        D3D11_BIND_FLAG bindFlags,
        _Outptr_ ID3D11Buffer** pBuffer) noexcept
    {
        return CreateStaticBuffer(device, data.data(), data.size(), sizeof(T::value_type), bindFlags, pBuffer);
    }

    // Helper for creating a Direct3D input layout.
    HRESULT __cdecl CreateInputLayout(_In_ ID3D11Device* device,
        _In_ IEffect* effect,
        _In_reads_(count) const D3D11_INPUT_ELEMENT_DESC* desc,
        size_t count,
        _Outptr_ ID3D11InputLayout** pInputLayout) noexcept;

    template<typename T>
    HRESULT CreateInputLayout(_In_ ID3D11Device* device,
        _In_ IEffect* effect,
        _Outptr_ ID3D11InputLayout** pInputLayout) noexcept
    {
        return CreateInputLayout(device, effect, T::InputElements, T::InputElementCount, pInputLayout);
    }

    // Strongly typed wrapper around a Direct3D constant buffer.
    namespace Internal
    {
        // Base class, not to be used directly: clients should access this via the derived PrimitiveBatch<T>.
        class ConstantBufferBase
        {
        protected:
            void __cdecl CreateBuffer(_In_ ID3D11Device* device, size_t bytes, _Outptr_ ID3D11Buffer** pBuffer);
        };
    }

    template<typename T>
    class ConstantBuffer : public Internal::ConstantBufferBase
    {
    public:
        // Constructor.
        ConstantBuffer() = default;
        explicit ConstantBuffer(_In_ ID3D11Device* device) noexcept(false)
        {
            CreateBuffer(device, sizeof(T), mConstantBuffer.GetAddressOf());
        }

        ConstantBuffer(ConstantBuffer&&) = default;
        ConstantBuffer& operator= (ConstantBuffer&&) = default;

        ConstantBuffer(ConstantBuffer const&) = delete;
        ConstantBuffer& operator= (ConstantBuffer const&) = delete;

        void Create(_In_ ID3D11Device* device)
        {
            CreateBuffer(device, sizeof(T), mConstantBuffer.ReleaseAndGetAddressOf());
        }

        // Writes new data into the constant buffer.
#if defined(_XBOX_ONE) && defined(_TITLE)
        void __cdecl SetData(_In_ ID3D11DeviceContext* deviceContext, T const& value, void** grfxMemory)
        {
            assert(grfxMemory != nullptr);

            void* ptr = GraphicsMemory::Get().Allocate(deviceContext, sizeof(T), 64);
            assert(ptr != nullptr);

            *(T*)ptr = value;

            *grfxMemory = ptr;
        }
#else

        void __cdecl SetData(_In_ ID3D11DeviceContext* deviceContext, T const& value) noexcept
        {
            assert(mConstantBuffer);

            D3D11_MAPPED_SUBRESOURCE mappedResource;
            if (SUCCEEDED(deviceContext->Map(mConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
            {
                *static_cast<T*>(mappedResource.pData) = value;

                deviceContext->Unmap(mConstantBuffer.Get(), 0);
            }
        }
#endif // _XBOX_ONE && _TITLE

        // Looks up the underlying D3D constant buffer.
        ID3D11Buffer* GetBuffer() const noexcept { return mConstantBuffer.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> mConstantBuffer;
    };
}
