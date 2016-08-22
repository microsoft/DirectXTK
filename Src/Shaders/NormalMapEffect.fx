// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://create.msdn.com/en-US/education/catalog/sample/stock_effects


Texture2D<float4> Texture : register(t0);
Texture2D<float3> SpecularTexture : register(t1);
Texture2D<float3> NormalTexture : register(t2);

sampler Sampler : register(s0);

cbuffer Parameters : register(b0)
{
    float4 DiffuseColor             : packoffset(c0);
    float3 EmissiveColor            : packoffset(c1);
    float3 SpecularColor            : packoffset(c2);
    float  SpecularPower            : packoffset(c2.w);

    float3 LightDirection[3]        : packoffset(c3);
    float3 LightDiffuseColor[3]     : packoffset(c6);
    float3 LightSpecularColor[3]    : packoffset(c9);

    float3 EyePosition              : packoffset(c12);

    float3 FogColor                 : packoffset(c13);
    float4 FogVector                : packoffset(c14);

    float4x4 World                  : packoffset(c15);
    float3x3 WorldInverseTranspose  : packoffset(c19);
    float4x4 WorldViewProj          : packoffset(c22);
};


#include "Structures.fxh"
#include "Common.fxh"
#include "Lighting.fxh"

// Vertex shader: pixel lighting + texture.
VSOutputPixelLightingTxTangent VSNormalPixelLightingTx(VSInputNmTxTangent vin)
{
    VSOutputPixelLightingTxTangent vout;

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, vin.Normal);
    SetCommonVSOutputParamsPixelLighting;

    vout.Diffuse = float4(1, 1, 1, DiffuseColor.a);
    vout.TexCoord = vin.TexCoord;

    // For normal mapping, we need tangent to form tangent space transform
    vout.TangentWS = normalize(mul(vin.Tangent.xyz, WorldInverseTranspose));

    return vout;
}


// Vertex shader: pixel lighting + texture + vertex color.
VSOutputPixelLightingTxTangent VSNormalPixelLightingTxVc(VSInputNmTxVcTangent vin)
{
    VSOutputPixelLightingTxTangent vout;

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, vin.Normal);
    SetCommonVSOutputParamsPixelLighting;

    vout.Diffuse.rgb = vin.Color.rgb;
    vout.Diffuse.a = vin.Color.a * DiffuseColor.a;
    vout.TexCoord = vin.TexCoord;

    // For normal mapping, we need tangent to form tangent space transform
    vout.TangentWS = normalize(mul(vin.Tangent.xyz, WorldInverseTranspose));

    return vout;
}

// Pixel shader: pixel lighting + texture + no fog
float4 PSNormalPixelLightingTxNoFog(PSInputPixelLightingTxTangent pin) : SV_Target0
{
    float3 eyeVector = normalize(EyePosition - pin.PositionWS.xyz);

    // Before lighting, peturb the surface's normal by the one given in normal map.
    float3 localNormal = (NormalTexture.Sample(Sampler, pin.TexCoord).xyz * 2) - 1;
    float3 normal = PeturbNormal(localNormal, pin.NormalWS, pin.TangentWS);

    // Do lighting
    ColorPair lightResult = ComputeLights(eyeVector, normal, 3);

    // Get color from albedo texture
    float4 color = Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;
    color.rgb *= lightResult.Diffuse;

    // Apply specular, modulated by the intensity given in the specular map
    float3 specIntensity = SpecularTexture.Sample(Sampler, pin.TexCoord);
    AddSpecular(color, lightResult.Specular * specIntensity);

    return color;
}

// Pixel shader: pixel lighting + texture
float4 PSNormalPixelLightingTx(PSInputPixelLightingTxTangent pin) : SV_Target0
{
    float3 eyeVector = normalize(EyePosition - pin.PositionWS.xyz);
 
    // Before lighting, peturb the surface's normal by the one given in normal map.
    float3 localNormal = (NormalTexture.Sample(Sampler, pin.TexCoord).xyz * 2) - 1;
    float3 normal = PeturbNormal(localNormal, pin.NormalWS, pin.TangentWS);

    // Do lighting
    ColorPair lightResult = ComputeLights(eyeVector, normal, 3);

    // Get color from albedo texture
    float4 color = Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;
    color.rgb *= lightResult.Diffuse;

    // Apply specular, modulated by the intensity given in the specular map
    float3 specIntensity = SpecularTexture.Sample(Sampler, pin.TexCoord);
    AddSpecular(color, lightResult.Specular * specIntensity);

    ApplyFog(color, pin.PositionWS.w);
    return color;
}


// Pixel shader: pixel lighting + texture + no fog + no specular
float4 PSNormalPixelLightingTxNoFogSpec(PSInputPixelLightingTxTangent pin) : SV_Target0
{
    float3 eyeVector = normalize(EyePosition - pin.PositionWS.xyz);

    // Before lighting, peturb the surface's normal by the one given in normal map.
    float3 localNormal = (NormalTexture.Sample(Sampler, pin.TexCoord).xyz * 2) - 1;
    float3 normal = PeturbNormal(localNormal, pin.NormalWS, pin.TangentWS);

    // Do lighting
    ColorPair lightResult = ComputeLights(eyeVector, normal, 3);

    // Get color from albedo texture
    float4 color = Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;
    color.rgb *= lightResult.Diffuse;

    // Apply specular
    AddSpecular(color, lightResult.Specular);

    return color;
}

// Pixel shader: pixel lighting + texture + no specular
float4 PSNormalPixelLightingTxNoSpec(PSInputPixelLightingTxTangent pin) : SV_Target0
{
    float3 eyeVector = normalize(EyePosition - pin.PositionWS.xyz);

    // Before lighting, peturb the surface's normal by the one given in normal map.
    float3 localNormal = (NormalTexture.Sample(Sampler, pin.TexCoord).xyz * 2) - 1;
    float3 normal = PeturbNormal(localNormal, pin.NormalWS, pin.TangentWS);

    // Do lighting
    ColorPair lightResult = ComputeLights(eyeVector, normal, 3);

    // Get color from albedo texture
    float4 color = Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;
    color.rgb *= lightResult.Diffuse;

    // Apply specular
    AddSpecular(color, lightResult.Specular);

    ApplyFog(color, pin.PositionWS.w);
    return color;
}
