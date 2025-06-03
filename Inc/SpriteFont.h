//--------------------------------------------------------------------------------------
// File: SpriteFont.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#include "SpriteBatch.h"

#include <cstddef>
#include <cstdint>
#include <memory>

#ifndef DIRECTX_TOOLKIT_API
#ifdef DIRECTX_TOOLKIT_EXPORT
#ifdef __GNUC__
#define DIRECTX_TOOLKIT_API __attribute__ ((dllexport))
#else
#define DIRECTX_TOOLKIT_API __declspec(dllexport)
#endif
#elif defined(DIRECTX_TOOLKIT_IMPORT)
#ifdef __GNUC__
#define DIRECTX_TOOLKIT_API __attribute__ ((dllimport))
#else
#define DIRECTX_TOOLKIT_API __declspec(dllimport)
#endif
#else
#define DIRECTX_TOOLKIT_API
#endif
#endif


namespace DirectX
{
    inline namespace DX11
    {
        class SpriteFont
        {
        public:
            struct Glyph;

            DIRECTX_TOOLKIT_API SpriteFont(
                _In_ ID3D11Device* device,
                _In_z_ wchar_t const* fileName,
                bool forceSRGB = false);
            DIRECTX_TOOLKIT_API SpriteFont(
                _In_ ID3D11Device* device,
                _In_reads_bytes_(dataSize) uint8_t const* dataBlob, _In_ size_t dataSize,
                bool forceSRGB = false);
            DIRECTX_TOOLKIT_API SpriteFont(
                _In_ ID3D11ShaderResourceView* texture,
                _In_reads_(glyphCount) Glyph const* glyphs, _In_ size_t glyphCount,
                _In_ float lineSpacing);

            DIRECTX_TOOLKIT_API SpriteFont(SpriteFont&&) noexcept;
            DIRECTX_TOOLKIT_API SpriteFont& operator= (SpriteFont&&) noexcept;

            SpriteFont(SpriteFont const&) = delete;
            SpriteFont& operator= (SpriteFont const&) = delete;

            DIRECTX_TOOLKIT_API virtual ~SpriteFont();

            // Wide-character / UTF-16LE
            DIRECTX_TOOLKIT_API void XM_CALLCONV DrawString(
                _In_ SpriteBatch* spriteBatch,
                _In_z_ wchar_t const* text,
                XMFLOAT2 const& position,
                FXMVECTOR color = Colors::White, float rotation = 0, XMFLOAT2 const& origin = Float2Zero, float scale = 1,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0) const;
            DIRECTX_TOOLKIT_API void XM_CALLCONV DrawString(
                _In_ SpriteBatch* spriteBatch,
                _In_z_ wchar_t const* text,
                XMFLOAT2 const& position,
                FXMVECTOR color, float rotation, XMFLOAT2 const& origin, XMFLOAT2 const& scale,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0) const;
            DIRECTX_TOOLKIT_API void XM_CALLCONV DrawString(
                _In_ SpriteBatch* spriteBatch,
                _In_z_ wchar_t const* text,
                FXMVECTOR position,
                FXMVECTOR color = Colors::White, float rotation = 0, FXMVECTOR origin = g_XMZero, float scale = 1,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0) const;
            DIRECTX_TOOLKIT_API void XM_CALLCONV DrawString(
                _In_ SpriteBatch* spriteBatch,
                _In_z_ wchar_t const* text,
                FXMVECTOR position,
                FXMVECTOR color, float rotation, FXMVECTOR origin, GXMVECTOR scale,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0) const;

            DIRECTX_TOOLKIT_API XMVECTOR XM_CALLCONV MeasureString(
                _In_z_ wchar_t const* text,
                bool ignoreWhitespace = true) const;

            DIRECTX_TOOLKIT_API RECT __cdecl MeasureDrawBounds(
                _In_z_ wchar_t const* text,
                XMFLOAT2 const& position,
                bool ignoreWhitespace = true) const;
            DIRECTX_TOOLKIT_API RECT XM_CALLCONV MeasureDrawBounds(
                _In_z_ wchar_t const* text,
                FXMVECTOR position,
                bool ignoreWhitespace = true) const;

            // UTF-8
            DIRECTX_TOOLKIT_API void XM_CALLCONV DrawString(
                _In_ SpriteBatch* spriteBatch,
                _In_z_ char const* text,
                XMFLOAT2 const& position,
                FXMVECTOR color = Colors::White, float rotation = 0, XMFLOAT2 const& origin = Float2Zero, float scale = 1,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0) const;
            DIRECTX_TOOLKIT_API void XM_CALLCONV DrawString(
                _In_ SpriteBatch* spriteBatch,
                _In_z_ char const* text,
                XMFLOAT2 const& position,
                FXMVECTOR color, float rotation, XMFLOAT2 const& origin, XMFLOAT2 const& scale,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0) const;
            DIRECTX_TOOLKIT_API void XM_CALLCONV DrawString(
                _In_ SpriteBatch* spriteBatch,
                _In_z_ char const* text,
                FXMVECTOR position,
                FXMVECTOR color = Colors::White, float rotation = 0, FXMVECTOR origin = g_XMZero, float scale = 1,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0) const;
            DIRECTX_TOOLKIT_API void XM_CALLCONV DrawString(
                _In_ SpriteBatch* spriteBatch,
                _In_z_ char const* text,
                FXMVECTOR position,
                FXMVECTOR color, float rotation, FXMVECTOR origin, GXMVECTOR scale,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0) const;

            DIRECTX_TOOLKIT_API XMVECTOR XM_CALLCONV MeasureString(
                _In_z_ char const* text,
                bool ignoreWhitespace = true) const;

            DIRECTX_TOOLKIT_API RECT __cdecl MeasureDrawBounds(
                _In_z_ char const* text,
                XMFLOAT2 const& position,
                bool ignoreWhitespace = true) const;
            DIRECTX_TOOLKIT_API RECT XM_CALLCONV MeasureDrawBounds(
                _In_z_ char const* text,
                FXMVECTOR position,
                bool ignoreWhitespace = true) const;

            // Spacing properties
            DIRECTX_TOOLKIT_API float __cdecl GetLineSpacing() const noexcept;
            DIRECTX_TOOLKIT_API void __cdecl SetLineSpacing(float spacing);

            // Font properties
            DIRECTX_TOOLKIT_API wchar_t __cdecl GetDefaultCharacter() const noexcept;
            DIRECTX_TOOLKIT_API void __cdecl SetDefaultCharacter(wchar_t character);

            DIRECTX_TOOLKIT_API bool __cdecl ContainsCharacter(wchar_t character) const;

            // Custom layout/rendering
            DIRECTX_TOOLKIT_API Glyph const* __cdecl FindGlyph(wchar_t character) const;
            DIRECTX_TOOLKIT_API void __cdecl GetSpriteSheet(ID3D11ShaderResourceView** texture) const;

            // Describes a single character glyph.
            struct Glyph
            {
                uint32_t Character;
                RECT Subrect;
                float XOffset;
                float YOffset;
                float XAdvance;
            };

        #if defined(_MSC_VER) && !defined(_NATIVE_WCHAR_T_DEFINED)
            DIRECTX_TOOLKIT_API SpriteFont(
                _In_ ID3D11Device* device,
                _In_z_ __wchar_t const* fileName,
                bool forceSRGB = false);

            DIRECTX_TOOLKIT_API void XM_CALLCONV DrawString(
                _In_ SpriteBatch* spriteBatch,
                _In_z_ __wchar_t const* text,
                XMFLOAT2 const& position,
                FXMVECTOR color = Colors::White, float rotation = 0, XMFLOAT2 const& origin = Float2Zero, float scale = 1,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0) const;
            DIRECTX_TOOLKIT_API void XM_CALLCONV DrawString(
                _In_ SpriteBatch* spriteBatch,
                _In_z_ __wchar_t const* text,
                XMFLOAT2 const& position,
                FXMVECTOR color, float rotation, XMFLOAT2 const& origin, XMFLOAT2 const& scale,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0) const;
            DIRECTX_TOOLKIT_API void XM_CALLCONV DrawString(
                _In_ SpriteBatch* spriteBatch,
                _In_z_ __wchar_t const* text,
                FXMVECTOR position,
                FXMVECTOR color = Colors::White, float rotation = 0, FXMVECTOR origin = g_XMZero, float scale = 1,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0) const;
            DIRECTX_TOOLKIT_API void XM_CALLCONV DrawString(
                _In_ SpriteBatch* spriteBatch,
                _In_z_ __wchar_t const* text,
                FXMVECTOR position,
                FXMVECTOR color, float rotation, FXMVECTOR origin, GXMVECTOR scale,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0) const;

            DIRECTX_TOOLKIT_API XMVECTOR XM_CALLCONV MeasureString(
                _In_z_ __wchar_t const* text,
                bool ignoreWhitespace = true) const;

            DIRECTX_TOOLKIT_API RECT __cdecl MeasureDrawBounds(
                _In_z_ __wchar_t const* text,
                XMFLOAT2 const& position,
                bool ignoreWhitespace = true) const;
            DIRECTX_TOOLKIT_API RECT XM_CALLCONV MeasureDrawBounds(
                _In_z_ __wchar_t const* text,
                FXMVECTOR position,
                bool ignoreWhitespace = true) const;

            DIRECTX_TOOLKIT_API void __cdecl SetDefaultCharacter(__wchar_t character);

            DIRECTX_TOOLKIT_API bool __cdecl ContainsCharacter(__wchar_t character) const;

            DIRECTX_TOOLKIT_API Glyph const* __cdecl FindGlyph(__wchar_t character) const;
        #endif // !_NATIVE_WCHAR_T_DEFINED

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;

            DIRECTX_TOOLKIT_API static const XMFLOAT2 Float2Zero;
        };
    }
}
