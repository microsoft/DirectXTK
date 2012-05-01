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
#include "VertexTypes.h"
#include "SharedResourcePool.h"
#include "Bezier.h"
#include <vector>

using namespace DirectX;
using namespace Microsoft::WRL;


namespace
{
    // Temporary collection types used when generating the geometry.
    typedef std::vector<VertexPositionNormalTexture> VertexCollection;
    
    
    class IndexCollection : public std::vector<uint16_t>
    {
    public:
        // Sanity check the range of 16 bit index values.
        void push_back(size_t value)
        {
            // Use >=, not > comparison, because some D3D level 9_x hardware does not support 0xFFFF index values.
            if (value >= USHRT_MAX)
                throw std::exception("Index value out of range: cannot tesselate primitive so finely");

            vector::push_back((uint16_t)value);
        }
    };
}


// Internal GeometricPrimitive implementation class.
class GeometricPrimitive::Impl
{
public:
    void Initialize(_In_ ID3D11DeviceContext* deviceContext, VertexCollection const& vertices, IndexCollection const& indices);

    void Draw(CXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color, _In_opt_ ID3D11ShaderResourceView* texture, bool wireframe, _In_opt_ std::function<void()> setCustomState);

private:
    ComPtr<ID3D11Buffer> mVertexBuffer;
    ComPtr<ID3D11Buffer> mIndexBuffer;

    UINT mIndexCount;


    // Only one of these helpers is allocated per D3D device context, even if there are multiple GeometricPrimitive instances.
    class SharedResources
    {
    public:
        SharedResources(_In_ ID3D11DeviceContext* deviceContext);

        void PrepareForRendering(CXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color, _In_opt_ ID3D11ShaderResourceView* texture, bool wireframe);

        ComPtr<ID3D11DeviceContext> deviceContext;
        
    private:
        std::unique_ptr<BasicEffect> effect;
        std::unique_ptr<CommonStates> stateObjects;

        ComPtr<ID3D11InputLayout> inputLayoutTextured;
        ComPtr<ID3D11InputLayout> inputLayoutUntextured;
    };


    // Per-device-context data.
    std::shared_ptr<SharedResources> mResources;

    static SharedResourcePool<ID3D11DeviceContext*, SharedResources> sharedResourcesPool;
};


// Global pool of per-device-context GeometricPrimitive resources.
SharedResourcePool<ID3D11DeviceContext*, GeometricPrimitive::Impl::SharedResources> GeometricPrimitive::Impl::sharedResourcesPool;


// Helper for creating a D3D input layout.
static void CreateInputLayout(_In_ ID3D11Device* device, IEffect* effect, _Out_ ID3D11InputLayout** pInputLayout)
{
    void const* shaderByteCode;
    size_t byteCodeLength;

    effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    ThrowIfFailed(
        device->CreateInputLayout(VertexPositionNormalTexture::InputElements,
                                  VertexPositionNormalTexture::InputElementCount,
                                  shaderByteCode, byteCodeLength,
                                  pInputLayout)
    );
}


// Per-device-context constructor.
GeometricPrimitive::Impl::SharedResources::SharedResources(_In_ ID3D11DeviceContext* deviceContext)
  : deviceContext(deviceContext)
{
    ComPtr<ID3D11Device> device;

    deviceContext->GetDevice(&device);

    // Create the BasicEffect.
    effect.reset(new BasicEffect(device.Get()));

    effect->EnableDefaultLighting();

    // Create state objects.
    stateObjects.reset(new CommonStates(device.Get()));

    // Create input layouts.
    effect->SetTextureEnabled(true);
    CreateInputLayout(device.Get(), effect.get(), &inputLayoutTextured);

    effect->SetTextureEnabled(false);
    CreateInputLayout(device.Get(), effect.get(), &inputLayoutUntextured);
}


