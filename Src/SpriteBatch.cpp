//--------------------------------------------------------------------------------------
// File: SpriteBatch.cpp
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

#include "SpriteBatch.h"
#include "ConstantBuffer.h"
#include "CommonStates.h"
#include "VertexTypes.h"
#include "SharedResourcePool.h"
#include "AlignedNew.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
    // Include the precompiled shader code.
    #if defined(_XBOX_ONE) && defined(_TITLE)
    #include "Shaders/Compiled/XboxOneSpriteEffect_SpriteVertexShader.inc"
    #include "Shaders/Compiled/XboxOneSpriteEffect_SpritePixelShader.inc"
    #else
    #include "Shaders/Compiled/SpriteEffect_SpriteVertexShader.inc"
    #include "Shaders/Compiled/SpriteEffect_SpritePixelShader.inc"
    #endif


    // Helper looks up the D3D device corresponding to a context interface.
    inline ComPtr<ID3D11Device> GetDevice(_In_ ID3D11DeviceContext* deviceContext)
    {
        ComPtr<ID3D11Device> device;
    
        deviceContext->GetDevice(&device);
        
        return device;
    }


    // Helper converts a RECT to XMVECTOR.
    inline XMVECTOR LoadRect(_In_ RECT const* rect)
    {
        XMVECTOR v = XMLoadInt4(reinterpret_cast<uint32_t const*>(rect));

        v = XMConvertVectorIntToFloat(v, 0);

        // Convert right/bottom to width/height.
        v -= XMVectorPermute<0, 1, 4, 5>(XMVectorZero(), v);

        return v;
    }
}


// Internal SpriteBatch implementation class.
__declspec(align(16)) class SpriteBatch::Impl : public AlignedNew<SpriteBatch::Impl>
{
public:
    Impl(_In_ ID3D11DeviceContext* deviceContext);

    void XM_CALLCONV Begin(SpriteSortMode sortMode,
        _In_opt_ ID3D11BlendState* blendState,
        _In_opt_ ID3D11SamplerState* samplerState,
        _In_opt_ ID3D11DepthStencilState* depthStencilState,
        _In_opt_ ID3D11RasterizerState* rasterizerState,
        _In_opt_ std::function<void()> setCustomShaders,
        FXMMATRIX transformMatrix);
    void End();

    void XM_CALLCONV Draw(_In_ ID3D11ShaderResourceView* texture,
        FXMVECTOR destination,
        _In_opt_ RECT const* sourceRectangle,
        FXMVECTOR color,
        FXMVECTOR originRotationDepth,
        int flags);


    // Info about a single sprite that is waiting to be drawn.
    __declspec(align(16)) struct SpriteInfo : public AlignedNew<SpriteInfo>
    {
        XMFLOAT4A source;
        XMFLOAT4A destination;
        XMFLOAT4A color;
        XMFLOAT4A originRotationDepth;
        ID3D11ShaderResourceView* texture;
        int flags;


        // Combine values from the public SpriteEffects enum with these internal-only flags.
        static const int SourceInTexels = 4;
        static const int DestSizeInPixels = 8;

        static_assert((SpriteEffects_FlipBoth & (SourceInTexels | DestSizeInPixels)) == 0, "Flag bits must not overlap");
    };

    DXGI_MODE_ROTATION mRotation;

    bool mSetViewport;
    D3D11_VIEWPORT mViewPort;

private:
    // Implementation helper methods.
    void GrowSpriteQueue();
    void PrepareForRendering();
    void FlushBatch();
    void SortSprites();
    void GrowSortedSprites();

    void RenderBatch(_In_ ID3D11ShaderResourceView* texture, _In_reads_(count) SpriteInfo const* const* sprites, size_t count);

    static void XM_CALLCONV RenderSprite(_In_ SpriteInfo const* sprite,
        _Out_writes_(VerticesPerSprite) VertexPositionColorTexture* vertices,
        FXMVECTOR textureSize,
        FXMVECTOR inverseTextureSize);

    static XMVECTOR GetTextureSize(_In_ ID3D11ShaderResourceView* texture);
    XMMATRIX GetViewportTransform(_In_ ID3D11DeviceContext* deviceContext, DXGI_MODE_ROTATION rotation );


    // Constants.
    static const size_t MaxBatchSize = 2048;
    static const size_t MinBatchSize = 128;
    static const size_t InitialQueueSize = 64;
    static const size_t VerticesPerSprite = 4;
    static const size_t IndicesPerSprite = 6;


    // Queue of sprites waiting to be drawn.
    std::unique_ptr<SpriteInfo[]> mSpriteQueue;

    size_t mSpriteQueueCount;
    size_t mSpriteQueueArraySize;


    // To avoid needlessly copying around bulky SpriteInfo structures, we leave that
    // actual data alone and just sort this array of pointers instead. But we want contiguous
    // memory for cache efficiency, so these pointers are just shortcuts into the single
    // mSpriteQueue array, and we take care to keep them in order when sorting is disabled.
    std::vector<SpriteInfo const*> mSortedSprites;


    // If each SpriteInfo instance held a refcount on its texture, could end up with
    // many redundant AddRef/Release calls on the same object, so instead we use
    // this separate list to hold just a single refcount each time we change texture.
    std::vector<ComPtr<ID3D11ShaderResourceView>> mSpriteTextureReferences;


    // Mode settings from the last Begin call.
    bool mInBeginEndPair;

    SpriteSortMode mSortMode;
    ComPtr<ID3D11BlendState> mBlendState;
    ComPtr<ID3D11SamplerState> mSamplerState;
    ComPtr<ID3D11DepthStencilState> mDepthStencilState;
    ComPtr<ID3D11RasterizerState> mRasterizerState;
    std::function<void()> mSetCustomShaders;
    XMMATRIX mTransformMatrix;


    // Only one of these helpers is allocated per D3D device, even if there are multiple SpriteBatch instances.
    struct DeviceResources
    {
        DeviceResources(_In_ ID3D11Device* device);

        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;
        ComPtr<ID3D11InputLayout> inputLayout;
        ComPtr<ID3D11Buffer> indexBuffer;

        CommonStates stateObjects;

    private:
        void CreateShaders(_In_ ID3D11Device* device);
        void CreateIndexBuffer(_In_ ID3D11Device* device);

        static std::vector<short> CreateIndexValues();
    };


    // Only one of these helpers is allocated per D3D device context, even if there are multiple SpriteBatch instances.
    struct ContextResources
    {
        ContextResources(_In_ ID3D11DeviceContext* deviceContext);

#if defined(_XBOX_ONE) && defined(_TITLE)
        ComPtr<ID3D11DeviceContextX> deviceContext;
#else
        ComPtr<ID3D11DeviceContext> deviceContext;
#endif

        ComPtr<ID3D11Buffer> vertexBuffer;

        ConstantBuffer<XMMATRIX> constantBuffer;

        size_t vertexBufferPosition;

        bool inImmediateMode;

    private:
        void CreateVertexBuffer();
    };


    // Per-device and per-context data.
    std::shared_ptr<DeviceResources> mDeviceResources;
    std::shared_ptr<ContextResources> mContextResources;

    static SharedResourcePool<ID3D11Device*, DeviceResources> deviceResourcesPool;
    static SharedResourcePool<ID3D11DeviceContext*, ContextResources> contextResourcesPool;
};


