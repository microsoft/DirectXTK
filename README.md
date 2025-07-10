![DirectX Logo](https://raw.githubusercontent.com/wiki/Microsoft/DirectXTK/X_jpg.jpg)

# DirectX Tool Kit for DirectX 11

http://go.microsoft.com/fwlink/?LinkId=248929

Copyright (c) Microsoft Corporation.

## July 9, 2025

This package contains the "DirectX Tool Kit", a collection of helper classes for writing Direct3D 11 C++ code for Win32 desktop applications for Windows 8.1 or later, Xbox One, and Universal Windows Platform (UWP) apps for Windows 10 and Windows 11.

This code is designed to build with Visual Studio 2019 (16.11), Visual Studio 2022, clang for Windows v12 or later, or MinGW 12.2. Use of the Windows 10 May 2020 Update SDK ([19041](https://walbourn.github.io/windows-10-may-2020-update-sdk/)) or later is required for Visual Studio.

These components are designed to work without requiring any content from the legacy DirectX SDK. For details, see [Where is the DirectX SDK?](https://aka.ms/dxsdk).

## Directory Layout

* ``Inc\``

  * Public Header Files (in the DirectX C++ namespace):

    * Audio.h - low-level audio API using XAudio2 (DirectXTK for Audio public header)
    * BufferHelpers.h - C++ helpers for creating D3D resources from CPU data
    * CommonStates.h - factory providing commonly used D3D state objects
    * DDSTextureLoader.h - light-weight DDS file texture loader
    * DirectXHelpers.h - misc C++ helpers for D3D programming
    * Effects.h - set of built-in shaders for common rendering tasks
    * GamePad.h - gamepad controller helper using XInput, Windows.Gaming.Input, or GameInput
    * GeometricPrimitive.h - draws basic shapes such as cubes and spheres
    * GraphicsMemory.h - helper for managing dynamic graphics memory allocation
    * Keyboard.h - keyboard state tracking helper
    * Model.h - draws meshes loaded from .CMO, .SDKMESH, or .VBO files
    * Mouse.h - mouse helper
    * PostProcess.h - set of built-in shaders for common post-processing operations
    * PrimitiveBatch.h - simple and efficient way to draw user primitives
    * ScreenGrab.h - light-weight screen shot saver
    * SimpleMath.h - simplified C++ wrapper for DirectXMath
    * SpriteBatch.h - simple & efficient 2D sprite rendering
    * SpriteFont.h - bitmap based text rendering
    * VertexTypes.h - structures for commonly used vertex data formats
    * WICTextureLoader.h - WIC-based image file texture loader
    * XboxDDSTextureLoader.h - Xbox One exclusive apps variant of DDSTextureLoader

* ``Src\``

  * DirectXTK source files and internal implementation headers

* ``Audio\``

  * DirectXTK for Audio source files and internal implementation headers

* ``MakeSpriteFont\``

  * Command line tool used to generate binary resources for use with SpriteFont

* ``XWBTool\``

  * Command line tool for building XACT-style wave banks for use with DirectXTK for Audio's WaveBank class

* ``build\``

  * Contains miscellaneous build files and scripts.

## Documentation

Documentation is available on the [GitHub wiki](https://github.com/Microsoft/DirectXTK/wiki).

## Notices

All content and source code for this package are subject to the terms of the [MIT License](https://github.com/microsoft/DirectXTK/blob/main/LICENSE).

For the latest version of DirectXTK, bug reports, etc. please visit the project site on [GitHub](https://github.com/microsoft/DirectXTK).

## Release Notes

FOR SECURITY ADVISORIES, see [GitHub](https://github.com/microsoft/DirectXTK/security/advisories).

For a full change history, see [CHANGELOG.md](https://github.com/microsoft/DirectXTK/blob/main/CHANGELOG.md).

* Starting with the March 2025 release, Windows 7 and Windows 8.0 support has been retired. For _DirectX ToolKit for Audio_ this means that `DirectXTKAudio_Desktop_*_Win7` has been removed, and `DirectXTKAudio_Desktop_*_Win8` has been integrated into the `DirectXTK_Desktop_*` vcxproj which uses XAudio 2.8 for Windows 8.1 compatibility.

  * Remove any References to or use of `DirectXTKAudio_Desktop_*_Win8.vcxproj` or `DirectXTKAudio_Desktop_*_Win7`. If using `DirectXTK_Desktop_*.vcxproj` you will be using XAudio 2.8 as before. Client code will need to build with `_WIN32_WINNT=0x0603`.

  * With the `directxtk_desktop_2019` NuGet package, you will be using XAudio 2.8 and no longer use the XAudio2Redist NuGet package. Client code will build with `_WIN32_WINNT=0x0603`.

  * If you want to use XAudio2Redist with Windows 8.1, the CMake project supports this with the build option `BUILD_XAUDIO_REDIST`. The CMake build option `BUILD_XAUDIO_WIN7` was renamed.

* Starting with the February 2023 release, the Mouse class implementation of relative mouse movement was updated to accumulate changes between calls to ``GetState``. By default, each time you call ``GetState`` the deltas are reset which works for scenarios where you use relative movement but only call the method once per frame. If you call it more than once per frame, then add an explicit call to ``EndOfInputFrame`` to use an explicit reset model instead.

* As of the September 2022 release, the library makes use of C++11 inline namespaces for differing types that have the same names in the DirectX 11 and DirectX 12 version of the _DirectX Tool Kit_. This provides a link-unique name such as ``DirectX::DX11::SpriteBatch`` that will appear in linker output messages. In most use cases, however, there is no need to add explicit ``DX11`` namespace resolution in client code.

* Starting with the July 2022 release, the ``bool forceSRGB`` parameter for DDSTextureLoader ``Ex`` functions is now a ``DDS_LOADER_FLAGS`` typed enum bitmask flag parameter. This may have a _breaking change_ impact to client code. Replace ``true`` with ``DDS_LOADER_FORCE_SRGB`` and ``false`` with ``DDS_LOADER_DEFAULT``.

* As of the October 2021 release, the DGSLEffect no longer directly supports skinning. Instead, make use of **SkinnedDGSLEffect** which is derived from DGSLEffect.

* Starting with the June 2020 release, this library makes use of [typed enum bitmask flags](https://walbourn.github.io/modern-c++-bitmask-types/) per the recommendation of the _C++ Standard_ section _17.5.2.1.3 Bitmask types_. This may have _breaking change_ impacts to client code:

  * You cannot pass the ``0`` literal as your flags value. Instead you must make use of the appropriate default enum value: ``AudioEngine_Default``, ``SoundEffectInstance_Default``, ``ModelLoader_Clockwise``, or ``WIC_LOADER_DEFAULT``.

  * Use the enum type instead of ``DWORD`` if building up flags values locally with bitmask operations. For example, ```WIC_LOADER_FLAGS flags = WIC_LOADER_DEFAULT; if (...) flags |= WIC_LOADER_FORCE_SRGB;```

* The UWP projects and the Win10 classic desktop project include configurations for the ARM64 platform. Building these requires installing the ARM64 toolset.

* For ARM64/AArch64 development, the VS 2022 compiler is strongly recommended over the VS 2019 toolset. The Windows SDK (26100 or later) is not compatible with VS 2019 for Win32 on ARM64 development. _Note that the ARM32/AArch32 platform is [deprecated](https://learn.microsoft.com/windows/arm/arm32-to-arm64)_.

* When using clang/LLVM for the ARM64/AArch64 platform, the Windows 11 SDK ([22000](https://walbourn.github.io/windows-sdk-for-windows-11/)) or later is required.

* The ``CompileShaders.cmd`` script must have Windows-style (CRLF) line-endings. If it is changed to Linux-style (LF) line-endings, it can fail to build all the required shaders.

* As of the October 2024 release, the xwbtool command-line tool also supports GNU-style long options using ``--``. All existing switches continue to function. MakeSpriteFont supports only `--version` and ``--help`` for POSIX-style parameters.

* Xbox One support for DirectX 11 requires the legacy Xbox One XDK. See February 2023 or earlier releases of _DirectX Tool Kit_ for the required MSBuild project files. It can also be built using the current CMake project from an _Xbox One XDK Developer Command Prompt_ with the addition of CMake 3.20 or later to the path via the `x64-Debug-Durango` or `x64-Release-Durango` CMake preset.

## Support

For questions, consider using [Stack Overflow](https://stackoverflow.com/questions/tagged/directxtk) with the _directxtk_ tag, or the [DirectX Discord Server](https://discord.gg/directx) in the _dx9-dx11-developers_ channel.

For bug reports and feature requests, please use GitHub [issues](https://github.com/microsoft/DirectXTK/issues) for this project.

## Contributing

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

Tests for new features should also be submitted as a PR to the [Test Suite](https://github.com/walbourn/directxtktest/wiki) repository.

Information on coding-style can be found [here](https://github.com/microsoft/DirectXTK/wiki/Implementation).

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft trademarks or logos is subject to and must follow [Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general). Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship. Any use of third-party trademarks or logos are subject to those third-party's policies.

## Credits

The _DirectX Tool Kit_ is the work of Shawn Hargreaves and Chuck Walbourn, with contributions from Aaron Rodriguez Hernandez and Dani Roman.

Thanks to Shanon Drone for the SDKMESH file format.

Thanks to Adrian Tsai for the geodesic sphere implementation.

Thanks to Garrett Serack for his help in creating the NuGet packages for DirectX Tool Kit.

Thanks to Roberto Sonnino for his help with the ``CMO``, DGSL rendering, and the VS Starter Kit animation.

Thanks to Pete Lewis and Justin Saunders for the normal-mapped and PBR shaders implementation.

Thanks to Andrew Farrier and Scott Matloff for their on-going help with code reviews.
