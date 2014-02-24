//--------------------------------------------------------------------------------------
// File: PrimitiveBatch.cpp
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
#include "PrimitiveBatch.h"
#include "DirectXHelpers.h"
#include "PlatformHelpers.h"

using namespace DirectX;
using namespace DirectX::Internal;
using namespace Microsoft::WRL;


// Internal PrimitiveBatch implementation class.
class PrimitiveBatchBase::Impl
{
public:
    Impl(_In_ ID3D11DeviceContext* deviceContext, size_t maxIndices, size_t maxVertices, size_t vertexSize);

    void Begin();
    void End();

    void Draw(D3D11_PRIMITIVE_TOPOLOGY topology, bool isIndexed, _In_opt_count_(indexCount) uint16_t const* indices, size_t indexCount, size_t vertexCount, _Out_ void** pMappedVertices);

private:
    void FlushBatch();

    ComPtr<ID3D11DeviceContext> mDeviceContext;
    ComPtr<ID3D11Buffer> mIndexBuffer;
    ComPtr<ID3D11Buffer> mVertexBuffer;

    size_t mMaxIndices;
    size_t mMaxVertices;
    size_t mVertexSize;

    bool mInBeginEndPair;
    
    D3D11_PRIMITIVE_TOPOLOGY mCurrentTopology;
    bool mCurrentlyIndexed;

    size_t mCurrentIndex;
    size_t mCurrentVertex;
    
    size_t mBaseIndex;
    size_t mBaseVertex;

    D3D11_MAPPED_SUBRESOURCE mMappedIndices;
    D3D11_MAPPED_SUBRESOURCE mMappedVertices;
};


// Helper for creating a D3D vertex or index buffer.
static void CreateBuffer(_In_ ID3D11Device* device, size_t bufferSize, D3D11_BIND_FLAG bindFlag, _Out_ ID3D11Buffer** pBuffer)
{
    D3D11_BUFFER_DESC desc = { 0 };

    desc.ByteWidth = (UINT)bufferSize;
    desc.BindFlags = bindFlag;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ThrowIfFailed(
        device->CreateBuffer(&desc, nullptr, pBuffer)
    );

    SetDebugObjectName(*pBuffer, "DirectXTK:PrimitiveBatch");
}


// Constructor.
PrimitiveBatchBase::Impl::Impl(_In_ ID3D11DeviceContext* deviceContext, size_t maxIndices, size_t maxVertices, size_t vertexSize)
  : mDeviceContext(deviceContext),
    mMaxIndices(maxIndices),
    mMaxVertices(maxVertices),
    mVertexSize(vertexSize),
    mInBeginEndPair(false),
    mCurrentTopology(D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED),
    mCurrentlyIndexed(false),
    mCurrentIndex(0),
    mCurrentVertex(0),
    mBaseIndex(0),
    mBaseVertex(0)
{
    ComPtr<ID3D11Device> device;
    
    deviceContext->GetDevice(&device);

    // If you only intend to draw non-indexed geometry, specify maxIndices = 0 to skip creating the index buffer.
    if (maxIndices > 0)
    {
        CreateBuffer(device.Get(), maxIndices * sizeof(uint16_t), D3D11_BIND_INDEX_BUFFER, &mIndexBuffer);
    }

    // Create the vertex buffer.
    CreateBuffer(device.Get(), maxVertices * vertexSize, D3D11_BIND_VERTEX_BUFFER, &mVertexBuffer);
}


// Begins a batch of primitive drawing operations.
void PrimitiveBatchBase::Impl::Begin()
{
    if (mInBeginEndPair)
        throw std::exception("Cannot nest Begin calls");

    // Bind the index buffer.
    if (mMaxIndices > 0)
    {
        mDeviceContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    }

    // Bind the vertex buffer.
    auto vertexBuffer = mVertexBuffer.Get();
    UINT vertexStride = (UINT)mVertexSize;
    UINT vertexOffset = 0;

    mDeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexStride, &vertexOffset);
     
    // If this is a deferred D3D context, reset position so the first Map calls will use D3D11_MAP_WRITE_DISCARD.
    if (mDeviceContext->GetType() == D3D11_DEVICE_CONTEXT_DEFERRED)
    {
        mCurrentIndex = 0;
        mCurrentVertex = 0;
    }

    mInBeginEndPair = true;
}


// Ends a batch of primitive drawing operations.
void PrimitiveBatchBase::Impl::End()
{
    if (!mInBeginEndPair)
        throw std::exception("Begin must be called before End");

    FlushBatch();

    mInBeginEndPair = false;
}


// Can we combine adjacent primitives using this topology into a single draw call?
static bool CanBatchPrimitives(D3D11_PRIMITIVE_TOPOLOGY topology)
{
    switch (topology)
    {
        case D3D11_PRIMITIVE_TOPOLOGY_POINTLIST:
        case D3D11_PRIMITIVE_TOPOLOGY_LINELIST:
        case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
            // Lists can easily be merged.
            return true;

        default:
            // Strips cannot.
            return false;
    }

    // We could also merge indexed strips by inserting degenerates,
    // but that's not always a perf win, so let's keep things simple.
}