// Global pools of per-device and per-context SpriteBatch resources.
SharedResourcePool<ID3D11Device*, SpriteBatch::Impl::DeviceResources> SpriteBatch::Impl::deviceResourcesPool;
SharedResourcePool<ID3D11DeviceContext*, SpriteBatch::Impl::ContextResources> SpriteBatch::Impl::contextResourcesPool;


// Constants.
const XMMATRIX SpriteBatch::MatrixIdentity = XMMatrixIdentity();
const XMFLOAT2 SpriteBatch::Float2Zero(0, 0);

// Per-device constructor.
SpriteBatch::Impl::DeviceResources::DeviceResources(_In_ ID3D11Device* device)
  : stateObjects(device)
{
    CreateShaders(device);
    CreateIndexBuffer(device);
}


// Creates the SpriteBatch shaders and input layout.
void SpriteBatch::Impl::DeviceResources::CreateShaders(_In_ ID3D11Device* device)
{
    ThrowIfFailed(
        device->CreateVertexShader(SpriteEffect_SpriteVertexShader,
                                   sizeof(SpriteEffect_SpriteVertexShader),
                                   nullptr,
                                   &vertexShader)
    );
    
    ThrowIfFailed(
        device->CreatePixelShader(SpriteEffect_SpritePixelShader,
                                  sizeof(SpriteEffect_SpritePixelShader),
                                  nullptr,
                                  &pixelShader)
    );

    ThrowIfFailed(
        device->CreateInputLayout(VertexPositionColorTexture::InputElements,
                                  VertexPositionColorTexture::InputElementCount,
                                  SpriteEffect_SpriteVertexShader,
                                  sizeof(SpriteEffect_SpriteVertexShader),
                                  &inputLayout)
    );

    SetDebugObjectName(vertexShader.Get(), "DirectXTK:SpriteBatch");
    SetDebugObjectName(pixelShader.Get(),  "DirectXTK:SpriteBatch");
    SetDebugObjectName(inputLayout.Get(),  "DirectXTK:SpriteBatch");
}


// Creates the SpriteBatch index buffer.
void SpriteBatch::Impl::DeviceResources::CreateIndexBuffer(_In_ ID3D11Device* device)
{
    D3D11_BUFFER_DESC indexBufferDesc = {};

    static_assert( ( MaxBatchSize * VerticesPerSprite ) < USHRT_MAX, "MaxBatchSize too large for 16-bit indices" );

    indexBufferDesc.ByteWidth = sizeof(short) * MaxBatchSize * IndicesPerSprite;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    auto indexValues = CreateIndexValues();

    D3D11_SUBRESOURCE_DATA indexDataDesc = {};

    indexDataDesc.pSysMem = indexValues.data();

    ThrowIfFailed(
        device->CreateBuffer(&indexBufferDesc, &indexDataDesc, &indexBuffer)
    );

    SetDebugObjectName(indexBuffer.Get(), "DirectXTK:SpriteBatch");
}


