# Step 1: Project Setup

First, determine if the user needs a new project or already has one.

**Ask the user:** Do you want to create a new project, or do you already have an existing Direct3D 11 project?

## Creating a New Project

If the user wants a new project, use the `d3d11game_vcpkg` template from <https://github.com/walbourn/directx-vs-templates/>.

**Ask the user:**

1. What project name would you like? (Default: `Direct3DGame`)
2. Do you want an MSBuild (.vcxproj) project or a CMake project (CMake recommended)?
3. Where should the project be created? (Default: `$Env:USERPROFILE\source`)

Then download the templates from the latest GitHub release and run the appropriate script:

```powershell
# Download and extract the templates repo from the latest release
$release = (Invoke-RestMethod -Uri "https://api.github.com/repos/walbourn/directx-vs-templates/releases/latest").tag_name
$templatesZip = "$env:TEMP\directx-vs-templates.zip"
$templatesDir = "$env:TEMP\directx-vs-templates"
Invoke-WebRequest -Uri "https://github.com/walbourn/directx-vs-templates/archive/refs/tags/$release.zip" -OutFile $templatesZip
Expand-Archive -Path $templatesZip -DestinationPath $templatesDir -Force
$repoRoot = Get-ChildItem -Path $templatesDir -Directory | Select-Object -First 1
```

### MSBuild

```powershell
& "$($repoRoot.FullName)\VSIX\createmsbuild.ps1" d3d11game_vcpkg <ProjectName> <TargetDir>
```

Parameters:
- `TemplateDir`: Use `d3d11game_vcpkg` for the DirectX 11 game template with vcpkg integration.
- `ProjectName`: The project name (default: `Direct3DGame`). Used for the `.vcxproj` filename and solution.
- `TargetDir`: Output directory (default: `$Env:USERPROFILE\source`). A subdirectory named `<ProjectName>` is created here.
- `PlatformToolset`: VS platform toolset (default: `v143` for VS 2022, use `v144` for VS 2026).

### CMake

```powershell
& "$($repoRoot.FullName)\VSIX\createcmake.ps1" d3d11game_vcpkg <ProjectName> <TargetDir>
```

Parameters:
- `TemplateDir`: Use `d3d11game_vcpkg` for the DirectX 11 game template with vcpkg integration.
- `ProjectName`: The project name (default: `Direct3DGame`). Used for the CMake project name.
- `TargetDir`: Output directory (default: `$Env:USERPROFILE\source`). A subdirectory named `<ProjectName>` is created here.

### Cleanup

After the project is created, clean up the temp files:

```powershell
Remove-Item -Path $templatesZip -Force
Remove-Item -Path $templatesDir -Recurse -Force
```

### Note for git repositories

If the project is being created in a git repository, then the `.gitignore` file at the root of the repository should be updated to exclude the vcpkg installed files and build artifacts.

```plaintext
**/vcpkg_installed/
```

## Using an Existing Project

If the user already has a project, verify they have:

1. A Direct3D 11 device and device context
2. A render loop with SwapChain and Present
3. DirectXTK integrated via vcpkg, NuGet, or project reference (see the **directxtk-usage** skill for integration guidance)

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

The `d3d11game_vcpkg` template creates a minimal Direct3D 11 application with the following architecture:

### Overview

The basics of a Win32 Direct3D 11 application include:

1. **Win32 window creation** — A window is created using the Win32 API.
2. **Direct3D 11 device initialization** — The DXGI API is used to enumerate adapters, and then a ID3D11Device object is created which is used for creating resources. In addition to creating the device, a ID3D11DeviceContext object is created which is used for rendering operations and state management.
3. **Render loop** — For a game application, a loop is run to update the game state and render the scene each frame. Unlike in traditional applications, the game loop is real-time and continuous rather than waiting for user input.
4. **Swap chain management** — A IDXGISwapChain object is used to manage the presentation of rendered frames. If the application rendering gets too far ahead of the display, it will wait until more frames are fully processed.

### Main.cpp — Application Entry Point

- **Win32 window creation** — Registers a `WNDCLASSEX`, creates an `HWND` with `CreateWindowExW`, and runs a standard `PeekMessage` loop. When no messages are pending, `Game::Tick()` is called to drive the game loop.
- **Window message handling** — The `WndProc` handles resize (`WM_SIZE`), minimize/restore (suspend/resume), ALT+ENTER fullscreen toggle, display changes, and power management.
- **Hybrid GPU preference** — Exports `NvOptimusEnablement` and `AmdPowerXpressRequestHighPerformance` to prefer discrete GPUs on hybrid systems.
- **COM initialization** — Calls `CoInitializeEx` (multithreaded) which is required for WIC texture loading later.

> For details on the fullscreen management, see [this blog post](https://walbourn.github.io/care-and-feeding-of-modern-swapchains/).

### DeviceResources — Direct3D 11 Device Wrapper

The `DeviceResources` class encapsulates all Direct3D device and swap chain management:

- **`IDXGIFactory2`** — Created via `CreateDXGIFactory1`. Used to enumerate adapters, create the swap chain, and check feature support (tearing, HDR, flip model).
- **`ID3D11Device1`** — Created via `D3D11CreateDevice` with a preferred hardware adapter (falls back to WARP in debug builds). Requests the highest feature level available (up to 11.1).
- **`ID3D11DeviceContext1`** — The immediate context for issuing draw calls and state changes. Obtained alongside the device.
- **`IDXGISwapChain1`** — Created via `IDXGIFactory2::CreateSwapChainForHwnd`. Uses `DXGI_SWAP_EFFECT_FLIP_DISCARD` for modern flip-model presentation when supported.
- **`ID3D11RenderTargetView`** — View into the swap chain's back buffer (`GetBuffer(0)` → `CreateRenderTargetView`).
- **`ID3D11Texture2D`** (depth stencil) — Created with `D3D11_BIND_DEPTH_STENCIL` for depth testing.
- **`ID3D11DepthStencilView`** — View into the depth buffer for the output-merger stage.
- **`ID3DUserDefinedAnnotation`** — For PIX/graphics debugger event markers.
- **Debug layer** — In debug builds, enables `D3D11_CREATE_DEVICE_DEBUG` and configures `ID3D11InfoQueue` to break on errors/corruption.

> Since we are going to ultimately be rendering a 3D scene in the tutorial, we are using the default values for the DeviceResources ctor. If we were only rendering 2D, we would pass ``DXGI_FORMAT_UNKNOWN`` for the *depthBufferFormat* parameter.

### Game Loop

The template's game loop follows a fixed pattern:

1. `Game::Tick()` → calls `Update` (game logic with a step timer) then `Render`
2. `Render` → clears render target/depth stencil, draws, calls `DeviceResources::Present`
3. `Present` → calls `IDXGISwapChain1::Present1` (with tearing flag if supported)
4. Device lost handling — if `Present` returns `DXGI_ERROR_DEVICE_REMOVED`, calls `HandleDeviceLost` which recreates all resources and notifies the `Game` via `IDeviceNotify`
