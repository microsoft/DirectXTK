//--------------------------------------------------------------------------------------
// File: NormalMapEffect.cpp
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
#include "EffectCommon.h"

using namespace DirectX;


// Constant buffer layout. Must match the shader!
struct NormalMapEffectConstants
{
    XMVECTOR diffuseColor;
    XMVECTOR emissiveColor;
    XMVECTOR specularColorAndPower;
    
    XMVECTOR lightDirection[IEffectLights::MaxDirectionalLights];
    XMVECTOR lightDiffuseColor[IEffectLights::MaxDirectionalLights];
    XMVECTOR lightSpecularColor[IEffectLights::MaxDirectionalLights];

    XMVECTOR eyePosition;

    XMVECTOR fogColor;
    XMVECTOR fogVector;

    XMMATRIX world;
    XMVECTOR worldInverseTranspose[3];
    XMMATRIX worldViewProj;
};

static_assert( ( sizeof(NormalMapEffectConstants) % 16 ) == 0, "CB size not padded correctly" );


// Traits type describes our characteristics to the EffectBase template.
struct NormalMapEffectTraits
{
    typedef NormalMapEffectConstants ConstantBufferType;

    static const int VertexShaderCount = 4;
    static const int PixelShaderCount = 4;
    static const int ShaderPermutationCount = 16;
};


// Internal NormalMapEffect implementation class.
class NormalMapEffect::Impl : public EffectBase<NormalMapEffectTraits>
{
public:
    Impl(_In_ ID3D11Device* device);

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> specularTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalTexture;

    bool vertexColorEnabled;
    bool biasedVertexNormals;
  
    EffectLights lights;

    int GetCurrentShaderPermutation() const;

    void Apply(_In_ ID3D11DeviceContext* deviceContext);
};


// Include the precompiled shader code.
namespace
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTx.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxVc.inc"

    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxBn.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxVcBn.inc"

    #include "Shaders/Compiled/XboxOneNormalMapEffect_PSNormalPixelLightingTx.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_PSNormalPixelLightingTxNoFog.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_PSNormalPixelLightingTxNoSpec.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_PSNormalPixelLightingTxNoFogSpec.inc"
#else    
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTx.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxVc.inc"

    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxBn.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxVcBn.inc"

    #include "Shaders/Compiled/NormalMapEffect_PSNormalPixelLightingTx.inc"
    #include "Shaders/Compiled/NormalMapEffect_PSNormalPixelLightingTxNoFog.inc"
    #include "Shaders/Compiled/NormalMapEffect_PSNormalPixelLightingTxNoSpec.inc"
    #include "Shaders/Compiled/NormalMapEffect_PSNormalPixelLightingTxNoFogSpec.inc"
#endif
}


template<>
const ShaderBytecode EffectBase<NormalMapEffectTraits>::VertexShaderBytecode[] =
{    
    { NormalMapEffect_VSNormalPixelLightingTx,     sizeof(NormalMapEffect_VSNormalPixelLightingTx)     },
    { NormalMapEffect_VSNormalPixelLightingTxVc,   sizeof(NormalMapEffect_VSNormalPixelLightingTxVc)   },

    { NormalMapEffect_VSNormalPixelLightingTxBn,   sizeof(NormalMapEffect_VSNormalPixelLightingTxBn)   },
    { NormalMapEffect_VSNormalPixelLightingTxVcBn, sizeof(NormalMapEffect_VSNormalPixelLightingTxVcBn) },
};


template<>
const int EffectBase<NormalMapEffectTraits>::VertexShaderIndices[] =
{    
    0,      // pixel lighting + texture
    0,      // pixel lighting + texture, no fog
    1,      // pixel lighting + texture + vertex color
    1,      // pixel lighting + texture + vertex color, no fog

    0,      // pixel lighting + texture, no specular
    0,      // pixel lighting + texture, no fog or specular
    1,      // pixel lighting + texture + vertex color, no specular
    1,      // pixel lighting + texture + vertex color, no fog or specular

    2,      // pixel lighting (biased vertex normal/tangent) + texture
    2,      // pixel lighting (biased vertex normal/tangent) + texture, no fog
    3,      // pixel lighting (biased vertex normal/tangent) + texture + vertex color
    3,      // pixel lighting (biased vertex normal/tangent) + texture + vertex color, no fog

    2,      // pixel lighting (biased vertex normal/tangent) + texture, no specular
    2,      // pixel lighting (biased vertex normal/tangent) + texture, no fog or specular
    3,      // pixel lighting (biased vertex normal/tangent) + texture + vertex color, no specular
    3,      // pixel lighting (biased vertex normal/tangent) + texture + vertex color, no fog or specular
};


