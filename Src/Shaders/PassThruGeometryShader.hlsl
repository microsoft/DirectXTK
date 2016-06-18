// Per-vertex data from the vertex shader.
struct GeometryShaderInput
{
	min16float4 color    : COLOR0;							//The RGBA normalized color
	min16float2 texCoord : TEXCOORD0;						//The U - V coordinates
	min16float4 position : SV_Position;					//The normalized XYZW position
	uint  viewId : TEXCOORD1;						//When calling DrawIndexedInstanced the single instance id passed in from DirectX
};

// Per-vertex data passed to the rasterizer.
struct GeometryShaderOutput
{
	min16float4 color   : COLOR0;
	min16float4 pos     : SV_POSITION;
	uint        rtvId   : SV_RenderTargetArrayIndex;
};

// This geometry shader is a pass-through that leaves the geometry unmodified 
// and sets the render target array index.
[maxvertexcount(3)]
void SpriteGeometryShader(triangle GeometryShaderInput input[3], inout TriangleStream<GeometryShaderOutput> outStream)
{
	GeometryShaderOutput output;
	[unroll(3)]
	for (int i = 0; i < 3; ++i)
	{
		output.pos = input[i].position;
		output.color = input[i].color;
		output.rtvId = input[i].viewId;
		outStream.Append(output);
	}
}
