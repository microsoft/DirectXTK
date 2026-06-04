# DirectX Tool Kit — API Reference Overview

This document provides a categorized summary of all public classes and helpers in DirectX Tool Kit for DirectX 11. API signatures are defined in the public headers in the `Inc/` directory.

For detailed documentation on each class, see the [DirectXTK Wiki](https://github.com/microsoft/DirectXTK/wiki).

## Rendering

| Header | Classes / Functions | Purpose |
| --- | --- | --- |
| `SpriteBatch.h` | `SpriteBatch` | Hardware-accelerated 2D sprite rendering with batching, rotation, and scaling. |
| `SpriteFont.h` | `SpriteFont` | Bitmap font rendering using SpriteBatch. Supports MakeSpriteFont and BMFont formats. |
| `PrimitiveBatch.h` | `PrimitiveBatch<T>` | Low-level immediate-mode rendering of user-defined vertex types. |
| `GeometricPrimitive.h` | `GeometricPrimitive` | Factory for common 3D shapes (sphere, cube, cylinder, torus, teapot, etc.). |
| `Model.h` | `Model`, `ModelMesh`, `ModelMeshPart`, `ModelBone` | Loading and rendering of 3D models from CMO, SDKMESH and VBO file formats. |

> Model requires RTTI (`/GR`) to be enabled.

## Effects (Shaders)

| Header | Classes | Purpose |
| --- | --- | --- |
| `Effects.h` | `BasicEffect` | Per-pixel lighting with texture, vertex color, and fog support. |
| | `AlphaTestEffect` | Alpha testing (clip pixels below a threshold). |
| | `DualTextureEffect` | Two-layer multitexture blending (e.g., lightmaps). |
| | `EnvironmentMapEffect` | Cubic environment map reflections. |
| | `SkinnedEffect` | Vertex skinning with up to 72 bones. |
| | `SkinnedNormalMapEffect` | Skinned rendering with normal maps. |
| | `NormalMapEffect` | Normal map and optional specular map rendering. |
| | `PBREffect` | Physically-based rendering (metalness/roughness workflow). |
| | `SkinnedPBREffect` | PBR with vertex skinning. |
| | `DebugEffect` | Diagnostic visualization (normals, tangents, texture coordinates). |
| | `IEffect`, `IEffectMatrices`, `IEffectLights`, `IEffectFog`, `IEffectSkinning` | Interfaces implemented by effects. |
| | `EffectFactory`, `PBREffectFactory` | Model-driven effect creation and caching. |

## Textures

| Header | Functions | Purpose |
| --- | --- | --- |
| `DDSTextureLoader.h` | `CreateDDSTextureFromFile`, `CreateDDSTextureFromMemory` | Load DDS textures (supports all D3D11 formats, mipmaps, cubemaps, arrays). |
| `WICTextureLoader.h` | `CreateWICTextureFromFile`, `CreateWICTextureFromMemory` | Load common image formats (PNG, JPEG, BMP, TIFF, GIF) via WIC. |
| `XboxDDSTextureLoader.h` | `CreateDDSTextureFromFile`, `CreateDDSTextureFromMemory` | Xbox-optimized DDS loader for tiled resources. |
| `ScreenGrab.h` | `SaveDDSTextureToFile`, `SaveWICTextureToFile` | Capture render targets to DDS or WIC image files. |

## States and Helpers

| Header | Classes / Functions | Purpose |
| --- | --- | --- |
| `CommonStates.h` | `CommonStates` | Pre-built blend, depth-stencil, rasterizer, and sampler state objects. |
| `DirectXHelpers.h` | `CreateInputLayoutFromEffect`, `SetDebugObjectName` | Utility functions for input layouts, debug naming, and resource manipulation. |
| | `MapGuard` | RAII-style guard for mapping/unmapping D3D11 buffers. |
| `BufferHelpers.h` | `CreateStaticBuffer`, `CreateConstantBuffer`, `CreateTextureFromMemory` | Helper functions for creating D3D11 buffers and textures. |
| `GraphicsMemory.h` | `GraphicsMemory` | Per-frame memory management for dynamic resources. Required only for the Xbox One platform. |
| `VertexTypes.h` | `VertexPosition`, `VertexPositionColor`, `VertexPositionTexture`, `VertexPositionNormal`, `VertexPositionNormalTexture`, `VertexPositionNormalColor`, `VertexPositionNormalColorTexture`, `VertexPositionNormalTangentColorTexture`, `VertexPositionNormalTangentColorTextureSkinning` | Pre-defined vertex structure types compatible with the built-in effects. |

## Post-Processing

| Header | Classes | Purpose |
| --- | --- | --- |
| `PostProcess.h` | `IPostProcess` | Base interface for post-processing effects. |
| | `BasicPostProcess` | Copy, monochrome, sepia, down-scale, bloom extract/blur. |
| | `DualPostProcess` | Merge/blend two textures (bloom combine, weighted average). |
| | `ToneMapPostProcess` | HDR tone mapping (Reinhard, ACESFilmic, etc.) and OETF curves. |

## Input

| Header | Classes | Purpose |
| --- | --- | --- |
| `Keyboard.h` | `Keyboard`, `Keyboard::State`, `Keyboard::KeyboardStateTracker` | Keyboard state polling and key-press/release tracking. |
| `Mouse.h` | `Mouse`, `Mouse::State`, `Mouse::ButtonStateTracker` | Mouse state polling with absolute and relative modes. |
| `GamePad.h` | `GamePad`, `GamePad::State`, `GamePad::ButtonStateTracker` | Gamepad state polling, vibration, and button tracking. Supports GameInput, XInput, and Windows.Gaming.Input backends. |

See [DirectX Tool Kit wiki: GamePad](https://github.com/Microsoft/DirectXTK/wiki/GamePad), [DirectX Tool Kit wiki: Keyboard](https://github.com/Microsoft/DirectXTK/wiki/Keyboard), and [DirectX Tool Kit wiki: Mouse](https://github.com/Microsoft/DirectXTK/wiki/Mouse) for full documentation.

## Audio

| Header | Classes | Purpose |
| --- | --- | --- |
| `Audio.h` | `AudioEngine` | XAudio2-based audio engine with 3D audio, mastering voice, and device management. |
| | `SoundEffect` | Loads WAV files and XACT-style wave banks for playback. |
| | `SoundEffectInstance` | Controls playback of a sound (play, pause, stop, loop, volume, pitch, pan). |
| | `SoundStreamInstance` | Streaming playback from disk for large audio files. |
| | `WaveBank` | Loads XACT-style wave bank (.xwb) files containing multiple sounds. |
| | `DynamicSoundEffectInstance` | Programmatic audio generation via callback-driven buffer submission. |
| | `AudioListener`, `AudioEmitter` | 3D audio positioning for spatialized sound. |

See the [DirectX Tool Kit for Audio wiki](https://github.com/Microsoft/DirectXTK/wiki/Audio) for full documentation.

## Math

| Header | Classes / Types | Purpose |
| --- | --- | --- |
| `SimpleMath.h` | `Vector2`, `Vector3`, `Vector4`, `Matrix`, `Quaternion`, `Plane`, `Color`, `Ray`, `Viewport`, `Rectangle` | Wrapper types around DirectXMath (XMFLOAT/XMVECTOR) providing operator overloads and convenience methods. |

All types support standard arithmetic operators and implicit conversion to/from DirectXMath SIMD types (`XMVECTOR`, `XMMATRIX`).

See the [SimpleMath wiki](https://github.com/Microsoft/DirectXTK/wiki/SimpleMath) for full documentation.

## Header Inclusion

All public headers are standalone — include only what you need:

```cpp
#include "SpriteBatch.h"    // For 2D sprite rendering
#include "Effects.h"        // For shader effects
#include "Model.h"          // For 3D model loading
#include "Audio.h"          // For audio playback
#include "SimpleMath.h"     // For math helper types
```

There is no single umbrella header. Each header declares its dependencies via forward declarations or includes as needed.

### VCPKG Usage

When using VCPKG+MSBuild integration (both classic and manifest mode), the public headers must be included with a `directxtk/` prefix.

```cpp
#include "directxtk/SpriteBatch.h"
#include "directxtk/Effects.h"
#include "directxtk/Model.h"
#include "directxtk/Audio.h"
#include "directxtk/SimpleMath.h"
```

When using VCPKG with CMake integration via `find_package`, the include path is added directly so the prefix is not required, but will generally work as well.
