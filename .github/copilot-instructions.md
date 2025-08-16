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
- You can make use of the nuget.org packages **directxtk_desktop_2019**, **directxtk_desktop_win10**, or **directxtk_uwp**.
- You can also use the library source code directly in your project or as a git submodule.

## General Guidelines

- **Code Style**: The project uses an .editorconfig file to enforce coding standards. Follow the rules defined in `.editorconfig` for indentation, line endings, and other formatting. Additional information can be found on the wiki at [Implementation](https://github.com/microsoft/DirectXTK/wiki/Implementation). The code requires C++11/C++14 features.
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
build/          # Miscellaneous build files and scripts.
Audio/          # DirectX Tool Kit for Audio implementation files.
Inc/            # Public header files.
Src/            # Implementation header and source files.
  Shaders/      # HLSL shader files.
MakeSpriteFont/ # CLI tool for capturing sprite fonts.
XWBTool/        # CLI tool for creating XACT-style wave banks.
Tests/          # Tests are designed to be cloned from a separate repository at this location.
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

### Patterns to Avoid

- Don’t use raw pointers for ownership.
- Avoid macros for constants—prefer `constexpr` or `inline` `const`.
- Don’t put implementation logic in header files unless using templates, although the SimpleMath library does use an .inl file for performance.
- Avoid using `using namespace` in header files to prevent polluting the global namespace.

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
