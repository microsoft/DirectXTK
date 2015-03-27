--------------------------------
DirectXTK - the DirectX Tool Kit
--------------------------------

Copyright (c) Microsoft Corporation. All rights reserved.

March 27, 2015

This package contains the "DirectX Tool Kit", a collection of helper classes for 
writing Direct3D 11 C++ code for Windows Store apps, Windows phone 8.x applications,
Xbox One exclusive apps, Xbox One hub apps, Windows 8.x Win32 desktop applications,
Windows 7 applications, and Windows Vista Direct3D 11.0 applications.

This code is designed to build with Visual Studio 2010, 2012, or 2013. It requires
the Windows 8.x SDK for functionality such as the DirectXMath library and the DXGI
1.2 headers. Visual Studio 2012 and 2013 already include the appropriate Windows SDK,
but Visual Studio 2010 users must install the standalone Windows 8.1 SDK. Details on
using the Windows 8.1 SDK with VS 2010 are described on the Visual C++ Team Blog:

<http://blogs.msdn.com/b/vcblog/archive/2012/11/23/using-the-windows-8-sdk-with-visual-studio-2010-configuring-multiple-projects.aspx>

These components are designed to work without requiring any content from the DirectX SDK. For details,
see "Where is the DirectX SDK?" <http://msdn.microsoft.com/en-us/library/ee663275.aspx>.

Inc\
    Public Header Files (in the DirectX C++ namespace):

    Audio.h - low-level audio API using XAudio2 (DirectXTK for Audio public header)
    CommonStates.h - factory providing commonly used D3D state objects
    DirectXHelpers.h - misc C++ helpers for D3D programming
    DDSTextureLoader.h - light-weight DDS file texture loader
    Effects.h - set of built-in shaders for common rendering tasks
    GamePad.h - gamepad controller helper using XInput
    GeometricPrimitive.h - draws basic shapes such as cubes and spheres
    Model.h - draws meshes loaded from .CMO, .SDKMESH, or .VBO files
    PrimitiveBatch.h - simple and efficient way to draw user primitives
    ScreenGrab.h - light-weight screen shot saver
    SimpleMath.h - simplified C++ wrapper for DirectXMath
    SpriteBatch.h - simple & efficient 2D sprite rendering
    SpriteFont.h - bitmap based text rendering
    VertexTypes.h - structures for commonly used vertex data formats
    WICTextureLoader.h - WIC-based image file texture loader
    XboxDDSTextureLoader.h - Xbox One exclusive apps variant of DDSTextureLoader

Src\
    DirectXTK source files and internal implementation headers

Audio\
    DirectXTK for Audio source files and internal implementation headers

MakeSpriteFont\
    Command line tool used to generate binary resources for use with SpriteFont

XWBTool\
    Command line tool for building XACT-style wave banks for use with DirectXTK for Audio's WaveBank class

All content and source code for this package are subject to the terms of the MIT License.
<http://opensource.org/licenses/MIT>.

For the latest version of DirectXTK, more detailed documentation, discussion forums, bug
reports and feature requests, please visit the Codeplex site.

http://go.microsoft.com/fwlink/?LinkId=248929

Note: Xbox One exclusive apps developers using the Xbox One XDK need to generate the
      Src\Shaders\Compiled\XboxOne*.inc files to build the library as they are not
      included in the distribution package. They are built by running the script
      in Src\Shaders - "CompileShaders xbox", and should be generated with the matching
      FXC compiler from the Xbox One XDK. While they will continue to work if outdated,
      a mismatch will cause runtime compilation overhead that would otherwise be avoided.



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

Alpha blending:

    Alpha blending defaults to using premultiplied alpha. To make use of 'straight'
    alpha textures, override the blending mode via the optional callback:

    CommonStates states(device);

    spriteBatch->Begin(SpriteSortMode_Deferred, nullptr, nullptr, nullptr, nullptr, [=]
    {
        deviceContext->OMSetBlendState( states.NonPremultiplied(), nullptr, 0xFFFFFFFF);
    });

Custom render states:

    By default SpriteBatch uses premultiplied alpha blending, no depth buffer, 
    counter clockwise culling, and linear filtering with clamp texture 
    addressing. You can change this by passing custom state objects to
    SpriteBatch::Begin. Pass null for any parameters that should use their default state.

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

