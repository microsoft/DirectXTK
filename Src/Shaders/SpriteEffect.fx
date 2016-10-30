// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://create.msdn.com/en-US/education/catalog/sample/stock_effects


// Modified by: THOTH Speed Engineers (c) 2016
// Modified Author: Dwight Goins
// Purpose: Fix the Vertex and Pixel shaders to render to Holographic (Stereoscopic) views (buffers)
//			as populated by the Hololens / Holographic cameras


Texture2D<float4> Texture : register(t0);
sampler TextureSampler : register(s0);


cbuffer Parameters : register(b0)
{
    row_major float4x4 MatrixTransform;
};

// Create a constant buffer which will hold the left and right camera Matrices
// These buffers will be populated by Holographic Cameras such as Hololens
// These buffers act as stereoscopic views in column-major format.
// Register this buffer so that we can fill it using slot 1 (the second slot) when calling SetConstantBuffer in DirectX
cbuffer ViewProjectionConstantBuffer : register(b1)
{
	float4x4 viewProjection[2];
};


void SpriteVertexShader(
	inout float4 color    : COLOR0,							//The RGBA normalized color
	inout float2 texCoord : TEXCOORD0,						//The U - V coordinates
	inout float4 position : SV_Position,					//The normalized XYZW position
	in uint  instId : SV_InstanceID,						//When calling DrawIndexedInstanced the single instance id passed in from DirectX
	out uint        rtvId : SV_RenderTargetArrayIndex		//The Index number of the array to use when the array is access
)
{
	// Note which view this vertex has been sent to. Used for matrix lookup.
	// Taking the modulo of the instance ID allows geometry instancing to be used
	// along with stereo instanced drawing; in that case, two copies of each 
	// instance would be drawn, one for left and one for right.
	position = mul(position, MatrixTransform);

	// get the instance ID get it's modulus (will either be 0 or 1)
	// we can uses these index numbers for Left and Right matrix from Holographic cameras
	uint idx = instId % 2;

	// Transform the vertex position into world space from a specific view of the holographic camera.
	// 0 - Left view, 1 - Right view (In the case of HoloLens)
	position = mul(position, viewProjection[idx]);

	// Set the Render Target Array Index for the Pixel Shader to 0 or 1 (Left or Right)
	// Note this only works on DirectX Level 5_0 and up Shaders 
	rtvId = idx;
}

void NonVPRT_SpriteVertexShader(
	inout float4 color    : COLOR0,							//The RGBA normalized color
	inout float2 texCoord : TEXCOORD0,						//The U - V coordinates
	inout float4 position : SV_Position,					//The normalized XYZW position
	in uint  instId : SV_InstanceID,						//When calling DrawIndexedInstanced the single instance id passed in from DirectX	
	out uint viewId :	TEXCOORD1
)
{
	// Note which view this vertex has been sent to. Used for matrix lookup.
	// Taking the modulo of the instance ID allows geometry instancing to be used
	// along with stereo instanced drawing; in that case, two copies of each 
	// instance would be drawn, one for left and one for right.
	position = mul(position, MatrixTransform);

	// get the instance ID get it's modulus (will either be 0 or 1)
	// we can uses these index numbers for Left and Right matrix from Holographic cameras
	uint idx = instId % 2;

	// Transform the vertex position into world space from a specific view of the holographic camera.
	// 0 - Left view, 1 - Right view (In the case of HoloLens)
	position = mul(position, viewProjection[idx]);

	viewId = idx;
}


float4 SpritePixelShader(float4 color    : COLOR0,
                         float2 texCoord : TEXCOORD0) : SV_Target0
{
    return Texture.Sample(TextureSampler, texCoord) * color;
}
