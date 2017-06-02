//--------------------------------------------------------------------------------------
// File: BasicPostProcess.cpp
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
#include "PostProcess.h"
#include "AlignedNew.h"
#include "ConstantBuffer.h"
#include "DemandCreate.h"
#include "DirectXHelpers.h"
#include "SharedResourcePool.h"

using namespace DirectX;

namespace
{
    static const int c_MaxSamples = 16;

    // Constant buffer layout. Must match the shader!
    struct PostProcessConstants
    {
        XMVECTOR sampleOffsets[c_MaxSamples];
        XMVECTOR sampleWeights[c_MaxSamples];
    };

    static_assert((sizeof(PostProcessConstants) % 16) == 0, "CB size not padded correctly");
}

// Include the precompiled shader code.
namespace
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    #include "Shaders/Compiled/XboxOnePostProcess_VSQuad.inc"

    #include "Shaders/Compiled/XboxOnePostProcess_PSCopy.inc"
#else
    #include "Shaders/Compiled/PostProcess_VSQuad.inc"

    #include "Shaders/Compiled/PostProcess_PSCopy.inc"
#endif
}

namespace
{
    struct ShaderBytecode
    {
        void const* code;
        size_t length;
    };

    const ShaderBytecode pixelShaders[] =
    {
        { PostProcess_PSCopy,   sizeof(PostProcess_PSCopy) },
    };

    static_assert(_countof(pixelShaders) == BasicPostProcess::Mode_Max, "array/max mismatch");

    // Factory for lazily instantiating shaders.
    class DeviceResources
    {
    public:
        DeviceResources(_In_ ID3D11Device* device)
            : mDevice(device)
        { }

        // Gets or lazily creates the sampler.
        ID3D11SamplerState* GetSampler()
        {
            return DemandCreate(mSampler, mMutex, [&](ID3D11SamplerState** pResult) -> HRESULT
            {
                static const D3D11_SAMPLER_DESC s_sampler =
                {
                    D3D11_FILTER_MIN_MAG_MIP_LINEAR,
                    D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP,
                    0.f,
                    D3D11_MAX_MAXANISOTROPY,
                    D3D11_COMPARISON_NEVER,
                    { 0.f, 0.f, 0.f, 0.f },
                    0.f,
                    FLT_MAX
                };

                HRESULT hr = mDevice->CreateSamplerState(&s_sampler, pResult);

                if (SUCCEEDED(hr))
                    SetDebugObjectName(*pResult, "BasicPostProcess");

                return hr;
            });
        }

        // Gets or lazily creates the vertex shader.
        ID3D11VertexShader* GetVertexShader()
        {
            return DemandCreate(mVertexShader, mMutex, [&](ID3D11VertexShader** pResult) -> HRESULT
            {
                HRESULT hr = mDevice->CreateVertexShader(PostProcess_VSQuad, sizeof(PostProcess_VSQuad), nullptr, pResult);

                if (SUCCEEDED(hr))
                    SetDebugObjectName(*pResult, "BasicPostProcess");

                return hr;
            });
        }

        // Gets or lazily creates the specified pixel shader.
        ID3D11PixelShader* GetPixelShader(int shaderIndex)
        {
            assert(shaderIndex >= 0 && shaderIndex < BasicPostProcess::Mode_Max);
            _Analysis_assume_(shaderIndex >= 0 && shaderIndex < BasicPostProcess::Mode_Max);

            return DemandCreate(mPixelShaders[shaderIndex], mMutex, [&](ID3D11PixelShader** pResult) -> HRESULT
            {
                HRESULT hr = mDevice->CreatePixelShader(pixelShaders[shaderIndex].code, pixelShaders[shaderIndex].length, nullptr, pResult);

                if (SUCCEEDED(hr))
                    SetDebugObjectName(*pResult, "BasicPostProcess");

                return hr;
            });
        }

