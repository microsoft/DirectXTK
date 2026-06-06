# Step 5: Drawing Text

This step adds text rendering using `SpriteFont` and `SpriteBatch`. See the wiki page: [Drawing text](https://github.com/microsoft/DirectXTK/wiki/Drawing-text).

## Setup

Copy the `courier.spritefont` file from this skill's `assets/` folder into the project's working directory:

```powershell
Copy-Item "<SkillDir>\assets\courier.spritefont" -Destination "<ProjectDir>"
```

> For details on how the sprite font is created, see the [MakeSpriteFont](https://github.com/microsoft/DirectXTK/wiki/MakeSpriteFont) wiki page.

## Code Changes

### pch.h

Add the following include to `pch.h` (after the existing DirectXTK includes):

```cpp
#include <directxtk/SpriteFont.h>
```

### Game.h

Add the following private member variable to the `Game` class:

```cpp
std::unique_ptr<DirectX::SpriteFont> m_font;
```

### Game.cpp — CreateDeviceDependentResources

In `CreateDeviceDependentResources`, load the sprite font:

```cpp
m_font = std::make_unique<DirectX::SpriteFont>(device, L"courier.spritefont");
```

### Game.cpp — Render

In the `Render` method, inside the existing `SpriteBatch` Begin/End block (after the sprite draw), add text rendering:

```cpp
m_spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, m_states->NonPremultiplied());

m_spriteBatch->Draw(m_texture.Get(), m_screenPos, nullptr,
    DirectX::Colors::White, 0.f, m_origin);

m_font->DrawString(m_spriteBatch.get(), L"Hello, World!",
    DirectX::XMFLOAT2(10.f, 10.f), DirectX::Colors::Yellow);

m_spriteBatch->End();
```

### Game.cpp — OnDeviceLost

In `OnDeviceLost`, release the font:

```cpp
m_font.reset();
```

## Build and Verify

Build the project using the commands from [Step 2](step2.md). Run the application from the project folder (where `courier.spritefont` is located).

You should see "Hello, World!" rendered in yellow text in the top-left corner of the window, with the cat sprite still centered.

## Further Reading

- [Drawing text](https://github.com/microsoft/DirectXTK/wiki/Drawing-text)
  - Creating a font
  - Using std::wstring for text
  - Using ASCII text
  - Drop-shadow effect
  - Outline effect
  - More to explore

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

### SpriteFont

`SpriteFont` is a text rendering class that works in conjunction with `SpriteBatch`. It manages:

- **Font atlas** — A single **`ID3D11Texture2D`** (with `D3D11_USAGE_IMMUTABLE`) containing all pre-rendered glyphs packed into a texture atlas, plus an **`ID3D11ShaderResourceView`** for shader access.
- **Glyph data** — A sorted array of `Glyph` structs (loaded from the `.spritefont` binary format) containing character code, source rectangle in the atlas, and advance/offset metrics.

**Text rendering pipeline:**

1. `DrawString` iterates the input text character by character via an internal `ForEachGlyph` helper.
2. For each character, it performs a binary search to find the corresponding `Glyph` (falling back to a default character if not found).
3. It computes the destination position using the glyph offset, advance width, and any rotation/scale/origin transforms.
4. It calls `SpriteBatch::Draw` for each glyph, passing the font atlas SRV and the glyph's source rectangle.
5. `SpriteBatch` batches all glyph quads together (since they share the same atlas texture) and renders them in a single `DrawIndexed` call.

The `.spritefont` binary format is produced by the **MakeSpriteFont** tool, which rasterizes a TrueType/OpenType font at a specific size and packs the glyphs into an atlas.