Orientation:

    For phones, laptops, and tablets the orientation of the display can be changed
    by the user. For Windows Store apps, DirectX applications are encouraged to
    handle the rotation internally rather than relying on DXGI's auto-rotation handling.
    In older versions of DirectXTK, you had to handle orientation changes via the custom
    transform matrix on Begin(). In the latest version of DirectXTK, you can handle it
    via a rotation setting (which is applied after any custom transformation).

    Windows Store apps for Windows 8:

    DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;
    switch (m_orientation)
    {
    case DisplayOrientations::Landscape: rotation = DXGI_MODE_ROTATION_IDENTITY;  break;
    case DisplayOrientations::Portrait: rotation = DXGI_MODE_ROTATION_ROTATE270; break;
    case DisplayOrientations::LandscapeFlipped: rotation = DXGI_MODE_ROTATION_ROTATE180; break;
    case DisplayOrientations::PortraitFlipped: rotation = DXGI_MODE_ROTATION_ROTATE90; break;
    }
    spriteBatch->SetRotation( rotation );

    Windows phone 8.0 apps (see http://www.catalinzima.com/2012/12/handling-orientation-in-a-windows-phone-8-game/):

    DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;
    switch (m_orientation)
    {
    case DisplayOrientations::Portrait: rotation = DXGI_MODE_ROTATION_IDENTITY;  break;
    case DisplayOrientations::Landscape: rotation = DXGI_MODE_ROTATION_ROTATE90; break;
    case DisplayOrientations::PortraitFlipped: rotation = DXGI_MODE_ROTATION_ROTATE180; break;
    case DisplayOrientations::LandscapeFlipped: rotation = DXGI_MODE_ROTATION_ROTATE270; break;
    }
    spriteBatch->SetRotation( rotation );

    Windows Store apps for Windows 8.1:

    spriteBatch->SetRotation( m_deviceResources->ComputeDisplayRotation() );

3D sprites:

    As explained in this article, you can use SpriteBatch to render particles and billboard text in 3D as well
    as the default 2D behavior.

    http://blogs.msdn.com/b/shawnhar/archive/2011/01/12/spritebatch-billboards-in-a-3d-world.aspx

    To simplify this application, you can set the rotation mode so that SpriteBatch doesn't apply a view transform
    matrix internally and assumes the full transform is accomplished by Begin's matrix parameter.

    spriteBatch->SetRotation( DXGI_MODE_ROTATION_UNSPECIFIED );

Further reading:

    http://www.shawnhargreaves.com/blogindex.html#spritebatch
    http://blogs.msdn.com/b/shawnhar/archive/2010/06/18/spritebatch-and-renderstates-in-xna-game-studio-4-0.aspx
    http://www.shawnhargreaves.com/blogindex.html#premultipliedalpha



----------
SpriteFont
----------

This is a native D3D11 implementation of a bitmap font renderer, similar to the 
SpriteFont type from XNA Game Studio, plus a command line tool (MakeSpriteFont) 
for building fonts into bitmap format. It is less fully featured than Direct2D 
and DirectWrite, but may be useful for those who want something simpler and 
lighter weight.

At build time:

    MakeSpriteFont.exe "Comic Sans" myfile.spritefont /FontSize:16

During initialization:

    std::unique_ptr<SpriteBatch> spriteBatch(new SpriteBatch(deviceContext));
    std::unique_ptr<SpriteFont> spriteFont(new SpriteFont(device, L"myfile.spritefont"));

Simple drawing:

    spriteBatch->Begin();
    spriteFont->DrawString(spriteBatch.get(), L"Hello, world!", XMFLOAT2(x, y));
    spriteBatch->End();

The Draw method has several overloads with parameters controlling color, 
rotation, origin point, scaling, horizontal or vertical mirroring, and layer 
depth. These work the same way as the equivalent SpriteBatch::Draw parameters.

SpriteFont has three constructors:

    - Pass a filename string to read a binary file created by MakeSpriteFont
    - Pass a buffer containing a MakeSpriteFont binary that was already loaded some other way
    - Pass an array of Glyph structs if you prefer to entirely bypass MakeSpriteFont

If you try to draw or call MeasureString with a character that is not included in 
the font, by default you will get an exception. Use SetDefaultCharacter to 
specify some other character that will be automatically substituted in place of 
any that are missing.

This implementation supports sparse fonts, so if you are localizing into 
languages such as Chinese, Japanese, or Korean, you can build a spritefont 
including only the specific characters needed by your program. This is usually a 
good idea for CJK languages, as a complete CJK character set is too large to fit 
in a Direct3D texture! (if you need full CJK support, Direct2D or DirectWrite would
be a better choice). SpriteFont does not support combining characters or
right-to-left (RTL) layout, so it will not work for languages with complex layout
requirements such as Arabic or Thai.

The MakeSpriteFont tool can process any TrueType font that is installed on your 
system (using GDI+ to rasterize them into a bitmap) or it can import character 
glyphs from a specially formatted bitmap file. This latter option allows you to 
create multicolored fonts, drawing special effects such as gradients or drop 
shadows directly into your glyph textures. Characters should be arranged in a 
grid ordered from top left to bottom right. Monochrome fonts should use white for 
solid areas and black for transparent areas. To include multicolored characters, 
add an alpha channel to the bitmap and use that to control which parts of each 
character are solid. The spaces between characters and around the edges of the 
grid should be filled with bright pink (red=255, green=0, blue=255). It doesn't 
matter if your grid includes lots of wasted space, because the converter will 
rearrange characters, packing everything as tightly as possible.

Command-line options for the MakeSpriteFont tool:

    /CharacterRegion:<region>
        Specifies which Unicode codepoints to include in the font. Can be 
        repeated to include more than one region. If not specified, the default 
        ASCII range (32-126) is used. Examples:
            /CharacterRegion:a-z
            /CharacterRegion:0x1200-0x1250
            /CharacterRegion:0x1234

    /DefaultCharacter:<value>
        Fallback character substituted in place of codepoints that are not 
        included in the font. If zero, missing characters throw exceptions.

    /FontSize:<value>
    /FontStyle:<value>
        Size and style (bold or italic) for TrueType fonts. Ignored when 
        converting a bitmap font.

    /LineSpacing:<value>
    /CharacterSpacing:<value>
        Spacing overrides. Zero is default spacing, negative closer together, 
        positive further apart.

    /Sharp
        Selects sharp antialising mode for TrueType rasterization. Otherwise,
        a smoother looking (but more blurry) antialiasing mode is used.

    /TextureFormat:<value>
        What format should the output texture be? Options:
            Auto
                The default. Chooses between CompressedMono and Rgba32 depending 
                on whether the font data is monochromatic or multicolored.
            Rgba32
                High quality and supports multicolored fonts, but wastes space.
            Bgra4444
                Good choice for color fonts on Windows Store apps and Windows phone
                platforms, as this format requires the DirectX 11.1 Runtime and a
                WDDM 1.2 driver.
            CompressedMono
                The smallest format, and works on all D3D platforms, but it only 
                supports monochromatic font data. This uses a special BC2 
                encoder: see comments in SpriteFontWriter.cs for details.

    /NoPremultiply
        By default, font textures use premultiplied alpha format. Pass this flag 
        if you want interpolative/straight alpha instead.

    /DebugOutputSpriteSheet:<filename>
        Dumps the generated texture to a bitmap file (useful when debugging the 
        MakeSpriteFont tool, not so much if you are just trying to use it).

Further reading:

    http://blogs.msdn.com/b/shawnhar/archive/2007/04/26/bitmap-fonts-in-xna.aspx
    http://create.msdn.com/en-US/education/catalog/utility/bitmap_font_maker



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

The library also includes support for the Visual Studio Shader Designer (DGSL) shaders

    - DGSLEffect supports both rigid and skinned animation with up to 8 textures.

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
Coordinate systems:

    The built-in effects work equally well for both right-handed and left-handed coordinate
    systems. The one difference is that the fog settings start & end for left-handed
    coordinate systems need to be negated (i.e. SetFogStart(6), SetFogEnd(8) for right-handed
    coordinates becomes SetFogStart(-6), SetFogEnd(-8) for left-handed coordinates).

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



--------------
PrimitiveBatch
--------------

This is a helper for easily and efficiently drawing dynamically generated 
geometry such as lines or trianges. It fills the same role as the legacy D3D9 
APIs DrawPrimitiveUP and DrawIndexedPrimitiveUP. Dynamic submission is a highly 
effective pattern for drawing procedural geometry, and convenient for debug 
rendering, but is not nearly as efficient as static vertex buffers. Excessive 
dynamic submission is a common source of performance problems in apps.

PrimitiveBatch manages the vertex and index buffers for you, using DISCARD and 
NO_OVERWRITE hints to avoid stalling the GPU pipeline. It automatically merges 
adjacent draw requests, so if you call DrawLine 100 times in a row, only a 
single GPU draw call will be generated.

PrimitiveBatch is responsible for setting the vertex buffer, index buffer, and 
primitive topology, then issuing the final draw call. Unlike the higher level 
SpriteBatch helper, it does not provide shaders, set the input layout, or set 
any state objects. PrimitiveBatch is often used in conjunction with BasicEffect 
and the structures from VertexTypes.h, but it can work with any other shader or 
vertex formats of your own.

To initialize a PrimitiveBatch for drawing VertexPositionColor data:

    std::unique_ptr<PrimitiveBatch<VertexPositionColor>> primitiveBatch(new PrimitiveBatch<VertexPositionColor>(deviceContext));

    The default values assume that your maximum batch size is 2048 vertices arranged in triangles. If you want to
    use larger batches, you need to provide the additional constructor parameters.

To set up a suitable BasicEffect and input layout:

    std::unique_ptr<BasicEffect> basicEffect(new BasicEffect(device));

    basicEffect->SetProjection(XMMatrixOrthographicOffCenterRH(0, screenHeight, screenWidth, 0, 0, 1));
    basicEffect->SetVertexColorEnabled(true);

    void const* shaderByteCode;
    size_t byteCodeLength;

    basicEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    ComPtr<ID3D11InputLayout> inputLayout;

    device->CreateInputLayout(VertexPositionColor::InputElements,
                              VertexPositionColor::InputElementCount,
                              shaderByteCode, byteCodeLength,
                              &inputLayout);

To draw a line:

    CommonStates states(device);
    deviceContext->OMSetBlendState( states.Opaque(), nullptr, 0xFFFFFFFF );
    deviceContext->OMSetDepthStencilState( states.DepthNone(), 0 );
    deviceContext->RSSetState( states.CullCounterClockwise() );

    basicEffect->Apply(deviceContext);
    deviceContext->IASetInputLayout(inputLayout.Get());

    primitiveBatch->Begin();
    primitiveBatch->DrawLine(VertexPositionColor(...), VertexPositionColor(...));
    primitiveBatch->End();

PrimitiveBatch provides five drawing methods:

    - DrawLine(v1, v2)
    - DrawTriangle(v1, v2, v3)
    - DrawQuad(v1, v2, v3, v4)
    - Draw(topology, vertices, vertexCount)
    - DrawIndexed(topology, indices, indexCount, vertices, vertexCount)

Optimization:

    For best performance, draw as much as possible inside the fewest separate 
    Begin/End blocks. This will reduce overhead and maximize potential for 
    batching.

    The PrimitiveBatch constructor allows you to specify what size index and 
    vertex buffers to allocate. You may want to tweak these values to fit your 
    workload, or if you only intend to draw non-indexed geometry, specify 
    maxIndices = 0 to entirely skip creating the index buffer.

Threading model:

    Each PrimitiveBatch instance only supports drawing from one thread at a 
    time, but you can simultaneously submit primitives on multiple threads if 
    you create a separate PrimitiveBatch instance per D3D11 deferred context.



------------------
GeometricPrimitive
------------------

This is a helper for drawing simple geometric shapes:

    - Cube (aka hexahedron)
    - Sphere
    - Geodesic Sphere
    - Cylinder
    - Cone
    - Torus
    - Tetrahedron
    - Octahedron
    - Dodecahedron
    - Icosahedron
    - Teapot

Initialization:

    std::unique_ptr<GeometricPrimitive> shape(GeometricPrimitive::CreateTeapot(deviceContext));

Simple drawing:

    shape->Draw(world, view, projection, Colors::CornflowerBlue);

The draw method accepts an optional texture parameter, wireframe flag, and a 
callback function which can be used to override the default rendering state:

    shape->Draw(world, view, projection, Colors::White, catTexture, false, [=]
    {
        deviceContext->OMSetBlendState(...);
    });

This makes use of a BasicEffect shared by all geometric primitives drawn on that device context.

Advanced drawing:

    IEffect* myeffect = ...

    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    shape->CreateInputLayout( myeffect, &inputLayout );

    shape->Draw( myeffect, inputLayout.Get() );

Coordinate Systems:

    These geometric primitives (based on the XNA Game Studio conventions) use right-handed
    coordinates. They can be used with left-handed coordinates by setting the rhcoords
    parameter on the factory methods to 'false' to reverse the winding ordering (the
    parameter defaults to 'true').

Alpha blending:

    Alpha blending defaults to using premultiplied alpha. To make use of 'straight' alpha
    textures, override the blending mode via the optional callback:

    CommonStates states(device);
    
    shape->Draw(world, view, projection, Colors::White, catTexture, false, [=]
    {
        deviceContext->OMSetBlendState( states.NonPremultiplied(), nullptr, 0xFFFFFFFF);
    });



-----
Model
-----

This is a class hierarchy for drawing simple meshes with support for loading rigid models from
Visual Studio 3D Starter Kit .CMO files, legacy DirectX SDK .SDKMESH files, and .VBO files.  It
is an implementation of a mesh renderer similar to the XNA Game Studio Model, ModelMesh, ModelMeshPart design.

NOTE: Support for loading keyframe animations is not yet included.

A Model consists of one or more ModelMesh instances. The ModelMesh instances can be shared by multiple
instances of Model. A ModelMesh instance consists of one or more ModelMeshPart instances.

Each ModelMeshPart references an index buffer, a vertex buffer, an input layout, an Effects instance,
and includes various metadata for drawing the geometry. Each ModelMeshPart represents a single material
to be drawn at the same time (i.e. a submesh).

Initialization:

    Model instances can be loaded from .CMO, .SDKMESH, or .VBO files, or from custom file formats.
    The Model loaders take an EffectFactory instance to facilitate the sharing of Effects and
    textures between models. For simplicity, provided Model loaders always return built-in Effect instances.
    Any references to specific shaders in the runtime mesh files are ignored.

    NOTE: EffectFactory and DGSLEffectFactory are declared in the Effects.h header.

    Visual Studio 2012 and 2013 include a built-in content pipeline that can generate .CMO files from an
    Autodesk FBX, as well as DDS texture files from various bitmap image formats, as part of the
    build process. See the Visual Studio 3D Starter Kit for details.
    http://code.msdn.microsoft.com/windowsapps/Visual-Studio-3D-Starter-455a15f1

    EffectFactory fx( device );

    auto teapot = Model::CreateFromCMO( device, L"teapot.cmo", fx );

    The legacy DirectX SDK has an exporter that will generate .SDKMESH files from an Autodesk FBX. The latest
    version of this exporter tool can be obtained from:
    http://go.microsoft.com/fwlink/?LinkId=226208

    auto tiny = Model::CreateFromSDKMESH( device, L"tiny.sdkmesh", fx );

    The VBO file format is a very simple geometry format containing a index buffer and a vertex buffer. It
    was originally introduced in the Windows 8.0 ResourceLoading sample, but can be generated by
    DirectXMesh's meshconvert utility:
    http://go.microsoft.com/fwlink/?LinkID=324981

    auto ship = Model::CreateFromVBO( device, L"ship.vbo" );

    A Model instance also contains a name (a wide-character string) for tracking and application logic. Model
    can be copied to create a new Model instance which will have shared references to the same set of ModelMesh
    instances (i.e. a 'shallow' copy).

Simple drawing:

    The Model::Draw functions provides a high-level, easy to use method for drawing models.

    CommonStates states(device);

    XMMATRIX local = XMMatrixTranslation( 1.f, 1.f, 1.f );
    local = XMMatrixMultiply( world, local );
    tiny->Draw( context, states, local, view, projection );

    There are optional parameters for rendering in wireframe and to provide a custom state override callback.

Advanced drawing:

    Rather than using the standard Model::Draw, the ModelMesh::Draw method can be used on each mesh in turn
    listed in the Model::meshes collection. ModelMesh::Draw can be used to draw all the opaque parts or the
    alpha parts individually. The ModelMesh::PrepareForRendering method can be used as a helper to setup
    common render state, or the developer can set up the state directly before calling ModelMesh::Draw.

    More detailed control over rendering can be had by skipping the use of Model::Draw and ModelMesh::Draw
    in favor of the ModelMeshPart::Draw method. Each Model::meshes collection can be scanned for each
    ModelMesh::meshParts collection to enumerate all ModelMeshPart instances. For this version of draw,
    the ModelMeshPart::effect and ModelMeshPart::inputLayout can be used, or a custom effect override can
    be used instead (be sure to create the appropriate matching input layout for the custom effect beforehand
    using ModelMeshPart::CreateInputLayout).

Effects control:

    The Model loaders create an appropriate Effects instance for each ModelMeshPart in a mesh. Generally all
    effects in a mesh should use the same lighting and fog settings, which is facilitated by the
    Model::UpdateEffects method. This calls back for each unique effect in the ModelMesh once.

    tiny->UpdateEffects([&](IEffect* effect)
    {
        auto lights = dynamic_cast<IEffectLights*>(effect);
        if ( lights )
        {
            XMVECTOR dir = XMVector3Rotate( g_XMOne, quat );
            lights->SetLightDirection( 0, dir );
        }
        auto fog = dynamic_cast<IEffectFog*>(effect);
        if ( fog )
        {
            fog->SetFogEnabled(true);
            fog->SetFogStart(6); // assuming RH coordinates
            fog->SetFogEnd(8);
            fog->SetFogColor(Colors::CornflowerBlue);
        }
    });

    It is also possible to change the Effects instance used by a given part (such as when overriding the
    default effect put in place from a Model loader) by calling ModelMeshPart::ModifyEffect. This will
    regenerate the ModelMeshPart::inputLayout appropriately.

    Be sure to call Model::Modified on all Model instances that reference the impacted ModelMesh instance
    to ensure the cache used by UpdateEffects is correctly updated. Model::Modified should also be called
    whenever a Model::meshes or ModelMesh::meshParts collection is modified.

    As noted above, it is also possible to render part or all of a model using a custom effect as an
    override, rather than changing the effect referenced by the ModelMeshPart::effect directly.

Alpha blending:

    Proper drawing of alpha-blended models can be a complicated procedure. Each ModelMeshPart has a bool value
    to indicate if the associated part is fully opaque (isAlpha is false), or has some level of alpha transparency
    (isAlpha is true). The Model::Draw routine handles some basic logic for the rendering, first rendering the
    opaque parts, then rendering the alpha parts.  More detailed control is provided by the ModelMesh::Draw method
    which can be used to draw all opaque parts of all meshes first, then go back and draw all alpha parts of
    all meshes second.

    To indicate the use of ‘straight’ alpha vs. ‘premultiplied’ alpha blending modes, ModelMesh::pmalpha is set by
    the various loaders functions controlled by a default parameter (which defaults false to indicate the
    texture files are using 'straight' alpha). If you make use of DirectXTex's texconv tool with the -pmalpha 
    switch, you should use pmalpha=true instead.

Custom render states:

    All the various Draw method provide a setCustomState callback which can be used to change the state just before the
    geometry is drawn.
 
    tiny->Draw( context, states, local, view, projection, false, [&]()
    {
        ID3D11ShaderResourceView* srv = nullptr;
        context->PSSetShaderResources( 0, 1, &srv );
    });

Coordinate systems:

    Meshes are authored in a specific winding order, typically using the standard counter-clockwise winding common in graphics.
    The choice of viewing handedness (right-handed vs. left-handed coordinates) is largely a matter of preference and
    convenience, but it impacts how the models are built and/or exported.

    The Visual Studio 3D Starter Kit’s .CMO files assume the developer is using right-handed coordinates. DirectXTK’s default
    parameters assume you are using right-handed coordinates as well, so the ‘ccw’ parameter defaults to true. If using a .CMO
    with left-handed coordinates, you should pass false for the ‘ccw’ parameter which will use clockwise winding instead.
    This makes the geometry visible, but could make textures on the model appear ‘flipped’ in U.
 
    The legacy DirectX SDK’s .SDKMESH files assume the developer is using left-handed coordinates. DirectXTK’s default parameters
    assume you are using right-handed coordinates, so the ‘ccw’ parameter defaults to false which will use clockwise winding
    and potentially have the ‘flipped in U’ texture problem. If using a .SDKMESH with left-handed coordinates, you should pass
    true for the ‘ccw’ parameter.

Feature Level Notes:

    If any ModelMeshPart makes use of 32-bit indices (i.e. ModelMeshPart:: indexFormat equals DXGI_FORMAT_R32_UINT) rather than
    16-bit indices (DXGI_FORMAT_R16_UINT), then that model requires Feature Level 9.2 or greater.

    If any ModelMeshPart uses adjacency (i.e. ModelMeshPart::primitiveType equals D3D_PRIMITIVE_TOPOLOGY_*_ADJ), then that model
    requires Feature Level 10.0 or greater. If using tessellation (i.e. D3D_PRIMITIVE_TOPOLOGY_?_CONTROL_POINT_PATCHLIST), then
    that model requires Feature Level 11.0 or greater.

    Keep in mind that there are maximum primitive count limits per ModelMeshPart based on feature level as well (65535 for
    Feature Level 9.1, 1048575 or Feature Level 9.2 and 9.3, and 4294967295 for Feature Level 10.0 or greater).

Threading model:

    The ModelMeshPart is tied to a device, but not a device context. This means that Model creation/loading is ‘free threaded’.
    Drawing can be done on the immediate context or by a deferred context, but keep in mind device contexts are not ‘free threaded’.



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



--------------
DirectXHelpers
--------------

C++ helpers for writing D3D code.

Exception-safe Direct3D 11 resource locking:

    Modern C++ development strongly encourages use of the RAII pattern for exception-safe resource management, ensuring that resources
    are properly cleaned up if C++ exceptions are thrown. Even without exception handling, it's generally cleaner code to use RAII and
    rely on the C++ scope variables rules to handle cleanup. Most of these cases are handled by the STL classes such as std::unique_ptr,
    std::shared_ptr, etc. and the recommended COM smart pointer Microsoft::WRL::ComPtr for Direct3D reference counted objects. One case
    that isn't so easily managed with existing classes is when you are mapping staging/dynamic Direct3D 11 resources. MapGuard solves
    this problem, and is modeled after std::lock_mutex.

    MapGuard map( context.Get(), tex.Get(), 0, D3D11_MAP_WRITE, 0 );
 
    for( size_t j = 0; j < 128; ++j )
    {
        auto ptr = map.scanline(j);
        ...
    }

Debug object naming:

   To help track down resource leaks, the Direct3D 11 debug layer allows you to provide ASCII debug names to Direct3D 11 objects. This
   is done with a specific GUID and the SetPrivateData method. Since you typically want to exclude this for release builds, it can get
   somewhat messy to add these to code. The SetDebugObjectName template greatly simplifies this for static debug name strings.

   SetDebugObjectName( tex.Get(), "MyTexture" );



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
    - VertexPositionNormalTangentColorTexture
    - VertexPositionNormalTangentColorTextureSkinning

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

If a Direct3D 11 context is provided and a shader resource view is requested (i.e.
textureView is non-null), then the texture will be created to support auto-generated
mipmaps and will have GenerateMips called on it before returning. If no context is
provided (i.e. d3dContext is null) or the pixel format is unsupported for auto-gen
mips by the current device, then the resulting texture will have only a single level.

DDSTextureLoader will load BGR 5:6:5 and BGRA 5:5:5:1 DDS files, but these formats will
fail to create on a system with DirectX 11.0 Runtime. The DXGI 1.2 version of
DDSTextureLoader will load BGRA 4:4:4:4 DDS files using DXGI_FORMAT_B4G4R4A4_UNORM.
The DirectX 11.1 Runtime, DXGI 1.2 headers, and WDDM 1.2 drivers are required to
support 16bpp pixel formats on all feature levels.

Threading model:

    Resource creation with Direct3D 11 is thread-safe. CreateDDSTextureFromFile blocks
    the calling thread for reading the file data. CreateDDSTextureFromMemory can be used
    to implement asynchronous loading.

Further reading:

    http://go.microsoft.com/fwlink/?LinkId=248926
    http://blogs.msdn.com/b/chuckw/archive/2011/10/28/directxtex.aspx
    http://blogs.msdn.com/b/chuckw/archive/2010/02/05/the-dds-file-format-lives.aspx



----------------
WICTextureLoader
----------------

WICTextureLoader.h contains a loader for BMP, JPEG, PNG, TIFF, GIF, HD Photo, and
other WIC-supported image formats. This performs any required pixel format conversions
or image resizing using WIC at load time as well.

NOTE: WICTextureLoader is not supported on Windows phone 8.0 because WIC is not 
available on that platform. It is available on Windows phone starting in version 8.1.

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
cubemaps. For these scenarios, use the .dds file format and DDSTextureLoader instead.

The DXGI 1.2 version of WICTextureLoader will load 16bpp pixel images as 5:6:5 or
5:5:5:1 rather than expand them to 32bpp RGBA. The DirectX 11.1 Runtime, DXGI 1.2
headers, and WDDM 1.2 drivers are required to support 16bpp pixel formats on all
feature levels.

Threading model:

    Resource creation with Direct3D 11 is thread-safe. CreateWICTextureFromFile blocks
    the calling thread for reading the file data. CreateWICTextureFromMemory can be used
    to implement asynchronous loading. Any use of either function with a
    ID3D11DeviceContext to support auto-gen of mipmaps is not thread-safe.

Further reading:

    http://go.microsoft.com/fwlink/?LinkId=248926
    http://blogs.msdn.com/b/chuckw/archive/2011/10/28/directxtex.aspx



----------
ScreenGrab
----------

ScreenGrab.h contains routines for writing out a texture, usually a render-target,
to either a .dds file or a WIC-supported bitmap file (BMP, JPEG, PNG, TIFF, etc.).

SaveDDSTextureToFile and SaveWICTextureToFile will save a texture to a file,
which is a 'screen shot' when used with a render target texture. Only 2D
textures are supported, and the routines will fail for 1D and 3D
textures (aka volume maps). For 2D array textures and cubemaps, only the
first image is written to disk. Mipmap levels are ignored by both routines.
MSAA textures are resolved before being written.

NOTE: For a complete DDS texture dump routine that supports all dimensions,
arrays, cubemaps, and mipmaps use the 'DirectXTex' library.

SaveDDSTextureToFile will store the data in the format of the original
resource (i.e. performs no conversions), but will prefer using legacy
.dds file headers when possible over the newer 'DX10' header extension for
better tools support. The DXGI 1.2 version supports writing 16bpp pixel
formats.

SaveWICTextureToFile will convert the pixel data if needed, and prefers to
use a non-alpha format (alpha channels in render targets can result in some
strange looking screenshot files). The caller can also provide a specific
pixel target format GUID to use as well. The caller provides the GUID of the
specific file container format to use.

NOTE: SaveWICTextureToFile is not supported on Windows phone 8.0 because WIC is not 
available on that platform. It is available on Windows phone starting in version 8.1.

Capturing a screenshot:

    WRL::ComPtr<ID3D11Texture2D> backBuffer;
    hr = pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&backBuffer );
    if ( SUCCEEDED(hr) )
    {
        hr = SaveWICTextureToFile( pContext, backBuffer, GUID_ContainerFormatBmp, L"SCREENSHOT.BMP" ) );
    }

