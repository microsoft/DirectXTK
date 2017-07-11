//--------------------------------------------------------------------------------------
// File: SkinnedEffect.cpp
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
struct SkinnedEffectConstants
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

    XMVECTOR bones[SkinnedEffect::MaxBones][3];
};

static_assert( ( sizeof(SkinnedEffectConstants) % 16 ) == 0, "CB size not padded correctly" );


// Traits type describes our characteristics to the EffectBase template.
struct SkinnedEffectTraits
{
    typedef SkinnedEffectConstants ConstantBufferType;

    static const int VertexShaderCount = 18;
    static const int PixelShaderCount = 3;
    static const int ShaderPermutationCount = 36;
};


// Internal SkinnedEffect implementation class.
class SkinnedEffect::Impl : public EffectBase<SkinnedEffectTraits>
{
public:
    Impl(_In_ ID3D11Device* device);

    bool preferPerPixelLighting;
    bool biasedVertexNormals;
    int weightsPerVertex;

    EffectLights lights;

    int GetCurrentShaderPermutation() const;

    void Apply(_In_ ID3D11DeviceContext* deviceContext);
};


// Include the precompiled shader code.
namespace
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedVertexLightingOneBone.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedVertexLightingTwoBones.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedVertexLightingFourBones.inc"

    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedOneLightOneBone.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedOneLightTwoBones.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedOneLightFourBones.inc"

    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedPixelLightingOneBone.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedPixelLightingTwoBones.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedPixelLightingFourBones.inc"

    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedVertexLightingOneBoneBn.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedVertexLightingTwoBonesBn.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedVertexLightingFourBonesBn.inc"

    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedOneLightOneBoneBn.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedOneLightTwoBonesBn.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedOneLightFourBonesBn.inc"

    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedPixelLightingOneBoneBn.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedPixelLightingTwoBonesBn.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_VSSkinnedPixelLightingFourBonesBn.inc"

    #include "Shaders/Compiled/XboxOneSkinnedEffect_PSSkinnedVertexLighting.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_PSSkinnedVertexLightingNoFog.inc"
    #include "Shaders/Compiled/XboxOneSkinnedEffect_PSSkinnedPixelLighting.inc"
#else
    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedVertexLightingOneBone.inc"
    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedVertexLightingTwoBones.inc"
    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedVertexLightingFourBones.inc"

    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedOneLightOneBone.inc"
    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedOneLightTwoBones.inc"
    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedOneLightFourBones.inc"

    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedPixelLightingOneBone.inc"
    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedPixelLightingTwoBones.inc"
    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedPixelLightingFourBones.inc"

    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedVertexLightingOneBoneBn.inc"
    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedVertexLightingTwoBonesBn.inc"
    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedVertexLightingFourBonesBn.inc"

    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedOneLightOneBoneBn.inc"
    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedOneLightTwoBonesBn.inc"
    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedOneLightFourBonesBn.inc"

    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedPixelLightingOneBoneBn.inc"
    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedPixelLightingTwoBonesBn.inc"
    #include "Shaders/Compiled/SkinnedEffect_VSSkinnedPixelLightingFourBonesBn.inc"

    #include "Shaders/Compiled/SkinnedEffect_PSSkinnedVertexLighting.inc"
    #include "Shaders/Compiled/SkinnedEffect_PSSkinnedVertexLightingNoFog.inc"
    #include "Shaders/Compiled/SkinnedEffect_PSSkinnedPixelLighting.inc"
#endif
}


