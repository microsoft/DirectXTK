--------------------------------
DirectXTK - the DirectX Tool Kit
--------------------------------

Copyright (c) Microsoft Corporation. All rights reserved.

March 29, 2012

This package contains the "DirectX Tool Kit", a collection of helper classes for
writing Direct3D 11 code for Metro style apps, Windows 8 Desktop, and Windows 7 'classic'
applications in C++.

This code is designed to build with either Visual Studio 11 which includes the Windows 8.0 SDK
or Visual Studio 2010 with the standalone Windows 8.0 SDK installed using the "Platform Toolset" set
to "Windows 8.0 SDK".  It makes use of the DirectXMath library and optionally the DXGI 1.2 headers
from the Windows 8.0 SDK as well as other headers.

These components are designed to work without requiring any content from the DirectX SDK. For details,
see "Where is the DirectX SDK"? <http://msdn.microsoft.com/en-us/library/ee663275.aspx>

Inc\
    Public Header Files:

    SpriteBatch.h - simple & efficient 2D sprite rendering
    Effects.h - set of built-in shaders for common rendering tasks
    GeometricPrimitive.h - draws basic shapes such as cubes and spheres
    CommonStates.h - factory providing commonly used D3D state objects
    VertexTypes.h - structures for commonly used vertex data formats
    DDSTextureLoader.h - light-weight DDS file texture loader
    WICTextureLoader.h - WIC-based image file texture loader

Src\
    DirectXTK source files and internal implementation headers

All content and source code for this package are bound to the Microsoft Public License (Ms-PL)
<http://www.microsoft.com/en-us/openness/licenses.aspx#MPL>.

http://go.microsoft.com/fwlink/?LinkId=248929


-----------
SpriteBatch
-----------

This is a native D3D11 implementation of the SpriteBatch helper from XNA Game 
Studio, providing identical functionality and API.

During initialization:

    std::unique_ptr<SpriteBatch> spriteBatch(new SpriteBatch(deviceContext));

Simple drawing:

    spriteBatch->Begin();
    spriteBatch->Draw(texture, XMFLOAT2(x, y));
    spriteBatch->End();

The Draw method has many overloads with parameters controlling:

    - Specify position as XMFLOAT2, XMVECTOR or RECT
    - Optional source rectangle for drawing just part of a sprite sheet
    - Tint color
    - Rotation (in radians)
    - Origin point (position, scaling and rotation are relative to this)
    - Scale
    - SpriteEffects enum (for horizontal or vertical mirroring)
    - Layer depth (for sorting)

Sorting:

    The first parameter to SpriteBatch::Begin is a SpriteSortMode enum. For 
    most efficient rendering, use SpriteSortMode_Deferred (which batches up 
    sprites, then submits them all to the GPU during the End call), and 
    manually draw everything in texture order. If it is not possible to draw 
    in texture order, the second most efficient approach is to use 
    SpriteSortMode_Texture, which will automatically sort on your behalf.

    When drawing scenes with multiple depth layers, SpriteSortMode_BackToFront 
    or SpriteSortMode_FrontToBack will sort by the layerDepth parameter 
    specified to each Draw call.

    SpriteSortMode_Immediate disables all batching, submitting a separate D3D 
    draw call for each sprite. This is expensive, but convenient in rare cases 
    when you need to set shader constants differently per sprite.

    Multiple SpriteBatch instances are lightweight. It is reasonable to 
    create several, Begin them at the same time with different sort modes, 
    submit sprites to different batches in arbitrary orders as you traverse a 
    scene, then End the batches in whatever order you want these groups of 
    sprites to be drawn.

Custom render states:

    By default SpriteBatch uses premultiplied alpha blending, no depth buffer, 
    counter clockwise culling, and linear filtering with clamp texture 
    addressing. You can change this (if for instance you do not wish to use 
    premultiplied alpha) by passing custom state objects to SpriteBatch::Begin. 
    Pass null for any parameters that should use their default state.

    To use SpriteBatch with a custom pixel shader (handy for 2D postprocessing 
    effects such as bloom or blur) or even a custom vertex shader, use the 
    setCustomShaders parameter to specify a state setting callback function:

    spriteBatch->Begin(SpriteSortMode_Deferred, nullptr, nullptr, nullptr, nullptr, [=]
    {
        deviceContext->PSSetShader(...);
        deviceContext->PSSetConstantBuffers(...);
        deviceContext->PSSetShaderResources(...);
    });

    SpriteBatch automatically sets pixel shader resource #0 to the texture 
    specified by each Draw call, so you only need to call PSSetResources for 
    any additional textures required by your shader.

    SpriteBatch::Begin also has a transformMatrix parameter, which can be used 
    for global transforms such as scaling or translation of an entire scene.

Threading model:

    Creation is fully asynchronous, so you can instantiate multiple SpriteBatch 
    instances at the same time on different threads. Each SpriteBatch instance 
    only supports drawing from one thread at a time, but you can simultaneously 
    submit sprites on multiple threads if you create a separate SpriteBatch 
    instance per D3D11 deferred context.

