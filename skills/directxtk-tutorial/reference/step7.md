# Step 7: 3D Shapes

This step introduces rendering 3D geometry using `GeometricPrimitive`. We create and render the classic Utah teapot with default lighting and a simple rotation animation. See the wiki page: [3D shapes](https://github.com/microsoft/DirectXTK/wiki/3D-shapes).

## Background

In the previous step, we used `PrimitiveBatch` to draw geometry with manually specified vertices. `GeometricPrimitive` takes a different approach — it procedurally generates common 3D shapes (sphere, cube, torus, teapot, etc.) and stores them in efficient **static** vertex and index buffers on the GPU.

Key differences from `PrimitiveBatch`:

| | PrimitiveBatch | GeometricPrimitive |
| --- | --- | --- |
| Buffer type | Dynamic (CPU-writable each frame) | Static (GPU-only, created once) |
| Index buffer | Optional | Always used |
| Shader setup | Manual (you set effect + input layout) | Built-in default or custom effect |
| Use case | Immediate-mode debug/simple drawing | Efficient 3D shape rendering |

## Code Changes

### Game.cpp — Render (comment out previous step)

Comment out or remove the triangle rendering code from Step 6:

```cpp
// context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
// context->OMSetDepthStencilState(m_states->DepthNone(), 0);
// context->RSSetState(m_states->CullNone());
// m_effect->Apply(context);
// context->IASetInputLayout(m_inputLayout.Get());
// m_batch->Begin();
// ...
// m_batch->End();
```

### pch.h

Ensure the following includes are in `pch.h`:

```cpp
#include <directxtk/GeometricPrimitive.h>
#include <directxtk/SimpleMath.h>
```

### Game.h

Add the following private member variables to the `Game` class:

```cpp
DirectX::SimpleMath::Matrix m_world;
DirectX::SimpleMath::Matrix m_view;
DirectX::SimpleMath::Matrix m_proj;

std::unique_ptr<DirectX::GeometricPrimitive> m_shape;
```

### Game.cpp — CreateDeviceDependentResources

In `CreateDeviceDependentResources`, create the teapot shape:

```cpp
auto context = m_deviceResources->GetD3DDeviceContext();
m_shape = DirectX::GeometricPrimitive::CreateTeapot(context);

m_world = DirectX::SimpleMath::Matrix::Identity;
```

### Game.cpp — CreateWindowSizeDependentResources

In `CreateWindowSizeDependentResources`, set up a 3D camera with a perspective projection:

```cpp
auto size = m_deviceResources->GetOutputSize();

m_view = DirectX::SimpleMath::Matrix::CreateLookAt(
    DirectX::SimpleMath::Vector3(2.f, 2.f, 2.f),
    DirectX::SimpleMath::Vector3::Zero,
    DirectX::SimpleMath::Vector3::UnitY);

m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
    DirectX::XM_PI / 4.f,
    float(size.right) / float(size.bottom),
    0.1f, 10.f);
```

### Game.cpp — Update

In `Update`, rotate the teapot over time:

```cpp
auto time = static_cast<float>(timer.GetTotalSeconds());

m_world = DirectX::SimpleMath::Matrix::CreateRotationZ(cosf(time) * 2.f);
```

### Game.cpp — Render

In the `Render` method, after clearing the render target and before `Present`, draw the teapot:

```cpp
m_shape->Draw(m_world, m_view, m_proj);
```

> That's it! `GeometricPrimitive::Draw` handles setting up a default `BasicEffect` with lighting, the input layout, render states, and issuing the indexed draw call — all in one line.

### Game.cpp — OnDeviceLost

In `OnDeviceLost`, release the shape:

```cpp
m_shape.reset();
```

## Build and Verify

Build the project using the commands from [Step 2](step2.md). When you run the application, you should see a white, lit Utah teapot rotating on the cornflower blue background.

The teapot is rendered with default lighting (three directional lights) and backface culling, giving it a solid 3D appearance.

## Available Shapes

`GeometricPrimitive` can create many shapes. Replace `CreateTeapot` with any of these:

| Factory Method | Shape |
| --- | --- |
| `CreateSphere` | UV sphere |
| `CreateGeoSphere` | Geodesic sphere |
| `CreateCube` | Unit cube |
| `CreateBox` | Axis-aligned box with custom dimensions |
| `CreateCylinder` | Cylinder |
| `CreateCone` | Cone |
| `CreateTorus` | Torus (donut) |
| `CreateTetrahedron` | Tetrahedron (4 faces) |
| `CreateOctahedron` | Octahedron (8 faces) |
| `CreateDodecahedron` | Dodecahedron (12 faces) |
| `CreateIcosahedron` | Icosahedron (20 faces) |
| `CreateTeapot` | Utah teapot |

## Next Steps

See [Next Steps](nextsteps.md) for where to go from here.

## Further Reading

- [3D shapes](https://github.com/microsoft/DirectXTK/wiki/3D-shapes)
  - Adding textures to shapes
  - Using custom lighting and effects
  - Custom vertex formats
  - Custom geometry
- [Rendering a model](https://github.com/microsoft/DirectXTK/wiki/Rendering-a-model)

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

### GeometricPrimitive

`GeometricPrimitive` procedurally generates indexed triangle meshes and stores them in GPU buffers:

- **`ID3D11Buffer`** (static vertex buffer) — Created with `D3D11_USAGE_IMMUTABLE` containing `VertexPositionNormalTexture` data (position, normal, and texture coordinates). Because it's immutable, it resides entirely in fast GPU memory.
- **`ID3D11Buffer`** (static index buffer) — Created with `D3D11_USAGE_IMMUTABLE` containing `uint16_t` indices. Indexed rendering reduces vertex count by sharing vertices between adjacent triangles.
- **Internal `BasicEffect`** — When using the simple `Draw(world, view, proj)` overload, `GeometricPrimitive` creates and manages its own `BasicEffect` with default lighting enabled. It also creates its own `ID3D11InputLayout` matching `VertexPositionNormalTexture`.

### The 3D Camera

This step introduces a proper 3D camera setup:

- **View matrix** (`CreateLookAt`) — Defines where the camera is (eye position), what it's looking at (focus point), and which direction is "up". This transforms world-space coordinates into camera-space (view-space).
- **Projection matrix** (`CreatePerspectiveFieldOfView`) — Defines the camera's field of view, aspect ratio, and near/far clipping planes. This transforms view-space into clip-space, applying perspective foreshortening so distant objects appear smaller.

Together, these matrices transform the teapot's local vertex positions through: **World → View → Projection → Clip Space** → (GPU viewport transform) → **Screen pixels**.

### Static vs. Dynamic Buffers

In Step 6, `PrimitiveBatch` used `D3D11_USAGE_DYNAMIC` buffers that the CPU writes to every frame. In this step, `GeometricPrimitive` uses `D3D11_USAGE_IMMUTABLE` buffers:

- **Dynamic**: CPU writes vertex data each frame via `Map`/`Unmap`. Good for geometry that changes per-frame (debug lines, UI).
- **Immutable**: Created once with initial data, never modified. The GPU can optimize storage and access patterns. Much faster for fixed geometry.

The transformation (rotation) is applied via the world matrix in the constant buffer — the vertex data itself never changes.

### The Utah Teapot

The [Utah teapot](https://en.wikipedia.org/wiki/Utah_teapot) is a classic test model in computer graphics, originally created by Martin Newell in 1975. DirectX Tool Kit generates it from Bézier patch data, tessellated to the specified level (default: 8).