// Sets up D3D device state ready for drawing a primitive.
void GeometricPrimitive::Impl::SharedResources::PrepareForRendering(CXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color, _In_opt_ ID3D11ShaderResourceView* texture, bool wireframe)
{
    float alpha = XMVectorGetW(color);

    // Set the blend and depth stencil state.
    ID3D11BlendState* blendState;
    ID3D11DepthStencilState* depthStencilState;

    if (alpha < 1)
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
    deviceContext->RSSetState(wireframe ? stateObjects->Wireframe() : stateObjects->CullCounterClockwise());

    // Set the texture, sampler state, and input layout.
    if (texture)
    {
        effect->SetTextureEnabled(true);
        effect->SetTexture(texture);

        ID3D11SamplerState* samplerState = stateObjects->LinearClamp();

        deviceContext->PSSetSamplers(0, 1, &samplerState);

        deviceContext->IASetInputLayout(inputLayoutTextured.Get());
    }
    else
    {
        effect->SetTextureEnabled(false);
        
        deviceContext->IASetInputLayout(inputLayoutUntextured.Get());
    }

    // Set effect parameters.
    effect->SetWorld(world);
    effect->SetView(view);
    effect->SetProjection(projection);

    effect->SetDiffuseColor(color);
    effect->SetAlpha(alpha);

    // Activate our shaders, constant buffers, and texture.
    effect->Apply(deviceContext.Get());
}


// Helper for creating a D3D vertex or index buffer.
template<typename T>
static void CreateBuffer(_In_ ID3D11Device* device, T const& data, D3D11_BIND_FLAG bindFlags, _Out_ ID3D11Buffer** pBuffer)
{
    D3D11_BUFFER_DESC bufferDesc = { 0 };

    bufferDesc.ByteWidth = (UINT)data.size() * sizeof(T::value_type);
    bufferDesc.BindFlags = bindFlags;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA dataDesc = { 0 };

    dataDesc.pSysMem = &data.front();

    ThrowIfFailed(
        device->CreateBuffer(&bufferDesc, &dataDesc, pBuffer)
    );
}


// Initializes a geometric primitive instance that will draw the specified vertex and index data.
void GeometricPrimitive::Impl::Initialize(_In_ ID3D11DeviceContext* deviceContext, VertexCollection const& vertices, IndexCollection const& indices)
{
    mResources = sharedResourcesPool.DemandCreate(deviceContext);

    ComPtr<ID3D11Device> device;

    deviceContext->GetDevice(&device);

    CreateBuffer(device.Get(), vertices, D3D11_BIND_VERTEX_BUFFER, &mVertexBuffer);
    CreateBuffer(device.Get(), indices, D3D11_BIND_INDEX_BUFFER, &mIndexBuffer);

    mIndexCount = (UINT)indices.size();
}


