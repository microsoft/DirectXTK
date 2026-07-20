//--------------------------------------------------------------------------------------
// File: NPREffect.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "EffectCommon.h"

namespace DirectX
{
    namespace EffectDirtyFlags
    {
        constexpr int ConstantBufferBones = 0x100000;
    }
}

using namespace DirectX;

namespace
{
    // Constant buffer layout. Must match the shader!
    struct NPREffectConstants
    {
        XMVECTOR lightDirectionAndCelBands;
        XMVECTOR diffuseColorAndAlpha;
        XMVECTOR specularColorAndSpecularThreshold;
        XMVECTOR rimColorAndPower;
        XMVECTOR extraSettings; // .x = SpecularSmoothing, .y = RimStrength, .z = RimStart, .w = RimEnd
        XMVECTOR goochCoolColorAndAlpha;
        XMVECTOR goochWarmColorAndBeta;
        XMVECTOR eyePosition;
        XMMATRIX world;
        XMVECTOR worldInverseTranspose[3];
        XMMATRIX worldViewProj;
    };

    static_assert((sizeof(NPREffectConstants) % 16) == 0, "CB size not padded correctly");

    XM_ALIGNED_STRUCT(16) BoneConstants
    {
        XMVECTOR Bones[SkinnedNPREffect::MaxBones][3];
    };

    static_assert((sizeof(BoneConstants) % 16) == 0, "CB size not padded correctly");

    // Traits type describes our characteristics to the EffectBase template.
    struct NPREffectTraits
    {
        using ConstantBufferType = NPREffectConstants;

        static constexpr int VertexShaderCount = 18;
        static constexpr int PixelShaderCount = 6;
        static constexpr int ShaderPermutationCount = 54;

        static constexpr int ModeCount = 3;
    };


    // Default values
    constexpr XMVECTORF32 s_defaultLightDir = { { { 0.f, -1.f, 0.f, 4.f } } };
    constexpr XMVECTORF32 s_defaultDiffuse = { { { 1.f, 1.f, 1.f, 1.f } } };
    constexpr XMVECTORF32 s_defaultSpecular = { { { 1.f, 1.f, 1.f, 0.95f } } };
    constexpr XMVECTORF32 s_defaultRim = { { { 0.f, 0.f, 0.f, 4.f } } };
    constexpr XMVECTORF32 s_defaultExtraSettings = { { { 0.004f, 0.75f, 0.2f, 0.6f } } };
    constexpr XMVECTORF32 s_defaultCool = { { { 0.f, 0.f, 0.55f, 0.25f } } };
    constexpr XMVECTORF32 s_defaultWarm = { { { 0.3f, 0.3f, 0.f, 0.25f } } };
}

// Internal NPREffect implementation class.
class NPREffect::Impl : public EffectBase<NPREffectTraits>
{
public:
    explicit Impl(_In_ ID3D11Device* device);

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    Impl(Impl&&) = default;
    Impl& operator=(Impl&&) = default;

    void Initialize(_In_ ID3D11Device* device, bool enableSkinning);

    bool vertexColorEnabled;
    bool biasedVertexNormals;
    bool instancing;
    bool textureEnabled;
    int weightsPerVertex;

    NPREffect::Mode nprMode;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> matcap;

    int GetCurrentShaderPermutation() const noexcept;

    void Apply(_In_ ID3D11DeviceContext* deviceContext);

    BoneConstants boneConstants;

private:
    ConstantBuffer<BoneConstants> mBones;
};


