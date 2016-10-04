// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://create.msdn.com/en-US/education/catalog/sample/stock_effects


Texture2D<float4>   Texture        : register(t0);
TextureCube<float4> EnvironmentMap : register(t1);

sampler Sampler       : register(s0);
sampler EnvMapSampler : register(s1);


cbuffer Parameters : register(b0)
{
    float3 EnvironmentMapSpecular   : packoffset(c0);
    float  EnvironmentMapAmount     : packoffset(c1.x);
    float  FresnelFactor            : packoffset(c1.y);

    float4 DiffuseColor             : packoffset(c2);
    float3 EmissiveColor            : packoffset(c3);

    float3 LightDirection[3]        : packoffset(c4);
    float3 LightDiffuseColor[3]     : packoffset(c7);

    float3 EyePosition              : packoffset(c10);

    float3 FogColor                 : packoffset(c11);
    float4 FogVector                : packoffset(c12);

    float4x4 World                  : packoffset(c13);
    float3x3 WorldInverseTranspose  : packoffset(c17);
    float4x4 WorldViewProj          : packoffset(c20);
};


// We don't use these parameters, but Lighting.fxh won't compile without them.
#define SpecularPower       0
#define SpecularColor       0
#define LightSpecularColor  float3(0, 0, 0)


#include "Structures.fxh"
#include "Common.fxh"
#include "Lighting.fxh"


float ComputeFresnelFactor(float3 eyeVector, float3 worldNormal)
{
    float viewAngle = dot(eyeVector, worldNormal);

    return pow(max(1 - abs(viewAngle), 0), FresnelFactor) * EnvironmentMapAmount;
}


VSOutputTxEnvMap ComputeEnvMapVSOutput(VSInputNmTx vin, float3 normal, uniform bool useFresnel, uniform int numLights)
{
    VSOutputTxEnvMap vout;

    float4 pos_ws = mul(vin.Position, World);
    float3 eyeVector = normalize(EyePosition - pos_ws.xyz);
    float3 worldNormal = normalize(mul(normal, WorldInverseTranspose));

    ColorPair lightResult = ComputeLights(eyeVector, worldNormal, numLights);

    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.Diffuse = float4(lightResult.Diffuse, DiffuseColor.a);

    if (useFresnel)
        vout.Specular.rgb = ComputeFresnelFactor(eyeVector, worldNormal);
    else
        vout.Specular.rgb = EnvironmentMapAmount;

    vout.Specular.a = ComputeFogFactor(vin.Position);
    vout.TexCoord = vin.TexCoord;
    vout.EnvCoord = reflect(-eyeVector, worldNormal);

    return vout;
}


float4 ComputeEnvMapPSOutput(PSInputPixelLightingTx pin, uniform bool useFresnel)
{
    float4 color = Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;

    float3 eyeVector = normalize(EyePosition - pin.PositionWS.xyz);
    float3 worldNormal = normalize(pin.NormalWS);

    ColorPair lightResult = ComputeLights(eyeVector, worldNormal, 3);

    color.rgb *= lightResult.Diffuse;

    float3 envcoord = reflect(-eyeVector, worldNormal);

    float4 envmap = EnvironmentMap.Sample(EnvMapSampler, envcoord) * color.a;

    float3 amount;
    if (useFresnel)
        amount = ComputeFresnelFactor(eyeVector, worldNormal);
    else
        amount = EnvironmentMapAmount;

    color.rgb = lerp(color.rgb, envmap.rgb, amount.rgb);
    color.rgb += EnvironmentMapSpecular * envmap.a;

    return color;
}


// Vertex shader: basic.
VSOutputTxEnvMap VSEnvMap(VSInputNmTx vin)
{
    return ComputeEnvMapVSOutput(vin, vin.Normal, false, 3);
}

VSOutputTxEnvMap VSEnvMapBn(VSInputNmTx vin)
{
    float3 normal = BiasX2(vin.Normal);

    return ComputeEnvMapVSOutput(vin, normal, false, 3);
}


// Vertex shader: fresnel.
VSOutputTxEnvMap VSEnvMapFresnel(VSInputNmTx vin)
{
    return ComputeEnvMapVSOutput(vin, vin.Normal, true, 3);
}

VSOutputTxEnvMap VSEnvMapFresnelBn(VSInputNmTx vin)
{
    float3 normal = BiasX2(vin.Normal);

    return ComputeEnvMapVSOutput(vin, normal, true, 3);
}


