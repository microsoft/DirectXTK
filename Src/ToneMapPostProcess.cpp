//--------------------------------------------------------------------------------------
// File: ToneMapPostProcess.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PostProcess.h"

#include "AlignedNew.h"
#include "CommonStates.h"
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

#if defined(_XBOX_ONE) && defined(_TITLE)
    const int PixelShaderCount = 15;
    const int ShaderPermutationCount = 24;
#else
    const int PixelShaderCount = 9;
    const int ShaderPermutationCount = 12;
#endif

    // Constant buffer layout. Must match the shader!
    __declspec(align(16)) struct ToneMapConstants
    {
        // linearExposure is .x
        // paperWhiteNits is .y
        XMVECTOR parameters;
    };

    static_assert((sizeof(ToneMapConstants) % 16) == 0, "CB size not padded correctly");
}

// Include the precompiled shader code.
namespace
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    #include "Shaders/Compiled/XboxOneToneMap_VSQuad.inc"

    #include "Shaders/Compiled/XboxOneToneMap_PSCopy.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSSaturate.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSReinhard.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSACESFilmic.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PS_SRGB.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSSaturate_SRGB.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSReinhard_SRGB.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSACESFilmic_SRGB.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSHDR10.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSHDR10_Saturate.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSHDR10_Reinhard.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSHDR10_ACESFilmic.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSHDR10_Saturate_SRGB.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSHDR10_Reinhard_SRGB.inc"
    #include "Shaders/Compiled/XboxOneToneMap_PSHDR10_ACESFilmic_SRGB.inc"
#else
    #include "Shaders/Compiled/ToneMap_VSQuad.inc"

    #include "Shaders/Compiled/ToneMap_PSCopy.inc"
    #include "Shaders/Compiled/ToneMap_PSSaturate.inc"
    #include "Shaders/Compiled/ToneMap_PSReinhard.inc"
    #include "Shaders/Compiled/ToneMap_PSACESFilmic.inc"
    #include "Shaders/Compiled/ToneMap_PS_SRGB.inc"
    #include "Shaders/Compiled/ToneMap_PSSaturate_SRGB.inc"
    #include "Shaders/Compiled/ToneMap_PSReinhard_SRGB.inc"
    #include "Shaders/Compiled/ToneMap_PSACESFilmic_SRGB.inc"
    #include "Shaders/Compiled/ToneMap_PSHDR10.inc"
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
        { ToneMap_PSCopy,                   sizeof(ToneMap_PSCopy) },
        { ToneMap_PSSaturate,               sizeof(ToneMap_PSSaturate) },
        { ToneMap_PSReinhard,               sizeof(ToneMap_PSReinhard) },
        { ToneMap_PSACESFilmic,             sizeof(ToneMap_PSACESFilmic) },
        { ToneMap_PS_SRGB,                  sizeof(ToneMap_PS_SRGB) },
        { ToneMap_PSSaturate_SRGB,          sizeof(ToneMap_PSSaturate_SRGB) },
        { ToneMap_PSReinhard_SRGB,          sizeof(ToneMap_PSReinhard_SRGB) },
        { ToneMap_PSACESFilmic_SRGB,        sizeof(ToneMap_PSACESFilmic_SRGB) },
        { ToneMap_PSHDR10,                  sizeof(ToneMap_PSHDR10) },

#if defined(_XBOX_ONE) && defined(_TITLE)
        // Shaders that generate both HDR10 and GameDVR SDR signals via Multiple Render Targets.
        { ToneMap_PSHDR10_Saturate,         sizeof(ToneMap_PSHDR10_Saturate) },
        { ToneMap_PSHDR10_Reinhard,         sizeof(ToneMap_PSHDR10_Reinhard) },
        { ToneMap_PSHDR10_ACESFilmic,       sizeof(ToneMap_PSHDR10_ACESFilmic) },
        { ToneMap_PSHDR10_Saturate_SRGB,    sizeof(ToneMap_PSHDR10_Saturate_SRGB) },
        { ToneMap_PSHDR10_Reinhard_SRGB,    sizeof(ToneMap_PSHDR10_Reinhard_SRGB) },
        { ToneMap_PSHDR10_ACESFilmic_SRGB,  sizeof(ToneMap_PSHDR10_ACESFilmic_SRGB) },
