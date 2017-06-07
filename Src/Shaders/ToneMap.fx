// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929

Texture2D<float4> HDRTexture : register(t0);
sampler Sampler : register(s0);


cbuffer Parameters : register(b0)
{
    float4 paperWhiteNits;
};



#include "Structures.fxh"


// Vertex shader: self-created quad.
VSInputTx VSQuad(uint vI : SV_VertexId)
{
    VSInputTx vout;

    float2 texcoord = float2(vI & 1, vI >> 1);
    vout.TexCoord = texcoord;

    vout.Position = float4((texcoord.x - 0.5f) * 2, -(texcoord.y - 0.5f) * 2, 0, 1);
    return vout;
}


//--------------------------------------------------------------------------------------
// Pixel shader: saturate (clips above 1.0)
float4 PSSaturate(VSInputTx pin) : SV_Target0
{
    float4 hdr = HDRTexture.Sample(Sampler, pin.TexCoord);
    float3 sdr = saturate(hdr.xyz);
    return float4(sdr, hdr.a);
}


// Pixel shader: reinhard operator
float3 Reinhard(float3 color)
{
    return color / (1.0f + color);
}

float4 PSReinhard(VSInputTx pin) : SV_Target0
{
    float4 hdr = HDRTexture.Sample(Sampler, pin.TexCoord);
    float3 sdr = Reinhard(hdr.xyz);
    return float4(sdr, hdr.a);
}


// Pixel shader: filmic operator
float3 Filmic(float3 color)
{
    float3 x = max(0.0f, color - 0.004f);
    return pow((x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f), 2.2f);
}

float4 PSFilmic(VSInputTx pin) : SV_Target0
{
    float4 hdr = HDRTexture.Sample(Sampler, pin.TexCoord);
    float3 sdr = Filmic(hdr.xyz);
    return float4(sdr, hdr.a);
}


// Pixel shader: HDR10, using Rec.2020 color primaries and ST.2084 curve
float3 HDR10(float3 color)
{
    const float3x3 from709to2020 =
    {
        { 0.6274040f, 0.3292820f, 0.0433136f },
        { 0.0690970f, 0.9195400f, 0.0113612f },
        { 0.0163916f, 0.0880132f, 0.8955950f }
    };

    // Rotate from Rec.709 to Rec.2020 primaries
    float3 rgb = mul(from709to2020, color);

    // ST.2084 spec defines max nits as 10,000 nits
    float3 normalized = rgb * paperWhiteNits.x / 10000.f;

    // Apply ST.2084 curve
    return pow((0.8359375f + 18.8515625f * pow(abs(normalized), 0.1593017578f)) / (1.0f + 18.6875f * pow(abs(normalized), 0.1593017578f)), 78.84375f);
}

float4 PSHDR10(VSInputTx pin) : SV_Target0
{
    float4 hdr = HDRTexture.Sample(Sampler, pin.TexCoord);
    float3 rgb = HDR10(hdr.xyz);
    return float4(rgb, hdr.a);
}


//--------------------------------------------------------------------------------------
struct MRTOut
{
    float4 hdr : SV_Target0;
    float4 sdr : SV_Target1;
};

MRTOut PSHDR10_Saturate(VSInputTx pin)
{
    MRTOut output;

    float4 hdr = HDRTexture.Sample(Sampler, pin.TexCoord);
    float3 rgb = HDR10(hdr.xyz);
    output.hdr = float4(rgb, hdr.a);

    float3 sdr = saturate(hdr.xyz);
    output.sdr = float4(sdr, hdr.a);

    return output;
}

MRTOut PSHDR10_Reinhard(VSInputTx pin)
{
    MRTOut output;

    float4 hdr = HDRTexture.Sample(Sampler, pin.TexCoord);
    float3 rgb = HDR10(hdr.xyz);
    output.hdr = float4(rgb, hdr.a);

    float3 sdr = Reinhard(hdr.xyz);
    output.sdr = float4(sdr, hdr.a);

    return output;
}

MRTOut PSHDR10_Filmic(VSInputTx pin)
{
    MRTOut output;

    float4 hdr = HDRTexture.Sample(Sampler, pin.TexCoord);
    float3 rgb = HDR10(hdr.xyz);
    output.hdr = float4(rgb, hdr.a);

    float3 sdr = Filmic(hdr.xyz);
    output.sdr = float4(sdr, hdr.a);

    return output;
}