// Vertex shader: one light.
VSOutputTxEnvMap VSEnvMapOneLight(VSInputNmTx vin)
{
    return ComputeEnvMapVSOutput(vin, vin.Normal, false, 1);
}

VSOutputTxEnvMap VSEnvMapOneLightBn(VSInputNmTx vin)
{
    float3 normal = BiasX2(vin.Normal);

    return ComputeEnvMapVSOutput(vin, normal, false, 1);
}


// Vertex shader: one light, fresnel.
VSOutputTxEnvMap VSEnvMapOneLightFresnel(VSInputNmTx vin)
{
    return ComputeEnvMapVSOutput(vin, vin.Normal, true, 1);
}

VSOutputTxEnvMap VSEnvMapOneLightFresnelBn(VSInputNmTx vin)
{
    float3 normal = BiasX2(vin.Normal);

    return ComputeEnvMapVSOutput(vin, normal, true, 1);
}


// Vertex shader: pixel lighting.
VSOutputPixelLightingTx VSEnvMapPixelLighting(VSInputNmTx vin)
{
    VSOutputPixelLightingTx vout;

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, vin.Normal);
    SetCommonVSOutputParamsPixelLighting;

    vout.Diffuse = float4(1, 1, 1, DiffuseColor.a);
    vout.TexCoord = vin.TexCoord;

    return vout;
}

VSOutputPixelLightingTx VSEnvMapPixelLightingBn(VSInputNmTx vin)
{
    VSOutputPixelLightingTx vout;

    float3 normal = BiasX2(vin.Normal);

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, normal);
    SetCommonVSOutputParamsPixelLighting;

    vout.Diffuse = float4(1, 1, 1, DiffuseColor.a);
    vout.TexCoord = vin.TexCoord;

    return vout;
}


// Pixel shader: basic.
float4 PSEnvMap(PSInputTxEnvMap pin) : SV_Target0
{
    float4 color = Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;
    float4 envmap = EnvironmentMap.Sample(EnvMapSampler, pin.EnvCoord) * color.a;

    color.rgb = lerp(color.rgb, envmap.rgb, pin.Specular.rgb);

    ApplyFog(color, pin.Specular.w);

    return color;
}


// Pixel shader: no fog.
float4 PSEnvMapNoFog(PSInputTxEnvMap pin) : SV_Target0
{
    float4 color = Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;
    float4 envmap = EnvironmentMap.Sample(EnvMapSampler, pin.EnvCoord) * color.a;

    color.rgb = lerp(color.rgb, envmap.rgb, pin.Specular.rgb);

    return color;
}


// Pixel shader: specular.
float4 PSEnvMapSpecular(PSInputTxEnvMap pin) : SV_Target0
{
    float4 color = Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;
    float4 envmap = EnvironmentMap.Sample(EnvMapSampler, pin.EnvCoord) * color.a;

    color.rgb = lerp(color.rgb, envmap.rgb, pin.Specular.rgb);
    color.rgb += EnvironmentMapSpecular * envmap.a;

    ApplyFog(color, pin.Specular.w);

    return color;
}


// Pixel shader: specular, no fog.
float4 PSEnvMapSpecularNoFog(PSInputTxEnvMap pin) : SV_Target0
{
    float4 color = Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;
    float4 envmap = EnvironmentMap.Sample(EnvMapSampler, pin.EnvCoord) * color.a;

    color.rgb = lerp(color.rgb, envmap.rgb, pin.Specular.rgb);
    color.rgb += EnvironmentMapSpecular * envmap.a;

    return color;
}


// Pixel shader: pixel lighting.
float4 PSEnvMapPixelLighting(PSInputPixelLightingTx pin) : SV_Target0
{
    float4 color = ComputeEnvMapPSOutput(pin, false);

    ApplyFog(color, pin.PositionWS.w);

    return color;
}


// Pixel shader: pixel lighting + no fog.
float4 PSEnvMapPixelLightingNoFog(PSInputPixelLightingTx pin) : SV_Target0
{
    float4 color = ComputeEnvMapPSOutput(pin, false);

    return color;
}


// Pixel shader: pixel lighting + fresnel
float4 PSEnvMapPixelLightingFresnel(PSInputPixelLightingTx pin) : SV_Target0
{
    float4 color = ComputeEnvMapPSOutput(pin, true);

    ApplyFog(color, pin.PositionWS.w);

    return color;
}


// Pixel shader: pixel lighting + fresnel + no fog.
float4 PSEnvMapPixelLightingFresnelNoFog(PSInputPixelLightingTx pin) : SV_Target0
{
    float4 color = ComputeEnvMapPSOutput(pin, true);

    return color;
}
