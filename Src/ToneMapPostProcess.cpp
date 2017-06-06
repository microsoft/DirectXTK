//--------------------------------------------------------------------------------------
// File: ToneMapPostProcess.cpp
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

using Microsoft::WRL::ComPtr;

namespace
{
    const int Dirty_ConstantBuffer  = 0x01;
    const int Dirty_Parameters      = 0x02;

    // Constant buffer layout. Must match the shader!
    __declspec(align(16)) struct ToneMapConstants
    {
        XMVECTOR paperWhiteNits;
    };

    static_assert((sizeof(ToneMapConstants) % 16) == 0, "CB size not padded correctly");
}

// Include the precompiled shader code.
namespace
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    #include "Shaders/Compiled/XboxOneToneMap_VSQuad.inc"

    #include "Shaders/Compiled/XboxOneToneMap_PSSaturate.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSReinhard.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSFilmic.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSHDR10.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSHDR10_Saturate.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSHDR10_Reinhard.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSHDR10_Filmic.inc"
#else
    #include "Shaders/Compiled/ToneMap_VSQuad.inc"

    #include "Shaders/Compiled/ToneMap_PSSaturate.inc"
    #include "Shaders/Compiled/ToneMap_PSReinhard.inc"
    #include "Shaders/Compiled/ToneMap_PSFilmic.inc"
    #include "Shaders/Compiled/ToneMap_PSHDR10.inc"
    #include "Shaders/Compiled/ToneMap_PSHDR10_Saturate.inc"
    #include "Shaders/Compiled/ToneMap_PSHDR10_Reinhard.inc"
    #include "Shaders/Compiled/ToneMap_PSHDR10_Filmic.inc"
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
        { ToneMap_PSSaturate,       sizeof(ToneMap_PSSaturate) },
        { ToneMap_PSReinhard,       sizeof(ToneMap_PSReinhard) },
        { ToneMap_PSFilmic,         sizeof(ToneMap_PSFilmic) },
        { ToneMap_PSHDR10,          sizeof(ToneMap_PSHDR10) },
        { ToneMap_PSHDR10_Saturate, sizeof(ToneMap_PSHDR10_Saturate) },
        { ToneMap_PSHDR10_Reinhard, sizeof(ToneMap_PSHDR10_Reinhard) },
        { ToneMap_PSHDR10_Filmic,   sizeof(ToneMap_PSHDR10_Filmic) },
    };

    static_assert(_countof(pixelShaders) == ToneMapPostProcess::Effect_Max, "array/max mismatch");

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
                    D3D11_FILTER_MIN_MAG_MIP_POINT,
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
                    SetDebugObjectName(*pResult, "ToneMapPostProcess");

                return hr;
            });
        }

        // Gets or lazily creates the vertex shader.
        ID3D11VertexShader* GetVertexShader()
        {
            return DemandCreate(mVertexShader, mMutex, [&](ID3D11VertexShader** pResult) -> HRESULT
            {
                HRESULT hr = mDevice->CreateVertexShader(ToneMap_VSQuad, sizeof(ToneMap_VSQuad), nullptr, pResult);

                if (SUCCEEDED(hr))
                    SetDebugObjectName(*pResult, "ToneMapPostProcess");

                return hr;
            });
        }

        // Gets or lazily creates the specified pixel shader.
        ID3D11PixelShader* GetPixelShader(int shaderIndex)
        {
            assert(shaderIndex >= 0 && shaderIndex < ToneMapPostProcess::Effect_Max);
            _Analysis_assume_(shaderIndex >= 0 && shaderIndex < ToneMapPostProcess::Effect_Max);

            return DemandCreate(mPixelShaders[shaderIndex], mMutex, [&](ID3D11PixelShader** pResult) -> HRESULT
            {
                HRESULT hr = mDevice->CreatePixelShader(pixelShaders[shaderIndex].code, pixelShaders[shaderIndex].length, nullptr, pResult);

                if (SUCCEEDED(hr))
                    SetDebugObjectName(*pResult, "ToneMapPostProcess");

                return hr;
            });
        }

    protected:
        ComPtr<ID3D11Device>        mDevice;
        ComPtr<ID3D11SamplerState>  mSampler;
        ComPtr<ID3D11VertexShader>  mVertexShader;
        ComPtr<ID3D11PixelShader>   mPixelShaders[ToneMapPostProcess::Effect_Max];
        std::mutex                  mMutex;
    };
}

class ToneMapPostProcess::Impl : public AlignedNew<ToneMapConstants>
{
public:
    Impl(_In_ ID3D11Device* device);

    void Process(_In_ ID3D11DeviceContext* deviceContext, std::function<void __cdecl()>& setCustomState);

    void SetConstants(bool value = true) { mUseConstants = value; mDirtyFlags = INT_MAX; }
    void SetDirtyFlag() { mDirtyFlags = INT_MAX; }

