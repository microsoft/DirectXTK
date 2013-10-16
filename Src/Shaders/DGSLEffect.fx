// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

//
// Based on the Visual Studio 3D Starter Kit
//
// http://aka.ms/vs3dkit
//

cbuffer MaterialVars : register (b0)
{
    float4 MaterialAmbient;
    float4 MaterialDiffuse;
    float4 MaterialSpecular;
    float4 MaterialEmissive;
    float MaterialSpecularPower;
};
  
cbuffer ObjectVars : register(b2)
{
    float4x4 LocalToWorld4x4;
    float4x4 LocalToProjected4x4;
    float4x4 WorldToLocal4x4;
    float4x4 WorldToView4x4;
    float4x4 UVTransform4x4;
    float3 EyePosition;
};

struct A2V
{
    float4 pos : SV_Position;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
};

struct A2V_Vc
{
    float4 pos : SV_Position;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};
  
struct V2P
{
    float4 pos : SV_POSITION;
    float4 diffuse : COLOR;
    float2 uv : TEXCOORD0;
    float3 worldNorm : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    float3 toEye : TEXCOORD3;
    float4 tangent : TEXCOORD4;
    float3 normal : TEXCOORD5;
};

V2P main(A2V vertex)
{
    V2P result;
  
    float3 wp = mul(vertex.pos, LocalToWorld4x4).xyz;
  
    // set output data
    result.pos = mul(vertex.pos, LocalToProjected4x4);
    result.diffuse = MaterialDiffuse;
    result.uv = mul(float4(vertex.uv.x, vertex.uv.y, 0, 1), UVTransform4x4).xy;
    result.worldNorm = mul(vertex.normal, (float3x3)LocalToWorld4x4);
    result.worldPos = wp;
    result.toEye = EyePosition - wp;
    result.tangent = vertex.tangent;
    result.normal = vertex.normal;
  
    return result;
}

V2P mainVc(A2V_Vc vertex)
{
    V2P result;
  
    float3 wp = mul(vertex.pos, LocalToWorld4x4).xyz;
  
    // set output data
    result.pos = mul(vertex.pos, LocalToProjected4x4);
    result.diffuse = vertex.color * MaterialDiffuse;
    result.uv = mul(float4(vertex.uv.x, vertex.uv.y, 0, 1), UVTransform4x4).xy;
    result.worldNorm = mul(vertex.normal, (float3x3)LocalToWorld4x4);
    result.worldPos = wp;
    result.toEye = EyePosition - wp;
    result.tangent = vertex.tangent;
    result.normal = vertex.normal;
  
    return result;
}