Threading model:

    Since these functions use ID3D11DeviceContext, they are not thread-safe.

Further reading:

    http://go.microsoft.com/fwlink/?LinkId=248926
    http://blogs.msdn.com/b/chuckw/archive/2010/02/05/the-dds-file-format-lives.aspx



----------
SimpleMath
----------

SimpleMath.h wraps the DirectXMath SIMD vector/matrix math API with an easier 
to use C++ interface. It provides the following types, with similar names, 
methods, and operator overloads to the XNA Game Studio math API:

    - Vector2
    - Vector3
    - Vector4
    - Matrix
    - Quaternion
    - Plane
    - Ray
    - Color

Why wrap DirectXMath?

DirectXMath provides highly optimized vector and matrix math functions, which 
take advantage of SSE SIMD intrinsics when compiled for x86/x64, or the ARM 
NEON instruction set when compiled for an ARM platform such as Windows RT or 
Windows phone. The downside of being designed for efficient SIMD usage is that 
DirectXMath can be somewhat complicated to work with. Developers must be aware 
of correct type usage (understanding the difference between SIMD register types 
such as XMVECTOR vs. memory storage types such as XMFLOAT4), must take care to 
maintain correct alignment for SIMD heap allocations, and must carefully 
structure their code to avoid accessing individual components from a SIMD 
register. This complexity is necessary for optimal SIMD performance, but 
sometimes you just want to get stuff working without so much hassle!