    // Fields.
    ToneMapPostProcess::Effect              fx;
    ToneMapConstants                        constants;
    ComPtr<ID3D11ShaderResourceView>        hdrTexture;
    float                                   paperWhiteNits;

private:
    bool                                    mUseConstants;
    int                                     mDirtyFlags;

    ConstantBuffer<ToneMapConstants>        mConstantBuffer;

    // Per-device resources.
    std::shared_ptr<DeviceResources>        mDeviceResources;

    static SharedResourcePool<ID3D11Device*, DeviceResources> deviceResourcesPool;
};


// Global pool of per-device ToneMapPostProcess resources.
SharedResourcePool<ID3D11Device*, DeviceResources> ToneMapPostProcess::Impl::deviceResourcesPool;


// Constructor.
ToneMapPostProcess::Impl::Impl(_In_ ID3D11Device* device)
    : mConstantBuffer(device),
    mDeviceResources(deviceResourcesPool.DemandCreate(device)),
    fx(ToneMapPostProcess::Saturate),
    paperWhiteNits(200.f),
    mUseConstants(false),
    mDirtyFlags(INT_MAX),
    constants{}
{
    if (device->GetFeatureLevel() < D3D_FEATURE_LEVEL_10_0)
    {
        throw std::exception("ToneMapPostProcess requires Feature Level 10.0 or later");
    }
}


// Sets our state onto the D3D device.
void ToneMapPostProcess::Impl::Process(_In_ ID3D11DeviceContext* deviceContext, std::function<void __cdecl()>& setCustomState)
{
    // Set the texture.
    ID3D11ShaderResourceView* textures[1] = { hdrTexture.Get() };
    deviceContext->PSSetShaderResources(0, 1, textures);

    auto sampler = mDeviceResources->GetSampler();
    deviceContext->PSSetSamplers(0, 1, &sampler);

    // Set shaders.
    auto vertexShader = mDeviceResources->GetVertexShader();
    auto pixelShader = mDeviceResources->GetPixelShader(fx);

    deviceContext->VSSetShader(vertexShader, nullptr, 0);
    deviceContext->PSSetShader(pixelShader, nullptr, 0);

    // Set constants.
    if (mUseConstants)
    {
        if (mDirtyFlags & Dirty_Parameters)
        {
            mDirtyFlags &= ~Dirty_Parameters;
            mDirtyFlags |= Dirty_ConstantBuffer;

            switch (fx)
            {
            case HDR10:
            case HDR10_Saturate:
            case HDR10_Reinhard:
            case HDR10_Filmic:
                constants.paperWhiteNits = XMVectorSet(paperWhiteNits, 0.f, 0.f, 0.f);
                break;
            }
        }

#if defined(_XBOX_ONE) && defined(_TITLE)
        void *grfxMemory;
        mConstantBuffer.SetData(deviceContext, constants, &grfxMemory);

        Microsoft::WRL::ComPtr<ID3D11DeviceContextX> deviceContextX;
        ThrowIfFailed(deviceContext->QueryInterface(IID_GRAPHICS_PPV_ARGS(deviceContextX.GetAddressOf())));

        auto buffer = mConstantBuffer.GetBuffer();

        deviceContextX->PSSetPlacementConstantBuffer(0, buffer, grfxMemory);
#else
        if (mDirtyFlags & Dirty_ConstantBuffer)
        {
            mDirtyFlags &= ~Dirty_ConstantBuffer;
            mConstantBuffer.SetData(deviceContext, constants);
        }

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
ToneMapPostProcess::ToneMapPostProcess(_In_ ID3D11Device* device)
  : pImpl(new Impl(device))
{
}


// Move constructor.
ToneMapPostProcess::ToneMapPostProcess(ToneMapPostProcess&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
ToneMapPostProcess& ToneMapPostProcess::operator= (ToneMapPostProcess&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
ToneMapPostProcess::~ToneMapPostProcess()
{
}


// IEffect methods.
void ToneMapPostProcess::Process(_In_ ID3D11DeviceContext* deviceContext, _In_opt_ std::function<void __cdecl()> setCustomState)
{
    pImpl->Process(deviceContext, setCustomState);
}


// Properties
void ToneMapPostProcess::SetHDRSourceTexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->hdrTexture = value;
}


void ToneMapPostProcess::SetEffect(Effect fx)
{
    pImpl->fx = fx;

    switch (fx)
    {
    case Saturate:
    case Reinhard:
    case Filmic:
        // These shaders don't use the constant buffer
        pImpl->SetConstants(false);
        break;

    default:
        pImpl->SetConstants(true);
        break;
    }
}


void ToneMapPostProcess::SetHDR10Parameter(float paperWhiteNits)
{
    pImpl->paperWhiteNits = paperWhiteNits;
    pImpl->SetDirtyFlag();
}