#pragma region Shaders
// Include the precompiled shader code.
namespace
{
#if defined(_XBOX_ONE) && defined(_TITLE)
#include "XboxOneNPREffect_VSNPREffect.inc"
#include "XboxOneNPREffect_VSNPREffectInst.inc"

#include "XboxOneNPREffect_VSNPREffectVc.inc"
#include "XboxOneNPREffect_VSNPREffectVcInst.inc"

#include "XboxOneNPREffect_VSNPREffectBn.inc"
#include "XboxOneNPREffect_VSNPREffectBnInst.inc"

#include "XboxOneNPREffect_VSNPREffectVcBn.inc"
#include "XboxOneNPREffect_VSNPREffectVcBnInst.inc"

#include "XboxOneNPREffect_VSNPREffectTx.inc"
#include "XboxOneNPREffect_VSNPREffectInstTx.inc"

#include "XboxOneNPREffect_VSNPREffectVcTx.inc"
#include "XboxOneNPREffect_VSNPREffectVcInstTx.inc"

#include "XboxOneNPREffect_VSNPREffectBnTx.inc"
#include "XboxOneNPREffect_VSNPREffectBnInstTx.inc"

#include "XboxOneNPREffect_VSNPREffectVcBnTx.inc"
#include "XboxOneNPREffect_VSNPREffectVcBnInstTx.inc"

#include "XboxOneNPREffect_VSSkinnedNPREffectTx.inc"
#include "XboxOneNPREffect_VSSkinnedNPREffectTxBn.inc"

#include "XboxOneNPREffect_PSCelShading.inc"
#include "XboxOneNPREffect_PSCelShadingTx.inc"

#include "XboxOneNPREffect_PSGoochShading.inc"
#include "XboxOneNPREffect_PSGoochShadingTx.inc"

#include "XboxOneNPREffect_PSMatCapShading.inc"
#include "XboxOneNPREffect_PSMatCapShadingTx.inc"
#else
#include "NPREffect_VSNPREffect.inc"
#include "NPREffect_VSNPREffectInst.inc"

#include "NPREffect_VSNPREffectVc.inc"
#include "NPREffect_VSNPREffectVcInst.inc"

#include "NPREffect_VSNPREffectBn.inc"
#include "NPREffect_VSNPREffectBnInst.inc"

#include "NPREffect_VSNPREffectVcBn.inc"
#include "NPREffect_VSNPREffectVcBnInst.inc"

#include "NPREffect_VSNPREffectTx.inc"
#include "NPREffect_VSNPREffectInstTx.inc"

#include "NPREffect_VSNPREffectVcTx.inc"
#include "NPREffect_VSNPREffectVcInstTx.inc"

#include "NPREffect_VSNPREffectBnTx.inc"
#include "NPREffect_VSNPREffectBnInstTx.inc"

#include "NPREffect_VSNPREffectVcBnTx.inc"
#include "NPREffect_VSNPREffectVcBnInstTx.inc"

#include "NPREffect_VSSkinnedNPREffectTx.inc"
#include "NPREffect_VSSkinnedNPREffectTxBn.inc"

#include "NPREffect_PSCelShading.inc"
#include "NPREffect_PSCelShadingTx.inc"

#include "NPREffect_PSGoochShading.inc"
#include "NPREffect_PSGoochShadingTx.inc"

#include "NPREffect_PSMatCapShading.inc"
#include "NPREffect_PSMatCapShadingTx.inc"
#endif
}


template<>
const ShaderBytecode EffectBase<NPREffectTraits>::VertexShaderBytecode[] =
{
    { NPREffect_VSNPREffect,            sizeof(NPREffect_VSNPREffect)            },
    { NPREffect_VSNPREffectVc,          sizeof(NPREffect_VSNPREffectVc)          },
    { NPREffect_VSNPREffectBn,          sizeof(NPREffect_VSNPREffectBn)          },
    { NPREffect_VSNPREffectVcBn,        sizeof(NPREffect_VSNPREffectVcBn)        },
    { NPREffect_VSNPREffectInst,        sizeof(NPREffect_VSNPREffectInst)        },
    { NPREffect_VSNPREffectVcInst,      sizeof(NPREffect_VSNPREffectVcInst)      },
    { NPREffect_VSNPREffectBnInst,      sizeof(NPREffect_VSNPREffectBnInst)      },
    { NPREffect_VSNPREffectVcBnInst,    sizeof(NPREffect_VSNPREffectVcBnInst)    },
    { NPREffect_VSNPREffectTx,          sizeof(NPREffect_VSNPREffectTx)          },
    { NPREffect_VSNPREffectVcTx,        sizeof(NPREffect_VSNPREffectVcTx)        },
    { NPREffect_VSNPREffectBnTx,        sizeof(NPREffect_VSNPREffectBnTx)        },
    { NPREffect_VSNPREffectVcBnTx,      sizeof(NPREffect_VSNPREffectVcBnTx)      },
    { NPREffect_VSNPREffectInstTx,      sizeof(NPREffect_VSNPREffectInstTx)      },
    { NPREffect_VSNPREffectVcInstTx,    sizeof(NPREffect_VSNPREffectVcInstTx)    },
    { NPREffect_VSNPREffectBnInstTx,    sizeof(NPREffect_VSNPREffectBnInstTx)    },
    { NPREffect_VSNPREffectVcBnInstTx,  sizeof(NPREffect_VSNPREffectVcBnInstTx)  },
    { NPREffect_VSSkinnedNPREffectTx,   sizeof(NPREffect_VSSkinnedNPREffectTx)   },
    { NPREffect_VSSkinnedNPREffectTxBn, sizeof(NPREffect_VSSkinnedNPREffectTxBn) },
};


