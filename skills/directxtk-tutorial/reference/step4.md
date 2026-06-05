# Step 4: Sprites and Textures

This step adds 2D sprite rendering using `SpriteBatch` and loads a texture with `WICTextureLoader`. See the wiki page: [Sprites and textures](https://github.com/microsoft/DirectXTK/wiki/Sprites-and-textures).

## Setup

Copy the `cat.png` file from this skill's `assets/` folder into the project's working directory (next to the executable, or in the project root for CMake builds):

```powershell
Copy-Item "<SkillDir>\assets\cat.png" -Destination "<ProjectDir>"
```

## Code Changes

### pch.h

Ensure the following includes are in `pch.h`:

```cpp
#include <directxtk/CommonStates.h>
#include <directxtk/SimpleMath.h>
#include <directxtk/SpriteBatch.h>
#include <directxtk/WICTextureLoader.h>
```

### Game.h

Add the following private member variables to the `Game` class:

```cpp
std::unique_ptr<DirectX::CommonStates> m_states;
std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
DirectX::SimpleMath::Vector2 m_screenPos;
DirectX::SimpleMath::Vector2 m_origin;
```

### Game.cpp — CreateDeviceDependentResources

In `CreateDeviceDependentResources`, create the `SpriteBatch`, load the texture, and compute its origin:

```cpp
auto context = m_deviceResources->GetD3DDeviceContext();
m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);

auto device = m_deviceResources->GetD3DDevice();
m_states = std::make_unique<DirectX::CommonStates>(device);

ComPtr<ID3D11Resource> resource;
DX::ThrowIfFailed(
    DirectX::CreateWICTextureFromFile(device, L"cat.png",
        resource.GetAddressOf(),
        m_texture.ReleaseAndGetAddressOf()));

ComPtr<ID3D11Texture2D> cat;
DX::ThrowIfFailed(resource.As(&cat));

CD3D11_TEXTURE2D_DESC catDesc;
cat->GetDesc(&catDesc);

m_origin.x = float(catDesc.Width / 2);
m_origin.y = float(catDesc.Height / 2);
```

### Game.cpp — CreateWindowSizeDependentResources

In `CreateWindowSizeDependentResources`, compute the screen center position:

```cpp
auto size = m_deviceResources->GetOutputSize();
m_screenPos.x = float(size.right) / 2.f;
m_screenPos.y = float(size.bottom) / 2.f;
```

### Game.cpp — Render

In the `Render` method, after clearing the render target and before `Present`, draw the sprite:

```cpp
m_spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, m_states->NonPremultiplied());

m_spriteBatch->Draw(m_texture.Get(), m_screenPos, nullptr,
    DirectX::Colors::White, 0.f, m_origin);

m_spriteBatch->End();
```

### Game.cpp — OnDeviceLost

In `OnDeviceLost`, release the resources:

```cpp
m_states.reset();
m_spriteBatch.reset();
m_texture.Reset();
```

## Build and Verify

Build the project using the commands from [Step 2](step2.md). To run the application, make sure the working directory is set to the project folder (where `cat.png` is located):

```powershell
cd <ProjectDir>
```

For MSBuild, the executable will be in a subdirectory like `x64\Debug\<ProjectName>.exe`. For CMake, it will be in `out\build\x64-Debug\<ProjectName>.exe` (or the ARM64 equivalent).

When you run the application, you should see the cat image rendered in the center of the window on top of the cornflower blue background.

## Further Reading

For more things you can do with sprites at this point:

- [Sprites and textures](https://github.com/microsoft/DirectXTK/wiki/Sprites-and-textures)
  - Using DDS files for textures
  - Using pre-multiplied alpha
  - Rotating a sprite
  - Scaling a sprite
  - Tinting a sprite
  - Tiling a sprite
  - Stretching a sprite
  - Drawing a background image
- [More tricks with sprites](https://github.com/microsoft/DirectXTK/wiki/More-tricks-with-sprites)
  - Animating sprites
  - Creating a scrolling background
  - More to explore

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

### SpriteBatch

`SpriteBatch` is a high-performance batched 2D renderer. Under the hood it creates and manages:

- **`ID3D11VertexShader`** and **`ID3D11PixelShader`** — Built-in HLSL shaders for textured quad rendering (compiled and embedded at build time).
- **`ID3D11InputLayout`** — Describes the per-vertex data layout (position, color, texture coordinates).
- **`ID3D11Buffer`** (dynamic vertex buffer) — Holds batched sprite quads. Uses `Map`/`Unmap` with `D3D11_MAP_WRITE_DISCARD` or `D3D11_MAP_WRITE_NO_OVERWRITE` for efficient streaming.
- **`ID3D11Buffer`** (index buffer) — Pre-generated indices for drawing quads as triangle lists.
- **`ID3D11Buffer`** (constant buffer) — Contains the orthographic projection matrix for screen-space rendering.

When you call `Begin()`/`End()`, SpriteBatch sets the full pipeline state: blend state (`OMSetBlendState`), depth/stencil state (`OMSetDepthStencilState`), rasterizer state (`RSSetState`), sampler (`PSSetSamplers`), shaders, input layout, and buffers. It then issues `DrawIndexed` calls, batching sprites that share the same texture into a single draw call.

### CommonStates

`CommonStates` is a lazy cache of reusable fixed-function pipeline state objects:

- **`ID3D11BlendState`** — `Opaque`, `AlphaBlend`, `Additive`, `NonPremultiplied`. The `NonPremultiplied` state used here applies standard alpha blending for textures with straight (non-premultiplied) alpha.
- **`ID3D11DepthStencilState`** — `DepthNone`, `DepthDefault`, `DepthRead`, etc.
- **`ID3D11RasterizerState`** — `CullNone`, `CullClockwise`, `CullCounterClockwise`, `Wireframe`.
- **`ID3D11SamplerState`** — `PointWrap`, `PointClamp`, `LinearWrap`, `LinearClamp`, `AnisotropicWrap`, `AnisotropicClamp`.

Each state object is created on first access via `ID3D11Device::CreateBlendState` (or the corresponding `Create*State` method) and cached for reuse.

### WICTextureLoader

`CreateWICTextureFromFile` loads an image file using the Windows Imaging Component (WIC) and creates D3D11 resources:

- Uses **Windows Imaging Component (WIC)** (`IWICImagingFactory`, `IWICBitmapFrameDecode`, `IWICFormatConverter`) to decode the image file and convert pixel formats.
- Creates an **`ID3D11Texture2D`** with `D3D11_USAGE_IMMUTABLE` (or `DEFAULT` if mipmaps are requested) via `ID3D11Device::CreateTexture2D`.
- Creates an **`ID3D11ShaderResourceView`** so the texture can be bound to pixel shader slots for sampling.
- For auto-generated mipmaps, uses `ID3D11DeviceContext::GenerateMips` after uploading the base level with `UpdateSubresource`.

### Shaders

The SpriteBatch shaders are built-in HLSL shader 'blobs'. They are compiled at build time using the `CompileShader` batch file, and then included as binary data into the C++ library directly.

The shaders in this case are fairly trivial:

***Vertex Shader***

```hlsl
cbuffer Parameters : register(b0)
{
    row_major float4x4 MatrixTransform;
};


void SpriteVertexShader(inout float4 color    : COLOR0,
    inout float2 texCoord : TEXCOORD0,
    inout float4 position : SV_Position)
{
    position = mul(position, MatrixTransform);
}
```

***Pixel Shader***

```hlsl
Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 SpritePixelShader(float4 color : COLOR0, float2 texCoord : TEXCOORD0) : SV_Target
{
    return Texture.Sample(Sampler, texCoord) * color;
}
```
