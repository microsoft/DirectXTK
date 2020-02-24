![DirectX Logo](https://github.com/Microsoft/DirectXTK/wiki/X_jpg.jpg)

# DirectX Tool Kit for DirectX 11

http://go.microsoft.com/fwlink/?LinkId=248929

Copyright (c) Microsoft Corporation. All rights reserved.

**February 24, 2020**

This package contains the "DirectX Tool Kit", a collection of helper classes for writing Direct3D 11 C++ code for Universal Windows Platform (UWP) apps for Windows 10, Xbox One, Windows 8.x Win32 desktop applications and Windows 7 Service Pack 1 applications.

This code is designed to build with Visual Studio 2017 ([15.9](https://walbourn.github.io/vs-2017-15-9-update/)), Visual Studio 2019, or clang for Windows v9. It is recommended that you make use of the Windows 10 May 2019 Update SDK ([18362](https://walbourn.github.io/windows-10-may-2019-update/)).

These components are designed to work without requiring any content from the legacy DirectX SDK. For details, see [Where is the DirectX SDK?](https://aka.ms/dxsdk).

## Directory Layout

* ``Inc\``

  + Public Header Files (in the DirectX C++ namespace):

    * Audio.h - low-level audio API using XAudio2 (DirectXTK for Audio public header)
    * CommonStates.h - factory providing commonly used D3D state objects
    * DDSTextureLoader.h - light-weight DDS file texture loader
    * DirectXHelpers.h - misc C++ helpers for D3D programming
    * Effects.h - set of built-in shaders for common rendering tasks
    * GamePad.h - gamepad controller helper using XInput
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

  + DirectXTK source files and internal implementation headers

* ``Audio\``

  + DirectXTK for Audio source files and internal implementation headers

* ``MakeSpriteFont\``

  + Command line tool used to generate binary resources for use with SpriteFont

* ``XWBTool\``

  +  Command line tool for building XACT-style wave banks for use with DirectXTK for Audio's WaveBank class

# Documentation

Documentation is available on the [GitHub wiki](https://github.com/Microsoft/DirectXTK/wiki).

## Notices

All content and source code for this package are subject to the terms of the [MIT License](http://opensource.org/licenses/MIT).

For the latest version of DirectXTK, bug reports, etc. please visit the project site on [GitHub](https://github.com/microsoft/DirectXTK).

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Xbox One

Developers using the Xbox One XDK need to generate the ``Src\Shaders\Compiled\XboxOne*.inc`` files to build the library as they are not included in the distribution package. They are built by running the script in ``Src\Shaders`` - ``CompileShaders xbox`` from the *Xbox One XDK Developer Command Prompt*. They are XDK version-specific. While they will continue to work if outdated, a mismatch will cause runtime compilation overhead that would otherwise be avoided.

## Release Notes

* The VS 2017/2019 projects make use of ``/permissive-`` for improved C++ standard conformance. Use of a Windows 10 SDK prior to the Fall Creators Update (16299) or an Xbox One XDK prior to June 2017 QFE 4 may result in failures due to problems with the system headers. You can work around these by disabling this switch in the project files which is found in the ``<ConformanceMode>`` elements, or in some cases adding ``/Zc:twoPhase-`` to the ``<AdditionalOptions>`` elements.

* The VS 2017 projects require the 15.5 update or later. For UWP and Win32 classic desktop projects with the 15.5 - 15.7 updates, you need to install the standalone Windows 10 SDK (17763) which is otherwise included in the 15.8.6 or later update. Older VS 2017 updates will fail to load the projects due to use of the <ConformanceMode> element. If using the 15.5 or 15.6 updates, you will see ``warning D9002: ignoring unknown option '/Zc:__cplusplus'`` because this switch isn't supported until 15.7. It is safe to ignore this warning, or you can edit the project files ``<AdditionalOptions>`` elements.

* The VS 2019 projects use a ``<WindowsTargetPlatformVersion>`` of ``10.0`` which indicates to use the latest installed version. This should be Windows 10 SDK (17763) or later.

* The UWP projects and the VS 2019 Win10 classic desktop project include configurations for the ARM64 platform. These require VS 2017 (15.9 update) or VS 2019 to build, with the ARM64 toolset installed.

* The CompileShaders.cmd script must have Windows-style (CRLF) line-endings. If it is changed to Linux-style (LF) line-endings, it can fail to build all the required shaders.

