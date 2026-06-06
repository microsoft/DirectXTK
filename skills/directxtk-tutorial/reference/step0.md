# Step 0: Prerequisites

Before starting the tutorial, verify the user has the necessary tools and environment set up.

**Ask the user:** Have you already installed the tools below, or would you like help checking?

## Required Software

| Tool | Minimum Version | Purpose |
| ------ | ----------------- | --------- |
| **Visual Studio 2022** or **Visual Studio 2026** | 17.4+ / any | IDE and C++ compiler |
| **Desktop development with C++** workload | — | Includes MSVC toolset, Windows SDK, and CMake tools |
| **VCPKG component** (`Microsoft.VisualStudio.Component.Vcpkg`) | — | Package manager integration for dependencies |
| **Windows SDK** | 10.0.22000.0+ (Windows 11) | DirectX 11 headers and libraries |
| **Git** | any | Clone templates and manage source control |

> Visual Studio Community edition is sufficient. The **VCPKG component** is included in the "Desktop development with C++" workload by default in VS 2022 17.7+ and VS 2026.

## Optional but Recommended

| Tool | Purpose |
| ------ | --------- |
| **CMake** (3.21+) | Required only if creating a CMake project instead of MSBuild |
| **Windows Terminal** | Better terminal experience for PowerShell commands |
| **Visual Studio Graphics Debugger** | GPU debugging |

## Verification Steps

Run the following checks in a PowerShell terminal to confirm the environment is ready:

### Visual Studio

```powershell
# Verify a VS 2022 or later install with the C++ Desktop workload
& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -version "[17.0," -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath
```

### Windows SDK

```powershell
# Check for a Windows 11 SDK (10.0.22000.0 or later)
Get-ChildItem "HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots" -ErrorAction SilentlyContinue |
    Get-ItemProperty | Select-Object -ExpandProperty KitsRoot10 -ErrorAction SilentlyContinue
```

### VCPKG

```powershell
# Verify the vcpkg component is present in the VS install
$vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -version "[17.0," -requires Microsoft.VisualStudio.Component.Vcpkg -property installationPath | Select-Object -First 1
Test-Path "$vsPath\VC\vcpkg\vcpkg.exe"
```

### Git

```powershell
git --version
```

## Troubleshooting

- **Missing VCPKG component** — Open the Visual Studio Installer, click **Modify**, and ensure the "Desktop development with C++" workload is checked. In the Individual Components tab, confirm `Microsoft.VisualStudio.Component.Vcpkg` is selected.
- **Windows SDK not found** — Install a Windows 11 SDK (10.0.22000.0 or later) through the Visual Studio Installer under Individual Components.
- **Git not found** — Install Git from <https://git-scm.com/> or via `winget install Git.Git`.

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

### Why vcpkg?

The tutorial uses vcpkg in **manifest mode** to manage DirectX Tool Kit as a dependency. This approach:

1. **Pins dependency versions** via `vcpkg.json` and `vcpkg-configuration.json`, ensuring reproducible builds.
2. **Avoids manual library setup** — no need to download, build, or configure include/library paths yourself.
3. **Supports multiple configurations** — vcpkg handles Debug/Release and x64/ARM64 builds through triplets.

### DirectX 11 and Windows

Direct3D 11 is available on Windows 10 and Windows 11 (all editions). The Windows SDK provides the `d3d11.h`, `dxgi1_2.h`, and related headers. No separate DirectX SDK download is required — the legacy standalone DirectX SDK (June 2010) is not needed for this tutorial.

> For more background on the modern Windows SDK vs. the legacy DirectX SDK, see [this blog post](https://walbourn.github.io/where-is-the-directx-sdk-2024-edition/).
