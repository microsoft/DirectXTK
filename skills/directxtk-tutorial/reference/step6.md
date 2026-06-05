# Step 6: Simple Rendering

This step introduces rendering custom geometry using `BasicEffect`, `PrimitiveBatch`, and `VertexPositionColor`. We draw a triangle with per-vertex colors (red, green, blue) that blend smoothly across the surface. See the wiki page: [Simple rendering](https://github.com/microsoft/DirectXTK/wiki/Simple-rendering).

## Background

To draw geometry with Direct3D 11, you need:

- A **vertex buffer** containing the vertices to draw
- An **input layout** describing the memory layout of each vertex
- A **primitive topology** indicating how vertices form shapes (points, lines, triangles)
- A compiled **vertex shader** and **pixel shader**
- **State objects** for rasterizer, depth/stencil, and blend state

DirectX Tool Kit simplifies this:

| Requirement | DirectXTK Class |
|-------------|----------------|
| Vertex buffer + topology | `PrimitiveBatch<T>` |
| Input layout + shaders | `BasicEffect` + `CreateInputLayoutFromEffect` |
| State objects | `CommonStates` |
| Vertex format | `VertexPositionColor` |

## Code Changes

### Game.cpp — Render (comment out sprite drawing keeping the text)

First, comment out or remove the sprite drawing code from Steps 4 in the `Render` method, since the triangle will cover the sprite:

```cpp
// m_spriteBatch->Draw(m_texture.Get(), m_screenPos, nullptr,
//     DirectX::Colors::White, 0.f, m_origin);
```

### pch.h

Ensure the following includes are in `pch.h`:

```cpp
#include <directxtk/CommonStates.h>
#include <directxtk/DirectXHelpers.h>
#include <directxtk/Effects.h>
#include <directxtk/PrimitiveBatch.h>
#include <directxtk/SimpleMath.h>
#include <directxtk/VertexTypes.h>
```

### Game.h

Add the following private member variables to the `Game` class:

```cpp
using VertexType = DirectX::VertexPositionColor;

std::unique_ptr<DirectX::BasicEffect> m_effect;
std::unique_ptr<DirectX::PrimitiveBatch<VertexType>> m_batch;
Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
```

Add this as well if not already present:

```cpp
std::unique_ptr<DirectX::CommonStates> m_states;
```

### Game.cpp — CreateDeviceDependentResources

In `CreateDeviceDependentResources`, create the effect, input layout, and primitive batch:

These both should be in the function already, but if it is missing add it:

```cpp
auto device = m_deviceResources->GetD3DDevice();
auto context = m_deviceResources->GetD3DDeviceContext();
```

If CommonStates is not already created, create it:

```cpp
m_states = std::make_unique<DirectX::CommonStates>(device);
```

Then add the effect, input layout, and primitive batch:

```cpp
m_effect = std::make_unique<DirectX::BasicEffect>(device);
m_effect->SetVertexColorEnabled(true);

DX::ThrowIfFailed(
    DirectX::CreateInputLayoutFromEffect<VertexType>(device, m_effect.get(),
        m_inputLayout.ReleaseAndGetAddressOf())
);

m_batch = std::make_unique<DirectX::PrimitiveBatch<VertexType>>(context);
```

> **Important:** Call `SetVertexColorEnabled(true)` *before* creating the input layout. This ensures `BasicEffect` selects the correct shader permutation that includes the `COLOR` vertex element.

### Game.cpp — CreateWindowSizeDependentResources

In `CreateWindowSizeDependentResources`, set up a projection matrix so we can use pixel coordinates (matching the SpriteBatch coordinate system):

```cpp
auto size = m_deviceResources->GetOutputSize();

DirectX::SimpleMath::Matrix proj = DirectX::SimpleMath::Matrix::CreateOrthographicOffCenter(
    0.f, float(size.right), float(size.bottom), 0.f, 0.f, 1.f);
m_effect->SetProjection(proj);
```

### Game.cpp — Render

In the `Render` method, after clearing the render target and before `Present`, draw the triangle:

```cpp
context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
context->OMSetDepthStencilState(m_states->DepthNone(), 0);
context->RSSetState(m_states->CullNone());

m_effect->Apply(context);

context->IASetInputLayout(m_inputLayout.Get());

m_batch->Begin();

VertexPositionColor v1(DirectX::SimpleMath::Vector3(400.f, 150.f, 0.f), DirectX::Colors::Red);
VertexPositionColor v2(DirectX::SimpleMath::Vector3(600.f, 450.f, 0.f), DirectX::Colors::Green);
VertexPositionColor v3(DirectX::SimpleMath::Vector3(200.f, 450.f, 0.f), DirectX::Colors::Blue);

m_batch->DrawTriangle(v1, v2, v3);

m_batch->End();
```

### Game.cpp — OnDeviceLost

In `OnDeviceLost`, release the resources:

If `m_states` is not already in the function, reset it:

```cpp
m_states.reset();
```

Then add the reset for our new member variables:

```cpp
m_effect.reset();
m_batch.reset();
m_inputLayout.Reset();
```

## Build and Verify

Build the project using the commands from [Step 2](step2.md). When you run the application, you should see a triangle with smoothly interpolated red, green, and blue colors — the classic "rainbow triangle" — rendered on the cornflower blue background.

The per-vertex colors are interpolated by the GPU rasterizer across the triangle surface, producing a smooth gradient between the three corner colors.

## Coordinate Systems

The code above uses **pixel coordinates** (origin at top-left, y-axis pointing down) thanks to the orthographic projection matrix. The triangle vertices are specified in screen pixels, matching the coordinate system used by `SpriteBatch`.

Without the projection matrix, `BasicEffect` uses **normalized device coordinates** (-1 to +1 in both axes, origin at center). You can use either approach depending on your needs.

## More to explore

- [Simple rendering](https://github.com/microsoft/DirectXTK/wiki/Simple-rendering)
  - Drawing with normalized coordinates
  - State objects and backface culling
  - Drawing with textures
  - Drawing with lighting and normal maps
- [Line drawing and anti-aliasing](https://github.com/microsoft/DirectXTK/wiki/Line-drawing-and-anti-aliasing)
  - Drawing a grid
  - Anti-aliasing

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

### BasicEffect

`BasicEffect` is a versatile shader class that supports multiple rendering modes. Under the hood it manages:

- **Multiple shader permutations** — Pre-compiled vertex and pixel shader blobs embedded in the library. The active permutation depends on which features are enabled (vertex color, texture, lighting, fog).
- **`ID3D11Buffer`** (constant buffer) — Contains the world/view/projection matrices, material colors, and light parameters. Updated via `Map`/`Unmap` when `Apply()` is called.
- **Shader selection** — `GetVertexShaderBytecode` returns the bytecode for the currently selected permutation, which is why you must configure the effect (e.g., `SetVertexColorEnabled`) before creating the input layout.

When `SetVertexColorEnabled(true)` is set, the vertex shader passes through the per-vertex `COLOR` attribute, and the pixel shader multiplies the interpolated vertex color with the material's diffuse color.

### PrimitiveBatch

`PrimitiveBatch<T>` is a lightweight immediate-mode geometry renderer, similar to the old D3D9 `DrawPrimitiveUP` API:

- **`ID3D11Buffer`** (dynamic vertex buffer) — Created with `D3D11_USAGE_DYNAMIC` and `D3D11_CPU_ACCESS_WRITE`. Uses `Map`/`Unmap` with `D3D11_MAP_WRITE_DISCARD` or `D3D11_MAP_WRITE_NO_OVERWRITE` for efficient CPU-to-GPU streaming.
- **`ID3D11Buffer`** (dynamic index buffer) — Used when drawing indexed primitives (e.g., `DrawQuad`, `DrawIndexed`).
- **No shader management** — Unlike `SpriteBatch`, `PrimitiveBatch` does not set shaders, input layout, or state. You must set these yourself (typically via an `IEffect` and `CommonStates`).
- **Topology** — Each draw call specifies the primitive topology. `DrawTriangle` uses `D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST`, `DrawLine` uses `LINELIST`.

### Input Layout

The `ID3D11InputLayout` object is the bridge between vertex buffer data and the vertex shader:

- Created via `ID3D11Device::CreateInputLayout`, which takes the vertex element descriptions (from `VertexPositionColor::InputElements`) and the compiled vertex shader bytecode (from `BasicEffect::GetVertexShaderBytecode`).
- `CreateInputLayoutFromEffect<T>` is a helper template that combines `T::InputElements` with the effect's shader bytecode in a single call.
- `VertexPositionColor` declares two elements: `POSITION` (XMFLOAT3) and `COLOR` (XMFLOAT4).

### The Rendering Pipeline

When you call the render code above, here's what happens at the D3D11 level:

1. `OMSetBlendState` / `OMSetDepthStencilState` / `RSSetState` — Configure the fixed-function output-merger and rasterizer stages.
2. `m_effect->Apply(context)` — Binds the vertex shader, pixel shader, and updates the constant buffer with the current matrices/colors.
3. `IASetInputLayout` — Tells the input assembler how to read vertex data.
4. `m_batch->Begin()` — Prepares the dynamic vertex buffer for writing.
5. `DrawTriangle` — Maps the vertex buffer, copies 3 vertices, sets the primitive topology to `TRIANGLELIST`, and issues a `Draw(3, ...)` call.
6. `m_batch->End()` — Flushes any remaining batched geometry.

### Shaders

The BasicEffect shaders are built-in HLSL shader 'blobs'. They are compiled at build time using the `CompileShader` batch file, and then included as binary data into the C++ library directly. They are built in many combinations to support different rendering options:

- Rendering without textures
- Render with per-vertex colors
- Rendering with textures
- Rendering with 1 light
- Rendering with 3 lights
- Rendering with per-pixel lighting
- Rendering with fog
- Various combinations of these features
