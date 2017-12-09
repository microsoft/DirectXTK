// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929


cbuffer Parameters : register(b0)
{
    float3 AmbientDown              : packoffset(c0);
    float  Alpha                    : packoffset(c0.w);
    float3 AmbientRange             : packoffset(c1);

    float4x4 World                  : packoffset(c2);
    float3x3 WorldInverseTranspose  : packoffset(c6);
    float4x4 WorldViewProj          : packoffset(c10);
};


#include "Structures.fxh"
#include "Utilities.fxh"


// Vertex shader: basic
VSOutputPixelLightingTxTangent VSDebug(VSInputNmTxTangent vin)
{
    VSOutputPixelLightingTxTangent vout;

    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.PositionWS = float4(mul(vin.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(vin.Normal, WorldInverseTranspose));
    vout.Diffuse = float4(1, 1, 1, Alpha);
    vout.TexCoord = vin.TexCoord;
    vout.TangentWS = normalize(mul(vin.Tangent.xyz, WorldInverseTranspose));

    return vout;
}

VSOutputPixelLightingTxTangent VSDebugBn(VSInputNmTxTangent vin)
{
    VSOutputPixelLightingTxTangent vout;

    float3 normal = BiasX2(vin.Normal);

    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.PositionWS = float4(mul(vin.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(normal, WorldInverseTranspose));
    vout.Diffuse = float4(1, 1, 1, Alpha);
    vout.TexCoord = vin.TexCoord;

    // For normal mapping, we need tangent to form tangent space transform
    float3 tangent = BiasX2(vin.Tangent.xyz);

    vout.TangentWS = normalize(mul(tangent, WorldInverseTranspose));

    return vout;
}


// Vertex shader: vertex color.
VSOutputPixelLightingTxTangent VSDebugVc(VSInputNmTxVcTangent vin)
{
    VSOutputPixelLightingTxTangent vout;

    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.PositionWS = float4(mul(vin.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(vin.Normal, WorldInverseTranspose));
    vout.Diffuse.rgb = vin.Color.rgb;
    vout.Diffuse.a = vin.Color.a * Alpha;
    vout.TexCoord = vin.TexCoord;
    vout.TangentWS = normalize(mul(vin.Tangent.xyz, WorldInverseTranspose));

    return vout;
}

VSOutputPixelLightingTxTangent VSDebugVcBn(VSInputNmTxVcTangent vin)
{
    VSOutputPixelLightingTxTangent vout;

    float3 normal = BiasX2(vin.Normal);

    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.PositionWS = float4(mul(vin.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(normal, WorldInverseTranspose));
    vout.Diffuse.rgb = vin.Color.rgb;
    vout.Diffuse.a = vin.Color.a * Alpha;
    vout.TexCoord = vin.TexCoord;

    // For normal mapping, we need tangent to form tangent space transform
    float3 tangent = BiasX2(vin.Tangent.xyz);

    vout.TangentWS = normalize(mul(tangent, WorldInverseTranspose));

    return vout;
}


// Pixel shader: default
float3 CalcHemiAmbient(float3 normal, float3 color)
{
    float3 up = BiasD2(normal);
    float3 ambient = AmbientDown + up.y * AmbientRange;
    return ambient * color;
}

float4 PSHemiAmbient(PSInputPixelLightingTxTangent pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);

    // Do lighting
    float3 color = CalcHemiAmbient(normal, pin.Diffuse.rgb);

    return float4(color, pin.Diffuse.a);
}


// Pixel shader: RGB normals
float4 PSRGBNormals(PSInputPixelLightingTxTangent pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);

    float3 color = BiasD2(normal);

    return float4(color, pin.Diffuse.a);
}

// Pixel shader: RGB tangents
float4 PSRGBTangents(PSInputPixelLightingTxTangent pin) : SV_Target0
{
    float3 tangent = normalize(pin.TangentWS);

    float3 color = BiasD2(tangent);

    return float4(color, pin.Diffuse.a);
}

// Pixel shader: RGB bi-tangents
float4 PSRGBBiTangents(PSInputPixelLightingTxTangent pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);
    float3 tangent = normalize(pin.TangentWS);
    float3 bitangent = cross(normal, tangent);

    float3 color = BiasD2(bitangent);

    return float4(color, pin.Diffuse.a);
}
