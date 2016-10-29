//--------------------------------------------------------------------------------------
// File: GeometricPrimitive.cpp
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

#include "pch.h"
#include "GeometricPrimitive.h"
#include "Effects.h"
#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "SharedResourcePool.h"
#include "Geometry.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;


namespace
{
    // Helper for creating a D3D vertex or index buffer.
    template<typename T>
    static void CreateBuffer(_In_ ID3D11Device* device, T const& data, D3D11_BIND_FLAG bindFlags, _Outptr_ ID3D11Buffer** pBuffer)
    {
        assert(pBuffer != 0);

        D3D11_BUFFER_DESC bufferDesc = {};

        bufferDesc.ByteWidth = (UINT)data.size() * sizeof(T::value_type);
        bufferDesc.BindFlags = bindFlags;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;

        D3D11_SUBRESOURCE_DATA dataDesc = {};

        dataDesc.pSysMem = data.data();

        ThrowIfFailed(
            device->CreateBuffer(&bufferDesc, &dataDesc, pBuffer)
        );

        _Analysis_assume_(*pBuffer != 0);

        SetDebugObjectName(*pBuffer, "DirectXTK:GeometricPrimitive");
    }


    // Helper for creating a D3D input layout.
    void CreateInputLayout(_In_ ID3D11Device* device, IEffect* effect, _Outptr_ ID3D11InputLayout** pInputLayout)
    {
        assert(pInputLayout != 0);

        void const* shaderByteCode;
        size_t byteCodeLength;

        effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        ThrowIfFailed(
            device->CreateInputLayout(VertexPositionNormalTexture::InputElements,
                VertexPositionNormalTexture::InputElementCount,
                shaderByteCode, byteCodeLength,
                pInputLayout)
        );

        _Analysis_assume_(*pInputLayout != 0);

        SetDebugObjectName(*pInputLayout, "DirectXTK:GeometricPrimitive");
    }
}


// Internal GeometricPrimitive implementation class.
class GeometricPrimitive::Impl
{
public:
    void Initialize(_In_ ID3D11DeviceContext* deviceContext, const VertexCollection& vertices, const IndexCollection& indices);

    void XM_CALLCONV Draw(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color, _In_opt_ ID3D11ShaderResourceView* texture, bool wireframe, _In_opt_ std::function<void()> setCustomState) const;

    void Draw(_In_ IEffect* effect, _In_ ID3D11InputLayout* inputLayout, bool alpha, bool wireframe, _In_opt_ std::function<void()> setCustomState) const;

    void CreateInputLayout(_In_ IEffect* effect, _Outptr_ ID3D11InputLayout** inputLayout) const;

private:
    ComPtr<ID3D11Buffer> mVertexBuffer;
    ComPtr<ID3D11Buffer> mIndexBuffer;

    UINT mIndexCount;

    // Only one of these helpers is allocated per D3D device context, even if there are multiple GeometricPrimitive instances.
    class SharedResources
    {
    public:
        SharedResources(_In_ ID3D11DeviceContext* deviceContext);

        void PrepareForRendering(bool alpha, bool wireframe) const;

        ComPtr<ID3D11DeviceContext> deviceContext;
        std::unique_ptr<BasicEffect> effect;

        ComPtr<ID3D11InputLayout> inputLayoutTextured;
        ComPtr<ID3D11InputLayout> inputLayoutUntextured;

        std::unique_ptr<CommonStates> stateObjects;
    };


    // Per-device-context data.
    std::shared_ptr<SharedResources> mResources;

    static SharedResourcePool<ID3D11DeviceContext*, SharedResources> sharedResourcesPool;
};


// Global pool of per-device-context GeometricPrimitive resources.
SharedResourcePool<ID3D11DeviceContext*, GeometricPrimitive::Impl::SharedResources> GeometricPrimitive::Impl::sharedResourcesPool;


// Per-device-context constructor.
GeometricPrimitive::Impl::SharedResources::SharedResources(_In_ ID3D11DeviceContext* deviceContext)
    : deviceContext(deviceContext)
{
    ComPtr<ID3D11Device> device;
    deviceContext->GetDevice(&device);

    // Create the BasicEffect.
    effect = std::make_unique<BasicEffect>(device.Get());

    effect->EnableDefaultLighting();

    // Create state objects.
    stateObjects = std::make_unique<CommonStates>(device.Get());

    // Create input layouts.
    effect->SetTextureEnabled(true);
    ::CreateInputLayout(device.Get(), effect.get(), &inputLayoutTextured);

    effect->SetTextureEnabled(false);
    ::CreateInputLayout(device.Get(), effect.get(), &inputLayoutUntextured);
}