// Helper for populating the SpriteBatch index buffer.
std::vector<short> SpriteBatch::Impl::DeviceResources::CreateIndexValues()
{
    std::vector<short> indices;

    indices.reserve(MaxBatchSize * IndicesPerSprite);

    for (short i = 0; i < MaxBatchSize * VerticesPerSprite; i += VerticesPerSprite)
    {
        indices.push_back(i);
        indices.push_back(i + 1);
        indices.push_back(i + 2);

        indices.push_back(i + 1);
        indices.push_back(i + 3);
        indices.push_back(i + 2);
    }

    return indices;
}


// Per-context constructor.
SpriteBatch::Impl::ContextResources::ContextResources(_In_ ID3D11DeviceContext* context)
  :constantBuffer(GetDevice(context).Get()),
    vertexBufferPosition(0),
    inImmediateMode(false)
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    ThrowIfFailed(context->QueryInterface(IID_GRAPHICS_PPV_ARGS(deviceContext.GetAddressOf())));
#else
    deviceContext = context;
#endif

    CreateVertexBuffer();
}


// Creates the SpriteBatch vertex buffer.
void SpriteBatch::Impl::ContextResources::CreateVertexBuffer()
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    D3D11_BUFFER_DESC vertexBufferDesc = {};

    vertexBufferDesc.ByteWidth = sizeof(VertexPositionColorTexture) * MaxBatchSize * VerticesPerSprite;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    auto device = GetDevice(deviceContext.Get());

    ComPtr<ID3D11DeviceX> deviceX;
    ThrowIfFailed(device.As(&deviceX));

    ThrowIfFailed(
        deviceX->CreatePlacementBuffer(&vertexBufferDesc, nullptr, &vertexBuffer)
        );

    SetDebugObjectName(vertexBuffer.Get(), "DirectXTK:SpriteBatch");
#else
    D3D11_BUFFER_DESC vertexBufferDesc = {};

    vertexBufferDesc.ByteWidth = sizeof(VertexPositionColorTexture) * MaxBatchSize * VerticesPerSprite;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ThrowIfFailed(
        GetDevice(deviceContext.Get())->CreateBuffer(&vertexBufferDesc, nullptr, &vertexBuffer)
    );

    SetDebugObjectName(vertexBuffer.Get(), "DirectXTK:SpriteBatch");
#endif
}


// Per-SpriteBatch constructor.
SpriteBatch::Impl::Impl(_In_ ID3D11DeviceContext* deviceContext)
  : mRotation( DXGI_MODE_ROTATION_IDENTITY ),
    mSetViewport(false),
    mSpriteQueueCount(0),
    mSpriteQueueArraySize(0),
    mInBeginEndPair(false),
    mSortMode(SpriteSortMode_Deferred),
    mTransformMatrix(MatrixIdentity),
    mDeviceResources(deviceResourcesPool.DemandCreate(GetDevice(deviceContext).Get())),
    mContextResources(contextResourcesPool.DemandCreate(deviceContext))
{
}


// Begins a batch of sprite drawing operations.
_Use_decl_annotations_
void XM_CALLCONV SpriteBatch::Impl::Begin(SpriteSortMode sortMode,
    ID3D11BlendState* blendState,
    ID3D11SamplerState* samplerState,
    ID3D11DepthStencilState* depthStencilState,
    ID3D11RasterizerState* rasterizerState,
    std::function<void()> setCustomShaders,
    FXMMATRIX transformMatrix)
{
    if (mInBeginEndPair)
        throw std::exception("Cannot nest Begin calls on a single SpriteBatch");

    mSortMode = sortMode;
    mBlendState = blendState;
    mSamplerState = samplerState;
    mDepthStencilState = depthStencilState;
    mRasterizerState = rasterizerState;
    mSetCustomShaders = setCustomShaders;
    mTransformMatrix = transformMatrix;

    if (sortMode == SpriteSortMode_Immediate)
    {
        // If we are in immediate mode, set device state ready for drawing.
        if (mContextResources->inImmediateMode)
            throw std::exception("Only one SpriteBatch at a time can use SpriteSortMode_Immediate");

        PrepareForRendering();

        mContextResources->inImmediateMode = true;
    }
            
    mInBeginEndPair = true;
}


// Ends a batch of sprite drawing operations.
void SpriteBatch::Impl::End()
{
    if (!mInBeginEndPair)
        throw std::exception("Begin must be called before End");

    if (mSortMode == SpriteSortMode_Immediate)
    {
        // If we are in immediate mode, sprites have already been drawn.
        mContextResources->inImmediateMode = false;
    }
    else
    {
        // Draw the queued sprites now.
        if (mContextResources->inImmediateMode)
            throw std::exception("Cannot end one SpriteBatch while another is using SpriteSortMode_Immediate");

        PrepareForRendering();
        FlushBatch();
    }

    // Break circular reference chains, in case the state lambda closed
    // over an object that holds a reference to this SpriteBatch.
    mSetCustomShaders = nullptr;

    mInBeginEndPair = false;
}