template<>
const ShaderBytecode EffectBase<NormalMapEffectTraits>::PixelShaderBytecode[] =
{
    { NormalMapEffect_PSNormalPixelLightingTx,          sizeof(NormalMapEffect_PSNormalPixelLightingTx)          },
    { NormalMapEffect_PSNormalPixelLightingTxNoFog,     sizeof(NormalMapEffect_PSNormalPixelLightingTxNoFog)     },
    { NormalMapEffect_PSNormalPixelLightingTxNoSpec,    sizeof(NormalMapEffect_PSNormalPixelLightingTxNoSpec)    },
    { NormalMapEffect_PSNormalPixelLightingTxNoFogSpec, sizeof(NormalMapEffect_PSNormalPixelLightingTxNoFogSpec) },
};


template<>
const int EffectBase<NormalMapEffectTraits>::PixelShaderIndices[] =
{    
    0,      // pixel lighting + texture
    1,      // pixel lighting + texture, no fog
    0,      // pixel lighting + texture + vertex color
    1,      // pixel lighting + texture + vertex color, no fog

    2,      // pixel lighting + texture, no specular
    3,      // pixel lighting + texture, no fog or specular
    2,      // pixel lighting + texture + vertex color, no specular
    3,      // pixel lighting + texture + vertex color, no fog or specular

    0,      // pixel lighting (biased vertex normal/tangent) + texture
    1,      // pixel lighting (biased vertex normal/tangent) + texture, no fog
    0,      // pixel lighting (biased vertex normal/tangent) + texture + vertex color
    1,      // pixel lighting (biased vertex normal/tangent) + texture + vertex color, no fog

    2,      // pixel lighting (biased vertex normal/tangent) + texture, no specular
    3,      // pixel lighting (biased vertex normal/tangent) + texture, no fog or specular
    2,      // pixel lighting (biased vertex normal/tangent) + texture + vertex color, no specular
    3,      // pixel lighting (biased vertex normal/tangent) + texture + vertex color, no fog or specular
};


// Global pool of per-device NormalMapEffect resources.
template<>
SharedResourcePool<ID3D11Device*, EffectBase<NormalMapEffectTraits>::DeviceResources> EffectBase<NormalMapEffectTraits>::deviceResourcesPool;


// Constructor.
NormalMapEffect::Impl::Impl(_In_ ID3D11Device* device)
  : EffectBase(device),
    vertexColorEnabled(false),
    biasedVertexNormals(false)
{
    static_assert( _countof(EffectBase<NormalMapEffectTraits>::VertexShaderIndices) == NormalMapEffectTraits::ShaderPermutationCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<NormalMapEffectTraits>::VertexShaderBytecode) == NormalMapEffectTraits::VertexShaderCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<NormalMapEffectTraits>::PixelShaderBytecode) == NormalMapEffectTraits::PixelShaderCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<NormalMapEffectTraits>::PixelShaderIndices) == NormalMapEffectTraits::ShaderPermutationCount, "array/max mismatch" );

    lights.InitializeConstants(constants.specularColorAndPower, constants.lightDirection, constants.lightDiffuseColor, constants.lightSpecularColor);
}


int NormalMapEffect::Impl::GetCurrentShaderPermutation() const
{
    int permutation = 0;

    // Use optimized shaders if fog is disabled.
    if (!fog.enabled)
    {
        permutation += 1;
    }

    // Support vertex coloring?
    if (vertexColorEnabled)
    {
        permutation += 2;
    }

    // Specular map?
    if (!specularTexture)
    {
        permutation += 4;
    }

    if (biasedVertexNormals)
    {
        // Compressed normals & tangents need to be scaled and biased in the vertex shader.
        permutation += 8;
    }

    return permutation;
}


// Sets our state onto the D3D device.
void NormalMapEffect::Impl::Apply(_In_ ID3D11DeviceContext* deviceContext)
{
    // Compute derived parameter values.
    matrices.SetConstants(dirtyFlags, constants.worldViewProj);

    fog.SetConstants(dirtyFlags, matrices.worldView, constants.fogVector);
            
    lights.SetConstants(dirtyFlags, matrices, constants.world, constants.worldInverseTranspose, constants.eyePosition, constants.diffuseColor, constants.emissiveColor, true);

    // Set the textures
    ID3D11ShaderResourceView* textures[] = { texture.Get(), specularTexture.Get(), normalTexture.Get()};
    deviceContext->PSSetShaderResources(0, _countof(textures), textures);
    
    // Set shaders and constant buffers.
    ApplyShaders(deviceContext, GetCurrentShaderPermutation());
}


// Public constructor.
NormalMapEffect::NormalMapEffect(_In_ ID3D11Device* device)
  : pImpl(new Impl(device))
{
}