// Sets up D3D device state ready for drawing a primitive.
void GeometricPrimitive::Impl::SharedResources::PrepareForRendering(bool alpha, bool wireframe) const
{
    // Set the blend and depth stencil state.
    ID3D11BlendState* blendState;
    ID3D11DepthStencilState* depthStencilState;

    if (alpha)
    {
        // Alpha blended rendering.
        blendState = stateObjects->AlphaBlend();
        depthStencilState = stateObjects->DepthRead();
    }
    else
    {
        // Opaque rendering.
        blendState = stateObjects->Opaque();
        depthStencilState = stateObjects->DepthDefault();
    }

    deviceContext->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
    deviceContext->OMSetDepthStencilState(depthStencilState, 0);

    // Set the rasterizer state.
    if (wireframe)
        deviceContext->RSSetState(stateObjects->Wireframe());
    else
        deviceContext->RSSetState(stateObjects->CullCounterClockwise());

    ID3D11SamplerState* samplerState = stateObjects->LinearWrap();

    deviceContext->PSSetSamplers(0, 1, &samplerState);
}


// Initializes a geometric primitive instance that will draw the specified vertex and index data.
_Use_decl_annotations_
void GeometricPrimitive::Impl::Initialize(ID3D11DeviceContext* deviceContext, const VertexCollection& vertices, const IndexCollection& indices)
{
    if (vertices.size() >= USHRT_MAX)
        throw std::exception("Too many vertices for 16-bit index buffer");

    mResources = sharedResourcesPool.DemandCreate(deviceContext);

    ComPtr<ID3D11Device> device;
    deviceContext->GetDevice(&device);

    CreateBuffer(device.Get(), vertices, D3D11_BIND_VERTEX_BUFFER, &mVertexBuffer);
    CreateBuffer(device.Get(), indices, D3D11_BIND_INDEX_BUFFER, &mIndexBuffer);

    mIndexCount = static_cast<UINT>(indices.size());
}


// Draws the primitive.
_Use_decl_annotations_
void XM_CALLCONV GeometricPrimitive::Impl::Draw(
    FXMMATRIX world,
    CXMMATRIX view,
    CXMMATRIX projection,
    FXMVECTOR color,
    ID3D11ShaderResourceView* texture,
    bool wireframe,
    std::function<void()> setCustomState) const
{
    assert(mResources != 0);
    auto effect = mResources->effect.get();
    assert(effect != 0);

    ID3D11InputLayout *inputLayout;
    if (texture)
    {
        effect->SetTextureEnabled(true);
        effect->SetTexture(texture);

        inputLayout = mResources->inputLayoutTextured.Get();
    }
    else
    {
        effect->SetTextureEnabled(false);

        inputLayout = mResources->inputLayoutUntextured.Get();
    }

    // Set effect parameters.
    effect->SetMatrices(world, view, projection);

    effect->SetColorAndAlpha(color);

    float alpha = XMVectorGetW(color);
    Draw(effect, inputLayout, (alpha < 1.f), wireframe, setCustomState);
}


// Draw the primitive using a custom effect.
_Use_decl_annotations_
void GeometricPrimitive::Impl::Draw(
    IEffect* effect,
    ID3D11InputLayout* inputLayout,
    bool alpha,
    bool wireframe,
    std::function<void()> setCustomState) const
{
    assert(mResources != 0);
    auto deviceContext = mResources->deviceContext.Get();
    assert(deviceContext != 0);

    // Set state objects.
    mResources->PrepareForRendering(alpha, wireframe);

    // Set input layout.
    assert(inputLayout != 0);
    deviceContext->IASetInputLayout(inputLayout);

    // Activate our shaders, constant buffers, texture, etc.
    assert(effect != 0);
    effect->Apply(deviceContext);

    // Set the vertex and index buffer.
    auto vertexBuffer = mVertexBuffer.Get();
    UINT vertexStride = sizeof(VertexPositionNormalTexture);
    UINT vertexOffset = 0;

    deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexStride, &vertexOffset);

    deviceContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    // Hook lets the caller replace our shaders or state settings with whatever else they see fit.
    if (setCustomState)
    {
        setCustomState();
    }

    // Draw the primitive.
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    deviceContext->DrawIndexed(mIndexCount, 0, 0);
}


// Create input layout for drawing with a custom effect.
_Use_decl_annotations_
void GeometricPrimitive::Impl::CreateInputLayout(IEffect* effect, ID3D11InputLayout** inputLayout) const
{
    assert(effect != 0);
    assert(inputLayout != 0);

    assert(mResources != 0);
    auto deviceContext = mResources->deviceContext.Get();
    assert(deviceContext != 0);

    ComPtr<ID3D11Device> device;
    deviceContext->GetDevice(&device);

    ::CreateInputLayout(device.Get(), effect, inputLayout);
}