template<>
const ShaderBytecode EffectBase<SkinnedEffectTraits>::VertexShaderBytecode[] =
{
    { SkinnedEffect_VSSkinnedVertexLightingOneBone,     sizeof(SkinnedEffect_VSSkinnedVertexLightingOneBone)     },
    { SkinnedEffect_VSSkinnedVertexLightingTwoBones,    sizeof(SkinnedEffect_VSSkinnedVertexLightingTwoBones)    },
    { SkinnedEffect_VSSkinnedVertexLightingFourBones,   sizeof(SkinnedEffect_VSSkinnedVertexLightingFourBones)   },

    { SkinnedEffect_VSSkinnedOneLightOneBone,           sizeof(SkinnedEffect_VSSkinnedOneLightOneBone)           },
    { SkinnedEffect_VSSkinnedOneLightTwoBones,          sizeof(SkinnedEffect_VSSkinnedOneLightTwoBones)          },
    { SkinnedEffect_VSSkinnedOneLightFourBones,         sizeof(SkinnedEffect_VSSkinnedOneLightFourBones)         },

    { SkinnedEffect_VSSkinnedPixelLightingOneBone,      sizeof(SkinnedEffect_VSSkinnedPixelLightingOneBone)      },
    { SkinnedEffect_VSSkinnedPixelLightingTwoBones,     sizeof(SkinnedEffect_VSSkinnedPixelLightingTwoBones)     },
    { SkinnedEffect_VSSkinnedPixelLightingFourBones,    sizeof(SkinnedEffect_VSSkinnedPixelLightingFourBones)    },

    { SkinnedEffect_VSSkinnedVertexLightingOneBoneBn,   sizeof(SkinnedEffect_VSSkinnedVertexLightingOneBoneBn)   },
    { SkinnedEffect_VSSkinnedVertexLightingTwoBonesBn,  sizeof(SkinnedEffect_VSSkinnedVertexLightingTwoBonesBn)  },
    { SkinnedEffect_VSSkinnedVertexLightingFourBonesBn, sizeof(SkinnedEffect_VSSkinnedVertexLightingFourBonesBn) },

    { SkinnedEffect_VSSkinnedOneLightOneBoneBn,         sizeof(SkinnedEffect_VSSkinnedOneLightOneBoneBn)         },
    { SkinnedEffect_VSSkinnedOneLightTwoBonesBn,        sizeof(SkinnedEffect_VSSkinnedOneLightTwoBonesBn)        },
    { SkinnedEffect_VSSkinnedOneLightFourBonesBn,       sizeof(SkinnedEffect_VSSkinnedOneLightFourBonesBn)       },

    { SkinnedEffect_VSSkinnedPixelLightingOneBoneBn,    sizeof(SkinnedEffect_VSSkinnedPixelLightingOneBoneBn)    },
    { SkinnedEffect_VSSkinnedPixelLightingTwoBonesBn,   sizeof(SkinnedEffect_VSSkinnedPixelLightingTwoBonesBn)   },
    { SkinnedEffect_VSSkinnedPixelLightingFourBonesBn,  sizeof(SkinnedEffect_VSSkinnedPixelLightingFourBonesBn)  },

};


template<>
const int EffectBase<SkinnedEffectTraits>::VertexShaderIndices[] =
{
    0,      // vertex lighting, one bone
    0,      // vertex lighting, one bone, no fog
    1,      // vertex lighting, two bones
    1,      // vertex lighting, two bones, no fog
    2,      // vertex lighting, four bones
    2,      // vertex lighting, four bones, no fog
    
    3,      // one light, one bone
    3,      // one light, one bone, no fog
    4,      // one light, two bones
    4,      // one light, two bones, no fog
    5,      // one light, four bones
    5,      // one light, four bones, no fog
    
    6,      // pixel lighting, one bone
    6,      // pixel lighting, one bone, no fog
    7,      // pixel lighting, two bones
    7,      // pixel lighting, two bones, no fog
    8,      // pixel lighting, four bones
    8,      // pixel lighting, four bones, no fog

    9,      // vertex lighting (biased vertex normals), one bone
    9,      // vertex lighting (biased vertex normals), one bone, no fog
    10,     // vertex lighting (biased vertex normals), two bones
    10,     // vertex lighting (biased vertex normals), two bones, no fog
    11,     // vertex lighting (biased vertex normals), four bones
    11,     // vertex lighting (biased vertex normals), four bones, no fog

    12,     // one light (biased vertex normals), one bone
    12,     // one light (biased vertex normals), one bone, no fog
    13,     // one light (biased vertex normals), two bones
    13,     // one light (biased vertex normals), two bones, no fog
    14,     // one light (biased vertex normals), four bones
    14,     // one light (biased vertex normals), four bones, no fog

    15,     // pixel lighting (biased vertex normals), one bone
    15,     // pixel lighting (biased vertex normals), one bone, no fog
    16,     // pixel lighting (biased vertex normals), two bones
    16,     // pixel lighting (biased vertex normals), two bones, no fog
    17,     // pixel lighting (biased vertex normals), four bones
    17,     // pixel lighting (biased vertex normals), four bones, no fog
};