// Draws the primitive.
void GeometricPrimitive::Impl::Draw(CXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color, _In_opt_ ID3D11ShaderResourceView* texture, bool wireframe, _In_opt_ std::function<void()> setCustomState)
{
    auto deviceContext = mResources->deviceContext.Get();

    // Set state objects and shaders.
    mResources->PrepareForRendering(world, view, projection, color, texture, wireframe);

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


// Constructor.
GeometricPrimitive::GeometricPrimitive()
  : pImpl(new Impl())
{
}


// Destructor.
GeometricPrimitive::~GeometricPrimitive()
{
}


// Public Draw entrypoint.
void GeometricPrimitive::Draw(CXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color, _In_opt_ ID3D11ShaderResourceView* texture, bool wireframe, _In_opt_ std::function<void()> setCustomState)
{
    pImpl->Draw(world, view, projection, color, texture, wireframe, setCustomState);
}


// Creates a cube primitive.
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateCube(_In_ ID3D11DeviceContext* deviceContext, float size)
{
    // A cube has six faces, each one pointing in a different direction.
    const int FaceCount = 6;

    static const XMVECTORF32 faceNormals[FaceCount] =
    {
        {  0,  0,  1 },
        {  0,  0, -1 },
        {  1,  0,  0 },
        { -1,  0,  0 },
        {  0,  1,  0 },
        {  0, -1,  0 },
    };

    static const XMVECTORF32 textureCoordinates[4] =
    {
        { 1, 0 },
        { 1, 1 },
        { 0, 1 },
        { 0, 0 },
    };

    VertexCollection vertices;
    IndexCollection indices;

    size /= 2;

    // Create each face in turn.
    for (int i = 0; i < FaceCount; i++)
    {
        XMVECTOR normal = faceNormals[i];

        // Get two vectors perpendicular both to the face normal and to each other.
        XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

        XMVECTOR side1 = XMVector3Cross(normal, basis);
        XMVECTOR side2 = XMVector3Cross(normal, side1);

        // Six indices (two triangles) per face.
        indices.push_back(vertices.size() + 0);
        indices.push_back(vertices.size() + 1);
        indices.push_back(vertices.size() + 2);

        indices.push_back(vertices.size() + 0);
        indices.push_back(vertices.size() + 2);
        indices.push_back(vertices.size() + 3);

        // Four vertices per face.
        vertices.push_back(VertexPositionNormalTexture((normal - side1 - side2) * size, normal, textureCoordinates[0]));
        vertices.push_back(VertexPositionNormalTexture((normal - side1 + side2) * size, normal, textureCoordinates[1]));
        vertices.push_back(VertexPositionNormalTexture((normal + side1 + side2) * size, normal, textureCoordinates[2]));
        vertices.push_back(VertexPositionNormalTexture((normal + side1 - side2) * size, normal, textureCoordinates[3]));
    }

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());
    
    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}


// Creates a sphere primitive.
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateSphere(_In_ ID3D11DeviceContext* deviceContext, float diameter, size_t tessellation)
{
    VertexCollection vertices;
    IndexCollection indices;

    if (tessellation < 3)
        throw std::out_of_range("tesselation parameter out of range");

    size_t verticalSegments = tessellation;
    size_t horizontalSegments = tessellation * 2;

    float radius = diameter / 2;

    // Create rings of vertices at progressively higher latitudes.
    for (size_t i = 0; i <= verticalSegments; i++)
    {
        float v = 1 - (float)i / verticalSegments;

        float latitude = (i * XM_PI / verticalSegments) - XM_PIDIV2;
        float dy, dxz;

        XMScalarSinCos(&dy, &dxz, latitude);

        // Create a single ring of vertices at this latitude.
        for (size_t j = 0; j <= horizontalSegments; j++)
        {
            float u = (float)j / horizontalSegments;

            float longitude = j * XM_2PI / horizontalSegments;
            float dx, dz;

            XMScalarSinCos(&dx, &dz, longitude);

            dx *= dxz;
            dz *= dxz;

            XMVECTOR normal = XMVectorSet(dx, dy, dz, 0);
            XMVECTOR textureCoordinate = XMVectorSet(u, v, 0, 0);

            vertices.push_back(VertexPositionNormalTexture(normal * radius, normal, textureCoordinate));
        }
    }

    // Fill the index buffer with triangles joining each pair of latitude rings.
    size_t stride = horizontalSegments + 1;

    for (size_t i = 0; i < verticalSegments; i++)
    {
        for (size_t j = 0; j <= horizontalSegments; j++)
        {
            size_t nextI = i + 1;
            size_t nextJ = (j + 1) % stride;

            indices.push_back(i * stride + j);
            indices.push_back(nextI * stride + j);
            indices.push_back(i * stride + nextJ);

            indices.push_back(i * stride + nextJ);
            indices.push_back(nextI * stride + j);
            indices.push_back(nextI * stride + nextJ);
        }
    }

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());
    
    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}


// Helper computes a point on a unit circle, aligned to the x/z plane and centered on the origin.
static XMVECTOR GetCircleVector(size_t i, size_t tessellation)
{
    float angle = i * XM_2PI / tessellation;
    float dx, dz;

    XMScalarSinCos(&dx, &dz, angle);

    return XMVectorPermute<0, 1, 4, 5>(XMLoadFloat(&dx), XMLoadFloat(&dz));
}


