# Step 2: Building the Project

**Ask the user:** Would you like to build the project now?

Detect the local system architecture to determine the appropriate platform target:

```powershell
$arch = $env:PROCESSOR_ARCHITECTURE  # AMD64 or ARM64
```

## MSBuild

For x64 systems:

```powershell
msbuild <ProjectName>.vcxproj /p:Configuration=Debug /p:Platform=x64
```

For ARM64 systems:

```powershell
msbuild <ProjectName>.vcxproj /p:Configuration=Debug /p:Platform=ARM64
```

> The first build will take longer as vcpkg downloads and builds the dependencies.

## CMake

For x64 systems:

```powershell
cmake --preset=x64-Debug
cmake --build out\build\x64-Debug
```

For ARM64 systems:

```powershell
cmake --preset=arm64-Debug
cmake --build out\build\arm64-Debug
```

> The first configure will take longer as vcpkg downloads and builds the dependencies.

## Verifying the Build

Once the build succeeds, the user should have a working executable that displays a cornflower blue window. Confirm the build completed without errors before proceeding to the next tutorial step.

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

The project template used in this tutorial uses VCPKG manifest mode to handle dependency management. The `vcpkg.json` file defines the libraries needed by the application, and the `vcpkg-configuration.json` provides a commit id for the vcpkg registry on [GitHub](https://github.com/microsoft/vcpkg) which controls the initial version of the dependencies.

For this tutorial, we are relying on the default vcpkg integration offered in Visual Studio 2022 or later via `Microsoft.VisualStudio.Component.Vcpkg`.

### MSBuild integration

The vcpkg integration is handled directly in the `.vcxproj` project file:

```xml
  <PropertyGroup Label="Globals">
    <VcpkgEnableManifest>true</VcpkgEnableManifest>
  </PropertyGroup>
```

```xml
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration">
    <VcpkgHostTriplet>x64-windows</VcpkgHostTriplet>
  </PropertyGroup>
```

```xml
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <Import Condition="Exists('$(VCInstallDir)vcpkg\scripts\buildsystems\msbuild\vcpkg.props')"
          Project="$(VCInstallDir)vcpkg\scripts\buildsystems\msbuild\vcpkg.props" />
  <PropertyGroup>
    <VcpkgPageSchema>$(VCInstallDir)vcpkg\scripts\buildsystems\msbuild\vcpkg-general.xml</VcpkgPageSchema>
  </PropertyGroup>
```

```xml
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <VcpkgTriplet>x64-windows-static-md</VcpkgTriplet>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM64'">
    <VcpkgTriplet>arm64-windows-static-md</VcpkgTriplet>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <VcpkgTriplet>x64-windows-static-md</VcpkgTriplet>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM64'">
    <VcpkgTriplet>arm64-windows-static-md</VcpkgTriplet>
  </PropertyGroup>
```

```xml
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <Import Condition="Exists('$(VCInstallDir)vcpkg\scripts\buildsystems\msbuild\vcpkg.targets')" Project="$(VCInstallDir)vcpkg\scripts\buildsystems\msbuild\vcpkg.targets" />
```

```xml
  <Target Name="EnsureVCPKG" BeforeTargets="_CheckForInvalidConfigurationAndPlatform">
    <PropertyGroup>
      <ErrorText>This project requires the VCPKG integration support in Visual Studio. Add the Microsoft.VisualStudio.Component.Vcpkg component to your install.</ErrorText>
    </PropertyGroup>
    <Error Condition="!Exists('$(VCInstallDir)vcpkg')" Text="$(ErrorText)" />
  </Target>
```

> In more complex MSBuild solutions with multiple projects, the vcpkg integration is typically handled in a `Directory.Build.props` and `Directory.Build.targets` file.

### CMake integration

The `CMakePresets.json` sets `CMAKE_TOOLCHAIN_FILE` to vcpkg's toolchain discovered via the `VCPKG_ROOT` environment variable, which hooks into `find_package()` to locate installed packages.

### Triplets

The vcpkg **triplet** describes the architecture, configuration, and other properties of the build environment.

| Triplet | Description |
|---------|-------------|
| `x64-windows` | x64 architecture, Windows, dynamic linking for Multithreaded DLL CRT |
| `arm64-windows` | ARM64 architecture, Windows, dynamic linking for Multithreaded DLL CRT |
| `x64-windows-static-md` | x64 architecture, Windows, static linking for Multi-threaded CRT |
| `arm64-windows-static-md` | ARM64 architecture, Windows, static linking for Multi-threaded CRT |

There are two triplets active in a given build: the **host triplet** and the **target triplet**. The host triplet describes the machine on which the build is running, while the target triplet describes the machine for which the code is being built.