//--------------------------------------------------------------------------------------
// GeometricPrimitive
//--------------------------------------------------------------------------------------

// Constructor.
GeometricPrimitive::GeometricPrimitive()
    : pImpl(new Impl())
{
}


// Destructor.
GeometricPrimitive::~GeometricPrimitive()
{
}


// Public entrypoints.
_Use_decl_annotations_
void XM_CALLCONV GeometricPrimitive::Draw(
    FXMMATRIX world,
    CXMMATRIX view,
    CXMMATRIX projection,
    FXMVECTOR color,
    ID3D11ShaderResourceView* texture,
    bool wireframe,
    std::function<void()> setCustomState) const
{
    pImpl->Draw(world, view, projection, color, texture, wireframe, setCustomState);
}


_Use_decl_annotations_
void GeometricPrimitive::Draw(
    IEffect* effect,
    ID3D11InputLayout* inputLayout,
    bool alpha,
    bool wireframe,
    std::function<void()> setCustomState) const
{
    pImpl->Draw(effect, inputLayout, alpha, wireframe, setCustomState);
}


_Use_decl_annotations_
void GeometricPrimitive::CreateInputLayout(IEffect* effect, ID3D11InputLayout** inputLayout) const
{
    pImpl->CreateInputLayout(effect, inputLayout);
}


//--------------------------------------------------------------------------------------
// Cube (aka a Hexahedron) or Box
//--------------------------------------------------------------------------------------