// Helper creates a triangle fan to close the end of a cylinder.
static void CreateCylinderCap(VertexCollection& vertices, IndexCollection& indices, size_t tessellation, float height, float radius, bool isTop)
{
    // Create cap indices.
    for (size_t i = 0; i < tessellation - 2; i++)
    {
        size_t i1 = (i + 1) % tessellation;
        size_t i2 = (i + 2) % tessellation;

        if (isTop)
        {
            std::swap(i1, i2);
        }

        indices.push_back(vertices.size());
        indices.push_back(vertices.size() + i1);
        indices.push_back(vertices.size() + i2);
    }

    // Which end of the cylinder is this?
    XMVECTOR normal = g_XMIdentityR1;
    XMVECTOR textureScale = g_XMNegativeOneHalf;

    if (!isTop)
    {
        normal = -normal;
        textureScale *= g_XMNegateX;
    }

    // Create cap vertices.
    for (size_t i = 0; i < tessellation; i++)
    {
        XMVECTOR circleVector = GetCircleVector(i, tessellation);

        XMVECTOR position = (circleVector * radius) + (normal * height);

        XMVECTOR textureCoordinate = XMVectorMultiplyAdd(XMVectorSwizzle<0, 2, 3, 3>(circleVector), textureScale, g_XMOneHalf);

        vertices.push_back(VertexPositionNormalTexture(position, normal, textureCoordinate));
    }
}


// Creates a cylinder primitive.
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateCylinder(_In_ ID3D11DeviceContext* deviceContext, float height, float diameter, size_t tessellation)
{
    VertexCollection vertices;
    IndexCollection indices;

    if (tessellation < 3)
        throw std::out_of_range("tesselation parameter out of range");

    height /= 2;

    XMVECTOR topOffset = g_XMIdentityR1 * height;

    float radius = diameter / 2;
    size_t stride = tessellation + 1;

    // Create a ring of triangles around the outside of the cylinder.
    for (size_t i = 0; i <= tessellation; i++)
    {
        XMVECTOR normal = GetCircleVector(i, tessellation);

        XMVECTOR sideOffset = normal * radius;

        float u = (float)i / tessellation;

        XMVECTOR textureCoordinate = XMLoadFloat(&u);

        vertices.push_back(VertexPositionNormalTexture(sideOffset + topOffset, normal, textureCoordinate));
        vertices.push_back(VertexPositionNormalTexture(sideOffset - topOffset, normal, textureCoordinate + g_XMIdentityR1));

        indices.push_back(i * 2);
        indices.push_back((i * 2 + 2) % (stride * 2));
        indices.push_back(i * 2 + 1);

        indices.push_back(i * 2 + 1);
        indices.push_back((i * 2 + 2) % (stride * 2));
        indices.push_back((i * 2 + 3) % (stride * 2));
    }

    // Create flat triangle fan caps to seal the top and bottom.
    CreateCylinderCap(vertices, indices, tessellation, height, radius, true);
    CreateCylinderCap(vertices, indices, tessellation, height, radius, false);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());
    
    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}