    protected:
        Microsoft::WRL::ComPtr<ID3D11Device>        mDevice;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>  mSampler;
        Microsoft::WRL::ComPtr<ID3D11VertexShader>  mVertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>   mPixelShaders[BasicPostProcess::Mode_Max];
        std::mutex                                  mMutex;
    };
}

class BasicPostProcess::Impl : public AlignedNew<PostProcessConstants>
{
public:
    Impl(_In_ ID3D11Device* device);

    void Process(_In_ ID3D11DeviceContext* deviceContext, _In_ ID3D11ShaderResourceView* src, std::function<void __cdecl()>& setCustomState);

    // Fields.
    BasicPostProcess::Mode                  mode;
    PostProcessConstants                    constants;

private:
    ConstantBuffer<PostProcessConstants>    mConstantBuffer;

    // Per-device resources.
    std::shared_ptr<DeviceResources>        mDeviceResources;

    static SharedResourcePool<ID3D11Device*, DeviceResources> deviceResourcesPool;
};


// Global pool of per-device BasicPostProcess resources.
SharedResourcePool<ID3D11Device*, DeviceResources> BasicPostProcess::Impl::deviceResourcesPool;


// Constructor.
BasicPostProcess::Impl::Impl(_In_ ID3D11Device* device)
    : mConstantBuffer(device),
    mDeviceResources(deviceResourcesPool.DemandCreate(device)),
    mode(BasicPostProcess::Copy),
    constants{}
{
}


// Sets our state onto the D3D device.
void BasicPostProcess::Impl::Process(_In_ ID3D11DeviceContext* deviceContext, _In_ ID3D11ShaderResourceView* src, std::function<void __cdecl()>& setCustomState)
{
    // Set the texture.
    ID3D11ShaderResourceView* textures[1] = { src };
    deviceContext->PSSetShaderResources(0, 1, textures);

    auto sampler = mDeviceResources->GetSampler();
    deviceContext->PSSetSamplers(0, 1, &sampler);

    // Set shaders.
    auto vertexShader = mDeviceResources->GetVertexShader();
    auto pixelShader = mDeviceResources->GetPixelShader(mode);

    deviceContext->VSSetShader(vertexShader, nullptr, 0);
    deviceContext->PSSetShader(pixelShader, nullptr, 0);

    // Set constants.
    if (mode > BasicPostProcess::Copy)
    {
#if defined(_XBOX_ONE) && defined(_TITLE)
        void *grfxMemory;
        mConstantBuffer.SetData(deviceContext, constants, &grfxMemory);

        Microsoft::WRL::ComPtr<ID3D11DeviceContextX> deviceContextX;
        ThrowIfFailed(deviceContext->QueryInterface(IID_GRAPHICS_PPV_ARGS(deviceContextX.GetAddressOf())));

        auto buffer = mConstantBuffer.GetBuffer();

        deviceContextX->PSSetPlacementConstantBuffer(0, buffer, grfxMemory);
#else
        mConstantBuffer.SetData(deviceContext, constants);

        // Set the constant buffer.
        auto buffer = mConstantBuffer.GetBuffer();

        deviceContext->PSSetConstantBuffers(0, 1, &buffer);
#endif
    }

    if (setCustomState)
    {
        setCustomState();
    }

    // Draw quad.
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    deviceContext->Draw(4, 0);
}


// Public constructor.
BasicPostProcess::BasicPostProcess(_In_ ID3D11Device* device)
  : pImpl(new Impl(device))
{
}


// Move constructor.
BasicPostProcess::BasicPostProcess(BasicPostProcess&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
BasicPostProcess& BasicPostProcess::operator= (BasicPostProcess&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
BasicPostProcess::~BasicPostProcess()
{
}


// IEffect methods.
void BasicPostProcess::Process(_In_ ID3D11DeviceContext* deviceContext, _In_ ID3D11ShaderResourceView* src, _In_opt_ std::function<void __cdecl()> setCustomState)
{
    pImpl->Process(deviceContext, src, setCustomState);
}


// Properties
void BasicPostProcess::Set(Mode mode)
{
    pImpl->mode = mode;
}