template<>
const int EffectBase<NPREffectTraits>::VertexShaderIndices[] =
{
    0,      // cel shading
    0,      // gooch shading
    0,      // matcap shading

    1,      // vertex color + cel shading
    1,      // vertex color + gooch shading
    1,      // vertex color + matcap shading

    2,      // cel shading (biased vertex normal)
    2,      // gooch shading (biased vertex normal)
    2,      // matcap shading (biased vertex normal)

    3,      // vertex color (biased vertex normal) + cel shading
    3,      // vertex color (biased vertex normal) + gooch shading
    3,      // vertex color (biased vertex normal) + matcap shading

    4,      // instancing + cel shading
    4,      // instancing + gooch shading
    4,      // instancing + matcap shading

    5,      // instancing + vertex color + cel shading
    5,      // instancing + vertex color + gooch shading
    5,      // instancing + vertex color + matcap shading

    6,      // instancing (biased vertex normal) + cel shading
    6,      // instancing (biased vertex normal) + gooch shading
    6,      // instancing (biased vertex normal) + matcap shading

    7,      // instancing + vertex color (biased vertex normal) + cel shading
    7,      // instancing + vertex color (biased vertex normal) + gooch shading
    7,      // instancing + vertex color (biased vertex normal) + matcap shading

    8,      // cel shading + texture
    8,      // gooch shading + texture
    8,      // matcap shading + texture

    9,      // vertex color + cel shading + texture
    9,      // vertex color + gooch shading + texture
    9,      // vertex color + matcap shading + texture

    10,     // cel shading (biased vertex normal) + texture
    10,     // gooch shading (biased vertex normal) + texture
    10,     // matcap shading (biased vertex normal) + texture

    11,     // vertex color (biased vertex normal) + cel shading + texture
    11,     // vertex color (biased vertex normal) + gooch shading + texture
    11,     // vertex color (biased vertex normal) + matcap shading + texture

    12,     // instancing + cel shading + texture
    12,     // instancing + gooch shading + texture
    12,     // instancing + matcap shading + texture

    13,     // instancing + vertex color + cel shading + texture
    13,     // instancing + vertex color + gooch shading + texture
    13,     // instancing + vertex color + matcap shading + texture

    14,     // instancing (biased vertex normal) + cel shading + texture
    14,     // instancing (biased vertex normal) + gooch shading + texture
    14,     // instancing (biased vertex normal) + matcap shading + texture

    15,     // instancing + vertex color (biased vertex normal) + cel shading + texture
    15,     // instancing + vertex color (biased vertex normal) + gooch shading + texture
    15,     // instancing + vertex color (biased vertex normal) + matcap shading + texture

    16,     // skinning + cel shading
    16,     // skinning + gooch shading
    16,     // skinning + matcap shading

    17,     // skinning (biased vertex normal) + cel shading
    17,     // skinning (biased vertex normal) + gooch shading
    17,     // skinning (biased vertex normal) + matcap shading
};


template<>
const ShaderBytecode EffectBase<NPREffectTraits>::PixelShaderBytecode[] =
{
    { NPREffect_PSCelShading,      sizeof(NPREffect_PSCelShading)      },
    { NPREffect_PSGoochShading,    sizeof(NPREffect_PSGoochShading)    },
    { NPREffect_PSMatCapShading,   sizeof(NPREffect_PSMatCapShading)   },
    { NPREffect_PSCelShadingTx,    sizeof(NPREffect_PSCelShadingTx)    },
    { NPREffect_PSGoochShadingTx,  sizeof(NPREffect_PSGoochShadingTx)  },
    { NPREffect_PSMatCapShadingTx, sizeof(NPREffect_PSMatCapShadingTx) },
};