Enter SimpleMath...

These types derive from the equivalent DirectXMath memory storage types (for 
instance Vector3 is derived from XMFLOAT3), so they can be stored in arbitrary 
locations without worrying about SIMD alignment, and individual components can 
be accessed without bothering to call SIMD accessor functions. But unlike 
XMFLOAT3, the Vector3 type defines a rich set of methods and overloaded 
operators, so it can be directly manipulated without having to first load its 
value into an XMVECTOR. Vector3 also defines an operator for automatic 
conversion to XMVECTOR, so it can be passed directly to methods that were 
written to use the lower level DirectXMath types.

If that sounds horribly confusing, the short version is that the SimpleMath 
types pretty much "Just Work" the way you would expect them to.

By now you must be wondering, where is the catch? And of course there is one. 
SimpleMath hides the complexities of SIMD programming by automatically 
converting back and forth between memory and SIMD register types, which tends 
to generate additional load and store instructions. This can add significant 
overhead compared to the lower level DirectXMath approach, where SIMD loads and 
stores are under explicit control of the programmer.

You should use SimpleMath if you are:

    - Looking for a C++ math library with similar API to the C# XNA types
    - Porting existing XNA code from C# to C++
    - Wanting to optimize for programmer efficiency (simplicity, readability, 
      development speed) at the expense of runtime efficiency

