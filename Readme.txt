-----------------------------------------------
DirectXTK - the DirectX Tool Kit for DirectX 11
-----------------------------------------------

Copyright (c) Microsoft Corporation. All rights reserved.

December 5, 2016

This package contains the "DirectX Tool Kit", a collection of helper classes for 
writing Direct3D 11 C++ code for Universal Windows Platform (UWP) apps for Windows 10,
Windows Store apps, Windows phone 8.1 applications, Xbox One exclusive apps,
Windows 8.x Win32 desktop applications, Windows 7 applications, and
Windows Vista Direct3D 11.0 applications.

This code is designed to build with Visual Studio 2013 or 2015. It is recommended that you
make use of VS 2013 Update 5 or VS 2015 Update 3 and Windows 7 Service Pack 1 or later.

These components are designed to work without requiring any content from the DirectX SDK. For details,
see "Where is the DirectX SDK?" <http://msdn.microsoft.com/en-us/library/ee663275.aspx>.

Inc\
    Public Header Files (in the DirectX C++ namespace):

    Audio.h - low-level audio API using XAudio2 (DirectXTK for Audio public header)
    CommonStates.h - factory providing commonly used D3D state objects
    DDSTextureLoader.h - light-weight DDS file texture loader
    DirectXHelpers.h - misc C++ helpers for D3D programming
    Effects.h - set of built-in shaders for common rendering tasks
    GamePad.h - gamepad controller helper using XInput
    GeometricPrimitive.h - draws basic shapes such as cubes and spheres
    GraphicsMemory.h - helper for managing dynamic graphics memory allocation
    Keyboard.h - keyboard state tracking helper
    Model.h - draws meshes loaded from .CMO, .SDKMESH, or .VBO files
    Mouse.h - mouse helper
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

Documentation is available at <https://github.com/Microsoft/DirectXTK/wiki>.

For the latest version of DirectX Tool Kit, bug reports, etc. please visit the project site.

http://go.microsoft.com/fwlink/?LinkId=248929

Note: Xbox One exclusive apps developers using the Xbox One XDK need to generate the
      Src\Shaders\Compiled\XboxOne*.inc files to build the library as they are not
      included in the distribution package. They are built by running the script
      in Src\Shaders - "CompileShaders xbox", and should be generated with the matching
      FXC compiler from the Xbox One XDK. While they will continue to work if outdated,
      a mismatch will cause runtime compilation overhead that would otherwise be avoided.

This project has adopted the Microsoft Open Source Code of Conduct. For more information see the
Code of Conduct FAQ or contact opencode@microsoft.com with any additional questions or comments.

https://opensource.microsoft.com/codeofconduct/


---------------
RELEASE HISTORY
---------------

December 5, 2016
    Mouse and Keyboard classes updated with IsConnected method
    Windows10 project /ZW switch removed to support use in C++/WinRT projection apps
    VS 2017 RC projects added
    Minor code cleanup

October 6, 2016
    SDKMESH loader and BasicEffects support for compressed vertex normals with biasing
    WICTextureLoader Ex bool forceSRGB parameter is now a WIC_LOADER flag
    Minor code cleanup

September 15, 2016
    Minor code cleanup
    xwbtool: added wildcard support for input filename and optional -r switch for recursive search

September 1, 2016
    Added forceSRGB optional parameter to SpriteFont ctor
    EffectFactory method EnableForceSRGB added
    DGSLEffect now defaults to diffuse/alpha of 1
    Removed problematic ABI::Windows::Foundation::Rect interop for SimpleMath
    Minor code cleanup

August 4, 2016
    Regenerated shaders using Windows 10 Anniversary Update SDK (14393)

August 2, 2016
    Updated for VS 2015 Update 3 and Windows 10 SDK (14393)

August 1, 2016
    GamePad capabilities information updated for Universal Windows and Xbox One platforms
    Specular falloff lighting computation fix in shaders