template<>
const int EffectBase<NPREffectTraits>::PixelShaderIndices[] =
{
    0,      // cel shading
    1,      // gooch shading
    2,      // matcap shading

    0,      // vertex color + cel shading
    1,      // vertex color + gooch shading
    2,      // vertex color + matcap shading

    0,      // cel shading (biased vertex normal)
    1,      // gooch shading (biased vertex normal)
    2,      // matcap shading (biased vertex normal)

    0,      // vertex color (biased vertex normal) + cel shading
    1,      // vertex color (biased vertex normal) + gooch shading
    2,      // vertex color (biased vertex normal) + matcap shading

    0,      // instancing + cel shading
    1,      // instancing + gooch shading
    2,      // instancing + matcap shading

    0,      // instancing + vertex color + cel shading
    1,      // instancing + vertex color + gooch shading
    2,      // instancing + vertex color + matcap shading

    0,      // instancing (biased vertex normal) + cel shading
    1,      // instancing (biased vertex normal) + gooch shading
    2,      // instancing (biased vertex normal) + matcap shading

    0,      // instancing + vertex color (biased vertex normal) + cel shading
    1,      // instancing + vertex color (biased vertex normal) + gooch shading
    2,      // instancing + vertex color (biased vertex normal) + matcap shading

    3,      // cel shading + texture
    4,      // gooch shading + texture
    5,      // matcap shading + texture

    3,      // vertex color + cel shading + texture
    4,      // vertex color + gooch shading + texture
    5,      // vertex color + matcap shading + texture

    3,      // cel shading (biased vertex normal) + texture
    4,      // gooch shading (biased vertex normal) + texture
    5,      // matcap shading (biased vertex normal) + texture

    3,      // vertex color (biased vertex normal) + cel shading + texture
    4,      // vertex color (biased vertex normal) + gooch shading + texture
    5,      // vertex color (biased vertex normal) + matcap shading + texture

    3,      // instancing + cel shading + texture
    4,      // instancing + gooch shading + texture
    5,      // instancing + matcap shading + texture

    3,      // instancing + vertex color + cel shading + texture
    4,      // instancing + vertex color + gooch shading + texture
    5,      // instancing + vertex color + matcap shading + texture

    3,      // instancing (biased vertex normal) + cel shading + texture
    4,      // instancing (biased vertex normal) + gooch shading + texture
    5,      // instancing (biased vertex normal) + matcap shading + texture

    3,      // instancing + vertex color (biased vertex normal) + cel shading + texture
    4,      // instancing + vertex color (biased vertex normal) + gooch shading + texture
    5,      // instancing + vertex color (biased vertex normal) + matcap shading + texture

    0,      // skinning + cel shading
    1,      // skinning + gooch shading
    2,      // skinning + matcap shading

    3,      // skinning (biased vertex normal) + cel shading
    4,      // skinning (biased vertex normal) + gooch shading
    5,      // skinning (biased vertex normal) + matcap shading
};
#pragma endregion

// Global pool of per-device NPREffect resources.
template<>
SharedResourcePool<ID3D11Device*, EffectBase<NPREffectTraits>::DeviceResources> EffectBase<NPREffectTraits>::deviceResourcesPool = {};


// Constructor.
NPREffect::Impl::Impl(_In_ ID3D11Device* device)
    : EffectBase(device),
    vertexColorEnabled(false),
    biasedVertexNormals(false),
    instancing(false),
    textureEnabled(false),
    weightsPerVertex(0),
    nprMode(NPREffect::Mode_Cel)
{
    static_assert(static_cast<int>(std::size(EffectBase<NPREffectTraits>::VertexShaderIndices)) == NPREffectTraits::ShaderPermutationCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NPREffectTraits>::VertexShaderBytecode)) == NPREffectTraits::VertexShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NPREffectTraits>::PixelShaderBytecode)) == NPREffectTraits::PixelShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NPREffectTraits>::PixelShaderIndices)) == NPREffectTraits::ShaderPermutationCount, "array/max mismatch");
}

void NPREffect::Impl::Initialize(_In_ ID3D11Device* device, bool enableSkinning)
{
    constants.lightDirectionAndCelBands = s_defaultLightDir;
    constants.diffuseColorAndAlpha = s_defaultDiffuse;
    constants.specularColorAndSpecularThreshold = s_defaultSpecular;
    constants.rimColorAndPower = s_defaultRim;
    constants.extraSettings = s_defaultExtraSettings;
    constants.goochCoolColorAndAlpha = s_defaultCool;
    constants.goochWarmColorAndBeta = s_defaultWarm;
    constants.eyePosition = g_XMZero;

    if (enableSkinning)
    {
        weightsPerVertex = 4;

        mBones.Create(device);

        for (size_t j = 0; j < SkinnedNPREffect::MaxBones; ++j)
        {
            boneConstants.Bones[j][0] = g_XMIdentityR0;
            boneConstants.Bones[j][1] = g_XMIdentityR1;
            boneConstants.Bones[j][2] = g_XMIdentityR2;
        }
    }
}