You should go straight to the underlying DirectXMath API if you:

    - Want to create the fastest possible code
    - Enjoy the lateral thinking needed to express algorithms in raw SIMD

This need not be a global either/or decision. The SimpleMath types know how to 
convert themselves to and from the corresponding DirectXMath types, so it is 
easy to mix and match. You can use SimpleMath for the parts of your program 
where readability and development time matter most, then drop down to 
DirectXMath for performance hotspots where runtime efficiency is more important.



-------------------
GamePad
-------------------

This is a helper for simplified access to gamepad controllers through the XInput API modeled after the XNA C# GamePad class.

    GamePad is supported on Windows and Xbox One. On Windows Phone 8.x, you can create and query the GamePad class, but it
    will always report no devices are connected since gamepads are not currently supported by the Windows Phone platform.

Initialization:

    GamePad is a singleton.

    std::unique_ptr<GamePad> gamePad( new GamePad );

Basic use:

    GetState queries the controller status given a player index. If connected, it returns the status of the buttons (A, B,
    X, Y, left & right stick, left & right shoulder, back, and start), the directional pad (DPAD), the left & right thumb
    sticks, and the left & right triggers.

    auto state = gamePad->GetState( 0 );

    if ( state.IsConnected() )
    {
        if ( state.IsAPressed() )
            // Do action for button A being down

        if ( state.buttons.y )
            // Do action for button Y being down

        if ( state.IsDPadLeftPressed() )
            // Do action for DPAD Left being down

        if ( state.dpad.up || state.dpad.down || state.dpad.left || state.dpad.right )
            // Do action based on any DPAD change

        float posx = state.thumbSticks.leftX;
        float posy = state.thumbSticks.leftY;
            // These values are normalized to -1 to 1

        float throttle = state.triggers.right;
            // This value is normalized 0 -> 1

        if ( state.IsLeftTriggerPressed() )
            // Do action based on a left trigger pressed more than halfway

        if ( state.IsViewPressed() )
            // This is an alias for the Xbox 360 'Back' button
            // which is called 'View' on the Xbox One Controller. 
    }

