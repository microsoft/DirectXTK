---
name: directxtk-usage
description: Guide for integrating DirectX Tool Kit for DirectX 11 into new projects and understanding the API surface. Use this skill when asked about how to use DirectX Tool Kit, integrate it into a project, or get an overview of its classes and functionality.
---

# DirectX Tool Kit for DirectX 11 — Usage Guide

## Overview

The *DirectX Tool Kit* (aka DirectXTK) is a collection of helper classes for writing DirectX 11.x code in C++. It is designed to be used as a shared source project, NuGet package, or vcpkg port integrated into your game or graphics application.

- **Repository**: <https://github.com/microsoft/DirectXTK>
- **Documentation**: <https://github.com/microsoft/DirectXTK/wiki>
- **NuGet Packages**: `directxtk_desktop_win10`, `directxtk_uwp`
- **vcpkg Port**: `directxtk`

## Integration Methods

### vcpkg manifest-mode (Recommended)

In your `vcpkg.json` file, add the following:

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "dependencies": [
    "directxmath",
    "directxtk"
  ]
}
```

If using GameInput for the game input functionality, add the following:

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "dependencies": [
    "directxmath",
    {
      "name": "directxtk",
      "default-features": false,
      "features": [
        "gameinput"
      ]
    }
  ]
}
```

If using DirectX Tool Kit for Audio and GameInput, add the following:

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "dependencies": [
    "directxmath",
    {
      "name": "directxtk",
      "default-features": false,
      "features": [
        "gameinput",
        "xaudio2-9"
      ]
    }
  ]
}
```

### vcpkg (classic)

```bash
vcpkg install directxtk
```

Features: `xaudio2-9` (DirectX Tool Kit for Audio using XAudio 2.9), `gameinput` (Using GameInput for gamepad, keyboard, and mouse), `tools` (command-line tools). Triplets: `x64-windows`, `arm64-windows`, etc.

For DLL usage (`x64-windows` default triplet), define `DIRECTX_TOOLKIT_IMPORT` in your consuming project. For static library usage, use `-static-md` triplet variants.

CMakeLists.txt:

```cmake
find_package(directxtk CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE Microsoft::DirectXTK)
```

See the [d3d11game_vcpkg](https://github.com/walbourn/directx-vs-templates/tree/main/d3d11game_vcpkg) template for a complete example.

### NuGet

Use `directxtk_desktop_win10` for Win32 desktop applications or `directxtk_uwp` for UWP apps.

### Project Reference

Add the appropriate `.vcxproj` from the `DirectXTK/` folder to your solution and add a project reference. Add the `DirectXTK\Inc` directory to your Additional Include Directories.

## Minimum Setup

Every DirectXTK application needs:

1. A Direct3D 11 device
2. A Direct3D immediate device context
3. The application is responsible for the rendering, SwapChain management, and Present loop

## API Reference

The public API is defined in the header files in the `Inc/` directory. See the [reference overview](reference/overview.md) for a categorized summary of all classes and helpers.

Full documentation for each class is available on the [GitHub wiki](https://github.com/microsoft/DirectXTK/wiki).

API signatures are defined in the public headers under the `Inc/` directory. Always consult those headers for the authoritative function signatures, parameters, and SAL annotations.

## Key Concepts

### Resource Ownership

DirectXTK classes follow RAII principles. Most objects are created with `std::make_unique` and destroyed automatically. COM resources are managed with `Microsoft::WRL::ComPtr`.

### Device-Dependent vs Device-Independent

Most DirectXTK objects are **device-dependent** — they are created with a `ID3D11Device*` and must be recreated if the device is lost. Plan your resource management accordingly.

### Thread Safety

DirectXTK rendering classes (SpriteBatch, Effects, PrimitiveBatch, etc.) are **not** thread-safe. Use one instance per thread, or synchronize access externally. Resource creation (texture loading, model loading) can be done from any thread.

## Namespace

All classes and functions reside in the `DirectX` namespace. Headers that contain Direct3D 11-specific types use `inline namespace DX11` to avoid conflicts when both DX11 and DX12 toolkits are linked together.

```cpp
#include "SpriteBatch.h"

// Usage:
auto spriteBatch = std::make_unique<DirectX::SpriteBatch>(device, ...);
```

## Common Patterns

### Drawing Sprites

```cpp
#include "SpriteBatch.h"
#include "WICTextureLoader.h"

// Create
auto spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);
ComPtr<ID3D11ShaderResourceView> texture;
DirectX::CreateWICTextureFromFile(device, L"cat.png", nullptr, texture.GetAddressOf());

// Draw
spriteBatch->Begin();
spriteBatch->Draw(texture.Get(), DirectX::XMFLOAT2(100, 100));
spriteBatch->End();
```

### Loading and Drawing a 3D Model

```cpp
#include "Model.h"
#include "CommonStates.h"
#include "Effects.h"

// Create
auto states = std::make_unique<DirectX::CommonStates>(device);
auto modelResources = DirectX::Model::CreateFromSDKMESH(device, L"ship.sdkmesh");

// Draw
modelResources->Draw(context, *states, world, view, projection);
```

### Drawing Geometric Primitives

```cpp
#include "GeometricPrimitive.h"

auto sphere = DirectX::GeometricPrimitive::CreateSphere(context);
sphere->Draw(world, view, projection);
```

### Using Effects

```cpp
#include "Effects.h"

auto effect = std::make_unique<DirectX::BasicEffect>(device);
effect->SetWorld(world);
effect->SetView(view);
effect->SetProjection(projection);
effect->SetTexture(textureSRV.Get());
effect->Apply(context);
```

### Input Handling

```cpp
#include "Keyboard.h"
#include "Mouse.h"
#include "GamePad.h"

auto keyboard = std::make_unique<DirectX::Keyboard>();
auto mouse = std::make_unique<DirectX::Mouse>();
auto gamePad = std::make_unique<DirectX::GamePad>();

// In update loop
auto kb = keyboard->GetState();
if (kb.Escape)
    PostQuitMessage(0);

auto pad = gamePad->GetState(0);
if (pad.IsConnected())
{
    if (pad.IsAPressed())
        /* jump */;
}
```

### Audio

```cpp
#include "Audio.h"

// Create audio engine
auto audioEngine = std::make_unique<DirectX::AudioEngine>();

// Load and play a sound
auto soundEffect = std::make_unique<DirectX::SoundEffect>(audioEngine.get(), L"explosion.wav");
soundEffect->Play();

// Per-frame update
audioEngine->Update();
```

## Getting Started Tutorial

For a full step-by-step walkthrough, see the [Getting Started](https://github.com/microsoft/DirectXTK/wiki/Getting-Started) tutorial on the wiki.

## Further Reading

- [DirectXTK Wiki](https://github.com/microsoft/DirectXTK/wiki)
- [DirectX Tool Kit for Audio](https://github.com/microsoft/DirectXTK/wiki/Audio)
- [SimpleMath documentation](https://github.com/Microsoft/DirectXTK/wiki/SimpleMath)
- [Project templates](https://github.com/walbourn/directx-vs-templates)