July 18, 2016
    NormalMapEffect for normal-map with optional specular map rendering
    EnvironmentMapEffect now supports per-pixel lighting
    Effects updated with SetMatrices and SetColorAndAlpha methods
    SimpleMath: improved interop with DirectXMath constants
    Minor code cleanup

June 30, 2016
    MeasureDrawString added to SpriteFont; bad fix to MeasureString reverted
    GamePad tracker updated to track emulated buttons (i.e. leftStickUp)
    EffectFactory SetDirectory now checks current working directory (CWD) as well
    *breaking change* must include <d3d11.h> before including <SimpleMath.h>
    Code refactor for sharing some files with DirectX 12 version
    Minor code cleanup

May 31, 2016
    Added VertexPosition and VertexPositionDualTexture to VertexTypes
    Xbox One platform fix for PrimitiveBatch
    CompileShader script updated to build external pdbs
    Code cleanup

April 26, 2016
    Added Rectangle class to SimpleMath
    Fix for SDKMESH loader when loading models with 'extra' texture coordinate sets
    Made SimpleMath's Viewport ComputeTitleSafeArea less conservative
    Added view/menu alises to GamePad::ButtonStateTracker for Xbox One Controller naming
    Retired Windows phone 8.0 projects and obsolete adapter code
    Minor code and project file cleanup

February 23, 2016
    Fixed width computation bug in SpriteFont::MeasureString
    Fix to clean up partial or zero-length image files on failed write
    Fix to WaveBankReader for UWP platform
    Retired VS 2012 projects
    Xbox One platform updates
    Minor code and project file cleanup

January 5, 2016
    Xbox One platform updates
    *breaking change* Need to add use of GraphicsMemory class to Xbox One titles
    Minor code cleanup

November 30, 2015
    SimpleMath improvements including Viewport class
    Fixed bug with Keyboard for OpenBracket and later VK codes
    Fixed bug with Mouse that reset the scrollwheel on app activate
    MakeSpriteFont updated with /FastPack and /FeatureLevel switches
    Updated for VS 2015 Update 1 and Windows 10 SDK (10586)

October 30, 2015
    DirectXTK for Audio 3D updates
    *breaking change* emitters/listeners now use RH coordinates by default
    GeometricPrimitive support for custom geometry
    SimpleMath Matrix class improvements
    DDS support for legacy bumpmap formats (V8U8, Q8W8V8U8, V16U16)  
    Mouse fix for WinRT implementation with multiple buttons pressed
    Wireframe CommonStates no longer does backface culling
    Xbox One platform updates
    Minor code cleanup

August 18, 2015
    Xbox One platform updates

July 29, 2015
    - Added CreateBox method to GeometricPrimitive
    - Added 'invertn' optional parameter to CreateSphere
    - Updates for Keyboard, Mouse class
    - Fixed bug when loading older SDKMESH models
    - Updated for VS 2015 and Windows 10 SDK RTM
    - Retired VS 2010 and Windows Store 8.0 projects

July 1, 2015
    - Added Keyboard, Mouse class
    - Support for loading pre-lit models with SDKMESH
    - GamePad implemented using Windows.Gaming.Input for Windows 10
    - DirectXTK for Audio updates for xWMA support with XAudio 2.9
    - Added FindGlyph and GetSpriteSheet methods to SpriteFont

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
    SimpleMath updates; Matrix billboard methods; *breaking change*: Matrix::Identity() -> Matrix::Identity
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
    Updated Windows Store project for Visual Studio 2012 Release Candidate changes
    Cleaned up x64 Debug configuration warnings and switched to use "_DEBUG" instead of "DEBUG"
    Minor fix for DDSTextureLoader's retry fallback that can happen with 10level9 feature levels

May 2, 2012
    Added SpriteFont implementation and the MakeSpriteFont utility

March 29, 2012
    WICTextureLoader updated with Windows 8 WIC native pixel formats

March 6, 2012
    Fix for too much temp memory used by WICTextureLoader
    Add separate Visual Studio 11 projects for Desktop vs. Windows Store builds

March 5, 2012
    Bug fix for SpriteBatch with batches > 2048

February 24, 2012
    Original release