// Adds a single sprite to the queue.
_Use_decl_annotations_
void XM_CALLCONV SpriteBatch::Impl::Draw(ID3D11ShaderResourceView* texture,
    FXMVECTOR destination,
    RECT const* sourceRectangle,
    FXMVECTOR color,
    FXMVECTOR originRotationDepth,
    int flags)
{
    if (!texture)
        throw std::exception("Texture cannot be null");

    if (!mInBeginEndPair)
        throw std::exception("Begin must be called before Draw");

    // Get a pointer to the output sprite.
    if (mSpriteQueueCount >= mSpriteQueueArraySize)
    {
        GrowSpriteQueue();
    }

    SpriteInfo* sprite = &mSpriteQueue[mSpriteQueueCount];

    XMVECTOR dest = destination;

    if (sourceRectangle)
    {
        // User specified an explicit source region.
        XMVECTOR source = LoadRect(sourceRectangle);

        XMStoreFloat4A(&sprite->source, source);

        // If the destination size is relative to the source region, convert it to pixels.
        if (!(flags & SpriteInfo::DestSizeInPixels))
        {
            dest = XMVectorPermute<0, 1, 6, 7>(dest, dest * source); // dest.zw *= source.zw
        }

        flags |= SpriteInfo::SourceInTexels | SpriteInfo::DestSizeInPixels;
    }
    else
    {
        // No explicit source region, so use the entire texture.
        static const XMVECTORF32 wholeTexture = { 0, 0, 1, 1 };

        XMStoreFloat4A(&sprite->source, wholeTexture);
    }

    // Store sprite parameters.
    XMStoreFloat4A(&sprite->destination, dest);
    XMStoreFloat4A(&sprite->color, color);
    XMStoreFloat4A(&sprite->originRotationDepth, originRotationDepth);

    sprite->texture = texture;
    sprite->flags = flags;

    if (mSortMode == SpriteSortMode_Immediate)
    {
        // If we are in immediate mode, draw this sprite straight away.
        RenderBatch(texture, &sprite, 1);
    }
    else
    {
        // Queue this sprite for later sorting and batched rendering.
        mSpriteQueueCount++;

        // Make sure we hold a refcount on this texture until the sprite has been drawn. Only checking the
        // back of the vector means we will add duplicate references if the caller switches back and forth
        // between multiple repeated textures, but calling AddRef more times than strictly necessary hurts
        // nothing, and is faster than scanning the whole list or using a map to detect all duplicates.
        if (mSpriteTextureReferences.empty() || texture != mSpriteTextureReferences.back().Get())
        {
            mSpriteTextureReferences.emplace_back(texture);
        }
    }
}


// Dynamically expands the array used to store pending sprite information.
void SpriteBatch::Impl::GrowSpriteQueue()
{
    // Grow by a factor of 2.
    size_t newSize = std::max(InitialQueueSize, mSpriteQueueArraySize * 2);

    // Allocate the new array.
    std::unique_ptr<SpriteInfo[]> newArray(new SpriteInfo[newSize]);

    // Copy over any existing sprites.
    for (size_t i = 0; i < mSpriteQueueCount; i++)
    {
        newArray[i] = mSpriteQueue[i];
    }

    // Replace the previous array with the new one.
    mSpriteQueue = std::move(newArray);
    mSpriteQueueArraySize = newSize;

    // Clear any dangling SpriteInfo pointers left over from previous rendering.
    mSortedSprites.clear();
}


// Sets up D3D device state ready for drawing sprites.
void SpriteBatch::Impl::PrepareForRendering()
{
    auto deviceContext = mContextResources->deviceContext.Get();

    // Set state objects.
    auto blendState        = mBlendState        ? mBlendState.Get()        : mDeviceResources->stateObjects.AlphaBlend();
    auto depthStencilState = mDepthStencilState ? mDepthStencilState.Get() : mDeviceResources->stateObjects.DepthNone();
    auto rasterizerState   = mRasterizerState   ? mRasterizerState.Get()   : mDeviceResources->stateObjects.CullCounterClockwise();
    auto samplerState      = mSamplerState      ? mSamplerState.Get()      : mDeviceResources->stateObjects.LinearClamp();

    deviceContext->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
    deviceContext->OMSetDepthStencilState(depthStencilState, 0);
    deviceContext->RSSetState(rasterizerState);
    deviceContext->PSSetSamplers(0, 1, &samplerState);

    // Set shaders.
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->IASetInputLayout(mDeviceResources->inputLayout.Get());
    deviceContext->VSSetShader(mDeviceResources->vertexShader.Get(), nullptr, 0);
    deviceContext->PSSetShader(mDeviceResources->pixelShader.Get(), nullptr, 0);

    // Set the vertex and index buffer.
#if !defined(_XBOX_ONE) || !defined(_TITLE)
    auto vertexBuffer = mContextResources->vertexBuffer.Get();
    UINT vertexStride = sizeof(VertexPositionColorTexture);
    UINT vertexOffset = 0;

    deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexStride, &vertexOffset);
#endif

    deviceContext->IASetIndexBuffer(mDeviceResources->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    // Set the transform matrix.
    XMMATRIX transformMatrix = (mRotation == DXGI_MODE_ROTATION_UNSPECIFIED)
                               ? mTransformMatrix
                               : ( mTransformMatrix * GetViewportTransform(deviceContext, mRotation) );

#if defined(_XBOX_ONE) && defined(_TITLE)
    void* grfxMemory;
    mContextResources->constantBuffer.SetData(deviceContext, transformMatrix, &grfxMemory);

    deviceContext->VSSetPlacementConstantBuffer( 0, mContextResources->constantBuffer.GetBuffer(), grfxMemory );
#else
    mContextResources->constantBuffer.SetData(deviceContext, transformMatrix);

    ID3D11Buffer* constantBuffer = mContextResources->constantBuffer.GetBuffer();

    deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
#endif

    // If this is a deferred D3D context, reset position so the first Map call will use D3D11_MAP_WRITE_DISCARD.
    if (deviceContext->GetType() == D3D11_DEVICE_CONTEXT_DEFERRED)
    {
        mContextResources->vertexBufferPosition = 0;
    }

    // Hook lets the caller replace our settings with their own custom shaders.
    if (mSetCustomShaders)
    {
        mSetCustomShaders();
    }
}