// Move constructor.
NormalMapEffect::NormalMapEffect(NormalMapEffect&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
NormalMapEffect& NormalMapEffect::operator= (NormalMapEffect&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
NormalMapEffect::~NormalMapEffect()
{
}


// IEffect methods.
void NormalMapEffect::Apply(_In_ ID3D11DeviceContext* deviceContext)
{
    pImpl->Apply(deviceContext);
}


void NormalMapEffect::GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength)
{
    pImpl->GetVertexShaderBytecode(pImpl->GetCurrentShaderPermutation(), pShaderByteCode, pByteCodeLength);
}


// Camera settings.
void XM_CALLCONV NormalMapEffect::SetWorld(FXMMATRIX value)
{
    pImpl->matrices.world = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV NormalMapEffect::SetView(FXMMATRIX value)
{
    pImpl->matrices.view = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV NormalMapEffect::SetProjection(FXMMATRIX value)
{
    pImpl->matrices.projection = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj;
}


void XM_CALLCONV NormalMapEffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
{
    pImpl->matrices.world = world;
    pImpl->matrices.view = view;
    pImpl->matrices.projection = projection;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


// Material settings.
void XM_CALLCONV NormalMapEffect::SetDiffuseColor(FXMVECTOR value)
{
    pImpl->lights.diffuseColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV NormalMapEffect::SetEmissiveColor(FXMVECTOR value)
{
    pImpl->lights.emissiveColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV NormalMapEffect::SetSpecularColor(FXMVECTOR value)
{
    // Set xyz to new value, but preserve existing w (specular power).
    pImpl->constants.specularColorAndPower = XMVectorSelect(pImpl->constants.specularColorAndPower, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NormalMapEffect::SetSpecularPower(float value)
{
    // Set w to new value, but preserve existing xyz (specular color).
    pImpl->constants.specularColorAndPower = XMVectorSetW(pImpl->constants.specularColorAndPower, value);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NormalMapEffect::DisableSpecular()
{
    // Set specular color to black, power to 1
    // Note: Don't use a power of 0 or the shader will generate strange highlights on non-specular materials

    pImpl->constants.specularColorAndPower = g_XMIdentityR3; 

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NormalMapEffect::SetAlpha(float value)
{
    pImpl->lights.alpha = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV NormalMapEffect::SetColorAndAlpha(FXMVECTOR value)
{
    pImpl->lights.diffuseColor = value;
    pImpl->lights.alpha = XMVectorGetW(value);

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


// Light settings.
void NormalMapEffect::SetLightingEnabled(bool value)
{
    if (!value)
    {
        throw std::exception("NormalMapEffect does not support turning off lighting");
    }
}


void NormalMapEffect::SetPerPixelLighting(bool)
{
    // Unsupported interface method.
}


void XM_CALLCONV NormalMapEffect::SetAmbientLightColor(FXMVECTOR value)
{
    pImpl->lights.ambientLightColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void NormalMapEffect::SetLightEnabled(int whichLight, bool value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightEnabled(whichLight, value, pImpl->constants.lightDiffuseColor, pImpl->constants.lightSpecularColor);
}


void XM_CALLCONV NormalMapEffect::SetLightDirection(int whichLight, FXMVECTOR value)
{
    EffectLights::ValidateLightIndex(whichLight);

    pImpl->constants.lightDirection[whichLight] = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void XM_CALLCONV NormalMapEffect::SetLightDiffuseColor(int whichLight, FXMVECTOR value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightDiffuseColor(whichLight, value, pImpl->constants.lightDiffuseColor);
}


void XM_CALLCONV NormalMapEffect::SetLightSpecularColor(int whichLight, FXMVECTOR value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightSpecularColor(whichLight, value, pImpl->constants.lightSpecularColor);
}


void NormalMapEffect::EnableDefaultLighting()
{
    EffectLights::EnableDefaultLighting(this);
}


// Fog settings.
void NormalMapEffect::SetFogEnabled(bool value)
{
    pImpl->fog.enabled = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogEnable;
}


void NormalMapEffect::SetFogStart(float value)
{
    pImpl->fog.start = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void NormalMapEffect::SetFogEnd(float value)
{
    pImpl->fog.end = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void XM_CALLCONV NormalMapEffect::SetFogColor(FXMVECTOR value)
{
    pImpl->constants.fogColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Vertex color setting.
void NormalMapEffect::SetVertexColorEnabled(bool value)
{
    pImpl->vertexColorEnabled = value;
}


// Texture settings.
void NormalMapEffect::SetTexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->texture = value;
}


void NormalMapEffect::SetNormalTexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->normalTexture = value;
}


void NormalMapEffect::SetSpecularTexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->specularTexture = value;
}


// Normal compression settings.
void NormalMapEffect::SetBiasedVertexNormalsAndTangents(bool value)
{
    pImpl->biasedVertexNormals = value;
}