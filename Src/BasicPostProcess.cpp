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

using Microsoft::WRL::ComPtr;

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
    #include "Shaders/Compiled/XboxOnePostProcess_PSMonochrome.inc"
    #include "Shaders/Compiled/XboxOnePostProcess_PSDownScale2x2.inc"
    #include "Shaders/Compiled/XboxOnePostProcess_PSDownScale4x4.inc"
#else
    #include "Shaders/Compiled/PostProcess_VSQuad.inc"

    #include "Shaders/Compiled/PostProcess_PSCopy.inc"
    #include "Shaders/Compiled/PostProcess_PSMonochrome.inc"
    #include "Shaders/Compiled/PostProcess_PSDownScale2x2.inc"
    #include "Shaders/Compiled/PostProcess_PSDownScale4x4.inc"
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
        { PostProcess_PSCopy,       sizeof(PostProcess_PSCopy) },
        { PostProcess_PSMonochrome, sizeof(PostProcess_PSMonochrome) },
        { PostProcess_PSDownScale2x2, sizeof(PostProcess_PSDownScale2x2) },
        { PostProcess_PSDownScale4x4, sizeof(PostProcess_PSDownScale4x4) },
    };

    static_assert(_countof(pixelShaders) == BasicPostProcess::Effect_Max, "array/max mismatch");

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
            assert(shaderIndex >= 0 && shaderIndex < BasicPostProcess::Effect_Max);
            _Analysis_assume_(shaderIndex >= 0 && shaderIndex < BasicPostProcess::Effect_Max);

            return DemandCreate(mPixelShaders[shaderIndex], mMutex, [&](ID3D11PixelShader** pResult) -> HRESULT
            {
                HRESULT hr = mDevice->CreatePixelShader(pixelShaders[shaderIndex].code, pixelShaders[shaderIndex].length, nullptr, pResult);

                if (SUCCEEDED(hr))
                    SetDebugObjectName(*pResult, "BasicPostProcess");

                return hr;
            });
        }

    protected:
        ComPtr<ID3D11Device>        mDevice;
        ComPtr<ID3D11SamplerState>  mSampler;
        ComPtr<ID3D11VertexShader>  mVertexShader;
        ComPtr<ID3D11PixelShader>   mPixelShaders[BasicPostProcess::Effect_Max];
        std::mutex                  mMutex;
    };
}

class BasicPostProcess::Impl : public AlignedNew<PostProcessConstants>
{
public:
    Impl(_In_ ID3D11Device* device);

    void Process(_In_ ID3D11DeviceContext* deviceContext, std::function<void __cdecl()>& setCustomState);

    void NoConstants() { mUseConstants = false; }

    void DownScale2x2();
    void DownScale4x4();

    // Fields.
    BasicPostProcess::Effect                fx;
    PostProcessConstants                    constants;
    ComPtr<ID3D11ShaderResourceView>        texture;
    unsigned                                texWidth;
    unsigned                                texHeight;

private:
    bool                                    mUseConstants;

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
    fx(BasicPostProcess::Copy),
    texWidth(0),
    texHeight(0),
    mUseConstants(false),
    constants{}
{
    if (device->GetFeatureLevel() < D3D_FEATURE_LEVEL_10_0)
    {
        throw std::exception("BasicPostProcess requires Feature Level 10.0 or later");
    }
}


// Sets our state onto the D3D device.
void BasicPostProcess::Impl::Process(_In_ ID3D11DeviceContext* deviceContext, std::function<void __cdecl()>& setCustomState)
{
    // Set the texture.
    ID3D11ShaderResourceView* textures[1] = { texture.Get() };
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


void BasicPostProcess::Impl::DownScale2x2()
{
    mUseConstants = true;

    if ( !texWidth || !texHeight)
    {
        throw std::exception("Call SetSourceTexture before setting post-process effect");
    }

    float tu = 1.0f / float(texWidth);
    float tv = 1.0f / float(texHeight);

    // Sample from the 4 surrounding points. Since the center point will be in the exact
    // center of 4 texels, a 0.5f offset is needed to specify a texel center.
    auto ptr = reinterpret_cast<XMFLOAT4*>(constants.sampleOffsets);
    for (int y = 0; y < 2; y++)
    {
        for (int x = 0; x < 2; x++)
        {
            ptr->x = (float(x) - 0.5f) * tu;
            ptr->y = (float(y) - 0.5f) * tv;
            ++ptr;
        }
    }

}


void BasicPostProcess::Impl::DownScale4x4()
{
    mUseConstants = true;

    if (!texWidth || !texHeight)
    {
        throw std::exception("Call SetSourceTexture before setting post-process effect");
    }

    float tu = 1.0f / float(texWidth);
    float tv = 1.0f / float(texHeight);

    // Sample from the 16 surrounding points. Since the center point will be in the
    // exact center of 16 texels, a 1.5f offset is needed to specify a texel center.
    auto ptr = reinterpret_cast<XMFLOAT4*>(constants.sampleOffsets);
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            ptr->x = (float(x) - 1.5f) * tu;
            ptr->y = (float(y) - 1.5f) * tv;
            ++ptr;
        }
    }

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
void BasicPostProcess::Process(_In_ ID3D11DeviceContext* deviceContext, _In_opt_ std::function<void __cdecl()> setCustomState)
{
    pImpl->Process(deviceContext, setCustomState);
}


// Properties
void BasicPostProcess::SetSourceTexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->texture = value;

    if (value)
    {
        ComPtr<ID3D11Resource> res;
        value->GetResource(res.GetAddressOf());

        D3D11_RESOURCE_DIMENSION resType = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        res->GetType(&resType);

        switch (resType)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            ComPtr<ID3D11Texture1D> tex;
            ThrowIfFailed(res.As(&tex));

            D3D11_TEXTURE1D_DESC desc = {};
            tex->GetDesc(&desc);
            pImpl->texWidth = desc.Width;
            pImpl->texHeight = 1;
            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            ComPtr<ID3D11Texture2D> tex;
            ThrowIfFailed(res.As(&tex));

            D3D11_TEXTURE2D_DESC desc = {};
            tex->GetDesc(&desc);
            pImpl->texWidth = desc.Width;
            pImpl->texHeight = desc.Height;
            break;
        }

        default:
            throw std::exception("Unsupported texture type");
        }
    }
    else
    {
        pImpl->texWidth = pImpl->texHeight = 0;
    }
}


void BasicPostProcess::Set(Effect fx)
{
    pImpl->fx = fx;

    switch (fx)
    {
    case DownScale_2x2:
        pImpl->DownScale2x2();
        break;

    case DownScale_4x4:
        pImpl->DownScale4x4();
        break;

    default:
    case Copy:
    case Monochrome:
        pImpl->NoConstants();
        break;
    }
}