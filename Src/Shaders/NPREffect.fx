//--------------------------------------------------------------------------------------
// File: NPREffect.fx
//
// Non-photorealistic rendering effects (cel shading and Gooch shading)
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------


Texture2D<float4> Texture : register(t0);
SamplerState Sampler : register(s0);


cbuffer Parameters : register(b0)
{
    float3 LightDirection            : packoffset(c0);
    float  CelBands                  : packoffset(c0.w);

    float3 DiffuseColor              : packoffset(c1);
    float  Alpha                     : packoffset(c1.w);

    float3 SpecularColor             : packoffset(c2);
    float  SpecularThreshold         : packoffset(c2.w);

    float3 RimColor                  : packoffset(c3);
    float  RimPower                  : packoffset(c3.w);

    float  SpecularSmoothing         : packoffset(c4.x);
    float  RimStrength               : packoffset(c4.y);
    float  RimStart                  : packoffset(c4.z);
    float  RimEnd                    : packoffset(c4.w);

    float3 GoochCoolColor            : packoffset(c5);
    float  GoochAlpha                : packoffset(c5.w);

    float3 GoochWarmColor            : packoffset(c6);
    float  GoochBeta                 : packoffset(c6.w);

    float3 EyePosition               : packoffset(c7);

    float4x4 World                   : packoffset(c8);
    float3x3 WorldInverseTranspose   : packoffset(c12);
    float4x4 WorldViewProj           : packoffset(c15);
};


#include "Structures.fxh"
#include "Utilities.fxh"

VSOutputPixelLighting ComputeCommonVS(float4 position, float3 normal)
{
    VSOutputPixelLighting vout;
    vout.PositionPS = mul(position, WorldViewProj);
    vout.PositionWS = float4(mul(position, World).xyz, 1);
    vout.NormalWS = normalize(mul(normal, WorldInverseTranspose));
    return vout;
}

VSOutputPixelLightingTx ComputeCommonVSTx(float4 position, float3 normal, float2 texcoord)
{
    VSOutputPixelLightingTx vout;
    vout.TexCoord = texcoord;
    vout.PositionPS = mul(position, WorldViewProj);
    vout.PositionWS = float4(mul(position, World).xyz, 1);
    vout.NormalWS = normalize(mul(normal, WorldInverseTranspose));
    return vout;
}

// Vertex shader: basic.
VSOutputPixelLighting VSNPREffect(VSInputNm vin)
{
    VSOutputPixelLighting vout = ComputeCommonVS(vin.Position, vin.Normal);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

VSOutputPixelLighting VSNPREffectBn(VSInputNm vin)
{
    float3 normal = BiasX2(vin.Normal);

    VSOutputPixelLighting vout = ComputeCommonVS(vin.Position, normal);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}


// Vertex shader: vertex color.
VSOutputPixelLighting VSNPREffectVc(VSInputNmVc vin)
{
    VSOutputPixelLighting vout = ComputeCommonVS(vin.Position, vin.Normal);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}

VSOutputPixelLighting VSNPREffectVcBn(VSInputNmVc vin)
{
    float3 normal = BiasX2(vin.Normal);

    VSOutputPixelLighting vout = ComputeCommonVS(vin.Position, normal);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}


// Vertex shader: instancing.
VSOutputPixelLighting VSNPREffectInst(VSInputNmInst vin)
{
    CommonInstancing inst = ComputeCommonInstancing(vin.Position, vin.Normal, vin.Transform);
    VSOutputPixelLighting vout = ComputeCommonVS(inst.Position, inst.Normal);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

VSOutputPixelLighting VSNPREffectBnInst(VSInputNmInst vin)
{
    float3 normal = BiasX2(vin.Normal);

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, normal, vin.Transform);
    VSOutputPixelLighting vout = ComputeCommonVS(inst.Position, inst.Normal);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}


// Vertex shader: vertex color + instancing.
VSOutputPixelLighting VSNPREffectVcInst(VSInputNmVcInst vin)
{
    CommonInstancing inst = ComputeCommonInstancing(vin.Position, vin.Normal, vin.Transform);
    VSOutputPixelLighting vout = ComputeCommonVS(inst.Position, inst.Normal);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}

VSOutputPixelLighting VSNPREffectVcBnInst(VSInputNmVcInst vin)
{
    float3 normal = BiasX2(vin.Normal);

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, normal, vin.Transform);
    VSOutputPixelLighting vout = ComputeCommonVS(inst.Position, inst.Normal);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}


// Vertex shader: texture.
VSOutputPixelLightingTx VSNPREffectTx(VSInputNmTx vin)
{
    VSOutputPixelLightingTx vout = ComputeCommonVSTx(vin.Position, vin.Normal, vin.TexCoord);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

VSOutputPixelLightingTx VSNPREffectBnTx(VSInputNmTx vin)
{
    float3 normal = BiasX2(vin.Normal);

    VSOutputPixelLightingTx vout = ComputeCommonVSTx(vin.Position, normal, vin.TexCoord);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}


// Vertex shader: texture + vertex color.
VSOutputPixelLightingTx VSNPREffectVcTx(VSInputNmTxVc vin)
{
    VSOutputPixelLightingTx vout = ComputeCommonVSTx(vin.Position, vin.Normal, vin.TexCoord);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}

VSOutputPixelLightingTx VSNPREffectVcBnTx(VSInputNmTxVc vin)
{
    float3 normal = BiasX2(vin.Normal);

    VSOutputPixelLightingTx vout = ComputeCommonVSTx(vin.Position, normal, vin.TexCoord);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}


// Vertex shader: texture + instancing.
VSOutputPixelLightingTx VSNPREffectInstTx(VSInputNmTxInst vin)
{
    CommonInstancing inst = ComputeCommonInstancing(vin.Position, vin.Normal, vin.Transform);
    VSOutputPixelLightingTx vout = ComputeCommonVSTx(inst.Position, inst.Normal, vin.TexCoord);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

VSOutputPixelLightingTx VSNPREffectBnInstTx(VSInputNmTxInst vin)
{
    float3 normal = BiasX2(vin.Normal);

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, normal, vin.Transform);
    VSOutputPixelLightingTx vout = ComputeCommonVSTx(inst.Position, inst.Normal, vin.TexCoord);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}


// Vertex shader: texture + vertex color + instancing.
VSOutputPixelLightingTx VSNPREffectVcInstTx(VSInputNmTxVcInst vin)
{
    CommonInstancing inst = ComputeCommonInstancing(vin.Position, vin.Normal, vin.Transform);
    VSOutputPixelLightingTx vout = ComputeCommonVSTx(inst.Position, inst.Normal, vin.TexCoord);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}

VSOutputPixelLightingTx VSNPREffectVcBnInstTx(VSInputNmTxVcInst vin)
{
    float3 normal = BiasX2(vin.Normal);

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, normal, vin.Transform);
    VSOutputPixelLightingTx vout = ComputeCommonVSTx(inst.Position, inst.Normal, vin.TexCoord);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}