// Sends queued sprites to the graphics device.
void SpriteBatch::Impl::FlushBatch()
{
    if (!mSpriteQueueCount)
        return;

    SortSprites();

    // Walk through the sorted sprite list, looking for adjacent entries that share a texture.
    ID3D11ShaderResourceView* batchTexture = nullptr;
    size_t batchStart = 0;

    for (size_t pos = 0; pos < mSpriteQueueCount; pos++)
    {
        ID3D11ShaderResourceView* texture = mSortedSprites[pos]->texture;

        _Analysis_assume_(texture != nullptr);

        // Flush whenever the texture changes.
        if (texture != batchTexture)
        {
            if (pos > batchStart)
            {
                RenderBatch(batchTexture, &mSortedSprites[batchStart], pos - batchStart);
            }

            batchTexture = texture;
            batchStart = pos;
        }
    }

    // Flush the final batch.
    RenderBatch(batchTexture, &mSortedSprites[batchStart], mSpriteQueueCount - batchStart);

    // Reset the queue.
    mSpriteQueueCount = 0;
    mSpriteTextureReferences.clear();

    // When sorting is disabled, we persist mSortedSprites data from one batch to the next, to avoid
    // uneccessary work in GrowSortedSprites. But we never reuse these when sorting, because re-sorting
    // previously sorted items gives unstable ordering if some sprites have identical sort keys.
    if (mSortMode != SpriteSortMode_Deferred)
    {
        mSortedSprites.clear();
    }
}


// Sorts the array of queued sprites.
void SpriteBatch::Impl::SortSprites()
{
    // Fill the mSortedSprites vector.
    if (mSortedSprites.size() < mSpriteQueueCount)
    {
        GrowSortedSprites();
    }

    switch (mSortMode)
    {
        case SpriteSortMode_Texture:
            // Sort by texture.
            std::sort(mSortedSprites.begin(), mSortedSprites.begin() + mSpriteQueueCount, [](SpriteInfo const* x, SpriteInfo const* y) -> bool
            {
                return x->texture < y->texture;
            });
            break;

        case SpriteSortMode_BackToFront:
            // Sort back to front.
            std::sort(mSortedSprites.begin(), mSortedSprites.begin() + mSpriteQueueCount, [](SpriteInfo const* x, SpriteInfo const* y) -> bool
            {
                return x->originRotationDepth.w > y->originRotationDepth.w;
            });
            break;

        case SpriteSortMode_FrontToBack:
            // Sort front to back.
            std::sort(mSortedSprites.begin(), mSortedSprites.begin() + mSpriteQueueCount, [](SpriteInfo const* x, SpriteInfo const* y) -> bool
            {
                return x->originRotationDepth.w < y->originRotationDepth.w;
            });
            break;
    }
}


// Populates the mSortedSprites vector with pointers to individual elements of the mSpriteQueue array.
void SpriteBatch::Impl::GrowSortedSprites()
{
    size_t previousSize = mSortedSprites.size();

    mSortedSprites.resize(mSpriteQueueCount);

    for (size_t i = previousSize; i < mSpriteQueueCount; i++)
    {
        mSortedSprites[i] = &mSpriteQueue[i];
    }
}