template<>
const ShaderBytecode EffectBase<SkinnedEffectTraits>::PixelShaderBytecode[] =
{
    { SkinnedEffect_PSSkinnedVertexLighting,      sizeof(SkinnedEffect_PSSkinnedVertexLighting)      },
    { SkinnedEffect_PSSkinnedVertexLightingNoFog, sizeof(SkinnedEffect_PSSkinnedVertexLightingNoFog) },
    { SkinnedEffect_PSSkinnedPixelLighting,       sizeof(SkinnedEffect_PSSkinnedPixelLighting)       },
};


template<>
const int EffectBase<SkinnedEffectTraits>::PixelShaderIndices[] =
{
    0,      // vertex lighting, one bone
    1,      // vertex lighting, one bone, no fog
    0,      // vertex lighting, two bones
    1,      // vertex lighting, two bones, no fog
    0,      // vertex lighting, four bones
    1,      // vertex lighting, four bones, no fog
    
    0,      // one light, one bone
    1,      // one light, one bone, no fog
    0,      // one light, two bones
    1,      // one light, two bones, no fog
    0,      // one light, four bones
    1,      // one light, four bones, no fog
    
    2,      // pixel lighting, one bone
    2,      // pixel lighting, one bone, no fog
    2,      // pixel lighting, two bones
    2,      // pixel lighting, two bones, no fog
    2,      // pixel lighting, four bones
    2,      // pixel lighting, four bones, no fog

    0,      // vertex lighting (biased vertex normals), one bone
    1,      // vertex lighting (biased vertex normals), one bone, no fog
    0,      // vertex lighting (biased vertex normals), two bones
    1,      // vertex lighting (biased vertex normals), two bones, no fog
    0,      // vertex lighting (biased vertex normals), four bones
    1,      // vertex lighting (biased vertex normals), four bones, no fog

    0,      // one light (biased vertex normals), one bone
    1,      // one light (biased vertex normals), one bone, no fog
    0,      // one light (biased vertex normals), two bones
    1,      // one light (biased vertex normals), two bones, no fog
    0,      // one light (biased vertex normals), four bones
    1,      // one light (biased vertex normals), four bones, no fog

    2,      // pixel lighting (biased vertex normals), one bone
    2,      // pixel lighting (biased vertex normals), one bone, no fog
    2,      // pixel lighting (biased vertex normals), two bones
    2,      // pixel lighting (biased vertex normals), two bones, no fog
    2,      // pixel lighting (biased vertex normals), four bones
    2,      // pixel lighting (biased vertex normals), four bones, no fog
};


// Global pool of per-device SkinnedEffect resources.
template<>
SharedResourcePool<ID3D11Device*, EffectBase<SkinnedEffectTraits>::DeviceResources> EffectBase<SkinnedEffectTraits>::deviceResourcesPool;


// Constructor.
SkinnedEffect::Impl::Impl(_In_ ID3D11Device* device)
  : EffectBase(device),
    preferPerPixelLighting(false),
    biasedVertexNormals(false),
    weightsPerVertex(4)
{
    static_assert( _countof(EffectBase<SkinnedEffectTraits>::VertexShaderIndices) == SkinnedEffectTraits::ShaderPermutationCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<SkinnedEffectTraits>::VertexShaderBytecode) == SkinnedEffectTraits::VertexShaderCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<SkinnedEffectTraits>::PixelShaderBytecode) == SkinnedEffectTraits::PixelShaderCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<SkinnedEffectTraits>::PixelShaderIndices) == SkinnedEffectTraits::ShaderPermutationCount, "array/max mismatch" );

    lights.InitializeConstants(constants.specularColorAndPower, constants.lightDirection, constants.lightDiffuseColor, constants.lightSpecularColor);

    for (int i = 0; i < MaxBones; i++)
    {
        constants.bones[i][0] = g_XMIdentityR0;
        constants.bones[i][1] = g_XMIdentityR1;
        constants.bones[i][2] = g_XMIdentityR2;
    }
}