//--- Cel shading ---
float Quantize(float intensity, float bands)
{
    intensity = max(0, intensity);
    return floor(intensity * bands) / bands;
}

float HardEdgeSpecular(float dot1)
{
    return smoothstep(SpecularThreshold - SpecularSmoothing, SpecularThreshold + SpecularSmoothing, dot1);
}

float RimLighting(float dot1)
{
    float fresnel = pow(1.0 - saturate(dot1), RimPower);
    return smoothstep(RimStart, RimEnd, fresnel) * RimStrength;
}


// Pixel shader: cel shading.
float4 PSCelShading(PSInputPixelLighting pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);

    float3 lightDir = normalize(-LightDirection);
    float NdotL = dot(normal, lightDir);

    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float NdotV = max(0, dot(normal, viewDir));

    float3 halfVec = normalize(lightDir + viewDir);
    float NdotH = max(0, dot(normal, halfVec));

    // Quantize the diffuse lighting into discrete bands
    float quantized = Quantize(NdotL, CelBands);
    float3 color = pin.Diffuse.rgb * quantized;

    // Rim lighting
    float outline = RimLighting(NdotV);
    color = lerp(color, RimColor, outline);

    // Specular highlight (hard edge)
    float specular = HardEdgeSpecular(NdotH);
    color += SpecularColor * specular;

    return float4(color, pin.Diffuse.a);
    }


// Pixel shader: cel shading + texture.
float4 PSCelShadingTx(PSInputPixelLightingTx pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);

    float3 lightDir = normalize(-LightDirection);
    float NdotL = dot(normal, lightDir);

    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float NdotV = max(0, dot(normal, viewDir));
    float VdotL = max(0, dot(viewDir, lightDir));

    float3 halfVec = normalize(lightDir + viewDir);
    float NdotH = max(0, dot(normal, halfVec));

    // Sample base texture
    float4 texColor = Texture.Sample(Sampler, pin.TexCoord);

    // Quantize the diffuse lighting into discrete bands
    float quantized = Quantize(NdotL, CelBands);
    float3 color = pin.Diffuse.rgb * texColor.rgb * quantized;

    // Rim lighting
    float outline = RimLighting(NdotV);
    color = lerp(color, RimColor, outline);

    // Specular highlight (hard edge)
    float specular = HardEdgeSpecular(NdotH);
    color += SpecularColor * specular;

    return float4(color, pin.Diffuse.a);
}


//--- Gooch shading ---
float3 GoochShading(float dot1, float3 color)
{
    float t = (1.0 + dot1) * 0.5;

    float3 coolContrib = GoochCoolColor + GoochAlpha * color;
    float3 warmContrib = GoochWarmColor + GoochBeta * color;

    return lerp(coolContrib, warmContrib, t);
}


// Pixel shader: Gooch shading.
float4 PSGoochShading(PSInputPixelLighting pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);
    float3 lightDir = normalize(-LightDirection);
    float NdotL = dot(normal, lightDir);

    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float NdotV = max(0, dot(normal, viewDir));

    float3 reflectDir = reflect(LightDirection, normal);
    float RdotV = max(0, dot(viewDir, reflectDir));

    // Gooch diffuse term: blend between cool and warm based
    float3 color = GoochShading(NdotL, pin.Diffuse.rgb);

    // Rim lighting
    float outline = RimLighting(NdotV);
    color += lerp(color, RimColor, outline);

    // Specular highlight (hard edge)
    float specular = HardEdgeSpecular(RdotV);
    color += SpecularColor * specular;

    return float4(color, pin.Diffuse.a);
}


// Pixel shader: Gooch shading + texture.
float4 PSGoochShadingTx(PSInputPixelLightingTx pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);
    float3 lightDir = normalize(-LightDirection);
    float NdotL = dot(normal, lightDir);

    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float NdotV = max(0, dot(normal, viewDir));

    float3 reflectDir = reflect(LightDirection, normal);
    float RdotV = max(0, dot(viewDir, reflectDir));

    // Sample base texture
    float4 texColor = Texture.Sample(Sampler, pin.TexCoord);

    // Gooch diffuse term: blend between cool and warm based
    float3 color = GoochShading(NdotL, pin.Diffuse.rgb * texColor.rgb);

    // Rim lighting
    float outline = RimLighting(NdotV);
    color += lerp(color, RimColor, outline);

    // Specular highlight
    float specular = HardEdgeSpecular(RdotV);
    color += SpecularColor * specular;

    return float4(color, pin.Diffuse.a * texColor.a);
}
