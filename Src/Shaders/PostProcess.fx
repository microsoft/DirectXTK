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


// Pixel shader: copy.
float4 PSCopy(VSInputTx pin) : SV_Target0
{
    float4 color = Texture.Sample(Sampler, pin.TexCoord);
    return color;
}