Further reading:

    http://www.shawnhargreaves.com/blogindex.html#spritebatch
    http://blogs.msdn.com/b/shawnhar/archive/2010/06/18/spritebatch-and-renderstates-in-xna-game-studio-4-0.aspx
    http://www.shawnhargreaves.com/blogindex.html#premultipliedalpha



-------
Effects
-------

This is a native D3D11 implementation of the five built-in effects from XNA 
Game Studio, providing identical functionality and API:

    - BasicEffect supports texture mapping, vertex coloring, directional lighting, and fog
    - AlphaTestEffect supports per-pixel alpha testing
    - DualTextureEffect supports two layer multitexturing (for lightmaps or detail textures)
    - EnvironmentMapEffect supports cubic environment mapping
    - SkinnedEffect supports skinned animation

During initialization:

    std::unique_ptr<BasicEffect> effect(new BasicEffect(device));

Set effect parameters:

    effect->SetWorld(world);
    effect->SetView(view);
    effect->SetProjection(projection);

    effect->SetTexture(cat);
    effect->SetTextureEnabled(true);

    effect->EnableDefaultLighting();

Draw using the effect:

    effect->Apply(deviceContext);

    deviceContext->IASetInputLayout(...);
    deviceContext->IASetVertexBuffers(...);
    deviceContext->IASetIndexBuffer(...);
    deviceContext->IASetPrimitiveTopology(...);

    deviceContext->DrawIndexed(...);

To create an input layout matching the effect vertex shader input signature:

    // First, configure effect parameters the way you will be using it. Turning
    // lighting, texture map, or vertex color on/off alters which vertex shader
    // is used, so GetVertexShaderBytecode will return a different blob after
    // you alter these parameters. If you create an input layout using a
    // BasicEffect that had lighting disabled, but then later enable lighting,
    // that input layout will no longer match as it will not include the
    // now-necessary normal vector.

    void const* shaderByteCode;
    size_t byteCodeLength;

    effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    device->CreateInputLayout(VertexPositionNormalTexture::InputElements,
                              VertexPositionNormalTexture::InputElementCount,
                              shaderByteCode, byteCodeLength,
                              pInputLayout);

Threading model:

    Creation is fully asynchronous, so you can instantiate multiple effect 
    instances at the same time on different threads. Each instance only 
    supports drawing from one thread at a time, but you can simultaneously draw 
    on multiple threads if you create a separate effect instance per D3D11 
    deferred context.

Further reading:

    http://blogs.msdn.com/b/shawnhar/archive/2010/04/28/new-built-in-effects-in-xna-game-studio-4-0.aspx
    http://blogs.msdn.com/b/shawnhar/archive/2010/04/30/built-in-effects-permutations-and-performance.aspx
    http://blogs.msdn.com/b/shawnhar/archive/2010/04/25/basiceffect-optimizations-in-xna-game-studio-4-0.aspx
    http://blogs.msdn.com/b/shawnhar/archive/2008/08/22/basiceffect-a-misnomer.aspx
    http://blogs.msdn.com/b/shawnhar/archive/2010/08/04/dualtextureeffect.aspx
    http://blogs.msdn.com/b/shawnhar/archive/2010/08/09/environmentmapeffect.aspx



------------------
GeometricPrimitive
------------------

This is a helper for drawing simple geometric shapes:

    - Cube
    - Sphere
    - Cylinder
    - Torus
    - Teapot

During initialization:

    std::unique_ptr<GeometricPrimitive> shape(GeometricPrimitive::CreateTeapot(deviceContext));

Simple drawing:

    shape->Draw(world, view, projection, Colors::CornflowerBlue);

The draw method accepts an optional texture parameter, wireframe flag, and a 
callback function which can be used to override the default rendering state:

    shape->Draw(world, view, projection, Colors::White, catTexture, false, [=]
    {
        deviceContext->OMSetBlendState(...);
    });



------------
CommonStates
------------

The CommonStates class is a factory which simplifies setting the most common 
combinations of D3D rendering states.

During initialization:

    std::unique_ptr<CommonStates> states(new CommonStates(device));

Using this helper to set device state:

    deviceContext->OMSetBlendState(states->Opaque(), Colors::Black, 0xFFFFFFFF);
    deviceContext->OMSetDepthStencilState(states->DepthDefault(), 0);
    deviceContext->RSSetState(states->CullCounterClockwise());

    auto samplerState = states->LinearWrap();
    deviceContext->PSSetSamplers(0, 1, &samplerState);

Available states:

    ID3D11BlendState* Opaque();
    ID3D11BlendState* AlphaBlend();
    ID3D11BlendState* Additive();
    ID3D11BlendState* NonPremultiplied();

    ID3D11DepthStencilState* DepthNone();
    ID3D11DepthStencilState* DepthDefault();
    ID3D11DepthStencilState* DepthRead();

    ID3D11RasterizerState* CullNone();
    ID3D11RasterizerState* CullClockwise();
    ID3D11RasterizerState* CullCounterClockwise();
    ID3D11RasterizerState* Wireframe();

    ID3D11SamplerState* PointWrap();
    ID3D11SamplerState* PointClamp();
    ID3D11SamplerState* LinearWrap();
    ID3D11SamplerState* LinearClamp();
    ID3D11SamplerState* AnisotropicWrap();
    ID3D11SamplerState* AnisotropicClamp();



