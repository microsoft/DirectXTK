//--------------------------------------------------------------------------------------
// File: Model.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Model.h"

#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "Effects.h"
#include "PlatformHelpers.h"

using namespace DirectX;

#ifndef _CPPRTTI 
#error Model requires RTTI
#endif

//--------------------------------------------------------------------------------------
// ModelMeshPart
//--------------------------------------------------------------------------------------

ModelMeshPart::ModelMeshPart() throw() :
    indexCount(0),
    startIndex(0),
    vertexOffset(0),
    vertexStride(0),
    primitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST),
    indexFormat(DXGI_FORMAT_R16_UINT),
    isAlpha(false)
{
}


ModelMeshPart::~ModelMeshPart()
{
}


_Use_decl_annotations_
void ModelMeshPart::Draw(
    ID3D11DeviceContext* deviceContext,
    IEffect* ieffect,
    ID3D11InputLayout* iinputLayout,
    std::function<void()> setCustomState) const
{
    deviceContext->IASetInputLayout(iinputLayout);

    auto vb = vertexBuffer.Get();
    UINT vbStride = vertexStride;
    UINT vbOffset = 0;
    deviceContext->IASetVertexBuffers(0, 1, &vb, &vbStride, &vbOffset);

    // Note that if indexFormat is DXGI_FORMAT_R32_UINT, this model mesh part requires a Feature Level 9.2 or greater device
    deviceContext->IASetIndexBuffer(indexBuffer.Get(), indexFormat, 0);

    assert(ieffect != 0);
    ieffect->Apply(deviceContext);

    // Hook lets the caller replace our shaders or state settings with whatever else they see fit.
    if (setCustomState)
    {
        setCustomState();
    }

    // Draw the primitive.
    deviceContext->IASetPrimitiveTopology(primitiveType);

    deviceContext->DrawIndexed(indexCount, startIndex, vertexOffset);
}


_Use_decl_annotations_
void ModelMeshPart::CreateInputLayout(ID3D11Device* d3dDevice, IEffect* ieffect, ID3D11InputLayout** iinputLayout) const
{
    if (!vbDecl || vbDecl->empty())
        throw std::exception("Model mesh part missing vertex buffer input elements data");

    void const* shaderByteCode;
    size_t byteCodeLength;

    assert(ieffect != 0);
    ieffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    assert(d3dDevice != 0);

    ThrowIfFailed(
        d3dDevice->CreateInputLayout(vbDecl->data(),
        static_cast<UINT>(vbDecl->size()),
        shaderByteCode, byteCodeLength,
        iinputLayout)
    );

    _Analysis_assume_(*iinputLayout != 0);
}


_Use_decl_annotations_
void ModelMeshPart::ModifyEffect(ID3D11Device* d3dDevice, std::shared_ptr<IEffect>& ieffect, bool isalpha)
{
    if (!vbDecl || vbDecl->empty())
        throw std::exception("Model mesh part missing vertex buffer input elements data");

    assert(ieffect != 0);
    this->effect = ieffect;
    this->isAlpha = isalpha;

    void const* shaderByteCode;
    size_t byteCodeLength;

    effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    assert(d3dDevice != 0);

    ThrowIfFailed(
        d3dDevice->CreateInputLayout(vbDecl->data(),
        static_cast<UINT>(vbDecl->size()),
        shaderByteCode, byteCodeLength,
        &inputLayout)
    );
}


//--------------------------------------------------------------------------------------
// ModelMesh
//--------------------------------------------------------------------------------------

ModelMesh::ModelMesh() throw() :
    ccw(true),
    pmalpha(true)
{
}


ModelMesh::~ModelMesh()
{
}


