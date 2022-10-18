# DirectX Tool Kit for DirectX 11

http://go.microsoft.com/fwlink/?LinkId=248929

Release available for download on [GitHub](https://github.com/microsoft/DirectXTK/releases)

## Release History

### October 17, 2022
* Additional methods for *DirectX Tool Kit for Audio* emitter and listener for cone and falloff curves
* Added use of C++11 inline namespaces to make it possible to link both DX11 and DX12 versions at once
* Minor fix for ``CompileShaders.cmd`` to address additional 'paths with spaces' issues
* Minor CMake update

### July 29, 2022
* *breaking change* DDSTextureLoader ``Ex`` functions now use ``DDS_LOADER_FLAGS`` instead of ``bool forceSRGB`` parameter.
* MapGuard helper class updated with a new ``copy`` method
* Fixed Mouse race-condition with changing mode and resetting scroll wheel at the same time.
* CMake and MSBuild project updates
* Minor code review

### June 15, 2022
* GamePad, Keyboard, and Mouse updated to use GameInput on PC for the Gaming.Desktop.x64 platform
* *DirectX Tool Kit for Audio* updated for XAudio2Redist 1.2.9
* CMake project updates

### May 9, 2022
* C++20 spaceship operator updates for SimpleMath
* Minor updates for VS 2022 (17.2)
* CMake project updates (now supports MSVC, clang/LLVM, and MinGW)
* Added Microsoft GDK projects using the Gaming.Desktop.x64 platform
* Retired VS 2017 projects
* Minor code review
* Reformat source using updated .editorconfig settings

### March 24, 2022
* Fixed bug in UWP implementation of Mouse that combined vertical/horizontal scroll-wheel input
* Code refactoring for input classes (GamePad, Keyboard, and Mouse)
* Update build switches for SDL recommendations
* CMake project updates and UWP platform CMakePresets
* Dropped support for legacy Xbox One XDK prior to April 2018

### February 28, 2022
* SimpleMath Matrix updated with ToEuler and Vector3 version of CreateFromYawPitchRoll methods
* SimpleMath Quaternion updated with ToEuler, RotateTowards, FromToRotation, LookRotation, and Angle methods
* Keyboard updated with new IME On/Off v-keys
* Win32 Mouse now uses ``WM_ACTIVATE`` for more robust behavior
* *DirectX Tool Kit for Audio* updated for Advanced Format (4Kn) wavebank streaming
* Code and project review including fixing clang v13 warnings
* Added CMakePresets.json
* xwbtool: Added support for Advanced Format (4Kn) streaming wavebanks with ``-af``

### November 8, 2021
* VS 2022 support
* Minor code and project review
* makespritefont: Updated with 12.2 for FeatureLevel switch
* xwbtool: Fixed potential locale issue with ``-flist``

### October 18, 2021
* Fixed loading of skinned PBR models from SDKMESH v2
* *DirectX Tool Kit for Audio* updated for XAudio2Redist 1.2.8
* Minor code review updates

### October 13, 2021
* Added skinning support for **NormalMapEffect** and **PBREffect**
* Common states updated with support for reverse z-buffer rendering with **DepthReverseZ** and **DepthReadReverseZ** methods.
* Effect factory updates
  * Updated to use ``SkinnedNormalMapEffect`` / ``SkinnedPBREffect`` as appropriate.
  * Automatically disables use of normal mapping on 9.x feature levels
  * PBR now supports 'untextured' models (always requires texture coordinates) with use of diffuse color for constant albedo, and specular power for an estimated constant roughness.
* Model loader updates
  * SDKMESH loader no longer requires precomputed vertex tangents for normal mapping as we don't use them.
  * Added ``ModelLoader_DisableSkinning`` flag when dealing with legacy SDKMESH files with too many skinning bone influences for _MaxBone_
* Minor update for the Mouse implementation for GameInput
* Project and code cleanup

### September 30, 2021
* Added ModelBone support for transformation hierarchies
  * Rigid-body & skinned animation Draw support added to Model
* Added type aliases ``ModelMeshPart::InputLayoutCollection``, ``GeometricPrimitive::VertexCollection`` and ``IndexCollection``.
* EnvironmentMapEffect and NormalMapEffect will now use default diffuse/normal textures if none are set
* VS 2017 projects updated to require the Windows 10 SDK (19401)
* Code review updates

### August 1, 2021
* DebugEffect, NormalMapEffect, and PBREffect updated with instancing support
* GeometricPrimitive updated with DrawInstanced method
* ToneMapPostProcess updated with SetColorRotation method
* Added VS 2022 Preview projects
* MakeSpriteFont updated to use .NET 4.7.2
* Minor code review

### June 9, 2021
* DirectX Tool Kit for Audio updates:
  * Fixed mono source panning
  * Added ``EnableDefaultMultiChannel`` helper to AudioEmitter for multi-channel source setup
  * Added ``GetChannelCount`` accessor to SoundEffectInstance and SoundStreamInstance
  * ``Apply3D`` can now use X3DAUDIO_LISTENER and X3DAUDIO_EMITTER directly or the library helper structs.
* xwbtool: improved ``-flist`` switch to support wildcards and file exclusions
* CMake updated to support building with XAudio2Redist
* Minor code review

### April 6, 2021
* DDSTextureLoader reader updated to accept nVidia Texture Tool v1 single-channel and dual-channel files marked as RGB instead of LUMINANCE
* Minor code and project cleanup
* xwbtool: Updated with  descriptions for HRESULT failure codes

### January 9, 2021
* Code review for improved conformance
* CMake updated to support package install

### November 11, 2020
* Fixed ``/analyze`` warnings in GameInput usage
* Fixed *DirectX Tool Kit for Audio* use of XAudio 2.8 for Windows 8 w/ Windows 10 SDK
* Minor code and project cleanup
* *DirectX Tool Kit for Audio* updated for XAudio2Redist 1.2.4

### September 30, 2020
* GamePad class updated with ``c_MostRecent`` constant for ``-1`` player index special behavior
* Fixed bug in WICTextureLoader that resulted in ``WINCODEC_ERR_INSUFFICIENTBUFFER`` for some resize requests
* Fixed ``.wav`` file reading of MIDILoop chunk
* Minor code cleanup

### August 15, 2020
* EnvironmentMapEffect now supports cubemaps, spherical, and dual-parabola environment maps
* Code review and project updates
* *DirectX Tool Kit for Audio* updated for XAudio2Redist 1.2.3

### July 2, 2020
* Improved SpriteFont drawing performance in Debug builds
* Regenerated shaders using Windows 10 May 2020 Update SDK (19041)
* Code cleanup for some new VC++ 16.7 warnings and static code analysis
* CMake updates
* *DirectX Tool Kit for Audio* updated for XAudio2Redist 1.2.2

### June 1, 2020
* Added BufferHelpers header with functions **CreateStaticBuffer** / **CreateTextureFromMemory**, and the **ConstantBuffer** helper class
* Added **IsPowerOf2** and **CreateInputLayoutFromEffect** helpers to DirectXHelpers
* Converted to typed enum bitmask flags (see release notes for details on this potential *breaking change*)
  + ``AUDIO_ENGINE_FLAGS``, ``ModelLoaderFlags``, ``SOUND_EFFECT_INSTANCE_FLAGS``, and ``WIC_LOADER_FLAGS``
* WICTextureLoader for ``PNG`` codec now checks ``gAMA`` chunk to determine colorspace if the ``sRGB`` chunk is not found for legacy sRGB detection.
* ``WIC_LOADER_SRGB_DEFAULT`` flag added when loading image via WIC without explicit colorspace metadata
* Retired XAudio 2.7 for *DirectX Tool Kit for Audio*. Use XAudio 2.9, XAudio 2.8, or XAudio2Redist instead.
* CMake project updates

### May 10, 2020
* WICTextureLoader updated with new loader flags: ``FORCE_RGBA32``, ``FIT_POW2``, and ``MAKE_SQUARE``
* SimpleMath no longer forces use of d3d11.h or d3d12.h (can be used with d3d9.h for example)
* *DirectX Tool Kit for Audio* updated with **SoundStreamInstance** class for async I/O playback from XACT-style streaming wavebanks
* Code cleanup
* xwbtool: Updated with ``-l`` switch for case-sensitive file systems

### April 3, 2020
* SpriteFont **MeasureString** / **MeasureDrawBounds** fixes for !ignoreWhitespace
* Code review (``constexpr`` / ``noexcept`` usage)
* CMake updated for PCH usage with 3.16 or later

### February 24, 2020
* *breaking change* **Model::CreateFromxxx** changed to use ModelLoaderFlags instead of default bool parameters
* DirectX Tool Kit for Audio updated to support XAudio2Redist NuGet
* Added ``ignoreWhitespace`` defaulted parameter to SpriteFont Measure methods
* Fixed encoding issue with Utilities.fxh
* Code and project cleanup
* Retired VS 2015 projects
* xwbtool: Changed ``-n`` switch to a more safe ``-y`` switch

### December 17, 2019
* Added ARM64 platform to VS 2019 Win32 desktop Win10 project
* Added Vector ``operator/`` by float scalar to SimpleMath
* Updated CMake project
* Code cleaup

### October 17, 2019
* Added optional ``forceSRGB`` parameter to **SaveWICTextureToFile**
* GamePad updated to report VID/PID (when supported)
* Minor code cleanup

### August 21, 2019
* Added xwbtool to CMake project
* Minor code cleanup

### June 30, 2019
* Additional validation for Ex texture loaders
* Clang/LLVM warning cleanup
* Renamed ``DirectXTK_Windows10.vcxproj`` to ``_Windows10_2017.vcxproj``
* Added VS 2019 UWP project

### May 30, 2019
* PBREffect updated with additional set methods
* Added CMake project files
* Code cleanup

### April 26, 2019
* Added VS 2019 desktop projects
* Fixed guards w.r.t. to windows.h usage in Keyboard/Mouse headers
* Added C++/WinRT **SetWindow** helper to Keyboard/Mouse
* Code cleanup for texture loaders
* Officially dropped Windows Vista support

### February 7, 2019
* Model now supports loading _SDKMESH v2_ models
* **PBREffectFactory** added to support PBR materials
* PBREffect and NormalMapEffect shaders updated to support ``BC5_UNORM`` compressed normal maps
* SpriteFont: **DrawString** overloads for UTF-8 chars in addition to UTF-16LE wide chars

### November 16, 2018
* VS 2017 updated for Windows 10 October 2018 Update SDK (17763)
* ARM64 platform configurations added to UWP projects
* Minor code review

### October 31, 2018
* Model loader for SDKMESH now attempts to use legacy DE3CN compressed normals
  + This is an approximation only and emits a warning in debug builds

### October 25, 2018
* Use UTF-8 instead of ANSI for narrow strings
* Minor code review

### August 17, 2018
* Improved validation for 16k textures and other large resources
* Improved debug output for failed texture loads and screengrabs
* Updated for VS 2017 15.8
* Code cleanup

### July 3, 2018
* ModelMeshPart **DrawInstanced** method added
* Code and project cleanup

### May 31, 2018
* VS 2017 updated for Windows 10 April 2018 Update SDK (17134)
* Regenerated shaders using Windows 10 April 2018 Update SDK (17134)

### May 14, 2018
* Updated for VS 2017 15.7 update warnings
* Code and project cleanup
* Retired VS 2013 projects

### April 23, 2018
* ``AlignUp``, ``AlignDown`` template functions in DirectXHelpers.h
* Mouse support for cursor visibility
* SimpleMath and VertexTypes updated with default copy and move ctors
* SimpleMath updates to use ``constexpr``
* EffectFactory updated with **GetDevice** method
* PostProcess updated with 'big triangle' optimization
* Fix for ``CMO`` handling of skinning vertex data
* Code and project file cleanup
* xwbtool: Fixed Windows 7 compatibility issue

### February 7, 2018
* Mouse fix for cursor behavior when using Remote Desktop for Win32
* Updated for a few more VS 2017 warnings
* Code cleanup

### December 13, 2017
* **PBREffect** and **DebugEffect** added
* **NormalMapEffect** no longer requires or uses explicit vertex tangents
* *breaking change* NormalMapEffect::SetBiasedVertexNormalsAndTangents renamed to **SetBiasedVertexNormals**
* PBREffect, DebugEffect, & NormalMapEffect all require Direct3D hardware feature level 10.0 or better
* **VertexType** typedef added to GeometricPrimitive as alias for VertexPositionNormalTexture
* Updated for VS 2017 15.5 update warnings
* Code cleanup

### November 1, 2017
* VS 2017 updated for Windows 10 Fall Creators Update SDK (16299)
* Regenerated shaders using Windows 10 Fall Creators Update SDK (16299)

### September 22, 2017
* Updated for VS 2017 15.3 update ``/permissive-`` changes
* **ScreenGrab** updated to use non-sRGB metadata for PNG
* Mouse use of ``WM_INPUT`` updated for Remote Desktop scenarios
* Fix for ``CMO`` load issue when no materials are defined
* xwbtool: added ``-flist`` option

### July 28, 2017
* Fix for WIC writer when codec target format requires a palette
* Code cleanup

### June 21, 2017
* Post-processing support with the **BasicPostProcess**, **DualPostProcess**, and **ToneMapPostProcess** classes
* SDKMESH loader fix when loading legacy files with all zero materials
* DirectXTK for Audio: Minor fixes for environmental audio
* Minor code cleanup

### April 24, 2017
* VS 2017 project updates
* Regenerated shaders using Windows 10 Creators Update SDK (15063)
* Fixed **NormalMapEffect** shader selection for specular texture usage
* Fixed **AudioEngine** enumeration when using Single Threaded Apartment (STA)
* Fixed bug with **GamePad** (Windows.Gaming.Input) when no user bound

### April 7, 2017
* VS 2017 updated for Windows Creators Update SDK (15063)
* XboxDDSTextureLoader updates

### February 10, 2017
* **GamePad** now supports special value of ``-1`` for 'most recently connected controller'
* WIC format 40bppCMYKAlpha should be converted to RGBA8 rather than RGBA16
* DDS support for L8A8 with bitcount 8 rather than 16
* Minor code cleanup

### December 5, 2016
* Mouse and Keyboard classes updated with **IsConnected** method
* Windows10 project ``/ZW`` switch removed to support use in C++/WinRT projection apps
* VS 2017 RC projects added
* Minor code cleanup

### October 6, 2016
* SDKMESH loader and BasicEffects support for compressed vertex normals with biasing
* WICTextureLoader Ex bool forceSRGB parameter is now a **WIC_LOADER_FLAGS** flag
* Minor code cleanup

### September 15, 2016
* Minor code cleanup
* xwbtool: added wildcard support for input filename and optional ``-r`` switch for recursive search

### September 1, 2016
* Added ``forceSRGB`` optional parameter to SpriteFont ctor
* EffectFactory method **EnableForceSRGB** added
* DGSLEffect now defaults to diffuse/alpha of 1
* Removed problematic ABI::Windows::Foundation::Rect interop for SimpleMath
* Minor code cleanup

### August 4, 2016
* Regenerated shaders using Windows 10 Anniversary Update SDK (14393)

### August 2, 2016
* Updated for VS 2015 Update 3 and Windows 10 SDK (14393)

### August 1, 2016
* GamePad capabilities information updated for Universal Windows and Xbox One platforms
* Specular falloff lighting computation fix in shaders

### July 18, 2016
* **NormalMapEffect** for normal-map with optional specular map rendering
* **EnvironmentMapEffect** now supports per-pixel lighting
* Effects updated with **SetMatrices** and **SetColorAndAlpha** methods
* SimpleMath: improved interop with DirectXMath constants
* Minor code cleanup

### June 30, 2016
* **MeasureDrawString** added to SpriteFont; bad fix to MeasureString reverted
* GamePad tracker updated to track emulated buttons (i.e. leftStickUp)
* EffectFactory **SetDirectory** now checks current working directory (CWD) as well
* *breaking change* must include <d3d11.h> before including <SimpleMath.h>
* Code refactor for sharing some files with DirectX 12 version
* Minor code cleanup

### May 31, 2016
* Added **VertexPosition** and **VertexPositionDualTexture** to VertexTypes
* Xbox One platform fix for PrimitiveBatch
* CompileShader script updated to build external pdbs
* Code cleanup

### April 26, 2016
* Added **Rectangle** class to SimpleMath
* Fix for SDKMESH loader when loading models with 'extra' texture coordinate sets
* Made SimpleMath's Viewport **ComputeTitleSafeArea** less conservative
* Added view/menu aliases to GamePad::ButtonStateTracker for Xbox One Controller naming
* Retired Windows phone 8.0 projects and obsolete adapter code
* Minor code and project file cleanup

### February 23, 2016
* Fixed width computation bug in **SpriteFont::MeasureString**
* Fix to clean up partial or zero-length image files on failed write
* Fix to WaveBankReader for UWP platform
* Retired VS 2012 projects
* Xbox One platform updates
* Minor code and project file cleanup

### January 5, 2016
* Xbox One platform updates
* *breaking change* Need to add use of **GraphicsMemory** class to Xbox One titles
* Minor code cleanup

### November 30, 2015
* SimpleMath improvements including Viewport class
* Fixed bug with **Keyboard** for ``OpenBracket`` and later VK codes
* Fixed bug with **Mouse** that reset the scrollwheel on app activate
* ``MakeSpriteFont`` updated with ``/FastPack`` and ``/FeatureLevel`` switches
* Updated for VS 2015 Update 1 and Windows 10 SDK (10586)

### October 30, 2015
* DirectXTK for Audio 3D updates
* *breaking change* emitters/listeners now use RH coordinates by default
* **GeometricPrimitive** support for custom geometry
* SimpleMath Matrix class improvements
* DDS support for legacy bumpmap formats (V8U8, Q8W8V8U8, V16U16)
* Mouse fix for WinRT implementation with multiple buttons pressed
* Wireframe **CommonStates** no longer does backface culling
* Xbox One platform updates
* Minor code cleanup

### August 18, 2015
* Xbox One platform updates

### July 29, 2015
* Added **CreateBox** method to GeometricPrimitive
* Added ``invertn`` optional parameter to **CreateSphere**
* Updates for Keyboard, Mouse class
* Fixed bug when loading older SDKMESH models
* Updated for VS 2015 and Windows 10 SDK RTM
* Retired VS 2010 and Windows Store 8.0 projects

### July 1, 2015
* Added **Keyboard**, **Mouse** class
* Support for loading pre-lit models with SDKMESH
* **GamePad** implemented using ``Windows.Gaming.Input`` for Windows 10
* DirectXTK for Audio updates for xWMA support with XAudio 2.9
* Added **FindGlyph** and **GetSpriteSheet** methods to SpriteFont

### March 27, 2015
* Added projects for Windows apps Technical Preview
* GamePad temporarily uses 'null' device for universal Windows application platform

### February 25, 2015
* DirectXTK for Audio updates
  + *breaking change* pitch now defined as -1 to 1 with 0 as the default
  + One-shot Play method with volume, pitch, and pan
  + **GetMasterVolume** / **SetMasterVolume** method for AudioEngine
  + Fix for compact wavebank validation
  + Improved voice cleanup and shutdown
* Minor code cleanup and C++11 ``=default``/``=delete`` usage

### January 26, 2015
* GamePad class: emulate ``XInputEnable`` behavior for XInput 9.1.0
* DirectXTK for Audio fix for Stop followed by Play doing a proper restart
* DirectXTK for Audio fix when using XAudio 2.7 on a system with no audio device
* Updates for Xbox One platform support
* Minor code cleanup and C99 ``printf`` string conformance

### November 24, 2014
* SimpleMath fix for Matrix ``operator !=``
* DirectXTK for Audio workaround for XAudio 2.7 on Windows 7 problem
* Updates for Windows phone 8.1 platform support
* Updates for Visual Studio 2015 Technical Preview
* Minor code cleanup

### October 28, 2014
* Model support for loading from ``VBO`` files
* Model render now sets samplers on slots 0,1 by default for dual-texture effects
* Updates for Xbox One platform support
* Minor code cleanup

### September 5, 2014
* **GamePad** class: gamepad controller helper using XInput on Windows, IGamepad for Xbox One
* SimpleMath updates; Matrix billboard methods; *breaking change*: Matrix::Identity() -> Matrix::Identity
* SpriteBatch new optional **SetViewport** method
* SpriteFont fix for white-space character rendering optimization
* DDSTextureLoader fix for auto-gen mipmaps for volume textures
* Explicit calling-convention annotation for public headers
* Updates for Xbox One platform support
* Minor code and project cleanup

### July 15, 2014
* DirectXTK for Audio and XWBTool fixes
* Updates to Xbox One platform support

### April 3, 2014
* Windows phone 8.1 platform support

### February 24, 2014
* DirectXHelper: new utility header with **MapGuard** and public version of **SetDebugObjectName** template
* DDSTextureLoader: Optional support for auto-gen mipmaps
* DDSTextureLoader/ScreenGrab: support for Direct3D 11 video formats including legacy "YUY2" DDS files
* GeometricPrimtive: Handedness fix for tetrahedron, octahedron, dodecahedron, and icosahedron
* ``SpriteBatch::SetRotation(DXGI_MODE_ROTATION_UNSPECIFIED)`` to disable viewport matrix
* XboxDDSTextureLoader: optional forceSRGB parameter

### January 24, 2014
* DirectXTK for Audio updated with voice management and optional mastering volume limiter
* Added orientation rotation support to **SpriteBatch**
* Fixed a resource leak with ``GetDefaultTexture`` used by some Effects
* Code cleanup (removed ``DXGI_1_2_FORMATS`` control define; d2d1.h workaround not needed; ScopedObject typedef removed)

### December 24, 2013
* Added **DirectX Tool Kit for Audio** using XAudio2
* Xbox One platform support
* ``MakeSpriteFont`` tool updated with more progress feedback when capturing large fonts
* Minor updates for ``SDKMESH`` Model loader
* Fixed bug in ``CMO`` Model loader when handling multiple textures
* Improved debugging output

### October 28, 2013
* Updated for Visual Studio 2013 and Windows 8.1 SDK RTM
* Added **DGSLEffect**, **DGSLEffectFactory**, **VertexPositionNormalTangentColorTexture**, and **VertexPositionNormalTangentColorTextureSkinning**
* Model loading and effect factories support loading skinned models
* ``MakeSpriteFont`` now has a smooth vs. sharp antialiasing option: /sharp
* Model loading from ``CMOs`` now handles UV transforms for texture coordinates
* A number of small fixes for **EffectFactory**
* Minor code and project cleanup
* Added ``NO_D3D11_DEBUG_NAME`` compilation define to control population of Direct3D debug layer names for debug builds

### July 1, 2013
* VS 2013 Preview projects added and updates for DirectXMath 3.05 ``__vectorcall``
* Added use of sRGB WIC metadata for ``JPEG``, ``PNG``, and ``TIFF``
* SaveToWIC functions updated with new optional setCustomProps parameter and error check with optional targetFormat

### May 30, 2013
* Added more **GeometricPrimitives**: Cone, Tetrahedron, Octahedron, Dodecahedron, Icosahedron
* Updated to support loading new metadata from DDS files (if present)
* Fixed bug with loading of WIC 32bpp RGBE format images
* Fixed bug when skipping mipmaps in a 1D or 2D array texture DDS file

### February 22, 2013
* Added **SimpleMath** header
* Fixed bug that prevented properly overriding EffectFactory::CreateTexture
* Fixed forceSRGB logic in DDSTextureLoader and WICTextureLoader
* Break circular reference chains when using SpriteBatch with a setCustomShaders lambda
* Updated projects with ``/fp:fast`` for all configs, ``/arch:SSE2`` for Win32 configs
* Sensibly named .pdb output files
* Added ``WIC_USE_FACTORY_PROXY`` build option (uses WindowsCodecs.dll entrypoint rather than CoCreateInstance)

### January 25, 2013
* **GeometricPrimitive** support for left-handed coordinates and drawing with custom effects
* Model, ModelMesh, and ModelMeshPart added with loading of rigid non-animating models from .CMO and .SDKMESH files
* EffectFactory helper class added

### December 11, 2012
* Ex versions of **DDSTextureLoader** and **WICTextureLoader**
* Removed use of ATL's ``CComPtr`` in favor of WRL's ``ComPtr`` for all platforms to support VS Express editions
* Updated VS 2010 project for official 'property sheet' integration for Windows 8.0 SDK
* Minor fix to **CommonStates** for Feature Level 9.1
* Tweaked AlphaTestEffect.cpp to work around ARM NEON compiler codegen bug
* Added dxguid.lib as a default library for Debug builds to resolve GUID link issues

### November 15, 2012
* Added support for WIC2 when available on Windows 8 and Windows 7 with KB 2670838
* Cleaned up warning level 4 warnings

### October 30, 2012
* Added project files for Windows phone 8

### October 12, 2012
* Added **PrimitiveBatch** for drawing user primitives
* Debug object names for all D3D resources (for PIX and debug layer leak reporting)

### October 2, 2012
* Added **ScreenGrab** module
* Added **CreateGeoSphere** for drawing a geodesic sphere
* Put DDSTextureLoader and WICTextureLoader into the DirectX C++ namespace

### September 7, 2012
* Renamed project files for better naming consistency
* Updated WICTextureLoader for Windows 8 96bpp floating-point formats
* Win32 desktop projects updated to use Windows Vista (0x0600) rather than Windows 7 (0x0601) APIs
* Tweaked SpriteBatch.cpp to workaround ARM NEON compiler codegen bug

### May 31, 2012
* Updated Windows Store project for Visual Studio 2012 Release Candidate changes
* Cleaned up x64 Debug configuration warnings and switched to use ``_DEBUG`` instead of ``DEBUG``
* Minor fix for DDSTextureLoader's retry fallback that can happen with 10level9 feature levels

### May 2, 2012
* Added **SpriteFont** implementation and the MakeSpriteFont utility

### March 29, 2012
* WICTextureLoader updated with Windows 8 WIC native pixel formats

### March 6, 2012
* Fix for too much temp memory used by WICTextureLoader
* Add separate Visual Studio 11 projects for Desktop vs. Windows Store builds

### March 5, 2012
* Bug fix for SpriteBatch with batches > 2048

### February 24, 2012
* Original release