int NPREffect::Impl::GetCurrentShaderPermutation() const noexcept
{
    int permutation = static_cast<int>(nprMode);

    if (weightsPerVertex > 0)
    {
        if (biasedVertexNormals)
        {
            // Compressed normals need to be scaled and biased in the vertex shader.
            permutation += 3;
        }

        // Vertex skinning does not support vertex colors or instancing, always includes a texture.
        permutation += 48;

        return permutation;
    }

    // Support vertex coloring?
    if (vertexColorEnabled)
    {
        permutation += 3;
    }

    if (biasedVertexNormals)
    {
        // Compressed normals need to be scaled and biased in the vertex shader.
        permutation += 6;
    }

    if (instancing)
    {
        // Vertex shader needs to use vertex matrix transform.
        permutation += 12;
    }

    // Use shaders without texture coordinates?
    if (textureEnabled)
    {
        permutation += 24;
    }

    return permutation;
}


// Sets our state onto the D3D device.
void NPREffect::Impl::Apply(_In_ ID3D11DeviceContext* deviceContext)
{
    assert(deviceContext != nullptr);

    // Compute derived parameter values.
    matrices.SetConstants(
        dirtyFlags,
        constants.world,
        constants.worldInverseTranspose,
        constants.worldViewProj,
        constants.eyePosition);

    if (weightsPerVertex > 0)
    {
    #if defined(_XBOX_ONE) && defined(_TITLE)
        void* grfxMemoryBone;
        mBones.SetData(deviceContext, boneConstants, &grfxMemoryBone);

        ComPtr<ID3D11DeviceContextX> deviceContextX;
        ThrowIfFailed(deviceContext->QueryInterface(IID_GRAPHICS_PPV_ARGS(deviceContextX.GetAddressOf())));

        deviceContextX->VSSetPlacementConstantBuffer(1, mBones.GetBuffer(), grfxMemoryBone);
    #else
        if (dirtyFlags & EffectDirtyFlags::ConstantBufferBones)
        {
            mBones.SetData(deviceContext, boneConstants);
            dirtyFlags &= ~EffectDirtyFlags::ConstantBufferBones;
        }

        ID3D11Buffer* buffer = mBones.GetBuffer();
        deviceContext->VSSetConstantBuffers(1, 1, &buffer);
    #endif
    }

    // Set texture.
    switch (nprMode)
    {
    case Mode_MatCap:
        if (textureEnabled)
        {
            ID3D11ShaderResourceView* srvs[] = { texture.Get(), matcap.Get() };
            deviceContext->PSSetShaderResources(0, 2, srvs);
        }
        else
        {
            ID3D11ShaderResourceView* srv = matcap.Get();
            deviceContext->PSSetShaderResources(0, 1, &srv);
        }
        break;

    default:
        if (textureEnabled)
        {
            ID3D11ShaderResourceView* srv = texture.Get();
            deviceContext->PSSetShaderResources(0, 1, &srv);
        }
        break;
    }

    // Set shaders and constant buffers.
    ApplyShaders(deviceContext, GetCurrentShaderPermutation());
}


// Public constructor.
NPREffect::NPREffect(_In_ ID3D11Device* device, bool skinningEnabled)
    : pImpl(std::make_unique<Impl>(device))
{
    pImpl->Initialize(device, skinningEnabled);
}

NPREffect::NPREffect(NPREffect&&) noexcept = default;
NPREffect& NPREffect::operator= (NPREffect&&) noexcept = default;
NPREffect::~NPREffect() = default;


// IEffect methods.
void NPREffect::Apply(_In_ ID3D11DeviceContext* deviceContext)
{
    pImpl->Apply(deviceContext);
}


void NPREffect::GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength)
{
    pImpl->GetVertexShaderBytecode(pImpl->GetCurrentShaderPermutation(), pShaderByteCode, pByteCodeLength);
}


// Camera settings.
void XM_CALLCONV NPREffect::SetWorld(FXMMATRIX value)
{
    pImpl->matrices.world = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose;
}