// Submits a batch of sprites to the GPU.
_Use_decl_annotations_
void SpriteBatch::Impl::RenderBatch(ID3D11ShaderResourceView* texture, SpriteInfo const* const* sprites, size_t count)
{
    auto deviceContext = mContextResources->deviceContext.Get();

    // Draw using the specified texture.
    deviceContext->PSSetShaderResources(0, 1, &texture);

    XMVECTOR textureSize = GetTextureSize(texture);
    XMVECTOR inverseTextureSize = XMVectorReciprocal(textureSize);
            
    while (count > 0)
    {
        // How many sprites do we want to draw?
        size_t batchSize = count;

        // How many sprites does the D3D vertex buffer have room for?
        size_t remainingSpace = MaxBatchSize - mContextResources->vertexBufferPosition;

        if (batchSize > remainingSpace)
        {
            if (remainingSpace < MinBatchSize)
            {
                // If we are out of room, or about to submit an excessively small batch, wrap back to the start of the vertex buffer.
                mContextResources->vertexBufferPosition = 0;

                batchSize = std::min(count, MaxBatchSize);
            }
            else
            {
                // Take however many sprites fit in what's left of the vertex buffer.
                batchSize = remainingSpace;
            }
        }

#if defined(_XBOX_ONE) && defined(_TITLE)
        void *grfxMemory = GraphicsMemory::Get().Allocate(deviceContext, sizeof(VertexPositionColorTexture) * batchSize * VerticesPerSprite, 64);

        auto vertices = static_cast<VertexPositionColorTexture*>(grfxMemory);
#else
        // Lock the vertex buffer.
        D3D11_MAP mapType = (mContextResources->vertexBufferPosition == 0) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

        D3D11_MAPPED_SUBRESOURCE mappedBuffer;

        ThrowIfFailed(
            deviceContext->Map(mContextResources->vertexBuffer.Get(), 0, mapType, 0, &mappedBuffer)
        );

        auto vertices = static_cast<VertexPositionColorTexture*>(mappedBuffer.pData) + mContextResources->vertexBufferPosition * VerticesPerSprite;
#endif

        // Generate sprite vertex data.
        for (size_t i = 0; i < batchSize; i++)
        {
            assert(i < count);
            _Analysis_assume_(i < count);
            RenderSprite(sprites[i], vertices, textureSize, inverseTextureSize);

            vertices += VerticesPerSprite;
        }

#if defined(_XBOX_ONE) && defined(_TITLE)
        deviceContext->IASetPlacementVertexBuffer(0, mContextResources->vertexBuffer.Get(), grfxMemory, sizeof(VertexPositionColorTexture));
#else
        deviceContext->Unmap(mContextResources->vertexBuffer.Get(), 0);
#endif

        // Ok lads, the time has come for us draw ourselves some sprites!
        UINT startIndex = (UINT)mContextResources->vertexBufferPosition * IndicesPerSprite;
        UINT indexCount = (UINT)batchSize * IndicesPerSprite;

        deviceContext->DrawIndexed(indexCount, startIndex, 0);

        // Advance the buffer position.
#if !defined(_XBOX_ONE) || !defined(_TITLE)
        mContextResources->vertexBufferPosition += batchSize;
#endif

        sprites += batchSize;
        count -= batchSize;
    }
}


// Generates vertex data for drawing a single sprite.
_Use_decl_annotations_
void XM_CALLCONV SpriteBatch::Impl::RenderSprite(SpriteInfo const* sprite,
    VertexPositionColorTexture* vertices,
    FXMVECTOR textureSize,
    FXMVECTOR inverseTextureSize)
{
    // Load sprite parameters into SIMD registers.
    XMVECTOR source = XMLoadFloat4A(&sprite->source);
    XMVECTOR destination = XMLoadFloat4A(&sprite->destination);
    XMVECTOR color = XMLoadFloat4A(&sprite->color);
    XMVECTOR originRotationDepth = XMLoadFloat4A(&sprite->originRotationDepth);

    float rotation = sprite->originRotationDepth.z;
    int flags = sprite->flags;

    // Extract the source and destination sizes into separate vectors.
    XMVECTOR sourceSize = XMVectorSwizzle<2, 3, 2, 3>(source);
    XMVECTOR destinationSize = XMVectorSwizzle<2, 3, 2, 3>(destination);

    // Scale the origin offset by source size, taking care to avoid overflow if the source region is zero.
    XMVECTOR isZeroMask = XMVectorEqual(sourceSize, XMVectorZero());
    XMVECTOR nonZeroSourceSize = XMVectorSelect(sourceSize, g_XMEpsilon, isZeroMask);

    XMVECTOR origin = XMVectorDivide(originRotationDepth, nonZeroSourceSize);

    // Convert the source region from texels to mod-1 texture coordinate format.
    if (flags & SpriteInfo::SourceInTexels)
    {
        source *= inverseTextureSize;
        sourceSize *= inverseTextureSize;
    }
    else
    {
        origin *= inverseTextureSize;
    }

    // If the destination size is relative to the source region, convert it to pixels.
    if (!(flags & SpriteInfo::DestSizeInPixels))
    {
        destinationSize *= textureSize;
    }

    // Compute a 2x2 rotation matrix.
    XMVECTOR rotationMatrix1;
    XMVECTOR rotationMatrix2;

    if (rotation != 0)
    {
        float sin, cos;

        XMScalarSinCos(&sin, &cos, rotation);

        XMVECTOR sinV = XMLoadFloat(&sin);
        XMVECTOR cosV = XMLoadFloat(&cos);

        rotationMatrix1 = XMVectorMergeXY(cosV, sinV);
        rotationMatrix2 = XMVectorMergeXY(-sinV, cosV);
    }
    else
    {
        rotationMatrix1 = g_XMIdentityR0;
        rotationMatrix2 = g_XMIdentityR1;
    }
    
    // The four corner vertices are computed by transforming these unit-square positions.
    static XMVECTORF32 cornerOffsets[VerticesPerSprite] =
    {
        { 0, 0 },
        { 1, 0 },
        { 0, 1 },
        { 1, 1 },
    };

    // Tricksy alert! Texture coordinates are computed from the same cornerOffsets
    // table as vertex positions, but if the sprite is mirrored, this table
    // must be indexed in a different order. This is done as follows:
    //
    //    position = cornerOffsets[i]
    //    texcoord = cornerOffsets[i ^ SpriteEffects]

    static_assert(SpriteEffects_FlipHorizontally == 1 &&
                  SpriteEffects_FlipVertically == 2, "If you change these enum values, the mirroring implementation must be updated to match");

    int mirrorBits = flags & 3;

    // Generate the four output vertices.
    for (int i = 0; i < VerticesPerSprite; i++)
    {
        // Calculate position.
        XMVECTOR cornerOffset = (cornerOffsets[i] - origin) * destinationSize;
        
        // Apply 2x2 rotation matrix.
        XMVECTOR position1 = XMVectorMultiplyAdd(XMVectorSplatX(cornerOffset), rotationMatrix1, destination);
        XMVECTOR position2 = XMVectorMultiplyAdd(XMVectorSplatY(cornerOffset), rotationMatrix2, position1);

        // Set z = depth.
        XMVECTOR position = XMVectorPermute<0, 1, 7, 6>(position2, originRotationDepth);

        // Write position as a Float4, even though VertexPositionColor::position is an XMFLOAT3.
        // This is faster, and harmless as we are just clobbering the first element of the
        // following color field, which will immediately be overwritten with its correct value.
        XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&vertices[i].position), position);

        // Write the color.
        XMStoreFloat4(&vertices[i].color, color);

        // Compute and write the texture coordinate.
        XMVECTOR textureCoordinate = XMVectorMultiplyAdd(cornerOffsets[i ^ mirrorBits], sourceSize, source);

        XMStoreFloat2(&vertices[i].textureCoordinate, textureCoordinate);
    }
}