int SkinnedEffect::Impl::GetCurrentShaderPermutation() const
{
    int permutation = 0;

    // Use optimized shaders if fog is disabled.
    if (!fog.enabled)
    {
        permutation += 1;
    }

    // Evaluate 1, 2, or 4 weights per vertex?
    if (weightsPerVertex == 2)
    {
        permutation += 2;
    }
    else if (weightsPerVertex == 4)
    {
        permutation += 4;
    }

    if (preferPerPixelLighting)
    {
        // Do lighting in the pixel shader.
        permutation += 12;
    }
    else if (!lights.lightEnabled[1] && !lights.lightEnabled[2])
    {
        // Use the only-bother-with-the-first-light shader optimization.
        permutation += 6;
    }

    if (biasedVertexNormals)
    {
        // Compressed normals need to be scaled and biased in the vertex shader.
        permutation += 18;
    }

    return permutation;
}


// Sets our state onto the D3D device.
void SkinnedEffect::Impl::Apply(_In_ ID3D11DeviceContext* deviceContext)
{
    // Compute derived parameter values.
    matrices.SetConstants(dirtyFlags, constants.worldViewProj);

    fog.SetConstants(dirtyFlags, matrices.worldView, constants.fogVector);
            
    lights.SetConstants(dirtyFlags, matrices, constants.world, constants.worldInverseTranspose, constants.eyePosition, constants.diffuseColor, constants.emissiveColor, true);

    // Set the texture.
    auto textures = texture.Get();
    if ( !textures )
        textures = GetDefaultTexture();

    deviceContext->PSSetShaderResources(0, 1, &textures );
    
    // Set shaders and constant buffers.
    ApplyShaders(deviceContext, GetCurrentShaderPermutation());
}


// Public constructor.
SkinnedEffect::SkinnedEffect(_In_ ID3D11Device* device)
  : pImpl(new Impl(device))
{
}


