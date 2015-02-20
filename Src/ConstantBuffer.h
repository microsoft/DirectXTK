//--------------------------------------------------------------------------------------
// File: ConstantBuffer.h
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

#include "DirectXHelpers.h"
#include "PlatformHelpers.h"


namespace DirectX
{
    // Strongly typed wrapper around a D3D constant buffer.
    template<typename T>
    class ConstantBuffer
    {
    public:
        // Constructor.
        ConstantBuffer() DIRECTX_CTOR_DEFAULT
        explicit ConstantBuffer(_In_ ID3D11Device* device)
        {
            Create( device );
        }


        void Create(_In_ ID3D11Device* device)
        {
            D3D11_BUFFER_DESC desc = { 0 };

            desc.ByteWidth = sizeof(T);
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            ThrowIfFailed(
                device->CreateBuffer(&desc, nullptr, mConstantBuffer.ReleaseAndGetAddressOf() )
            );

            SetDebugObjectName(mConstantBuffer.Get(), "DirectXTK");
        }


        // Writes new data into the constant buffer.
        void SetData(_In_ ID3D11DeviceContext* deviceContext, T const& value)
        {
            assert( mConstantBuffer.Get() != 0 );

            D3D11_MAPPED_SUBRESOURCE mappedResource;
            
            ThrowIfFailed(
                deviceContext->Map(mConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)
            );

            *(T*)mappedResource.pData = value;

            deviceContext->Unmap(mConstantBuffer.Get(), 0);
        }


        // Looks up the underlying D3D constant buffer.
        ID3D11Buffer* GetBuffer()
        {
            return mConstantBuffer.Get();
        }


    private:
        // The underlying D3D object.
        Microsoft::WRL::ComPtr<ID3D11Buffer> mConstantBuffer;
        
        
        // Prevent copying.
        ConstantBuffer(ConstantBuffer const&) DIRECTX_CTOR_DELETE
        ConstantBuffer& operator= (ConstantBuffer const&) DIRECTX_CTOR_DELETE
    };
}