// Helper looks up the size of the specified texture.
XMVECTOR SpriteBatch::Impl::GetTextureSize(_In_ ID3D11ShaderResourceView* texture)
{
    // Convert resource view to underlying resource.
    ComPtr<ID3D11Resource> resource;

    texture->GetResource(&resource);
    
    // Cast to texture.
    ComPtr<ID3D11Texture2D> texture2D;
    
    if (FAILED(resource.As(&texture2D)))
    {
        throw std::exception("SpriteBatch can only draw Texture2D resources");
    }

    // Query the texture size.
    D3D11_TEXTURE2D_DESC desc;

    texture2D->GetDesc(&desc);

    // Convert to vector format.
    XMVECTOR size = XMVectorMergeXY(XMLoadInt(&desc.Width),
                                    XMLoadInt(&desc.Height));

    return XMConvertVectorUIntToFloat(size, 0);
}


// Generates a viewport transform matrix for rendering sprites using x-right y-down screen pixel coordinates.
XMMATRIX SpriteBatch::Impl::GetViewportTransform(_In_ ID3D11DeviceContext* deviceContext, DXGI_MODE_ROTATION rotation )
{
    // Look up the current viewport.
    if ( !mSetViewport )
    {
        UINT viewportCount = 1;

        deviceContext->RSGetViewports(&viewportCount, &mViewPort);

        if (viewportCount != 1)
            throw std::exception("No viewport is set");
    }
    
    // Compute the matrix.
    float xScale = (mViewPort.Width  > 0) ? 2.0f / mViewPort.Width  : 0.0f;
    float yScale = (mViewPort.Height > 0) ? 2.0f / mViewPort.Height : 0.0f;

    switch( rotation )
    {
    case DXGI_MODE_ROTATION_ROTATE90:
        return XMMATRIX
        (
             0,       -yScale,  0,  0,
             -xScale, 0,        0,  0,
             0,       0,        1,  0,
             1,       1,        0,  1
        );

    case DXGI_MODE_ROTATION_ROTATE270:
        return XMMATRIX
        (
             0,       yScale,   0,  0,
             xScale,  0,        0,  0,
             0,       0,        1,  0,
            -1,      -1,        0,  1
        );

    case DXGI_MODE_ROTATION_ROTATE180:
        return XMMATRIX
        (
            -xScale,  0,       0,  0,
             0,       yScale,  0,  0,
             0,       0,       1,  0,
             1,      -1,       0,  1
        );

    default:
        return XMMATRIX
        (
             xScale,  0,       0,  0,
             0,      -yScale,  0,  0,
             0,       0,       1,  0,
            -1,       1,       0,  1
        );
    }
}


// Public constructor.
SpriteBatch::SpriteBatch(_In_ ID3D11DeviceContext* deviceContext)
  : pImpl(new Impl(deviceContext))
{
}


