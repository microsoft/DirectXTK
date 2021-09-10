//--------------------------------------------------------------------------------------
// File: Model.cpp
//
// Copyright (c) Microsoft Corporation.
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

#if !defined(_CPPRTTI) && !defined(__GXX_RTTI)
#error Model requires RTTI
#endif

//--------------------------------------------------------------------------------------
// ModelMeshPart
//--------------------------------------------------------------------------------------

ModelMeshPart::ModelMeshPart() noexcept :
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

    assert(ieffect != nullptr);
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
void ModelMeshPart::DrawInstanced(
    ID3D11DeviceContext* deviceContext,
    IEffect* ieffect,
    ID3D11InputLayout* iinputLayout,
    uint32_t instanceCount, uint32_t startInstanceLocation,
    std::function<void()> setCustomState) const
{
    deviceContext->IASetInputLayout(iinputLayout);

    auto vb = vertexBuffer.Get();
    UINT vbStride = vertexStride;
    UINT vbOffset = 0;
    deviceContext->IASetVertexBuffers(0, 1, &vb, &vbStride, &vbOffset);

    // Note that if indexFormat is DXGI_FORMAT_R32_UINT, this model mesh part requires a Feature Level 9.2 or greater device
    deviceContext->IASetIndexBuffer(indexBuffer.Get(), indexFormat, 0);

    assert(ieffect != nullptr);
    ieffect->Apply(deviceContext);

    // Hook lets the caller replace our shaders or state settings with whatever else they see fit.
    if (setCustomState)
    {
        setCustomState();
    }

    // Draw the primitive.
    deviceContext->IASetPrimitiveTopology(primitiveType);

    deviceContext->DrawIndexedInstanced(
        indexCount, instanceCount, startIndex,
        vertexOffset,
        startInstanceLocation);
}


_Use_decl_annotations_
void ModelMeshPart::CreateInputLayout(ID3D11Device* d3dDevice, IEffect* ieffect, ID3D11InputLayout** iinputLayout) const
{
    if (iinputLayout)
    {
        *iinputLayout = nullptr;
    }

    if (!vbDecl || vbDecl->empty())
        throw std::runtime_error("Model mesh part missing vertex buffer input elements data");

    if (vbDecl->size() > D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT)
        throw std::runtime_error("Model mesh part input layout size is too large for DirectX 11");

    ThrowIfFailed(
        CreateInputLayoutFromEffect(d3dDevice, ieffect, vbDecl->data(), vbDecl->size(), iinputLayout)
    );

    assert(iinputLayout != nullptr && *iinputLayout != nullptr);
    _Analysis_assume_(iinputLayout != nullptr && *iinputLayout != nullptr);
}


_Use_decl_annotations_
void ModelMeshPart::ModifyEffect(ID3D11Device* d3dDevice, std::shared_ptr<IEffect>& ieffect, bool isalpha)
{
    if (!vbDecl || vbDecl->empty())
        throw std::runtime_error("Model mesh part missing vertex buffer input elements data");

    if (vbDecl->size() > D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT)
        throw std::runtime_error("Model mesh part input layout size is too large for DirectX 11");

    assert(ieffect != nullptr);
    this->effect = ieffect;
    this->isAlpha = isalpha;

    assert(d3dDevice != nullptr);

    ThrowIfFailed(
        CreateInputLayoutFromEffect(d3dDevice, effect.get(), vbDecl->data(), vbDecl->size(), inputLayout.ReleaseAndGetAddressOf())
    );
}


//--------------------------------------------------------------------------------------
// ModelMesh
//--------------------------------------------------------------------------------------

