//--------------------------------------------------------------------------------------
// File: DirectXHelpers.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DirectXHelpers.h"
#include "Effects.h"


using namespace DirectX;

_Use_decl_annotations_
HRESULT DirectX::CreateInputLayoutFromEffect(
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