#endif
    };

    static_assert(_countof(pixelShaders) == PixelShaderCount, "array/max mismatch");

    const int pixelShaderIndices[] =
    {
        // Linear EOTF
        0,  // Copy
        1,  // Saturate
        2,  // Reinhard
        3,  // ACES Filmic

        // Gamam22 EOTF
        4,  // SRGB
        5,  // Saturate_SRGB
        6,  // Reinhard_SRGB
        7,  // ACES Filmic

        // ST.2084 EOTF
        8,  // HDR10
        8,  // HDR10
        8,  // HDR10
        8,  // HDR10

#if defined(_XBOX_ONE) && defined(_TITLE)
        // MRT Linear EOTF
        9,  // HDR10+Saturate
        9,  // HDR10+Saturate
        10, // HDR10+Reinhard
        11, // HDR10+ACESFilmic

        // MRT Gamma22 EOTF
        12, // HDR10+Saturate_SRGB
        12, // HDR10+Saturate_SRGB
        13, // HDR10+Reinhard_SRGB
        14,  // HDR10+ACESFilmic

        // MRT ST.2084 EOTF
        9,  // HDR10+Saturate
        9,  // HDR10+Saturate
        10, // HDR10+Reinhard
        11, // HDR10+ACESFilmic
#endif
    };

    static_assert(_countof(pixelShaderIndices) == ShaderPermutationCount, "array/max mismatch");

    // Factory for lazily instantiating shaders.
    class DeviceResources
    {
    public:
        DeviceResources(_In_ ID3D11Device* device)
            : stateObjects(device),
            mDevice(device),
            mVertexShader{},
            mPixelShaders{},
            mMutex{}
        { }

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
        ID3D11PixelShader* GetPixelShader(int permutation)
        {
            assert(permutation >= 0 && permutation < ShaderPermutationCount);
            _Analysis_assume_(permutation >= 0 && permutation < ShaderPermutationCount);
            int shaderIndex = pixelShaderIndices[permutation];
            assert(shaderIndex >= 0 && shaderIndex < PixelShaderCount);
            _Analysis_assume_(shaderIndex >= 0 && shaderIndex < PixelShaderCount);

            return DemandCreate(mPixelShaders[shaderIndex], mMutex, [&](ID3D11PixelShader** pResult) -> HRESULT
            {
                HRESULT hr = mDevice->CreatePixelShader(pixelShaders[shaderIndex].code, pixelShaders[shaderIndex].length, nullptr, pResult);

                if (SUCCEEDED(hr))
                    SetDebugObjectName(*pResult, "ToneMapPostProcess");

                return hr;
            });
        }

        CommonStates                stateObjects;

    protected:
        ComPtr<ID3D11Device>        mDevice;
        ComPtr<ID3D11VertexShader>  mVertexShader;
        ComPtr<ID3D11PixelShader>   mPixelShaders[PixelShaderCount];
        std::mutex                  mMutex;
    };
}

class ToneMapPostProcess::Impl : public AlignedNew<ToneMapConstants>
{
public:
    Impl(_In_ ID3D11Device* device);

    void Process(_In_ ID3D11DeviceContext* deviceContext, std::function<void __cdecl()>& setCustomState);

    void SetDirtyFlag() { mDirtyFlags = INT_MAX; }

    int GetCurrentShaderPermutation() const;

    // Fields.
    ToneMapConstants                        constants;
    ComPtr<ID3D11ShaderResourceView>        hdrTexture;
    float                                   linearExposure;
    float                                   paperWhiteNits;