Vibration:

    Many controllers include vibration motors to provide force-feedback to the user, which can be controlled with
    SetVibration and the player index. The motor values range from 0 to 1.

    if ( gamePad->SetVibration( 0, 0.5f, 0.25f ) )
        // If true, the vibration was successfully set.

    Note: The trigger impulse motors on the Xbox One Controller are not accessible via XInput on Windows, and these
          motors are not present on the Xbox 360 Common Controller.

Device capabilities:

    The GamePad class provides a simplified model for the device capabilties.

    auto caps = gamePad->GetCapabilities( 0 );
    if ( caps.IsConnected() )
    {
        if ( caps.gamepadType == GamePad::Capabilities::FLIGHT_STICK )
            // Use specific controller layout based on a flight stick controller
        else
            // Default to treating any unknown type as a standard gamepad
    }

Button state tracker:

    A common pattern for gamepads is to trigger an event when a button is pressed or released, but that
    you don't want to trigger the event every single frame if the button is held down for more than a
    single frame. This helper class simplifies this.

    std::unique_ptr<GamePad::ButtonStateTracker> tracker( new  GamePad::ButtonStateTracker );

    ...

    auto state = gamdPad->GetState( 0 );
    if ( state.IsConnected() )
    {
        tracker->Update( state );

        if ( tracker->a == GamePad::ButtonStateTracker::PRESSED )
            // Take an action when Button A is first pressed, but don't do it again until
            // the button is released and then pressed again
    }

    Each button is reported by the tracker with a state UP, HELD, PRESSED, or RELEASED.

    When resuming from a pause or suspend, be sure to call Reset() on the tracker object to clear the state history.

Threading model:

    The GamePad class provides no special synchronziation above the underlying API. XInput on Windows is thread-safe
    through a internal global lock, so performance is best when only a single thread accesses the controller.

Further reading:

    http://blogs.msdn.com/b/chuckw/archive/2012/04/26/xinput-and-windows-8-consumer-preview.aspx



-------------------
DirectXTK for Audio
-------------------

The DirectXTK for Audio components implement a low-level audio API similar to 
XNA Game Studio's Microsoft.Xna.Framework.Audio. This consists of the following
classes all declared in the Audio.h header (in the Inc folder of the distribution):

    AudioEngine - This class represents an XAudio2 audio graph, device, and mastering voice
    SoundEffect - A container class for sound resources which can be loaded from .wav files
    SoundEffectInstance - Provides a single playing, paused, or stopped instance of a sound
    DynamicSoundEffectInstance - SoundEffectInstance where the application provides the audio data on demand
    WaveBank - A container class for sound resources packaged into an XACT-style .xwb wave bank
    AudioListener, AudioEmitter - Utility classes used with SoundEffectInstance::Apply3D

Note: DirectXTK for Audio uses XAudio 2.8 or XAudio 2.7. It does not make use of the legacy XACT Engine, XACT Cue, or XACT SoundBank.

During initialization:

    The first step in using DirectXTK for Audio is to create the AudioEngine, which creates an XAudio2 interface, an XAudio2 mastering voice,
    and other global resources.

    // This is only needed in Win32 desktop apps
    CoInitializeEx( nullptr, COINIT_MULTITHREADED );

    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
    #ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
    #endif
    std::unique_ptr<AudioEngine> audEngine( new AudioEngine( eflags ) );

Per-frame processing:

    The application should call Update() every frame to allow for per-frame engine updates,
    such as one-shot voice management. This could also be done in a worker thread rather than on the
    main rendering thread.

    if ( !audEngine->Update() )
    {
        // No audio device is active
        if ( audEngine->IsCriticalError() )
        {
            ...
        }    
    }

    Update() returns false if no audio is actually playing (either due to there being no
    audio device on the system at the time AudioEngine was created, or because XAudio2 encountered a
    Critical Error--typically due to speakers being unplugged). Calls to various DirectXTK for Audio
    methods can still be made in this state but no actual audio processing will take place.

Loading and a playing a looping sound:

    Creating SoundEffectInstances allows full control over the playback, and are provided with a
    dedicated XAudio2 source voice. This allows control of playback, looping, volume control,
    panning, and pitch-shifting.

    std::unique_ptr<SoundEffect> soundEffect( new SoundEffect( audEngine.get(), L"Sound.wav" ) );
    auto effect = soundEffect->CreateInstance();

    ...

    effect->Play( true );

Playing one-shots:

    A common way to play sounds is to trigger them in a 'fire-and-forget' mode. This is done by calling
    SoundEffect::Play() rather than creating a SoundEffectInstance. These use XAudio2 source voices
    managed by AudioEngine, are cleaned up automatically when they finish playing, and can overlap in time.
    One-shot sounds cannot be looped or have 3D positional effects.

    std::unique_ptr<SoundEffect> soundEffect( new SoundEffect( audEngine.get(), L"Explosion.wav" ) );

    soundEffect->Play();

    ...

    soundEffect->Play();