// Helper for locking a vertex or index buffer.
static void LockBuffer(_In_ ID3D11DeviceContext* deviceContext, _In_ ID3D11Buffer* buffer, size_t currentPosition, _Out_ size_t* basePosition, _Out_ D3D11_MAPPED_SUBRESOURCE* mappedResource)
{
    D3D11_MAP mapType = (currentPosition == 0) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

    ThrowIfFailed(
        deviceContext->Map(buffer, 0, mapType, 0, mappedResource)
    );

    *basePosition = currentPosition;
}


// Adds new geometry to the batch.
void PrimitiveBatchBase::Impl::Draw(D3D11_PRIMITIVE_TOPOLOGY topology, bool isIndexed, _In_opt_count_(indexCount) uint16_t const* indices, size_t indexCount, size_t vertexCount, _Out_ void** pMappedVertices)
{
    if (isIndexed && !indices)
        throw std::exception("Indices cannot be null");

    if (indexCount >= mMaxIndices)
        throw std::exception("Too many indices");

    if (vertexCount >= mMaxVertices)
        throw std::exception("Too many vertices");

    if (!mInBeginEndPair)
        throw std::exception("Begin must be called before Draw");

    // Can we merge this primitive in with an existing batch, or must we flush first?
    bool wrapIndexBuffer = (mCurrentIndex + indexCount > mMaxIndices);
    bool wrapVertexBuffer = (mCurrentVertex + vertexCount > mMaxVertices);

    if ((topology != mCurrentTopology) ||
        (isIndexed != mCurrentlyIndexed) ||
        !CanBatchPrimitives(topology) ||
        wrapIndexBuffer || wrapVertexBuffer)
    {
        FlushBatch();
    }

    if (wrapIndexBuffer)
        mCurrentIndex = 0;

    if (wrapVertexBuffer)
        mCurrentVertex = 0;

    // If we are not already in a batch, lock the buffers.
    if (mCurrentTopology == D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED)
    {
        if (isIndexed)
        {
            LockBuffer(mDeviceContext.Get(), mIndexBuffer.Get(), mCurrentIndex, &mBaseIndex, &mMappedIndices);
        }

        LockBuffer(mDeviceContext.Get(), mVertexBuffer.Get(), mCurrentVertex, &mBaseVertex, &mMappedVertices);

        mCurrentTopology = topology;
        mCurrentlyIndexed = isIndexed;
    }
    
    // Copy over the index data.
    if (isIndexed)
    {
        uint16_t* outputIndices = (uint16_t*)mMappedIndices.pData + mCurrentIndex;
        
        for (size_t i = 0; i < indexCount; i++)
        {
            outputIndices[i] = (uint16_t)(indices[i] + mCurrentVertex - mBaseVertex);
        }
 
        mCurrentIndex += indexCount;
    }

    // Return the output vertex data location.
    *pMappedVertices = (uint8_t*)mMappedVertices.pData + (mCurrentVertex * mVertexSize);

    mCurrentVertex += vertexCount;
}


// Sends queued primitives to the graphics device.
void PrimitiveBatchBase::Impl::FlushBatch()
{
    // Early out if there is nothing to flush.
    if (mCurrentTopology == D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED)
        return;

    mDeviceContext->IASetPrimitiveTopology(mCurrentTopology);

    mDeviceContext->Unmap(mVertexBuffer.Get(), 0);

    if (mCurrentlyIndexed)
    {
        // Draw indexed geometry.
        mDeviceContext->Unmap(mIndexBuffer.Get(), 0);

        mDeviceContext->DrawIndexed((UINT)(mCurrentIndex - mBaseIndex), (UINT)mBaseIndex, (UINT)mBaseVertex);
    }
    else
    {
        // Draw non-indexed geometry.
        mDeviceContext->Draw((UINT)(mCurrentVertex - mBaseVertex), (UINT)mBaseVertex);
    }

    mCurrentTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}


// Public constructor.
PrimitiveBatchBase::PrimitiveBatchBase(_In_ ID3D11DeviceContext* deviceContext, size_t maxIndices, size_t maxVertices, size_t vertexSize)
  : pImpl(new Impl(deviceContext, maxIndices, maxVertices, vertexSize))
{
}


// Move constructor.
PrimitiveBatchBase::PrimitiveBatchBase(PrimitiveBatchBase&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
PrimitiveBatchBase& PrimitiveBatchBase::operator= (PrimitiveBatchBase&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
PrimitiveBatchBase::~PrimitiveBatchBase()
{
}


void PrimitiveBatchBase::Begin()
{
    pImpl->Begin();
}


void PrimitiveBatchBase::End()
{
    pImpl->End();
}


void PrimitiveBatchBase::Draw(D3D11_PRIMITIVE_TOPOLOGY topology, bool isIndexed, _In_opt_count_(indexCount) uint16_t const* indices, size_t indexCount, size_t vertexCount, _Out_ void** pMappedVertices)
{
    pImpl->Draw(topology, isIndexed, indices, indexCount, vertexCount, pMappedVertices);
}
