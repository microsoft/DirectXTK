# GitHub Copilot Instructions

These instructions define how GitHub Copilot should assist with this project. The goal is to ensure consistent, high-quality code generation aligned with our conventions, stack, and best practices.

## Context

- **Project Type**: Graphics Library / DirectX / Direct3D 11 / Game Audio
- **Project Name**: DirectX Tool Kit for DirectX 11
- **Language**: C++
- **Framework / Libraries**: STL / CMake / CTest
- **Architecture**: Modular / RAII / OOP

## Getting Started

- See the tutorial at [Getting Started](https://github.com/microsoft/DirectXTK/wiki/Getting-Started).
- The recommended way to integrate *DirectX Tool Kit for DirectX 11* into your project is by using the *vcpkg* Package Manager. See [d3d11game_vcpkg](https://github.com/walbourn/directx-vs-templates/tree/main/d3d11game_vcpkg) for a template which uses VCPKG.
- You can make use of the nuget.org packages **directxtk_desktop_win10** or **directxtk_uwp**.
- You can also use the library source code directly in your project or as a git submodule.

## General Guidelines

- **Code Style**: The project uses an .editorconfig file to enforce coding standards. Follow the rules defined in `.editorconfig` for indentation, line endings, and other formatting. Additional information can be found on the wiki at [Implementation](https://github.com/microsoft/DirectXTK/wiki/Implementation). The library's public API requires C++11, and the project builds with C++17 (`CMAKE_CXX_STANDARD 17`). The command-line tools also use C++17, including `<filesystem>` for long file path support. This code is designed to build with Visual Studio 2022, Visual Studio 2026, clang for Windows v12 or later, or MinGW 12.2.
> Notable `.editorconfig` rules: C/C++ files use 4-space indentation, `crlf` line endings, and `latin1` charset — avoid non-ASCII characters in source files. HLSL files have separate indent/spacing rules defined in `.editorconfig`.
- **Documentation**: The project provides documentation in the form of wiki pages available at [Documentation](https://github.com/microsoft/DirectXTK/wiki/).
- **Error Handling**: Use C++ exceptions for error handling and uses RAII smart pointers to ensure resources are properly managed. For some functions that return HRESULT error codes, they are marked `noexcept`, use `std::nothrow` for memory allocation, and should not throw exceptions.
- **Testing**: Unit tests for this project are implemented in this repository [Test Suite](https://github.com/walbourn/directxtktest/) and can be run using CTest per the instructions at [Test Documentation](https://github.com/walbourn/directxtktest/wiki).
- **Security**: This project uses secure coding practices from the Microsoft Secure Coding Guidelines, and is subject to the `SECURITY.md` file in the root of the repository. Functions that read input from image file, geometry files, and audio files are subject to OneFuzz fuzz testing to ensure they are secure against malformed files.
- **Dependencies**: The project uses CMake and VCPKG for managing dependencies, making optional use of DirectXMath, GameInput, and XAudio2Redist. The project can be built without these dependencies, relying on the Windows SDK for core functionality.
- **Continuous Integration**: This project implements GitHub Actions for continuous integration, ensuring that all code changes are tested and validated before merging. This includes building the project for a number of configurations and toolsets, running a subset of unit tests, and static code analysis including GitHub super-linter, CodeQL, and MSVC Code Analysis.
- **Code of Conduct**: The project adheres to the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). All contributors are expected to follow this code of conduct in all interactions related to the project.

## File Structure

```txt
.azuredevops/   # Azure DevOps pipeline configuration and policy files.
.github/        # GitHub Actions workflow files and linter configuration files.
.nuget/         # NuGet package configuration files.
build/          # Miscellaneous build files and scripts, including OneFuzzConfig.json.
Audio/          # DirectX Tool Kit for Audio implementation files.
Inc/            # Public header files.
Src/            # Implementation header and source files.
  Shaders/      # HLSL shader files.
MakeSpriteFont/ # C# CLI tool for capturing sprite fonts.
XWBTool/        # C++ CLI tool for creating XACT-style wave banks.
Tests/          # Tests are designed to be cloned from a separate repository at this location.
wiki/           # Local clone of the GitHub wiki documentation repository.
```

## Patterns

### Patterns to Follow

- Use RAII for all resource ownership (memory, file handles, etc.).
- Many classes utilize the pImpl idiom to hide implementation details, and to enable optimized memory alignment in the implementation.
- Use `std::unique_ptr` for exclusive ownership and `std::shared_ptr` for shared ownership.
- Use `Microsoft::WRL::ComPtr` for COM object management.
- Make use of anonymous namespaces to limit scope of functions and variables.
- Make use of `assert` for debugging checks, but be sure to validate input parameters in release builds.
- Make use of the `DebugTrace` helper to log diagnostic messages, particularly at the point of throwing an exception.
- Explicitly `= delete` copy constructors and copy-assignment operators on all classes that use the pImpl idiom.
- Explicitly utilize `= default` or `=delete` for copy constructors, assignment operators, move constructors and move-assignment operators where appropriate.
- Use 16-byte alignment (`_aligned_malloc` / `_aligned_free`) to support SIMD operations in the implementation, but do not expose this requirement in public APIs.
- All implementation `.cpp` files include `pch.h` as their first include (precompiled header). MinGW builds skip precompiled headers.
- `Model` and related classes require RTTI (`/GR` on MSVC, `__GXX_RTTI` on GCC/Clang). The CMake build enables `/GR` automatically; do not disable RTTI when using `Model`.

#### Inline Namespace

All public headers that contain types shared with the DirectX 12 version of the *DirectX Tool Kit* use `inline namespace DX11` inside `namespace DirectX`. This provides link-unique names (e.g. `DirectX::DX11::SpriteBatch`) without requiring explicit `DX11` qualification in client code. When adding new public types that also exist in DirectXTK12, place them inside this inline namespace.

#### SAL Annotations

All public API functions must use SAL annotations on every parameter. Use `_Use_decl_annotations_` at the top of each implementation that has SAL in the header declaration — never repeat the annotations in the `.cpp` or `.inl` file.

Common annotations:

| Annotation | Meaning |
| --- | --- |
| `_In_` | Input parameter |
| `_Out_` | Output parameter |
| `_Inout_` | Bidirectional parameter |
| `_In_reads_bytes_(n)` | Input buffer with byte count |
| `_In_reads_(n)` | Input array with element count |
| `_In_z_` | Null-terminated input string |
| `_In_opt_` | Optional input parameter (may be null) |
| `_Out_opt_` | Optional output parameter |
| `_COM_Outptr_` | Output COM interface |

Example:

```cpp
// Header (BufferHelpers.h)
DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateStaticBuffer(
        _In_ ID3D11Device* device,
        _In_reads_bytes_(count* stride) const void* ptr,
        size_t count,
        size_t stride,
        unsigned int bindFlags,
        _COM_Outptr_ ID3D11Buffer** pBuffer) noexcept;

// Implementation (.cpp)
_Use_decl_annotations_
HRESULT DirectX::CreateStaticBuffer(
    ID3D11Device* device,
    const void* ptr,
    size_t count,
    size_t stride,
    unsigned int bindFlags,
    ID3D11Buffer** pBuffer) noexcept
{ ... }
```

#### Calling Convention and DLL Export

- All public functions use `__cdecl` explicitly for ABI stability.
- All public function declarations are prefixed with `DIRECTX_TOOLKIT_API`, which wraps `__declspec(dllexport)` / `__declspec(dllimport)` or the GCC `__attribute__` equivalent when using `BUILD_SHARED_LIBS` in CMake.

#### `noexcept` Rules

- All query and utility functions that cannot fail are marked `noexcept`.
- All HRESULT-returning I/O and processing functions are also `noexcept` — errors are communicated via return code, never via exceptions.
- Constructors and functions that perform heap allocation or utilize Standard C++ containers that may throw are marked `noexcept(false)`.

#### Enum Flags Pattern

Flags enums follow this pattern — a `uint32_t`-based unscoped enum with a `_DEFAULT = 0x0` base case, followed by a call to `DEFINE_ENUM_FLAG_OPERATORS` to enable `|`, `&`, and `~` operators:

```cpp
  enum DDS_LOADER_FLAGS : uint32_t
  {
      DDS_LOADER_DEFAULT = 0,
      DDS_LOADER_FORCE_SRGB = 0x1,
      DDS_LOADER_IGNORE_SRGB = 0x2,
      DDS_LOADER_IGNORE_MIPS = 0x20,
  };

DEFINE_ENUM_FLAG_OPERATORS(DDS_LOADER_FLAGS);
```

See [this blog post](https://walbourn.github.io/modern-c++-bitmask-types/) for more information on this pattern.

### Patterns to Avoid

- Don’t use raw pointers for ownership.
- Avoid macros for constants—prefer `constexpr` or `inline` `const`.
- Don’t put implementation logic in header files unless using templates, although the SimpleMath library does use an .inl file for performance.
- Avoid using `using namespace` in header files to prevent polluting the global namespace.

## Naming Conventions

| Element | Convention | Example |
| --- | --- | --- |
| Classes / structs | PascalCase | `VertexPosition` |
| Public functions | PascalCase + `__cdecl` | `ComputeDisplayArea` |
| Private data members | `m_` prefix | `m_count` |
| Enum type names | UPPER_SNAKE_CASE | `DDS_LOADER_FLAGS` |
| Enum values | UPPER_SNAKE_CASE | `DDS_LOADER_DEFAULT` |
| Files | PascalCase | `ScreenGrab.h`, `SpriteEffect.fx` |

## File Header Convention

Every source file (`.cpp`, `.h`, `.hlsl`, `.fx`, etc.) must begin with this block:

```cpp
//-------------------------------------------------------------------------------------
// File: {FileName}
//
// {One-line description}
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------
```

Section separators within files use:
- Major sections: `//-------------------------------------------------------------------------------------`
- Subsections:   `//---------------------------------------------------------------------------------`

The project does **not** use Doxygen. API documentation is maintained exclusively on the GitHub wiki.

## HLSL Shader Compilation

Shaders in `Src/Shaders/` are compiled with **FXC**, producing embedded C++ header files (`.inc`):

- Use `CompileShaders.cmd` in `Src/Shaders/` to regenerate the `.inc` files.
- The CMake option `USE_PREBUILT_SHADERS=ON` skips shader compilation and uses pre-built `.inc` files; requires `COMPILED_SHADERS` variable to be set.

## References

- [Source git repository on GitHub](https://github.com/microsoft/DirectXTK.git)
- [DirectXTK documentation git repository on GitHub](https://github.com/microsoft/DirectXTK.wiki.git)
- [DirectXTK test suite git repository on GitHub](https://github.com/walbourn/directxtktest.wiki.git).
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Microsoft Secure Coding Guidelines](https://learn.microsoft.com/en-us/security/develop/secure-coding-guidelines)
- [CMake Documentation](https://cmake.org/documentation/)
- [VCPKG Documentation](https://learn.microsoft.com/vcpkg/)
- [Games for Windows and the DirectX SDK blog - March 2012](https://walbourn.github.io/directxtk/)
- [Games for Windows and the DirectX SDK blog - January 2013](https://walbourn.github.io/directxtk-update/)
- [Games for Windows and the DirectX SDK blog - December 13](https://walbourn.github.io/directx-tool-kit-for-audio/)
- [Games for Windows and the DirectX SDK blog - September 2014](https://walbourn.github.io/directx-tool-kit-now-with-gamepads/)
- [Games for Windows and the DirectX SDK blog - August 2015](https://walbourn.github.io/directx-tool-kit-keyboard-and-mouse-support/)
- [Games for Windows and the DirectX SDK blog - September 2021](https://walbourn.github.io/latest-news-on-directx-tool-kit/)
- [Games for Windows and the DirectX SDK blog - October 2021](https://walbourn.github.io/directx-tool-kit-vertex-skinning-update/)
- [Games for Windows and the DirectX SDK blog - May 2020](https://walbourn.github.io/directx-tool-kit-for-audio-updates-and-a-direct3d-9-footnote/)

## No speculation

When creating documentation:

### Document Only What Exists

- Only document features, patterns, and decisions that are explicitly present in the source code.
- Only include configurations and requirements that are clearly specified.
- Do not make assumptions about implementation details.

### Handle Missing Information

- Ask the user questions to gather missing information.
- Document gaps in current implementation or specifications.
- List open questions that need to be addressed.

### Source Material

- Always cite the specific source file and line numbers for documented features.
- Link directly to relevant source code when possible.
- Indicate when information comes from requirements vs. implementation.

### Verification Process

- Review each documented item against source code whenever related to the task.
- Remove any speculative content.
- Ensure all documentation is verifiable against the current state of the codebase.

## Cross-platform Support Notes

- The code targets Win32 desktop applications for Windows 8.1 or later, Xbox One, Xbox Series X|S, and Universal Windows Platform (UWP) apps for Windows 10 and Windows 11.
- Portability and conformance of the code is validated by building with Visual C++, clang/LLVM for Windows, and MinGW.
- For Xbox development, the project provides MSBuild solutions for GDK (`DirectXTK_GDK_2022.sln`) and GDK with Xbox Extensions (`DirectXTK_GDKW_2022.sln`). The CMake build supports Xbox via the `XBOX_CONSOLE_TARGET` variable (`scarlett` or `xboxone`).
- The project ships MSBuild projects for Visual Studio 2022 (`.sln` / `.vcxproj`) and Visual Studio 2026 (`.slnx` / `.vcxproj`). VS 2019 projects have been retired.

### Platform and Compiler `#ifdef` Guards

Use these established guards — do not invent new ones:

| Guard | Purpose |
| --- | --- |
| `_WIN32` | Windows platform (desktop, UWP, Xbox) |
| `_GAMING_XBOX` | Xbox platform (GDK — covers both Xbox One and Xbox Series X\|S) |
| `_GAMING_XBOX_SCARLETT` | Xbox Series X\|S (GDK with Xbox Extensions) |
| `_GAMING_XBOX_XBOXONE` | Xbox One (GDK with Xbox Extensions) |
| `_XBOX_ONE && _TITLE` | Xbox One XDK (legacy) |
| `_MSC_VER` | MSVC-specific (and MSVC-like clang-cl) pragmas and warning suppression |
| `__clang__` | Clang/LLVM diagnostic suppressions |
| `__GNUC__` | MinGW/GCC DLL attribute equivalents |
| `_M_ARM64` / `_M_X64` / `_M_IX86` | Architecture-specific code paths for MSVC (`#ifdef`) |
| `_M_ARM64EC` | ARM64EC ABI (ARM64 code with x64 interop) for MSVC |
| `__aarch64__` / `__x86_64__` / `__i386__` | Additional architecture-specific symbols for MinGW/GNUC (`#if`) |
| `USING_GAMEINPUT` | GameInput API for GamePad, Keyboard, Mouse |
| `USING_WINDOWS_GAMING_INPUT` | Windows.Gaming.Input API for GamePad |
| `USING_XINPUT` | XInput API for GamePad, Keyboard, Mouse |
| `USING_COREWINDOW` | CoreWindow-based input (UWP) for Keyboard, Mouse |

> `_M_ARM`/ `__arm__` is legacy 32-bit ARM which is deprecated.

## Code Review Instructions

When reviewing code, focus on the following aspects:

- Adherence to coding standards defined in `.editorconfig` and on the [wiki](https://github.com/microsoft/DirectXTK/wiki/Implementation).
- Make coding recommendations based on the *C++ Core Guidelines*.
- Proper use of RAII and smart pointers.
- Correct error handling practices and C++ Exception safety.
- Clarity and maintainability of the code.
- Adequate comments where necessary.
- Public interfaces are located in `Inc\*.h` should be clearly defined and documented on the GitHub wiki.
- Compliance with the project's architecture and design patterns.
- Ensure that all public functions and classes are covered by unit tests located on [GitHub](https://github.com/walbourn/directxtktest.git) where applicable. Report any gaps in test coverage.
- Check for performance implications, especially in geometry processing algorithms.
- Provide brutally honest feedback on code quality, design, and potential improvements as needed.

## Documentation Review Instructions

When reviewing documentation, do the following:

- Read the code located in [this git repository](https://github.com/microsoft/DirectXTK.git) in the main branch.
- Review the public interface defined in the `Inc` folder.
- Read the documentation on the wiki located in [this git repository](https://github.com/microsoft/DirectXTK.wiki.git).
- Report any specific gaps in the documentation compared to the public interface.