Applying 3D audio effects to a sound:

    DirectXTK for Audio supports 3D positional audio with optional environmental reverb effects using X3DAudio.

    AUDIO_ENGINE_FLAGS eflags =  AudioEngine_EnvironmentalReverb | AudioEngine_ReverbUseFilters;
    #ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
    #endif
    std::unique_ptr<AudioEngine> audEngine( new AudioEngine( eflags ) );

    audEngine->SetReverb( Reverb_ConcertHall );

    std::unique_ptr<SoundEffect> soundEffect( new SoundEffect( audEngine.get(), L"Sound.wav" ) );
    auto effect = soundEffect->CreateInstance( SoundEffectInstance_Use3D | SoundEffectInstance_ReverbUseFilters );

    ...

    effect->Play(true);

    ...

    AudioListener listener;
    listener.SetPosition( ... );

    AudioEmitter emitter;
    emitter.SetPosition( ... );

    effect->Apply3D( listener, emitter );

    Note: A C++ exception is thrown if you call Apply3D for a SoundEffectInstance that was not created with SoundEffectInstance_Use3D

Using wave banks:

    Rather than loading individual .wav files, a more efficient method is to package them into a 
    "wave bank". This allows for more efficient loading and memory organization. DirectXTK for Audio's
    WaveBank class can be used to play one-shots or to create SoundEffectInstances from 'in-memory' wave banks.

    std::unique_ptr<WaveBank> wb( new WaveBank( audEngine.get(), L"wavebank.xwb" ) );

    A SoundEffectInstance can be created from a wavebank referencing a particular wave in the bank:

    auto effect = wb->CreateInstance( 10 );
    if ( !effect )
        // Error (invalid index for wave bank)

    ...

    effect->Play( true );

    One-shot sounds can also be played directly from the wave bank.

    wb->Play( 2 );
    wb->Play( 6 );

    XACT3-style "wave banks" can be created by using the XWBTool command-line tool, or they can be authored
    using XACT3 in the DirectX SDK. Note that the XWBTool will not perform any format conversions
    or compression, so more full-featured options are better handled with the XACT3 GUI or XACTBLD, or it can
    be used on .wav files already compressed by adpcmencode.exe, xwmaencode.exe, xma2encode.exe, etc.

    xwbtool -o wavebank.xwb Sound.wav Explosion.wav Music.wav

    DirectXTK for Audio does not make use of the XACT engine, nor does it make use of XACT "sound banks" .xsb
    or "cues". We only use .xwb wave banks as a method for packing .wav data.

Command-line options for the XWBTool:

    -s
        Creates as streaming wave bank, otherwise defaults to in-memory wave bank

    -o <filename>
        Sets output filename for .xwb file. Otherwise, it defaults to the same base name as the first input .wav file

    -h <h-filename>
        Generates a C/C++ header file with #defines for each of the sounds in the bank matched to their index

    -n
        Disables the default warning of overwriting an existing .xwb file

    -c / -nc
        Forces creation or prevents use of compact wave banks. By default, it will try to use a compact wave bank if possible.

    -f
        Includes entry friendly name strings in the wave bank for use with 'string' based versions of WaveBank::Play() and
        WaveBank::CreateInstance() rather than index-based versions.

Voice management

   Each instance of a SoundEffectInstance will allocate it's own source voice when played, which won't be released until it is
   destroyed. Each time a one-shot sound is played from a SoundEffect or a WaveBank, a voice will be created or a previously used
   one-shot voice will be reused if possible.

   SetDefaultSampleRate() is used to control the sample rate for voices in the one-shot pool. This should be the same rate
   as used by the majority of your content. It defaults to 44100 Hz.

   TrimVoicePool() can be used to free up any source voices in the 'idle' list for one-shots, and will cause all non-playing
   SoundEffectInstance objects to release their source voice. This is used for keeping the total source voice count under
   a limit for performance or when switching sections where audio content formats change dramatically.

   SetMaxVoicePool() can be used to set a limit on the number of one-shot source voices allocated and/or to put a limit on the
   source voices available for SoundEffectInstance. If there are insufficient one-shot voices, those sounds will not be heard.
   If there are insufficient voices for a SoundEffectInstance to play, then a C++ exception is thrown. These values default to
   'unlimited'.

Platform support:

    Windows 8.x, Windows Store apps, Windows phone 8.x, and Xbox One all include XAudio 2.8. Therefore, the
    standard DirectXTK.lib includes DirectXTK for Audio for all these platforms.

        * DirectXTK_Windows81
        * DirectXTK_Windows8
        * DirectXTK_WindowsPhone81
        * DirectXTK_XAMLSilverlight_WindowsPhone81
        * DirectXTK_WindowsPhone8
        * DirectXTK_XboxOneXDK
        * DirectXTK_XboxOneADK

    For Win32 desktop applications targeting Windows 8.x or later, you can make use of XAudio 2.8. The
    DirectXTKAudioWin8.lib contains the XAudio 2.8 version of DirectXTK for Audio, while DirectXTK.lib
    for Win32 desktop contains only the math/graphics components. To support Win32 desktop applications
    on Windows 7 and Windows Vista, we must make use XAudio 2.7, the legacy DirectX SDK, and the legacy
    DirectX End-User Runtime Redistribution packages (aka DirectSetup). The DirectXTKAudioDX.lib is the
    XAudio 2.7 version of DirectXTK for Audio.

        * DirectXTK_Desktop_201x + Audio\DirectXTKAudio_Desktop_201x_Win8
        * DirectXTK_Desktop_201x + Audio\DirectXTKAudio_Desktop_201x_DXSDK

    VS 2010 Note: We only support DirectXTK for Audio with the legacy DirectX SDK due to some issues with
    using the VS 2010 toolset and the Windows 8.x SDK WinRT headers.

    http://msdn.microsoft.com/en-us/library/windows/desktop/ee415802.aspx
    http://support.microsoft.com/kb/2728613
    http://msdn.microsoft.com/en-us/library/windows/desktop/ee663275.aspx

    DirectXTK for Audio supports wave content in PCM and a variant of MS-ADPCM formats. When built for Xbox One
    or using XAudio 2.7, it also supports xWMA. XMA2 is supported by the Xbox One XDK version only.

    DirectXTK makes use of the latest Direct3D 11.1 headers available in the Windows 8.x SDK, and there are a
    number of file conflicts between the Windows 8.x SDK and the legacy DirectX SDK. Therefore, when building
    for down-level support with XAudio 2.7, Audio.h explicitly includes the DirectX SDK version of XAudio2 headers
    with a full path name. These reflect the default install locations, and if you have installed it elsewhere you
    will need to update this header. The *_DXSDK.vcxproj files use the DXSDK_DIR environment variable, so only the
    Audio.h references need updating for an alternative location.
    
Threading model:

    The DirectXTK for Audio methods assume it is always called from a single thread. This is generally
    either the main thread or a worker thread dedicated to audio processing.  The XAudio2 engine itself
    makes use of lock-free mechanism to make it 'thread-safe'.
    
    Note that IVoiceNotify::OnBufferEnd is called from XAudio2's thread, so the callback must be very
    fast and use thread-safe operations.

Further reading:

    http://blogs.msdn.com/b/chuckw/archive/2012/05/15/learning-xaudio2.aspx
    http://blogs.msdn.com/b/chuckw/archive/2012/04/02/xaudio2-and-windows-8-consumer-preview.aspx



---------------
RELEASE HISTORY
---------------

March 27, 2015
    Added projects for Windows apps Technical Preview
    - GamePad temporarily uses 'null' device for universal Windows applicaton platform