ModelMesh::ModelMesh() noexcept :
    boneIndex(ModelBone::c_Invalid),
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
    assert(deviceContext != nullptr);

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
    assert(deviceContext != nullptr);

    for (const auto& it : meshParts)
    {
        auto part = it.get();
        assert(part != nullptr);

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


_Use_decl_annotations_
void XM_CALLCONV ModelMesh::DrawSkinned(
    ID3D11DeviceContext* deviceContext,
    size_t nbones, _In_reads_(nbones) const XMMATRIX* boneTransforms,
    FXMMATRIX world,
    CXMMATRIX view,
    CXMMATRIX projection,
    bool alpha,
    std::function<void()> setCustomState) const
{
    assert(deviceContext != nullptr);

    if (!nbones || !boneTransforms)
    {
        throw std::invalid_argument("Bone transforms array required");
    }

    ModelBone::TransformArray temp;

    for (const auto& mit : meshParts)
    {
        auto part = mit.get();
        assert(part != nullptr);

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

        auto iskinning = dynamic_cast<IEffectSkinning*>(part->effect.get());
        if (iskinning)
        {
            if (boneInfluences.empty())
            {
                // Direct-mapping of vertex bone indices to our master bone array
                iskinning->SetBoneTransforms(boneTransforms, nbones);
            }
            else
            {
                if (!temp)
                {
                    // Create temporary space on-demand
                    temp = ModelBone::MakeArray(IEffectSkinning::MaxBones);
                }

                size_t count = 0;
                for (auto it : boneInfluences)
                {
                    ++count;
                    if (count > IEffectSkinning::MaxBones)
                    {
                        throw std::runtime_error("Too many bones for skinning");
                    }

                    if (it >= nbones)
                    {
                        throw std::runtime_error("Invalid bone influence index");
                    }

                    temp[count] = boneTransforms[it];
                }

                assert(count == boneInfluences.size());

                iskinning->SetBoneTransforms(temp.get(), count);
            }
        }
        else if (imatrices)
        {
            // Fallback for if we encounter a non-skinning effect in the model
            XMMATRIX bm = (boneIndex != ModelBone::c_Invalid && boneIndex < nbones)
                ? boneTransforms[boneIndex] : XMMatrixIdentity();

            imatrices->SetWorld(XMMatrixMultiply(bm, world));
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
    bool wireframe,
    std::function<void()> setCustomState) const
{
    assert(deviceContext != nullptr);

    // Draw opaque parts
    for (const auto& it : meshes)
    {
        auto mesh = it.get();
        assert(mesh != nullptr);

        mesh->PrepareForRendering(deviceContext, states, false, wireframe);

        mesh->Draw(deviceContext, world, view, projection, false, setCustomState);
    }

    // Draw alpha parts
    for (const auto& it : meshes)
    {
        auto mesh = it.get();
        assert(mesh != nullptr);

        mesh->PrepareForRendering(deviceContext, states, true, wireframe);

        mesh->Draw(deviceContext, world, view, projection, true, setCustomState);
    }
}


_Use_decl_annotations_
void XM_CALLCONV Model::Draw(
    ID3D11DeviceContext* deviceContext,
    const CommonStates& states,
    size_t nbones,
    const XMMATRIX* boneTransforms,
    FXMMATRIX world,
    CXMMATRIX view,
    CXMMATRIX projection,
    bool wireframe,
    std::function<void()> setCustomState) const
{
    assert(deviceContext != nullptr);

    if (!nbones || !boneTransforms)
    {
        throw std::invalid_argument("Bone transforms array required");
    }

    // Draw opaque parts
    for (const auto& it : meshes)
    {
        auto mesh = it.get();
        assert(mesh != nullptr);

        mesh->PrepareForRendering(deviceContext, states, false, wireframe);

        XMMATRIX bm = (mesh->boneIndex != ModelBone::c_Invalid && mesh->boneIndex < nbones)
            ? boneTransforms[mesh->boneIndex] : XMMatrixIdentity();

        mesh->Draw(deviceContext, XMMatrixMultiply(bm, world), view, projection, false, setCustomState);
    }

    // Draw alpha parts
    for (const auto& it : meshes)
    {
        auto mesh = it.get();
        assert(mesh != nullptr);

        mesh->PrepareForRendering(deviceContext, states, true, wireframe);

        XMMATRIX bm = (mesh->boneIndex != ModelBone::c_Invalid && mesh->boneIndex < nbones)
            ? boneTransforms[mesh->boneIndex] : XMMatrixIdentity();

        mesh->Draw(deviceContext, XMMatrixMultiply(bm, world), view, projection, true, setCustomState);
    }
}


_Use_decl_annotations_
void XM_CALLCONV Model::DrawSkinned(
    ID3D11DeviceContext* deviceContext,
    const CommonStates& states,
    size_t nbones,
    const XMMATRIX* boneTransforms,
    FXMMATRIX world,
    CXMMATRIX view,
    CXMMATRIX projection,
    bool wireframe,
    std::function<void()> setCustomState) const
{
    assert(deviceContext != nullptr);
    assert(boneTransforms != nullptr);

    if (!nbones || !boneTransforms)
    {
        throw std::invalid_argument("Bone transforms array required");
    }

    // Draw opaque parts
    for (const auto& it : meshes)
    {
        auto mesh = it.get();
        assert(mesh != nullptr);

        mesh->PrepareForRendering(deviceContext, states, false, wireframe);

        mesh->DrawSkinned(deviceContext, nbones, boneTransforms, world, view, projection, false, setCustomState);
    }

    // Draw alpha parts
    for (const auto& it : meshes)
    {
        auto mesh = it.get();
        assert(mesh != nullptr);

        mesh->PrepareForRendering(deviceContext, states, true, wireframe);

        mesh->DrawSkinned(deviceContext, nbones, boneTransforms, world, view, projection, true, setCustomState);
    }
}


_Use_decl_annotations_
void Model::CopyAbsoluteBoneTransformsTo(size_t nbones, XMMATRIX* boneTransforms)
{
    if (!nbones || !boneTransforms)
    {
        throw std::invalid_argument("Bone transforms array required");
    }

    if (nbones < bones.size())
    {
        throw std::invalid_argument("Bone transforms is too small");
    }

    if (bones.empty() || !boneMatrices)
    {
        throw std::runtime_error("Model is missing bones");
    }

    memset(boneTransforms, 0, sizeof(XMMATRIX) * nbones);

    XMMATRIX id = XMMatrixIdentity();
    size_t visited = 0;
    ComputeBones(0, id, bones.size(), boneTransforms, visited);
}


_Use_decl_annotations_
void Model::CopyBoneTransformsFrom(size_t nbones, const XMMATRIX* boneTransforms)
{
    if (!nbones || !boneTransforms)
    {
        throw std::invalid_argument("Bone transforms array required");
    }

    if (nbones < bones.size())
    {
        throw std::invalid_argument("Bone transforms is too small");
    }

    if (!boneMatrices)
    {
        boneMatrices = ModelBone::MakeArray(bones.size());
    }

    memcpy(boneMatrices.get(), boneTransforms, bones.size() * sizeof(XMMATRIX));
}


_Use_decl_annotations_
void Model::ComputeBones(
    uint32_t index,
    FXMMATRIX parent,
    size_t nbones,
    XMMATRIX* boneTransforms,
    size_t& visited)
{
    if (index == ModelBone::c_Invalid || index >= nbones)
        return;

    assert(boneTransforms != nullptr);

    ++visited;
    if (visited > bones.size())
    {
        DebugTrace("ERROR: Model::CopyAbsoluteBoneTransformsTo encountered a cycle in the bones!\n");
        throw std::runtime_error("Model bones form an invalid graph");
    }

    XMMATRIX local = boneMatrices[index];
    local = XMMatrixMultiply(local, parent);
    boneTransforms[index] = local;

    if (bones[index].siblingIndex != ModelBone::c_Invalid)
    {
        ComputeBones(bones[index].siblingIndex, parent, nbones, boneTransforms, visited);
    }

    if (bones[index].childIndex != ModelBone::c_Invalid)
    {
        ComputeBones(bones[index].childIndex, local, nbones, boneTransforms, visited);
    }
}


void Model::UpdateEffects(_In_ std::function<void(IEffect*)> setEffect)
{
    if (mEffectCache.empty())
    {
        // This cache ensures we only set each effect once (could be shared)
        for (const auto& mit : meshes)
        {
            auto mesh = mit.get();
            assert(mesh != nullptr);

            for (const auto& it : mesh->meshParts)
            {
                if (it->effect)
                    mEffectCache.insert(it->effect.get());
            }
        }
    }

    assert(setEffect != nullptr);

    for (const auto it : mEffectCache)
    {
        setEffect(it);
    }
}
