//--------------------------------------------------------------------------------------
// File: SpriteFont.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"

#include <algorithm>
#include <vector>

#include "SpriteFont.h"
#include "DirectXHelpers.h"
#include "BinaryReader.h"
#include "LoaderHelpers.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;


// Internal SpriteFont implementation class.
class SpriteFont::Impl
{
public:
    Impl(_In_ ID3D11Device* device, _In_ BinaryReader* reader, bool forceSRGB);
    Impl(_In_ ID3D11ShaderResourceView* texture, _In_reads_(glyphCount) Glyph const* glyphs, _In_ size_t glyphCount, _In_ float lineSpacing);

    Glyph const* FindGlyph(wchar_t character) const;

    void SetDefaultCharacter(wchar_t character);

    template<typename TAction>
    void ForEachGlyph(_In_z_ wchar_t const* text, TAction action) const;


    // Fields.
    ComPtr<ID3D11ShaderResourceView> texture;
    std::vector<Glyph> glyphs;
    Glyph const* defaultGlyph;
    float lineSpacing;
};


// Constants.
const XMFLOAT2 SpriteFont::Float2Zero(0, 0);

static const char spriteFontMagic[] = "DXTKfont";


// Comparison operators make our sorted glyph vector work with std::binary_search and lower_bound.
namespace DirectX
{
    static inline bool operator< (SpriteFont::Glyph const& left, SpriteFont::Glyph const& right)
    {
        return left.Character < right.Character;
    }

    static inline bool operator< (wchar_t left, SpriteFont::Glyph const& right)
    {
        return left < right.Character;
    }

    static inline bool operator< (SpriteFont::Glyph const& left, wchar_t right)
    {
        return left.Character < right;
    }
}


// Reads a SpriteFont from the binary format created by the MakeSpriteFont utility.
SpriteFont::Impl::Impl(_In_ ID3D11Device* device, _In_ BinaryReader* reader, bool forceSRGB) :
    defaultGlyph(nullptr)
{
    // Validate the header.
    for (char const* magic = spriteFontMagic; *magic; magic++)
    {
        if (reader->Read<uint8_t>() != *magic)
        {
            DebugTrace("SpriteFont provided with an invalid .spritefont file\n");
            throw std::exception("Not a MakeSpriteFont output binary");
        }
    }

    // Read the glyph data.
    auto glyphCount = reader->Read<uint32_t>();
    auto glyphData = reader->ReadArray<Glyph>(glyphCount);

    glyphs.assign(glyphData, glyphData + glyphCount);

    // Read font properties.
    lineSpacing = reader->Read<float>();

    SetDefaultCharacter((wchar_t)reader->Read<uint32_t>());

    // Read the texture data.
    auto textureWidth = reader->Read<uint32_t>();
    auto textureHeight = reader->Read<uint32_t>();
    auto textureFormat = reader->Read<DXGI_FORMAT>();
    auto textureStride = reader->Read<uint32_t>();
    auto textureRows = reader->Read<uint32_t>();
    auto textureData = reader->ReadArray<uint8_t>(textureStride * textureRows);

    if (forceSRGB)
    {
        textureFormat = LoaderHelpers::MakeSRGB(textureFormat);
    }

    // Create the D3D texture.
    CD3D11_TEXTURE2D_DESC textureDesc(textureFormat, textureWidth, textureHeight, 1, 1, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE);
    CD3D11_SHADER_RESOURCE_VIEW_DESC viewDesc(D3D11_SRV_DIMENSION_TEXTURE2D, textureFormat);
    D3D11_SUBRESOURCE_DATA initData = { textureData, textureStride };
    ComPtr<ID3D11Texture2D> texture2D;

    ThrowIfFailed(
        device->CreateTexture2D(&textureDesc, &initData, &texture2D)
    );

    ThrowIfFailed(
        device->CreateShaderResourceView(texture2D.Get(), &viewDesc, &texture)
    );

    SetDebugObjectName(texture.Get(), "DirectXTK:SpriteFont");
    SetDebugObjectName(texture2D.Get(), "DirectXTK:SpriteFont");
}


// Constructs a SpriteFont from arbitrary user specified glyph data.
_Use_decl_annotations_
SpriteFont::Impl::Impl(ID3D11ShaderResourceView* texture, Glyph const* glyphs, size_t glyphCount, float lineSpacing)
    : texture(texture),
    glyphs(glyphs, glyphs + glyphCount),
    defaultGlyph(nullptr),
    lineSpacing(lineSpacing)
{
    if (!std::is_sorted(glyphs, glyphs + glyphCount))
    {
        throw std::exception("Glyphs must be in ascending codepoint order");
    }
}


// Looks up the requested glyph, falling back to the default character if it is not in the font.
SpriteFont::Glyph const* SpriteFont::Impl::FindGlyph(wchar_t character) const
{
    auto glyph = std::lower_bound(glyphs.begin(), glyphs.end(), character);

    if (glyph != glyphs.end() && glyph->Character == character)
    {
        return &*glyph;
    }

    if (defaultGlyph)
    {
        return defaultGlyph;
    }

    DebugTrace("SpriteFont encountered a character not in the font (%u, %C), and no default glyph was provided\n", character, character);
    throw std::exception("Character not in font");
}