February 25, 2015
    DirectXTK for Audio updates
    - *breaking change* pitch now defined as -1 to 1 with 0 as the default
    - One-shot Play method with volume, pitch, and pan
    - GetMasterVolume/SetMasterVolume method for AudioEngine
    - Fix for compact wavebank validation
    - Improved voice cleanup and shutdown
    Minor code cleanup and C++11 =default/=delete usage

January 26, 2015
    GamePad class: emulate XInputEnable behavior for XInput 9.1.0
    DirectXTK for Audio fix for Stop followed by Play doing a proper restart
    DirectXTK for Audio fix when using XAudio 2.7 on a system with no audio device
    Updates for Xbox One platform support
    Minor code cleanup and C99 printf string conformance

November 24, 2014
    SimpleMath fix for Matrix operator !=
    DirectXTK for Audio workaround for XAudio 2.7 on Windows 7 problem
    Updates for Windows phone 8.1 platform support
    Updates for Visual Studio 2015 Technical Preview
    Minor code cleanup

October 28, 2014
    Model support for loading from VBO files
    Model render now sets samplers on slots 0,1 by default for dual-texture effects
    Updates for Xbox One platform support
    Minor code cleanup

September 5, 2014
    GamePad class: gamepad controller helper using XInput on Windows, IGamepad for Xbox One
    SimpleMath updates; Matrix billboard methods; breaking change: Matrix::Identity() -> Matrix::Identity
    SpriteBatch new optional SetViewport method
    SpriteFont fix for white-space character rendering optimization
    DDSTextureLoader fix for auto-gen mipmaps for volume textures
    Explicit calling-convention annotation for public headers
    Updates for Xbox One platform support
    Minor code and project cleanup

July 15, 2014
    DirectXTK for Audio and XWBTool fixes
    Updates to Xbox One platform support

April 3, 2014
    Windows phone 8.1 platform support

February 24, 2014
    DirectXHelper: new utility header with MapGuard and public version of SetDebugObjectName template
    DDSTextureLoader: Optional support for auto-gen mipmaps
    DDSTextureLoader/ScreenGrab: support for Direct3D 11 video formats including legacy "YUY2" DDS files
    GeometricPrimtive: Handedness fix for tetrahedron, octahedron, dodecahedron, and icosahedron
    SpriteBatch::SetRotation(DXGI_MODE_ROTATION_UNSPECIFIED) to disable viewport matrix
    XboxDDSTextureLoader: optional forceSRGB parameter

January 24, 2014
    DirectXTK for Audio updated with voice management and optional mastering volume limiter
    Added orientation rotation support to SpriteBatch
    Fixed a resource leak with GetDefaultTexture() used by some Effects
    Code cleanup (removed DXGI_1_2_FORMATS control define; d2d1.h workaround not needed; ScopedObject typedef removed)

December 24, 2013
    DirectXTK for Audio
    Xbox One platform support
    MakeSpriteFont tool updated with more progress feedback when capturing large fonts
    Minor updates for .SDKMESH Model loader
    Fixed bug in .CMO Model loader when handling multiple textures
    Improved debugging output

October 28, 2013
    Updated for Visual Studio 2013 and Windows 8.1 SDK RTM
    Added DGSLEffect, DGSLEffectFactory, VertexPositionNormalTangentColorTexture, and VertexPositionNormalTangentColorTextureSkinning
    Model loading and effect factories support loading skinned models
    MakeSpriteFont now has a smooth vs. sharp antialiasing option: /sharp
    Model loading from CMOs now handles UV transforms for texture coordinates
    A number of small fixes for EffectFactory
    Minor code and project cleanup
    Added NO_D3D11_DEBUG_NAME compilation define to control population of Direct3D debug layer names for debug builds

July 1, 2013
    VS 2013 Preview projects added and updates for DirectXMath 3.05 __vectorcall
    Added use of sRGB WIC metadata for JPEG, PNG, and TIFF
    SaveToWIC functions updated with new optional setCustomProps parameter and error check with optional targetFormat

May 30, 2013
    Added more GeometricPrimitives: Cone, Tetrahedron, Octahedron, Dodecahedron, Icosahedron
    Updated to support loading new metadata from DDS files (if present)
    Fixed bug with loading of WIC 32bpp RGBE format images
    Fixed bug when skipping mipmaps in a 1D or 2D array texture DDS file

February 22, 2013
    Added SimpleMath header
    Fixed bug that prevented properly overriding EffectFactory::CreateTexture
    Fixed forceSRGB logic in DDSTextureLoader and WICTextureLoader
    Break circular reference chains when using SpriteBatch with a setCustomShaders lambda
    Updated projects with /fp:fast for all configs, /arch:SSE2 for Win32 configs
    Sensibly named .pdb output files
    Added WIC_USE_FACTORY_PROXY build option (uses WindowsCodecs.dll entrypoint rather than CoCreateInstance)

January 25, 2013
    GeometricPrimitive support for left-handed coordinates and drawing with custom effects 
    Model, ModelMesh, and ModelMeshPart added with loading of rigid non-animating models from .CMO and .SDKMESH files
    EffectFactory helper class added

December 11, 2012
    Ex versions of DDSTextureLoader and WICTextureLoader
    Removed use of ATL's CComPtr in favor of WRL's ComPtr for all platforms to support VS Express editions
    Updated VS 2010 project for official 'property sheet' integration for Windows 8.0 SDK
    Minor fix to CommonStates for Feature Level 9.1
    Tweaked AlphaTestEffect.cpp to work around ARM NEON compiler codegen bug
    Added dxguid.lib as a default library for Debug builds to resolve GUID link issues

November 15, 2012
    Added support for WIC2 when available on Windows 8 and Windows 7 with KB 2670838
    Cleaned up warning level 4 warnings

October 30, 2012
    Added project files for Windows phone 8

October 12, 2012
    Added PrimitiveBatch for drawing user primitives
    Debug object names for all D3D resources (for PIX and debug layer leak reporting)

October 2, 2012
    Added ScreenGrab module
    Added CreateGeoSphere for drawing a geodesic sphere
    Put DDSTextureLoader and WICTextureLoader into the DirectX C++ namespace

September 7, 2012
    Renamed project files for better naming consistency
    Updated WICTextureLoader for Windows 8 96bpp floating-point formats
    Win32 desktop projects updated to use Windows Vista (0x0600) rather than Windows 7 (0x0601) APIs
    Tweaked SpriteBatch.cpp to workaround ARM NEON compiler codegen bug

May 31, 2012
    Updated Metro project for Visual Studio 2012 Release Candidate changes
    Cleaned up x64 Debug configuration warnings and switched to use "_DEBUG" instead of "DEBUG"
    Minor fix for DDSTextureLoader's retry fallback that can happen with 10level9 feature levels

May 2, 2012
    Added SpriteFont implementation and the MakeSpriteFont utility

March 29, 2012
    WICTextureLoader updated with Windows 8 WIC native pixel formats

March 6, 2012
    Fix for too much temp memory used by WICTextureLoader
    Add separate Visual Studio 11 projects for Desktop vs. Metro builds

March 5, 2012
    Bug fix for SpriteBatch with batches > 2048

February 24, 2012
    Original release