-----------
VertexTypes
-----------

VertexTypes.h defines these commonly used vertex data structures:

    - VertexPositionColor
    - VertexPositionTexture
    - VertexPositionNormal
    - VertexPositionColorTexture
    - VertexPositionNormalColor
    - VertexPositionNormalTexture
    - VertexPositionNormalColorTexture

Each type also provides a D3D11_INPUT_ELEMENT_DESC array which can be used to 
create a matching input layout, for example:

    device->CreateInputLayout(VertexPositionColorTexture::InputElements,
                              VertexPositionColorTexture::InputElementCount,
                              vertexShaderCode, vertexShaderCodeSize,
                              &inputLayout);



----------------
DDSTextureLoader
----------------

DDSTextureLoader.h contains a simple light-weight loader for .dds files. It supports
1D and 2D textures, texture arrays, cubemaps, volume maps, and mipmap levels. This
version performs no legacy conversions, so the .dds file must directly map to a
DXGI_FORMAT (i.e. legacy 24 bpp .dds files will fail to load). It does support both
'legacy' and 'DX10' extension file header format .dds files.

For a full-featured DDS file reader, writer, and texture processing pipeline see
the 'Texconv' sample and the 'DirectXTex' library

CreateDDSTextureFromFile and CreateDDSTextureFromMemory can be called requesting only
a 'texture' resource (i.e. textureView is null), requesting only a shader resource
view (i.e. texture is null), or both.

If maxsize is non-zero, then any mipmap level larger than the maxsize present in the
file will be ignored and only smaller mipmap levels (if present) will be used. This
can be used to scale the content at load-time. Note this requires the .dds file contain
mipmaps to retry with a smaller size.

If maxsize is 0 and the initial attempt to create the texture fails, it will attempt
to trim off any mipmap levels larger than the minimum required size for the given
device Feature Level and retry. Note this requires the .dds file contains mipmaps to
retry with a smaller size.

DDSTextureLoader will load BGR 5:6:5 and BGRA 5:5:5:1 DDS files, but these formats will
fail to create on a system with DirectX 11.0 Runtime. The DXGI 1.2 version of
DDSTextureLoader will load BGRA 4:4:4:4 DDS files using DXGI_FORMAT_B4G4R4A4_UNORM.
The DirectX 11.1 Runtime, DXGI 1.2 headers, and WDDM 1.2 drivers are required to
support 16bpp pixel formats on all feature levels.

Further reading:
    http://go.microsoft.com/fwlink/?LinkId=248926
    http://blogs.msdn.com/b/chuckw/archive/2011/10/28/directxtex.aspx



----------------
WICTextureLoader
----------------

WICTextureLoader.h contains a loader for BMP, JPEG, PNG, TIFF, GIF, HD Photo, and
other WIC-supported image formats. This performs any required pixel format conversions
or image resizing using WIC at load time as well.

NOTE: WICTextureLoader cannot load .TGA files unless the system has a 3rd party WIC
codec installed. You must use the DirectXTex library for TGA file format support
without relying on an add-on WIC codec.

CreateWICTextureFromFile and CreateWICTextureFromMemory can be called requesting only
a 'texture' resource (i.e. textureView is null), requesting only a shader resource
view (i.e. texture is null), or both.

If maxsize is non-zero, then if the image is larger than maxsize it will be resized
so that the largest dimension matches 'maxsize' (the other dimension will be derived
respecting the original aspect-ratio of the image).

If the maxsize is 0, then the maxsize will be implicitly set to the minimum required
supported texture size based on the device's current feature level (2k, 4k, 8k, or 16k).

If a Direct3D 11 context is provided and a shader resource view is requested (i.e.
textureView is non-null), then the texture will be created to support auto-generated
mipmaps and will have GenerateMips called on it before returning. If no context is
provided (i.e. d3dContext is null) or the pixel format is unsupported for auto-gen
mips by the current device, then the resulting texture will have only a single level.

This loader does not support array textures, 1D textures, 3D volume textures, or 
cubemaps. For these scenarios, use the .DDS file format and DDSTextureLoader instead.

The DXGI 1.2 version of WICTextureLoader will load 16bpp pixel images as 5:6:5 or
5:5:5:1 rather than expand them to 32bpp RGBA. The DirectX 11.1 Runtime, DXGI 1.2
headers, and WDDM 1.2 drivers are required to support 16bpp pixel formats on all
feature levels.

Further reading:
    http://go.microsoft.com/fwlink/?LinkId=248926
    http://blogs.msdn.com/b/chuckw/archive/2011/10/28/directxtex.aspx



---------------
RELEASE HISTORY
---------------

March 29, 2012
    WICTextureLoader updated with Windows 8 WIC native pixel formats

March 6, 2012:
    Fix for too much temp memory used by WICTextureLoader
    Add separate Visual Studio 11 projects for Desktop vs. Metro builds

March 5, 2012:
    Bug fix for SpriteBatch with batches > 2048

February 24, 2012:
    Original release
