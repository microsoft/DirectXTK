// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929

static const int MAX_SAMPLES = 16;


Texture2D<float4> Texture : register(t0);
sampler Sampler : register(s0);


cbuffer Parameters : register(b0)
{
    float4 sampleOffsets[MAX_SAMPLES];
    float4 sampleWeights[MAX_SAMPLES];
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
// Pixel shader: copy.
float4 PSCopy(VSInputTx pin) : SV_Target0
{
    float4 color = Texture.Sample(Sampler, pin.TexCoord);
    return color;
}


// Pixel shader: monochrome.
float4 PSMonochrome(VSInputTx pin) : SV_Target0
{
    float4 result = Texture.Sample(Sampler, pin.TexCoord);
    float3 grayscale = float3(0.2125f, 0.7154f, 0.0721f);
    float3 output = dot(result.rgb, grayscale);
    return float4(output, result.a);
}


// Pixel shader: down-sample 2x2.
float4 PSDownScale2x2(VSInputTx pin) : SV_Target0
{
    const int NUM_SAMPLES = 4;
    float4 vColor = 0.0f;

    for( int i=0; i < NUM_SAMPLES; i++ )
    {
        vColor += Texture.Sample(Sampler, pin.TexCoord + sampleOffsets[i].xy);
    }
    
    return vColor / NUM_SAMPLES;
}


// Pixel shader: down-sample 4x4.
float4 PSDownScale4x4(VSInputTx pin) : SV_Target0
{
    const int NUM_SAMPLES = 16;
    float4 vColor = 0.0f;

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        vColor += Texture.Sample(Sampler, pin.TexCoord + sampleOffsets[i].xy);
    }

    return vColor / NUM_SAMPLES;
}


// Pixel shader: gaussian blur 5x5.
float4 PSGaussianBlur5x5(VSInputTx pin) : SV_Target0
{
    float4 vColor = 0.0f;

    for (int i = 0; i < 13; i++)
    {
        vColor += sampleWeights[i] * Texture.Sample(Sampler, pin.TexCoord + sampleOffsets[i].xy);
    }

    return vColor;
}


// Pixel shader: bloom
float4 PSBloomExtract(VSInputTx pin) : SV_Target0
{
    // Uses sampleWeights[0] as 'bloom threshold'
    float4 c = Texture.Sample(Sampler, pin.TexCoord);
    return saturate((c - sampleWeights[0]) / (1 - sampleWeights[0]));
}

float4 PSBloomBlur(VSInputTx pin) : SV_Target0
{
    float4 vColor = 0.0f;

    // Perform a one-directional gaussian blur
    for (int i = 0; i < 15; i++)
    {
        vColor += sampleWeights[i] * Texture.Sample(Sampler, pin.TexCoord + sampleOffsets[i].xy);
    }

    return vColor;
}


// Pixel shader: sample luminance (initial)
float4 PSSampleLuminanceInitial(VSInputTx pin) : SV_Target0
{
    const int NUM_SAMPLES = 9;
    float fSum = 0.0f;

    for( int i = 0; i < NUM_SAMPLES; i++ )
    {
        // Compute the sum of log(luminance) throughout the sample points
        float3 vColor = Texture.Sample(Sampler, pin.TexCoord + sampleOffsets[i].xy).rgb;
        float3 grayscale = float3(0.2125f, 0.7154f, 0.0721f);
        float  fLuminance = dot(vColor, grayscale);
        fSum += log(fLuminance + 0.0001f);
    }
    
    // Divide the sum to complete the average
    fSum /= NUM_SAMPLES;

    return float4(fSum, fSum, fSum, 1.0f);
}


// Pixel shader: sample luminance (final)
float4 PSSampleLuminanceFinal(VSInputTx pin) : SV_Target0
{
    const int NUM_SAMPLES = 16;
    float fSum = 0.0f;

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        // Compute the sum of luminance throughout the sample points
        fSum += Texture.Sample(Sampler, pin.TexCoord + sampleOffsets[i].xy).x;
    }

    // Divide the sum to complete the average
    fSum /= NUM_SAMPLES;

    // Perform an exp() to complete the average luminance calculation
    fSum = exp(fSum);

    return float4(fSum, fSum, fSum, 1.0f);
}


//--------------------------------------------------------------------------------------
Texture2D<float4> Texture2 : register(t1);

// Pixel shader: merge
float4 PSMerge(VSInputTx pin) : SV_Target0
{
    float4 vColor = sampleWeights[0] * Texture.Sample(Sampler, pin.TexCoord);
    vColor += sampleWeights[1] * Texture2.Sample(Sampler, pin.TexCoord);
    return vColor;
}