// Sets the missing-character fallback glyph.
void SpriteFont::Impl::SetDefaultCharacter(wchar_t character)
{
    defaultGlyph = nullptr;

    if (character)
    {
        defaultGlyph = FindGlyph(character);
    }
}


// The core glyph layout algorithm, shared between DrawString and MeasureString.
template<typename TAction>
void SpriteFont::Impl::ForEachGlyph(_In_z_ wchar_t const* text, TAction action) const
{
    float x = 0;
    float y = 0;

    for (; *text; text++)
    {
        wchar_t character = *text;

        switch (character)
        {
            case '\r':
                // Skip carriage returns.
                continue;

            case '\n':
                // New line.
                x = 0;
                y += lineSpacing;
                break;

            default:
                // Output this character.
                auto glyph = FindGlyph(character);

                x += glyph->XOffset;

                if (x < 0)
                    x = 0;

                float advance = glyph->Subrect.right - glyph->Subrect.left + glyph->XAdvance;

                if (!iswspace(character)
                    || ((glyph->Subrect.right - glyph->Subrect.left) > 1)
                    || ((glyph->Subrect.bottom - glyph->Subrect.top) > 1))
                {
                    action(glyph, x, y, advance);
                }

                x += advance;
                break;
        }
    }
}


// Construct from a binary file created by the MakeSpriteFont utility.
SpriteFont::SpriteFont(_In_ ID3D11Device* device, _In_z_ wchar_t const* fileName, bool forceSRGB)
{
    BinaryReader reader(fileName);

    pImpl = std::make_unique<Impl>(device, &reader, forceSRGB);
}


// Construct from a binary blob created by the MakeSpriteFont utility and already loaded into memory.
_Use_decl_annotations_
SpriteFont::SpriteFont(ID3D11Device* device, uint8_t const* dataBlob, size_t dataSize, bool forceSRGB)
{
    BinaryReader reader(dataBlob, dataSize);

    pImpl = std::make_unique<Impl>(device, &reader, forceSRGB);
}


// Construct from arbitrary user specified glyph data (for those not using the MakeSpriteFont utility).
_Use_decl_annotations_
SpriteFont::SpriteFont(ID3D11ShaderResourceView* texture, Glyph const* glyphs, size_t glyphCount, float lineSpacing)
    : pImpl(std::make_unique<Impl>(texture, glyphs, glyphCount, lineSpacing))
{
}


// Move constructor.
SpriteFont::SpriteFont(SpriteFont&& moveFrom) throw()
    : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
SpriteFont& SpriteFont::operator= (SpriteFont&& moveFrom) throw()
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
SpriteFont::~SpriteFont()
{
}


void XM_CALLCONV SpriteFont::DrawString(_In_ SpriteBatch* spriteBatch, _In_z_ wchar_t const* text, XMFLOAT2 const& position, FXMVECTOR color, float rotation, XMFLOAT2 const& origin, float scale, SpriteEffects effects, float layerDepth) const
{
    DrawString(spriteBatch, text, XMLoadFloat2(&position), color, rotation, XMLoadFloat2(&origin), XMVectorReplicate(scale), effects, layerDepth);
}


void XM_CALLCONV SpriteFont::DrawString(_In_ SpriteBatch* spriteBatch, _In_z_ wchar_t const* text, XMFLOAT2 const& position, FXMVECTOR color, float rotation, XMFLOAT2 const& origin, XMFLOAT2 const& scale, SpriteEffects effects, float layerDepth) const
{
    DrawString(spriteBatch, text, XMLoadFloat2(&position), color, rotation, XMLoadFloat2(&origin), XMLoadFloat2(&scale), effects, layerDepth);
}


void XM_CALLCONV SpriteFont::DrawString(_In_ SpriteBatch* spriteBatch, _In_z_ wchar_t const* text, FXMVECTOR position, FXMVECTOR color, float rotation, FXMVECTOR origin, float scale, SpriteEffects effects, float layerDepth) const
{
    DrawString(spriteBatch, text, position, color, rotation, origin, XMVectorReplicate(scale), effects, layerDepth);
}