void XM_CALLCONV NPREffect::SetView(FXMMATRIX value)
{
    pImpl->matrices.view = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::EyePosition;
}


void XM_CALLCONV NPREffect::SetProjection(FXMMATRIX value)
{
    pImpl->matrices.projection = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj;
}


void XM_CALLCONV NPREffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
{
    pImpl->matrices.world = world;
    pImpl->matrices.view = view;
    pImpl->matrices.projection = projection;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::EyePosition;
}


// Light settings.
void NPREffect::SetLightingEnabled(bool)
{
    // Unsupported interface.
}


void NPREffect::SetPerPixelLighting(bool)
{
    // Unsupported interface.
}


void NPREffect::SetAmbientLightColor(FXMVECTOR)
{
    // Unsupported interface.
}


void NPREffect::SetLightEnabled(int, bool)
{
    // Unsupported interface.
}


void NPREffect::SetLightDirection(int whichLight, FXMVECTOR value)
{
    if (whichLight != 0)
    {
        // Only support one light
        return;
    }

    // Set xyz to new value, but preserve existing w (cel bands).
    pImpl->constants.lightDirectionAndCelBands = XMVectorSelect(pImpl->constants.lightDirectionAndCelBands, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetLightDiffuseColor(int, FXMVECTOR)
{
    // Unsupported interface.
}


void NPREffect::SetLightSpecularColor(int, FXMVECTOR)
{
    // Unsupported interface.
}


void NPREffect::EnableDefaultLighting()
{
    // Set xyz to new value, but preserve existing w (cel bands).
    pImpl->constants.lightDirectionAndCelBands = XMVectorSelect(pImpl->constants.lightDirectionAndCelBands, s_defaultLightDir, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Material settings.
void NPREffect::SetDiffuseColor(FXMVECTOR value)
{
    // Set xyz, preserve w (alpha).
    pImpl->constants.diffuseColorAndAlpha = XMVectorSelect(pImpl->constants.diffuseColorAndAlpha, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetSpecularColor(FXMVECTOR value)
{
    // Set xyz, preserve w (specular threshold).
    pImpl->constants.specularColorAndSpecularThreshold = XMVectorSelect(pImpl->constants.specularColorAndSpecularThreshold, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetSpecularThreshold(float threshold, float smoothing)
{
    if (threshold < 0.f || threshold > 1.f)
    {
        throw std::invalid_argument("Specular threshold must be between 0 and 1");
    }

    if (smoothing < 0.f || smoothing > 1.f)
    {
        throw std::invalid_argument("Specular smoothing must be between 0 and 1");
    }

    // Set w of specularColorAndSpecularThreshold.
    pImpl->constants.specularColorAndSpecularThreshold = XMVectorSetW(pImpl->constants.specularColorAndSpecularThreshold, threshold);

    // Set x of extraSettings.
    pImpl->constants.extraSettings = XMVectorSetX(pImpl->constants.extraSettings, smoothing);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::DisableSpecular()
{
    // Set specular color to black, threshold to 1
    pImpl->constants.specularColorAndSpecularThreshold = g_XMIdentityR3;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetAlpha(float value)
{
    // Set w of diffuseColorAndAlpha.
    pImpl->constants.diffuseColorAndAlpha = XMVectorSetW(pImpl->constants.diffuseColorAndAlpha, value);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetColorAndAlpha(FXMVECTOR value)
{
    pImpl->constants.diffuseColorAndAlpha = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Texture settings.
void NPREffect::SetTextureEnabled(bool value)
{
    if (!value && (pImpl->weightsPerVertex > 0))
    {
        throw std::invalid_argument("Texturing is alaways enabled for SkinnedNPREffect");
    }

    pImpl->textureEnabled = value;
}


void NPREffect::SetTexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->texture = value;
}


// Shader mode setting.
void NPREffect::SetMode(Mode mode)
{
    if (static_cast<int>(mode) < 0 || static_cast<int>(mode) >= NPREffectTraits::ModeCount)
    {
        throw std::invalid_argument("Unsupported mode");
    }

    pImpl->nprMode = mode;
}


// Cel shading setting.
void NPREffect::SetCelShaderBands(int bands)
{
    if (bands < 1)
    {
        throw std::invalid_argument("Cel shading bands must be greater than 0");
    }

    // Set w of lightDirectionAndCelBands.
    pImpl->constants.lightDirectionAndCelBands = XMVectorSetW(pImpl->constants.lightDirectionAndCelBands, static_cast<float>(bands));

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Gooch shading settings.
void NPREffect::SetGoochCoolColor(FXMVECTOR value, float alpha)
{
    pImpl->constants.goochCoolColorAndAlpha = XMVectorSetW(value, alpha);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetGoochWarmColor(FXMVECTOR value, float beta)
{
    pImpl->constants.goochWarmColorAndBeta = XMVectorSetW(value, beta);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// MatCap shading setting.
void NPREffect::SetMatCap(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->matcap = value;
}


// Rim lighting settings.
void NPREffect::SetRimLightingColor(FXMVECTOR value)
{
    // Set xyz, preserve w (rim power).
    pImpl->constants.rimColorAndPower = XMVectorSelect(pImpl->constants.rimColorAndPower, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetRimLightingPower(float power)
{
    // Set w of rimColorAndPower.
    pImpl->constants.rimColorAndPower = XMVectorSetW(pImpl->constants.rimColorAndPower, power);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetRimLightingIntensity(float strength)
{
    if (strength < 0.f || strength > 1.f)
    {
        throw std::invalid_argument("Rim lighting strength must be between 0 and 1");
    }

    // Set y of extraSettings.
    pImpl->constants.extraSettings = XMVectorSetY(pImpl->constants.extraSettings, strength);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetRimLightingRange(float start, float end)
{
    if (start < 0.f || end > 1.f || start > end)
    {
        throw std::invalid_argument("Rim lighting start/end must be between 0 and 1");
    }

    // Set zw of extraSettings.
    XMVECTORF32 range = { { { 0.f, 0.f, start, end } } };
    pImpl->constants.extraSettings = XMVectorSelect(range, pImpl->constants.extraSettings, g_XMSelect1100);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::DisableRimLighting()
{
    pImpl->constants.rimColorAndPower = g_XMIdentityR3;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}

// Vertex color setting.
void NPREffect::SetVertexColorEnabled(bool value)
{
    if (value && (pImpl->weightsPerVertex > 0))
    {
        throw std::invalid_argument("Per-vertex color is not supported for SkinnedNPREffect");
    }

    pImpl->vertexColorEnabled = value;
}


// Normal compression settings.
void NPREffect::SetBiasedVertexNormals(bool value)
{
    pImpl->biasedVertexNormals = value;
}


// Instancing settings.
void NPREffect::SetInstancingEnabled(bool value)
{
    if (value && (pImpl->weightsPerVertex > 0))
    {
        throw std::invalid_argument("Instancing is not supported for SkinnedNPREffect");
    }

    pImpl->instancing = value;
}


//--------------------------------------------------------------------------------------
// SkinnedNPREffect
//--------------------------------------------------------------------------------------

SkinnedNPREffect::~SkinnedNPREffect()
{}

// Animation settings.
void SkinnedNPREffect::SetWeightsPerVertex(int value)
{
    if ((value != 1) &&
        (value != 2) &&
        (value != 4))
    {
        throw std::invalid_argument("WeightsPerVertex must be 1, 2, or 4");
    }

    pImpl->weightsPerVertex = value;
}


void SkinnedNPREffect::SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count)
{
    if (count > MaxBones)
        throw std::invalid_argument("count parameter exceeds MaxBones");

    auto boneConstant = pImpl->boneConstants.Bones;

    for (size_t i = 0; i < count; i++)
    {
    #if DIRECTX_MATH_VERSION >= 313
        XMStoreFloat3x4A(reinterpret_cast<XMFLOAT3X4A*>(&boneConstant[i]), value[i]);
    #else
            // Xbox One XDK has an older version of DirectXMath
        XMMATRIX boneMatrix = XMMatrixTranspose(value[i]);

        boneConstant[i][0] = boneMatrix.r[0];
        boneConstant[i][1] = boneMatrix.r[1];
        boneConstant[i][2] = boneMatrix.r[2];
    #endif
    }

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferBones;
}


void SkinnedNPREffect::ResetBoneTransforms()
{
    auto boneConstant = pImpl->boneConstants.Bones;

    for (size_t i = 0; i < MaxBones; ++i)
    {
        boneConstant[i][0] = g_XMIdentityR0;
        boneConstant[i][1] = g_XMIdentityR1;
        boneConstant[i][2] = g_XMIdentityR2;
    }

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferBones;
}
