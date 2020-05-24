//--------------------------------------------------------------------------------------
// File: BufferHelpers.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "BufferHelpers.h"
#include "Effects.h"
#include "PlatformHelpers.h"


using namespace DirectX;

_Use_decl_annotations_
HRESULT DirectX::CreateStaticBuffer(
    ID3D11Device* device,
    const void* ptr,
    size_t count,
    size_t stride,
    D3D11_BIND_FLAG bindFlags,
    ID3D11Buffer** pBuffer) noexcept
{
    uint64_t bytes = uint64_t(count) * uint64_t(stride);
    if (bytes >= UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(bytes);
    bufferDesc.BindFlags = bindFlags;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA initData = { ptr, 0, 0 };

    return device->CreateBuffer(&bufferDesc, &initData, pBuffer);
}


_Use_decl_annotations_
HRESULT DirectX::CreateInputLayout(
    ID3D11Device* device,
    IEffect* effect,
    const D3D11_INPUT_ELEMENT_DESC* desc,
    size_t count,
    ID3D11InputLayout** pInputLayout) noexcept
{
    assert(pInputLayout != nullptr);

    void const* shaderByteCode;
    size_t byteCodeLength;

    effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    return device->CreateInputLayout(
        desc, static_cast<UINT>(count),
        shaderByteCode, byteCodeLength,
        pInputLayout);
}


_Use_decl_annotations_
void Internal::ConstantBufferBase::CreateBuffer(
    ID3D11Device* device,
    size_t bytes,
    ID3D11Buffer** pBuffer)
{
    if (!pBuffer)
        throw std::invalid_argument("ConstantBuffer");

    *pBuffer = nullptr;

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = static_cast<UINT>(bytes);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

#if defined(_XBOX_ONE) && defined(_TITLE)

    Microsoft::WRL::ComPtr<ID3D11DeviceX> deviceX;
    ThrowIfFailed(device->QueryInterface(IID_GRAPHICS_PPV_ARGS(deviceX.GetAddressOf())));

    ThrowIfFailed(deviceX->CreatePlacementBuffer(&desc, nullptr, pBuffer));

#else

    desc.Usage = D3D11_USAGE_DYNAMIC;

    ThrowIfFailed(
        device->CreateBuffer(&desc, nullptr, pBuffer)
    );

#endif
}