// Creates a torus primitive.
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateTorus(_In_ ID3D11DeviceContext* deviceContext, float diameter, float thickness, size_t tessellation)
{
    VertexCollection vertices;
    IndexCollection indices;

    if (tessellation < 3)
        throw std::out_of_range("tesselation parameter out of range");

    size_t stride = tessellation + 1;

    // First we loop around the main ring of the torus.
    for (size_t i = 0; i <= tessellation; i++)
    {
        float u = (float)i / tessellation;

        float outerAngle = i * XM_2PI / tessellation - XM_PIDIV2;

        // Create a transform matrix that will align geometry to
        // slice perpendicularly though the current ring position.
        XMMATRIX transform = XMMatrixTranslation(diameter / 2, 0, 0) * XMMatrixRotationY(outerAngle);

        // Now we loop along the other axis, around the side of the tube.
        for (size_t j = 0; j <= tessellation; j++)
        {
            float v = 1 - (float)j / tessellation;

            float innerAngle = j * XM_2PI / tessellation + XM_PI;
            float dx, dy;

            XMScalarSinCos(&dy, &dx, innerAngle);

            // Create a vertex.
            XMVECTOR normal = XMVectorSet(dx, dy, 0, 0);
            XMVECTOR position = normal * thickness / 2;
            XMVECTOR textureCoordinate = XMVectorSet(u, v, 0, 0);

            position = XMVector3Transform(position, transform);
            normal = XMVector3TransformNormal(normal, transform);

            vertices.push_back(VertexPositionNormalTexture(position, normal, textureCoordinate));

            // And create indices for two triangles.
            size_t nextI = (i + 1) % stride;
            size_t nextJ = (j + 1) % stride;

            indices.push_back(i * stride + j);
            indices.push_back(i * stride + nextJ);
            indices.push_back(nextI * stride + j);

            indices.push_back(i * stride + nextJ);
            indices.push_back(nextI * stride + nextJ);
            indices.push_back(nextI * stride + j);
        }
    }

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());
    
    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}


// Include the teapot control point data.
namespace
{
    #include "TeapotData.inc"
}


// Tessellates the specified bezier patch.
static void TessellatePatch(VertexCollection& vertices, IndexCollection& indices, TeapotPatch const& patch, size_t tessellation, FXMVECTOR scale, bool isMirrored)
{
    // Look up the 16 control points for this patch.
    XMVECTOR controlPoints[16];

    for (int i = 0; i < 16; i++)
    {
        controlPoints[i] = TeapotControlPoints[patch.indices[i]] * scale;
    }

    // Create the index data.
    Bezier::CreatePatchIndices(tessellation, isMirrored, [&](size_t index)
    {
        indices.push_back(vertices.size() + index);
    });

    // Create the vertex data.
    Bezier::CreatePatchVertices(controlPoints, tessellation, isMirrored, [&](FXMVECTOR position, FXMVECTOR normal, FXMVECTOR textureCoordinate)
    {
        vertices.push_back(VertexPositionNormalTexture(position, normal, textureCoordinate));
    });
}

        
// Creates a teapot primitive.
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateTeapot(_In_ ID3D11DeviceContext* deviceContext, float size, size_t tessellation)
{
    VertexCollection vertices;
    IndexCollection indices;

    if (tessellation < 1)
        throw std::out_of_range("tesselation parameter out of range");

    XMVECTOR scaleVector = XMVectorReplicate(size);

    XMVECTOR scaleNegateX = scaleVector * g_XMNegateX;
    XMVECTOR scaleNegateZ = scaleVector * g_XMNegateZ;
    XMVECTOR scaleNegateXZ = scaleVector * g_XMNegateX * g_XMNegateZ;

    for (int i = 0; i < sizeof(TeapotPatches) / sizeof(TeapotPatches[0]); i++)
    {
        TeapotPatch const& patch = TeapotPatches[i];

        // Because the teapot is symmetrical from left to right, we only store
        // data for one side, then tessellate each patch twice, mirroring in X.
        TessellatePatch(vertices, indices, patch, tessellation, scaleVector, false);
        TessellatePatch(vertices, indices, patch, tessellation, scaleNegateX, true);

        if (patch.mirrorZ)
        {
            // Some parts of the teapot (the body, lid, and rim, but not the
            // handle or spout) are also symmetrical from front to back, so
            // we tessellate them four times, mirroring in Z as well as X.
            TessellatePatch(vertices, indices, patch, tessellation, scaleNegateZ, true);
            TessellatePatch(vertices, indices, patch, tessellation, scaleNegateXZ, false);
        }
    }

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());
    
    primitive->pImpl->Initialize(deviceContext, vertices, indices);

    return primitive;
}