_Use_decl_annotations_
void ModelMesh::PrepareForRendering(
    ID3D11DeviceContext* deviceContext,
    const CommonStates& states,
    bool alpha,
    bool wireframe) const
{
    assert(deviceContext != 0);

    // Set the blend and depth stencil state.
    ID3D11BlendState* blendState;
    ID3D11DepthStencilState* depthStencilState;

    if (alpha)
    {
        if (pmalpha)
        {
            blendState = states.AlphaBlend();
            depthStencilState = states.DepthRead();
        }
        else
        {
            blendState = states.NonPremultiplied();
            depthStencilState = states.DepthRead();
        }
    }
    else
    {
        blendState = states.Opaque();
        depthStencilState = states.DepthDefault();
    }

    deviceContext->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
    deviceContext->OMSetDepthStencilState(depthStencilState, 0);

    // Set the rasterizer state.
    if (wireframe)
        deviceContext->RSSetState(states.Wireframe());
    else
        deviceContext->RSSetState(ccw ? states.CullCounterClockwise() : states.CullClockwise());

    // Set sampler state.
    ID3D11SamplerState* samplers[] =
    {
        states.LinearWrap(),
        states.LinearWrap(),
    };

    deviceContext->PSSetSamplers(0, 2, samplers);
}


_Use_decl_annotations_
void XM_CALLCONV ModelMesh::Draw(
    ID3D11DeviceContext* deviceContext,
    FXMMATRIX world,
    CXMMATRIX view,
    CXMMATRIX projection,
    bool alpha,
    std::function<void()> setCustomState) const
{
    assert(deviceContext != 0);

    for (auto it = meshParts.cbegin(); it != meshParts.cend(); ++it)
    {
        auto part = (*it).get();
        assert(part != 0);

        if (part->isAlpha != alpha)
        {
            // Skip alpha parts when drawing opaque or skip opaque parts if drawing alpha
            continue;
        }

        auto imatrices = dynamic_cast<IEffectMatrices*>(part->effect.get());
        if (imatrices)
        {
            imatrices->SetMatrices(world, view, projection);
        }

        part->Draw(deviceContext, part->effect.get(), part->inputLayout.Get(), setCustomState);
    }
}


//--------------------------------------------------------------------------------------
// Model
//--------------------------------------------------------------------------------------

Model::~Model()
{
}


_Use_decl_annotations_
void XM_CALLCONV Model::Draw(
    ID3D11DeviceContext* deviceContext,
    const CommonStates& states,
    FXMMATRIX world,
    CXMMATRIX view,
    CXMMATRIX projection,
    bool wireframe, std::function<void()> setCustomState) const
{
    assert(deviceContext != 0);

    // Draw opaque parts
    for (auto it = meshes.cbegin(); it != meshes.cend(); ++it)
    {
        auto mesh = it->get();
        assert(mesh != 0);

        mesh->PrepareForRendering(deviceContext, states, false, wireframe);

        mesh->Draw(deviceContext, world, view, projection, false, setCustomState);
    }

    // Draw alpha parts
    for (auto it = meshes.cbegin(); it != meshes.cend(); ++it)
    {
        auto mesh = it->get();
        assert(mesh != 0);

        mesh->PrepareForRendering(deviceContext, states, true, wireframe);

        mesh->Draw(deviceContext, world, view, projection, true, setCustomState);
    }
}


void Model::UpdateEffects(_In_ std::function<void(IEffect*)> setEffect)
{
    if (mEffectCache.empty())
    {
        // This cache ensures we only set each effect once (could be shared)
        for (auto mit = meshes.cbegin(); mit != meshes.cend(); ++mit)
        {
            auto mesh = mit->get();
            assert(mesh != 0);

            for (auto it = mesh->meshParts.cbegin(); it != mesh->meshParts.cend(); ++it)
            {
                if ((*it)->effect != 0)
                    mEffectCache.insert((*it)->effect.get());
            }
        }
    }

    assert(setEffect != 0);

    for (auto it = mEffectCache.begin(); it != mEffectCache.end(); ++it)
    {
        setEffect(*it);
    }
}