void XM_CALLCONV SpriteFont::DrawString(_In_ SpriteBatch* spriteBatch, _In_z_ wchar_t const* text, FXMVECTOR position, FXMVECTOR color, float rotation, FXMVECTOR origin, GXMVECTOR scale, SpriteEffects effects, float layerDepth) const
{
    static_assert(SpriteEffects_FlipHorizontally == 1 &&
                  SpriteEffects_FlipVertically == 2, "If you change these enum values, the following tables must be updated to match");

    // Lookup table indicates which way to move along each axis per SpriteEffects enum value.
    static XMVECTORF32 axisDirectionTable[4] =
    {
        { { { -1, -1, 0, 0 } } },
        { { {  1, -1, 0, 0 } } },
        { { { -1,  1, 0, 0 } } },
        { { {  1,  1, 0, 0 } } },
    };

    // Lookup table indicates which axes are mirrored for each SpriteEffects enum value.
    static XMVECTORF32 axisIsMirroredTable[4] =
    {
        { { { 0, 0, 0, 0 } } },
        { { { 1, 0, 0, 0 } } },
        { { { 0, 1, 0, 0 } } },
        { { { 1, 1, 0, 0 } } },
    };

    XMVECTOR baseOffset = origin;

    // If the text is mirrored, offset the start position accordingly.
    if (effects)
    {
        baseOffset -= MeasureString(text) * axisIsMirroredTable[effects & 3];
    }

    // Draw each character in turn.
    pImpl->ForEachGlyph(text, [&](Glyph const* glyph, float x, float y, float advance)
    {
        UNREFERENCED_PARAMETER(advance);

        XMVECTOR offset = XMVectorMultiplyAdd(XMVectorSet(x, y + glyph->YOffset, 0, 0), axisDirectionTable[effects & 3], baseOffset);

        if (effects)
        {
            // For mirrored characters, specify bottom and/or right instead of top left.
            XMVECTOR glyphRect = XMConvertVectorIntToFloat(XMLoadInt4(reinterpret_cast<uint32_t const*>(&glyph->Subrect)), 0);

            // xy = glyph width/height.
            glyphRect = XMVectorSwizzle<2, 3, 0, 1>(glyphRect) - glyphRect;

            offset = XMVectorMultiplyAdd(glyphRect, axisIsMirroredTable[effects & 3], offset);
        }

        spriteBatch->Draw(pImpl->texture.Get(), position, &glyph->Subrect, color, rotation, offset, scale, effects, layerDepth);
    });
}


XMVECTOR XM_CALLCONV SpriteFont::MeasureString(_In_z_ wchar_t const* text) const
{
    XMVECTOR result = XMVectorZero();

    pImpl->ForEachGlyph(text, [&](Glyph const* glyph, float x, float y, float advance)
    {
        UNREFERENCED_PARAMETER(advance);

        float w = (float)(glyph->Subrect.right - glyph->Subrect.left);
        float h = (float)(glyph->Subrect.bottom - glyph->Subrect.top) + glyph->YOffset;

        h = std::max(h, pImpl->lineSpacing);

        result = XMVectorMax(result, XMVectorSet(x + w, y + h, 0, 0));
    });

    return result;
}


RECT SpriteFont::MeasureDrawBounds(_In_z_ wchar_t const* text, XMFLOAT2 const& position) const
{
    RECT result = { LONG_MAX, LONG_MAX, 0, 0 };

    pImpl->ForEachGlyph(text, [&](Glyph const* glyph, float x, float y, float advance)
    {
        float w = (float)(glyph->Subrect.right - glyph->Subrect.left);
        float h = (float)(glyph->Subrect.bottom - glyph->Subrect.top);

        float minX = position.x + x;
        float minY = position.y + y + glyph->YOffset;

        float maxX = std::max(minX + advance, minX + w);
        float maxY = minY + h;

        if (minX < result.left)
            result.left = long(minX);

        if (minY < result.top)
            result.top = long(minY);

        if (result.right < maxX)
            result.right = long(maxX);

        if (result.bottom < maxY)
            result.bottom = long(maxY);
    });

    if (result.left == LONG_MAX)
    {
        result.left = 0;
        result.top = 0;
    }

    return result;
}


RECT XM_CALLCONV SpriteFont::MeasureDrawBounds(_In_z_ wchar_t const* text, FXMVECTOR position) const
{
    XMFLOAT2 pos;
    XMStoreFloat2(&pos, position);

    return MeasureDrawBounds(text, pos);
}


// Spacing properties
float SpriteFont::GetLineSpacing() const
{
    return pImpl->lineSpacing;
}


void SpriteFont::SetLineSpacing(float spacing)
{
    pImpl->lineSpacing = spacing;
}


// Font properties
wchar_t SpriteFont::GetDefaultCharacter() const
{
    return pImpl->defaultGlyph ? (wchar_t)pImpl->defaultGlyph->Character : 0;
}


void SpriteFont::SetDefaultCharacter(wchar_t character)
{
    pImpl->SetDefaultCharacter(character);
}


bool SpriteFont::ContainsCharacter(wchar_t character) const
{
    return std::binary_search(pImpl->glyphs.begin(), pImpl->glyphs.end(), character);
}


// Custom layout/rendering
SpriteFont::Glyph const* SpriteFont::FindGlyph(wchar_t character) const
{
    return pImpl->FindGlyph(character);
}


void SpriteFont::GetSpriteSheet(ID3D11ShaderResourceView** texture) const
{
    if (!texture)
        return;

    ThrowIfFailed(pImpl->texture.CopyTo(texture));
}