// Move constructor.
SkinnedEffect::SkinnedEffect(SkinnedEffect&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
SkinnedEffect& SkinnedEffect::operator= (SkinnedEffect&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
SkinnedEffect::~SkinnedEffect()
{
}


// IEffect methods.
void SkinnedEffect::Apply(_In_ ID3D11DeviceContext* deviceContext)
{
    pImpl->Apply(deviceContext);
}


void SkinnedEffect::GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength)
{
    pImpl->GetVertexShaderBytecode(pImpl->GetCurrentShaderPermutation(), pShaderByteCode, pByteCodeLength);
}


// Camera settings.
void XM_CALLCONV SkinnedEffect::SetWorld(FXMMATRIX value)
{
    pImpl->matrices.world = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV SkinnedEffect::SetView(FXMMATRIX value)
{
    pImpl->matrices.view = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV SkinnedEffect::SetProjection(FXMMATRIX value)
{
    pImpl->matrices.projection = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj;
}


void XM_CALLCONV SkinnedEffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
{
    pImpl->matrices.world = world;
    pImpl->matrices.view = view;
    pImpl->matrices.projection = projection;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


// Material settings.
void XM_CALLCONV SkinnedEffect::SetDiffuseColor(FXMVECTOR value)
{
    pImpl->lights.diffuseColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV SkinnedEffect::SetEmissiveColor(FXMVECTOR value)
{
    pImpl->lights.emissiveColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV SkinnedEffect::SetSpecularColor(FXMVECTOR value)
{
    // Set xyz to new value, but preserve existing w (specular power).
    pImpl->constants.specularColorAndPower = XMVectorSelect(pImpl->constants.specularColorAndPower, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void SkinnedEffect::SetSpecularPower(float value)
{
    // Set w to new value, but preserve existing xyz (specular color).
    pImpl->constants.specularColorAndPower = XMVectorSetW(pImpl->constants.specularColorAndPower, value);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void SkinnedEffect::DisableSpecular()
{
    // Set specular color to black, power to 1
    // Note: Don't use a power of 0 or the shader will generate strange highlights on non-specular materials

    pImpl->constants.specularColorAndPower = g_XMIdentityR3; 

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void SkinnedEffect::SetAlpha(float value)
{
    pImpl->lights.alpha = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV SkinnedEffect::SetColorAndAlpha(FXMVECTOR value)
{
    pImpl->lights.diffuseColor = value;
    pImpl->lights.alpha = XMVectorGetW(value);

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


// Light settings.
void SkinnedEffect::SetLightingEnabled(bool value)
{
    if (!value)
    {
        throw std::exception("SkinnedEffect does not support turning off lighting");
    }
}


void SkinnedEffect::SetPerPixelLighting(bool value)
{
    pImpl->preferPerPixelLighting = value;
}


void XM_CALLCONV SkinnedEffect::SetAmbientLightColor(FXMVECTOR value)
{
    pImpl->lights.ambientLightColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void SkinnedEffect::SetLightEnabled(int whichLight, bool value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightEnabled(whichLight, value, pImpl->constants.lightDiffuseColor, pImpl->constants.lightSpecularColor);
}


void XM_CALLCONV SkinnedEffect::SetLightDirection(int whichLight, FXMVECTOR value)
{
    EffectLights::ValidateLightIndex(whichLight);

    pImpl->constants.lightDirection[whichLight] = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void XM_CALLCONV SkinnedEffect::SetLightDiffuseColor(int whichLight, FXMVECTOR value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightDiffuseColor(whichLight, value, pImpl->constants.lightDiffuseColor);
}


void XM_CALLCONV SkinnedEffect::SetLightSpecularColor(int whichLight, FXMVECTOR value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightSpecularColor(whichLight, value, pImpl->constants.lightSpecularColor);
}


void SkinnedEffect::EnableDefaultLighting()
{
    EffectLights::EnableDefaultLighting(this);
}


// Fog settings.
void SkinnedEffect::SetFogEnabled(bool value)
{
    pImpl->fog.enabled = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogEnable;
}


void SkinnedEffect::SetFogStart(float value)
{
    pImpl->fog.start = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void SkinnedEffect::SetFogEnd(float value)
{
    pImpl->fog.end = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void XM_CALLCONV SkinnedEffect::SetFogColor(FXMVECTOR value)
{
    pImpl->constants.fogColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Texture settings.
void SkinnedEffect::SetTexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->texture = value;
}


// Animation settings.
void SkinnedEffect::SetWeightsPerVertex(int value)
{
    if ((value != 1) &&
        (value != 2) &&
        (value != 4))
    {
        throw std::out_of_range("WeightsPerVertex must be 1, 2, or 4");
    }

    pImpl->weightsPerVertex = value;
}


void SkinnedEffect::SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count)
{
    if (count > MaxBones)
        throw std::out_of_range("count parameter out of range");

    auto boneConstant = pImpl->constants.bones;

    for (size_t i = 0; i < count; i++)
    {
        XMMATRIX boneMatrix = XMMatrixTranspose(value[i]);

        boneConstant[i][0] = boneMatrix.r[0];
        boneConstant[i][1] = boneMatrix.r[1];
        boneConstant[i][2] = boneMatrix.r[2];
    }

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void SkinnedEffect::ResetBoneTransforms()
{
    auto boneConstant = pImpl->constants.bones;

    for(size_t i = 0; i < MaxBones; ++i)
    {
        boneConstant[i][0] = g_XMIdentityR0;
        boneConstant[i][1] = g_XMIdentityR1;
        boneConstant[i][2] = g_XMIdentityR2;
    }

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Normal compression settings.
void SkinnedEffect::SetBiasedVertexNormals(bool value)
{
    pImpl->biasedVertexNormals = value;
}