// Move constructor.
SpriteBatch::SpriteBatch(SpriteBatch&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
SpriteBatch& SpriteBatch::operator= (SpriteBatch&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
SpriteBatch::~SpriteBatch()
{
}


_Use_decl_annotations_
void XM_CALLCONV SpriteBatch::Begin(SpriteSortMode sortMode,
    ID3D11BlendState* blendState,
    ID3D11SamplerState* samplerState,
    ID3D11DepthStencilState* depthStencilState,
    ID3D11RasterizerState* rasterizerState,
    std::function<void()> setCustomShaders,
    FXMMATRIX transformMatrix)
{
    pImpl->Begin(sortMode, blendState, samplerState, depthStencilState, rasterizerState, setCustomShaders, transformMatrix);
}


void SpriteBatch::End()
{
    pImpl->End();
}


_Use_decl_annotations_
void XM_CALLCONV SpriteBatch::Draw(ID3D11ShaderResourceView* texture, XMFLOAT2 const& position, FXMVECTOR color)
{
    XMVECTOR destination = XMVectorPermute<0, 1, 4, 5>(XMLoadFloat2(&position), g_XMOne); // x, y, 1, 1
    
    pImpl->Draw(texture, destination, nullptr, color, g_XMZero, 0);
}


_Use_decl_annotations_
void XM_CALLCONV SpriteBatch::Draw(ID3D11ShaderResourceView* texture,
    XMFLOAT2 const& position,
    RECT const* sourceRectangle,
    FXMVECTOR color,
    float rotation,
    XMFLOAT2 const& origin,
    float scale,
    SpriteEffects effects,
    float layerDepth)
{
    XMVECTOR destination = XMVectorPermute<0, 1, 4, 4>(XMLoadFloat2(&position), XMLoadFloat(&scale)); // x, y, scale, scale
    
    XMVECTOR originRotationDepth = XMVectorSet(origin.x, origin.y, rotation, layerDepth);

    pImpl->Draw(texture, destination, sourceRectangle, color, originRotationDepth, effects);
}


_Use_decl_annotations_
void XM_CALLCONV SpriteBatch::Draw(ID3D11ShaderResourceView* texture,
    XMFLOAT2 const& position,
    RECT const* sourceRectangle,
    FXMVECTOR color,
    float rotation,
    XMFLOAT2 const& origin,
    XMFLOAT2 const& scale,
    SpriteEffects effects,
    float layerDepth)
{
    XMVECTOR destination = XMVectorPermute<0, 1, 4, 5>(XMLoadFloat2(&position), XMLoadFloat2(&scale)); // x, y, scale.x, scale.y
    
    XMVECTOR originRotationDepth = XMVectorSet(origin.x, origin.y, rotation, layerDepth);
    
    pImpl->Draw(texture, destination, sourceRectangle, color, originRotationDepth, effects);
}


_Use_decl_annotations_
void XM_CALLCONV SpriteBatch::Draw(ID3D11ShaderResourceView* texture, FXMVECTOR position, FXMVECTOR color)
{
    XMVECTOR destination = XMVectorPermute<0, 1, 4, 5>(position, g_XMOne); // x, y, 1, 1
    
    pImpl->Draw(texture, destination, nullptr, color, g_XMZero, 0);
}


_Use_decl_annotations_
void XM_CALLCONV SpriteBatch::Draw(ID3D11ShaderResourceView* texture,
    FXMVECTOR position,
    RECT const* sourceRectangle,
    FXMVECTOR color,
    float rotation,
    FXMVECTOR origin,
    float scale,
    SpriteEffects effects,
    float layerDepth)
{
    XMVECTOR destination = XMVectorPermute<0, 1, 4, 4>(position, XMLoadFloat(&scale)); // x, y, scale, scale

    XMVECTOR rotationDepth = XMVectorMergeXY(XMVectorReplicate(rotation), XMVectorReplicate(layerDepth));

    XMVECTOR originRotationDepth = XMVectorPermute<0, 1, 4, 5>(origin, rotationDepth);
    
    pImpl->Draw(texture, destination, sourceRectangle, color, originRotationDepth, effects);
}


_Use_decl_annotations_
void XM_CALLCONV SpriteBatch::Draw(ID3D11ShaderResourceView* texture,
    FXMVECTOR position,
    RECT const* sourceRectangle,
    FXMVECTOR color,
    float rotation,
    FXMVECTOR origin,
    GXMVECTOR scale,
    SpriteEffects effects,
    float layerDepth)
{
    XMVECTOR destination = XMVectorPermute<0, 1, 4, 5>(position, scale); // x, y, scale.x, scale.y
    
    XMVECTOR rotationDepth = XMVectorMergeXY(XMVectorReplicate(rotation), XMVectorReplicate(layerDepth));

    XMVECTOR originRotationDepth = XMVectorPermute<0, 1, 4, 5>(origin, rotationDepth);

    pImpl->Draw(texture, destination, sourceRectangle, color, originRotationDepth, effects);
}


_Use_decl_annotations_
void XM_CALLCONV SpriteBatch::Draw(ID3D11ShaderResourceView* texture, RECT const& destinationRectangle, FXMVECTOR color)
{
    XMVECTOR destination = LoadRect(&destinationRectangle); // x, y, w, h

    pImpl->Draw(texture, destination, nullptr, color, g_XMZero, Impl::SpriteInfo::DestSizeInPixels);
}


_Use_decl_annotations_
void XM_CALLCONV SpriteBatch::Draw(ID3D11ShaderResourceView* texture,
    RECT const& destinationRectangle,
    RECT const* sourceRectangle,
    FXMVECTOR color,
    float rotation,
    XMFLOAT2 const& origin,
    SpriteEffects effects,
    float layerDepth)
{
    XMVECTOR destination = LoadRect(&destinationRectangle); // x, y, w, h

    XMVECTOR originRotationDepth = XMVectorSet(origin.x, origin.y, rotation, layerDepth);
    
    pImpl->Draw(texture, destination, sourceRectangle, color, originRotationDepth, effects | Impl::SpriteInfo::DestSizeInPixels);
}


void SpriteBatch::SetRotation( DXGI_MODE_ROTATION mode )
{
    pImpl->mRotation = mode;
}


DXGI_MODE_ROTATION SpriteBatch::GetRotation() const
{
    return pImpl->mRotation;
}


void SpriteBatch::SetViewport( const D3D11_VIEWPORT& viewPort )
{
    pImpl->mSetViewport = true;
    pImpl->mViewPort = viewPort;
}