    Operator                                op;
    TransferFunction                        func;
    bool                                    mrt;

private:
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
    : constants{},
    linearExposure(1.f),
    paperWhiteNits(200.f),
    op(None),
    func(Linear),
    mrt(false),
    mDirtyFlags(INT_MAX),
    mConstantBuffer(device),
    mDeviceResources(deviceResourcesPool.DemandCreate(device))
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

    auto sampler = mDeviceResources->stateObjects.PointClamp();
    deviceContext->PSSetSamplers(0, 1, &sampler);

    // Set state objects.
    deviceContext->OMSetBlendState(mDeviceResources->stateObjects.Opaque(), nullptr, 0xffffffff);
    deviceContext->OMSetDepthStencilState(mDeviceResources->stateObjects.DepthNone(), 0);
    deviceContext->RSSetState(mDeviceResources->stateObjects.CullNone());

    // Set shaders.
    auto vertexShader = mDeviceResources->GetVertexShader();
    auto pixelShader = mDeviceResources->GetPixelShader(GetCurrentShaderPermutation());

    deviceContext->VSSetShader(vertexShader, nullptr, 0);
    deviceContext->PSSetShader(pixelShader, nullptr, 0);

    // Set constants.
    if (mDirtyFlags & Dirty_Parameters)
    {
        mDirtyFlags &= ~Dirty_Parameters;
        mDirtyFlags |= Dirty_ConstantBuffer;

        constants.parameters = XMVectorSet(linearExposure, paperWhiteNits, 0.f, 0.f);
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

    if (setCustomState)
    {
        setCustomState();
    }

    // Draw quad.
    deviceContext->IASetInputLayout(nullptr);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    deviceContext->Draw(3, 0);
}


int ToneMapPostProcess::Impl::GetCurrentShaderPermutation() const
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    int permutation = (mrt) ? 12 : 0;
    return permutation + (static_cast<int>(func) * static_cast<int>(Operator_Max)) + static_cast<int>(op);
#else
    return (static_cast<int>(func) * static_cast<int>(Operator_Max)) + static_cast<int>(op);
#endif
}


// Public constructor.
ToneMapPostProcess::ToneMapPostProcess(_In_ ID3D11Device* device)
  : pImpl(std::make_unique<Impl>(device))
{
}


// Move constructor.
ToneMapPostProcess::ToneMapPostProcess(ToneMapPostProcess&& moveFrom) noexcept
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
ToneMapPostProcess& ToneMapPostProcess::operator= (ToneMapPostProcess&& moveFrom) noexcept
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
ToneMapPostProcess::~ToneMapPostProcess()
{
}


// IPostProcess methods.
void ToneMapPostProcess::Process(_In_ ID3D11DeviceContext* deviceContext, _In_opt_ std::function<void __cdecl()> setCustomState)
{
    pImpl->Process(deviceContext, setCustomState);
}


// Shader control.
void ToneMapPostProcess::SetOperator(Operator op)
{
    if (op < 0 || op >= Operator_Max)
        throw std::out_of_range("Tonemap operator not defined");

    pImpl->op = op;
}


void ToneMapPostProcess::SetTransferFunction(TransferFunction func)
{
    if (func < 0 || func >= TransferFunction_Max)
        throw std::out_of_range("Electro-optical transfer function not defined");

    pImpl->func = func;
}


#if defined(_XBOX_ONE) && defined(_TITLE)
void ToneMapPostProcess::SetMRTOutput(bool value)
{
    pImpl->mrt = value;
}
#endif


// Properties
void ToneMapPostProcess::SetHDRSourceTexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->hdrTexture = value;
}


void ToneMapPostProcess::SetExposure(float exposureValue)
{
    pImpl->linearExposure = powf(2.f, exposureValue);
    pImpl->SetDirtyFlag();
}


void ToneMapPostProcess::SetST2084Parameter(float paperWhiteNits)
{
    pImpl->paperWhiteNits = paperWhiteNits;
    pImpl->SetDirtyFlag();
}