_Use_decl_annotations_
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateCube(
    ID3D11DeviceContext* deviceContext,
    float size,
    bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeBox(vertices, indices, XMFLOAT3(size, size, size), rhcoords, false);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateCube(
    std::vector<VertexPositionNormalTexture>& vertices,
    std::vector<uint16_t>& indices,
    float size,
    bool rhcoords)
{
    ComputeBox(vertices, indices, XMFLOAT3(size, size, size), rhcoords, false);
}


// Creates a box primitive.
_Use_decl_annotations_
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateBox(
    ID3D11DeviceContext* deviceContext,
    const XMFLOAT3& size,
    bool rhcoords,
    bool invertn)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeBox(vertices, indices, size, rhcoords, invertn);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateBox(
    std::vector<VertexPositionNormalTexture>& vertices,
    std::vector<uint16_t>& indices,
    const XMFLOAT3& size,
    bool rhcoords,
    bool invertn)
{
    ComputeBox(vertices, indices, size, rhcoords, invertn);
}


//--------------------------------------------------------------------------------------
// Sphere
//--------------------------------------------------------------------------------------

_Use_decl_annotations_
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateSphere(
    ID3D11DeviceContext* deviceContext,
    float diameter,
    size_t tessellation,
    bool rhcoords,
    bool invertn)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeSphere(vertices, indices, diameter, tessellation, rhcoords, invertn);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateSphere(
    std::vector<VertexPositionNormalTexture>& vertices,
    std::vector<uint16_t>& indices,
    float diameter,
    size_t tessellation,
    bool rhcoords,
    bool invertn)
{
    ComputeSphere(vertices, indices, diameter, tessellation, rhcoords, invertn);
}


//--------------------------------------------------------------------------------------
// Geodesic sphere
//--------------------------------------------------------------------------------------

_Use_decl_annotations_
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateGeoSphere(
    ID3D11DeviceContext* deviceContext,
    float diameter,
    size_t tessellation,
    bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeGeoSphere(vertices, indices, diameter, tessellation, rhcoords);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateGeoSphere(
    std::vector<VertexPositionNormalTexture>& vertices,
    std::vector<uint16_t>& indices,
    float diameter,
    size_t tessellation, bool rhcoords)
{
    ComputeGeoSphere(vertices, indices, diameter, tessellation, rhcoords);
}


//--------------------------------------------------------------------------------------
// Cylinder / Cone
//--------------------------------------------------------------------------------------

// Creates a cylinder primitive.
_Use_decl_annotations_
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateCylinder(
    ID3D11DeviceContext* deviceContext,
    float height,
    float diameter,
    size_t tessellation,
    bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeCylinder(vertices, indices, height, diameter, tessellation, rhcoords);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateCylinder(
    std::vector<VertexPositionNormalTexture>& vertices,
    std::vector<uint16_t>& indices,
    float height,
    float diameter,
    size_t tessellation,
    bool rhcoords)
{
    ComputeCylinder(vertices, indices, height, diameter, tessellation, rhcoords);
}


// Creates a cone primitive.
_Use_decl_annotations_
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateCone(
    ID3D11DeviceContext* deviceContext,
    float diameter,
    float height,
    size_t tessellation,
    bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeCone(vertices, indices, diameter, height, tessellation, rhcoords);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateCone(
    std::vector<VertexPositionNormalTexture>& vertices,
    std::vector<uint16_t>& indices,
    float diameter,
    float height,
    size_t tessellation,
    bool rhcoords)
{
    ComputeCone(vertices, indices, diameter, height, tessellation, rhcoords);
}


//--------------------------------------------------------------------------------------
// Torus
//--------------------------------------------------------------------------------------

_Use_decl_annotations_
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateTorus(
    ID3D11DeviceContext* deviceContext,
    float diameter,
    float thickness,
    size_t tessellation,
    bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeTorus(vertices, indices, diameter, thickness, tessellation, rhcoords);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateTorus(
    std::vector<VertexPositionNormalTexture>& vertices,
    std::vector<uint16_t>& indices,
    float diameter,
    float thickness,
    size_t tessellation,
    bool rhcoords)
{
    ComputeTorus(vertices, indices, diameter, thickness, tessellation, rhcoords);
}


//--------------------------------------------------------------------------------------
// Tetrahedron
//--------------------------------------------------------------------------------------

_Use_decl_annotations_
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateTetrahedron(
    ID3D11DeviceContext* deviceContext,
    float size,
    bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeTetrahedron(vertices, indices, size, rhcoords);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateTetrahedron(
    std::vector<VertexPositionNormalTexture>& vertices,
    std::vector<uint16_t>& indices,
    float size,
    bool rhcoords)
{
    ComputeTetrahedron(vertices, indices, size, rhcoords);
}


//--------------------------------------------------------------------------------------
// Octahedron
//--------------------------------------------------------------------------------------

_Use_decl_annotations_
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateOctahedron(
    ID3D11DeviceContext* deviceContext,
    float size,
    bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeOctahedron(vertices, indices, size, rhcoords);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateOctahedron(
    std::vector<VertexPositionNormalTexture>& vertices,
    std::vector<uint16_t>& indices,
    float size,
    bool rhcoords)
{
    ComputeOctahedron(vertices, indices, size, rhcoords);
}


//--------------------------------------------------------------------------------------
// Dodecahedron
//--------------------------------------------------------------------------------------

_Use_decl_annotations_
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateDodecahedron(
    ID3D11DeviceContext* deviceContext,
    float size,
    bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeDodecahedron(vertices, indices, size, rhcoords);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateDodecahedron(
    std::vector<VertexPositionNormalTexture>& vertices,
    std::vector<uint16_t>& indices,
    float size,
    bool rhcoords)
{
    ComputeDodecahedron(vertices, indices, size, rhcoords);
}


//--------------------------------------------------------------------------------------
// Icosahedron
//--------------------------------------------------------------------------------------

_Use_decl_annotations_
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateIcosahedron(
    ID3D11DeviceContext* deviceContext,
    float size,
    bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeIcosahedron(vertices, indices, size, rhcoords);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateIcosahedron(
    std::vector<VertexPositionNormalTexture>& vertices,
    std::vector<uint16_t>& indices,
    float size,
    bool rhcoords)
{
    ComputeIcosahedron(vertices, indices, size, rhcoords);
}


//--------------------------------------------------------------------------------------
// Teapot
//--------------------------------------------------------------------------------------

_Use_decl_annotations_
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateTeapot(
    ID3D11DeviceContext* deviceContext,
    float size,
    size_t tessellation,
    bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeTeapot(vertices, indices, size, tessellation, rhcoords);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateTeapot(
    std::vector<VertexPositionNormalTexture>& vertices,
    std::vector<uint16_t>& indices,
    float size,
    size_t tessellation,
    bool rhcoords)
{
    ComputeTeapot(vertices, indices, size, tessellation, rhcoords);
}


//--------------------------------------------------------------------------------------
// Custom
//--------------------------------------------------------------------------------------

_Use_decl_annotations_
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateCustom(
    ID3D11DeviceContext* deviceContext,
    const std::vector<VertexPositionNormalTexture>& vertices,
    const std::vector<uint16_t>& indices)
{
    // Extra validation
    if (vertices.empty() || indices.empty())
        throw std::exception("Requires both vertices and indices");

    if (indices.size() % 3)
        throw std::exception("Expected triangular faces");

    size_t nVerts = vertices.size();
    if (nVerts >= USHRT_MAX)
        throw std::exception("Too many vertices for 16-bit index buffer");

    for (auto it = indices.cbegin(); it != indices.cend(); ++it)
    {
        if (*it >= nVerts)
        {
            throw std::exception("Index not in vertices list");
        }
    }

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}
