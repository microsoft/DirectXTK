//--------------------------------------------------------------------------------------
// File: Effects.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#ifndef __DIRECTXTK_EFFECTS_H__
#define __DIRECTXTK_EFFECTS_H__

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
#endif

#include <cstddef>
#include <cstdint>
#include <memory>

#include <DirectXMath.h>

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

#if defined(DIRECTX_TOOLKIT_IMPORT) && defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251 4275)
#endif


namespace DirectX
{
    inline namespace DX11
    {
        //------------------------------------------------------------------------------
        // Abstract interface representing any effect which can be applied onto a D3D device context.
        class DIRECTX_TOOLKIT_API IEffect
        {
        public:
            virtual ~IEffect() = default;

            IEffect(const IEffect&) = delete;
            IEffect& operator=(const IEffect&) = delete;

            virtual void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) = 0;

            virtual void __cdecl GetVertexShaderBytecode(
                _Out_ void const** pShaderByteCode,
                _Out_ size_t* pByteCodeLength) = 0;

        protected:
            IEffect() = default;
            IEffect(IEffect&&) = default;
            IEffect& operator=(IEffect&&) = default;
        };


        // Abstract interface for effects with world, view, and projection matrices.
        class DIRECTX_TOOLKIT_API IEffectMatrices
        {
        public:
            virtual ~IEffectMatrices() = default;

            IEffectMatrices(const IEffectMatrices&) = delete;
            IEffectMatrices& operator=(const IEffectMatrices&) = delete;

            virtual void XM_CALLCONV SetWorld(FXMMATRIX value) = 0;
            virtual void XM_CALLCONV SetView(FXMMATRIX value) = 0;
            virtual void XM_CALLCONV SetProjection(FXMMATRIX value) = 0;
            virtual void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection);

        protected:
            IEffectMatrices() = default;
            IEffectMatrices(IEffectMatrices&&) = default;
            IEffectMatrices& operator=(IEffectMatrices&&) = default;
        };


        // Abstract interface for effects which support directional lighting.
        class DIRECTX_TOOLKIT_API IEffectLights
        {
        public:
            virtual ~IEffectLights() = default;

            IEffectLights(const IEffectLights&) = delete;
            IEffectLights& operator=(const IEffectLights&) = delete;

            virtual void __cdecl SetLightingEnabled(bool value) = 0;
            virtual void __cdecl SetPerPixelLighting(bool value) = 0;
            virtual void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) = 0;

            virtual void __cdecl SetLightEnabled(int whichLight, bool value) = 0;
            virtual void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) = 0;
            virtual void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) = 0;
            virtual void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) = 0;

            virtual void __cdecl EnableDefaultLighting() = 0;

            static constexpr int MaxDirectionalLights = 3;

        protected:
            IEffectLights() = default;
            IEffectLights(IEffectLights&&) = default;
            IEffectLights& operator=(IEffectLights&&) = default;
        };


        // Abstract interface for effects which support fog.
        class DIRECTX_TOOLKIT_API IEffectFog
        {
        public:
            virtual ~IEffectFog() = default;

            IEffectFog(const IEffectFog&) = delete;
            IEffectFog& operator=(const IEffectFog&) = delete;

            virtual void __cdecl SetFogEnabled(bool value) = 0;
            virtual void __cdecl SetFogStart(float value) = 0;
            virtual void __cdecl SetFogEnd(float value) = 0;
            virtual void XM_CALLCONV SetFogColor(FXMVECTOR value) = 0;

        protected:
            IEffectFog() = default;
            IEffectFog(IEffectFog&&) = default;
            IEffectFog& operator=(IEffectFog&&) = default;
        };


        // Abstract interface for effects which support skinning
        class DIRECTX_TOOLKIT_API IEffectSkinning
        {
        public:
            virtual ~IEffectSkinning() = default;

            IEffectSkinning(const IEffectSkinning&) = delete;
            IEffectSkinning& operator=(const IEffectSkinning&) = delete;

            virtual void __cdecl SetWeightsPerVertex(int value) = 0;
            virtual void __cdecl SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) = 0;
            virtual void __cdecl ResetBoneTransforms() = 0;

            static constexpr int MaxBones = 72;

        protected:
            IEffectSkinning() = default;
            IEffectSkinning(IEffectSkinning&&) = default;
            IEffectSkinning& operator=(IEffectSkinning&&) = default;
        };

        //------------------------------------------------------------------------------
        // Built-in shader supports optional texture mapping, vertex coloring, directional lighting, and fog.
        class BasicEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog
        {
        public:
            DIRECTX_TOOLKIT_API explicit BasicEffect(_In_ ID3D11Device* device);

            DIRECTX_TOOLKIT_API BasicEffect(BasicEffect&&) noexcept;
            DIRECTX_TOOLKIT_API BasicEffect& operator= (BasicEffect&&) noexcept;

            BasicEffect(BasicEffect const&) = delete;
            BasicEffect& operator= (BasicEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~BasicEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) override;

            DIRECTX_TOOLKIT_API void __cdecl GetVertexShaderBytecode(
                _Out_ void const** pShaderByteCode,
                _Out_ size_t* pByteCodeLength) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Material settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetSpecularColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetSpecularPower(float value);
            DIRECTX_TOOLKIT_API void __cdecl DisableSpecular();
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

            // Light settings.
            DIRECTX_TOOLKIT_API void __cdecl SetLightingEnabled(bool value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetPerPixelLighting(bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl SetLightEnabled(int whichLight, bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl EnableDefaultLighting() override;

            // Fog settings.
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnabled(bool value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogStart(float value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnd(float value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

            // Vertex color setting.
            DIRECTX_TOOLKIT_API void __cdecl SetVertexColorEnabled(bool value);

            // Texture setting.
            DIRECTX_TOOLKIT_API void __cdecl SetTextureEnabled(bool value);
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(_In_opt_ ID3D11ShaderResourceView* value);

            // Normal compression settings.
            DIRECTX_TOOLKIT_API void __cdecl SetBiasedVertexNormals(bool value);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };



        // Built-in shader supports per-pixel alpha testing.
        class AlphaTestEffect : public IEffect, public IEffectMatrices, public IEffectFog
        {
        public:
            DIRECTX_TOOLKIT_API explicit AlphaTestEffect(_In_ ID3D11Device* device);

            DIRECTX_TOOLKIT_API AlphaTestEffect(AlphaTestEffect&&) noexcept;
            DIRECTX_TOOLKIT_API AlphaTestEffect& operator= (AlphaTestEffect&&) noexcept;

            AlphaTestEffect(AlphaTestEffect const&) = delete;
            AlphaTestEffect& operator= (AlphaTestEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~AlphaTestEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) override;

            DIRECTX_TOOLKIT_API void __cdecl GetVertexShaderBytecode(
                _Out_ void const** pShaderByteCode,
                _Out_ size_t* pByteCodeLength) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Material settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

            // Fog settings.
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnabled(bool value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogStart(float value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnd(float value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

            // Vertex color setting.
            DIRECTX_TOOLKIT_API void __cdecl SetVertexColorEnabled(bool value);

            // Texture setting.
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(_In_opt_ ID3D11ShaderResourceView* value);

            // Alpha test settings.
            DIRECTX_TOOLKIT_API void __cdecl SetAlphaFunction(D3D11_COMPARISON_FUNC value);
            DIRECTX_TOOLKIT_API void __cdecl SetReferenceAlpha(int value);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };



        // Built-in shader supports two layer multitexturing (eg. for lightmaps or detail textures).
        class DualTextureEffect : public IEffect, public IEffectMatrices, public IEffectFog
        {
        public:
            DIRECTX_TOOLKIT_API explicit DualTextureEffect(_In_ ID3D11Device* device);

            DIRECTX_TOOLKIT_API DualTextureEffect(DualTextureEffect&&) noexcept;
            DIRECTX_TOOLKIT_API DualTextureEffect& operator= (DualTextureEffect&&) noexcept;

            DualTextureEffect(DualTextureEffect const&) = delete;
            DualTextureEffect& operator= (DualTextureEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~DualTextureEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) override;

            DIRECTX_TOOLKIT_API void __cdecl GetVertexShaderBytecode(
                _Out_ void const** pShaderByteCode,
                _Out_ size_t* pByteCodeLength) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Material settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

            // Fog settings.
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnabled(bool value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogStart(float value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnd(float value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

            // Vertex color setting.
            DIRECTX_TOOLKIT_API void __cdecl SetVertexColorEnabled(bool value);

            // Texture settings.
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(_In_opt_ ID3D11ShaderResourceView* value);
            DIRECTX_TOOLKIT_API void __cdecl SetTexture2(_In_opt_ ID3D11ShaderResourceView* value);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };



        // Built-in shader supports cubic environment mapping.
        class EnvironmentMapEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog
        {
        public:
            enum Mapping : uint32_t
            {
                Mapping_Cube = 0,       // Cubic environment map
                Mapping_Sphere,         // Spherical environment map
                Mapping_DualParabola,   // Dual-parabola environment map (requires Feature Level 10.0)
            };

            DIRECTX_TOOLKIT_API explicit EnvironmentMapEffect(_In_ ID3D11Device* device);

            DIRECTX_TOOLKIT_API EnvironmentMapEffect(EnvironmentMapEffect&&) noexcept;
            DIRECTX_TOOLKIT_API EnvironmentMapEffect& operator= (EnvironmentMapEffect&&) noexcept;

            EnvironmentMapEffect(EnvironmentMapEffect const&) = delete;
            EnvironmentMapEffect& operator= (EnvironmentMapEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~EnvironmentMapEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) override;

            DIRECTX_TOOLKIT_API void __cdecl GetVertexShaderBytecode(
                _Out_ void const** pShaderByteCode,
                _Out_ size_t* pByteCodeLength) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Material settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

            // Light settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl SetPerPixelLighting(bool value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetLightEnabled(int whichLight, bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl EnableDefaultLighting() override;

            // Fog settings.
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnabled(bool value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogStart(float value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnd(float value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

            // Texture setting.
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(_In_opt_ ID3D11ShaderResourceView* value);
            DIRECTX_TOOLKIT_API void __cdecl SetEnvironmentMap(_In_opt_ ID3D11ShaderResourceView* value);

            // Environment map settings.
            DIRECTX_TOOLKIT_API void __cdecl SetMode(Mapping mapping);
            DIRECTX_TOOLKIT_API void __cdecl SetEnvironmentMapAmount(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetEnvironmentMapSpecular(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetFresnelFactor(float value);

            // Normal compression settings.
            DIRECTX_TOOLKIT_API void __cdecl SetBiasedVertexNormals(bool value);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;

            // Unsupported interface methods.
            DIRECTX_TOOLKIT_API void __cdecl SetLightingEnabled(bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;
        };



        // Built-in shader supports skinned animation.
        class SkinnedEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog, public IEffectSkinning
        {
        public:
            DIRECTX_TOOLKIT_API explicit SkinnedEffect(_In_ ID3D11Device* device);

            DIRECTX_TOOLKIT_API SkinnedEffect(SkinnedEffect&&) noexcept;
            DIRECTX_TOOLKIT_API SkinnedEffect& operator= (SkinnedEffect&&) noexcept;

            SkinnedEffect(SkinnedEffect const&) = delete;
            SkinnedEffect& operator= (SkinnedEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~SkinnedEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) override;

            DIRECTX_TOOLKIT_API void __cdecl GetVertexShaderBytecode(
                _Out_ void const** pShaderByteCode,
                _Out_ size_t* pByteCodeLength) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Material settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetSpecularColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetSpecularPower(float value);
            DIRECTX_TOOLKIT_API void __cdecl DisableSpecular();
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

            // Light settings.
            DIRECTX_TOOLKIT_API void __cdecl SetPerPixelLighting(bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl SetLightEnabled(int whichLight, bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl EnableDefaultLighting() override;

            // Fog settings.
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnabled(bool value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogStart(float value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnd(float value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

            // Texture setting.
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(_In_opt_ ID3D11ShaderResourceView* value);

            // Animation settings.
            DIRECTX_TOOLKIT_API void __cdecl SetWeightsPerVertex(int value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) override;
            DIRECTX_TOOLKIT_API void __cdecl ResetBoneTransforms() override;

            // Normal compression settings.
            DIRECTX_TOOLKIT_API void __cdecl SetBiasedVertexNormals(bool value);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;

            // Unsupported interface method.
            DIRECTX_TOOLKIT_API void __cdecl SetLightingEnabled(bool value) override;
        };

        //------------------------------------------------------------------------------
        // Built-in effect for Visual Studio Shader Designer (DGSL) shaders
        class DGSLEffect : public IEffect, public IEffectMatrices, public IEffectLights
        {
        public:
            DIRECTX_TOOLKIT_API inline explicit DGSLEffect(_In_ ID3D11Device* device, _In_opt_ ID3D11PixelShader* pixelShader = nullptr) :
                DGSLEffect(device, pixelShader, false)
            {}

            DIRECTX_TOOLKIT_API DGSLEffect(DGSLEffect&&) noexcept;
            DIRECTX_TOOLKIT_API DGSLEffect& operator= (DGSLEffect&&) noexcept;

            DGSLEffect(DGSLEffect const&) = delete;
            DGSLEffect& operator= (DGSLEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~DGSLEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) override;

            DIRECTX_TOOLKIT_API void __cdecl GetVertexShaderBytecode(
                _Out_ void const** pShaderByteCode,
                _Out_ size_t* pByteCodeLength) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Material settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetAmbientColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetSpecularColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetSpecularPower(float value);
            DIRECTX_TOOLKIT_API void __cdecl DisableSpecular();
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

            // Additional settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetUVTransform(FXMMATRIX value);
            DIRECTX_TOOLKIT_API void __cdecl SetViewport(float width, float height);
            DIRECTX_TOOLKIT_API void __cdecl SetTime(float time);
            DIRECTX_TOOLKIT_API void __cdecl SetAlphaDiscardEnable(bool value);

            // Light settings.
            DIRECTX_TOOLKIT_API void __cdecl SetLightingEnabled(bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl SetLightEnabled(int whichLight, bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl EnableDefaultLighting() override;

            static constexpr int MaxDirectionalLights = 4;

            // Vertex color setting.
            DIRECTX_TOOLKIT_API void __cdecl SetVertexColorEnabled(bool value);

            // Texture settings.
            DIRECTX_TOOLKIT_API void __cdecl SetTextureEnabled(bool value);
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(_In_opt_ ID3D11ShaderResourceView* value);
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(int whichTexture, _In_opt_ ID3D11ShaderResourceView* value);

            static constexpr int MaxTextures = 8;

        protected:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;

            DIRECTX_TOOLKIT_API DGSLEffect(_In_ ID3D11Device* device, _In_opt_ ID3D11PixelShader* pixelShader, bool skinningEnabled);

            // Unsupported interface methods.
            DIRECTX_TOOLKIT_API void __cdecl SetPerPixelLighting(bool value) override;
        };

        class DIRECTX_TOOLKIT_API SkinnedDGSLEffect : public DGSLEffect, public IEffectSkinning
        {
        public:
            explicit SkinnedDGSLEffect(_In_ ID3D11Device* device, _In_opt_ ID3D11PixelShader* pixelShader = nullptr) :
                DGSLEffect(device, pixelShader, true)
            {}

            SkinnedDGSLEffect(SkinnedDGSLEffect&&) = default;
            SkinnedDGSLEffect& operator= (SkinnedDGSLEffect&&) = default;

            SkinnedDGSLEffect(SkinnedDGSLEffect const&) = delete;
            SkinnedDGSLEffect& operator= (SkinnedDGSLEffect const&) = delete;

            ~SkinnedDGSLEffect() override;

            // Animation setting.
            void __cdecl SetWeightsPerVertex(int value) override;
            void __cdecl SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) override;
            void __cdecl ResetBoneTransforms() override;
        };

        //------------------------------------------------------------------------------
        // Built-in shader extends BasicEffect with normal maps and optional specular maps
        class NormalMapEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog
        {
        public:
            DIRECTX_TOOLKIT_API inline explicit NormalMapEffect(_In_ ID3D11Device* device) :
                NormalMapEffect(device, false)
            {}

            DIRECTX_TOOLKIT_API NormalMapEffect(NormalMapEffect&&) noexcept;
            DIRECTX_TOOLKIT_API NormalMapEffect& operator= (NormalMapEffect&&) noexcept;

            NormalMapEffect(NormalMapEffect const&) = delete;
            NormalMapEffect& operator= (NormalMapEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~NormalMapEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) override;

            DIRECTX_TOOLKIT_API void __cdecl GetVertexShaderBytecode(
                _Out_ void const** pShaderByteCode,
                _Out_ size_t* pByteCodeLength) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Material settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetSpecularColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetSpecularPower(float value);
            DIRECTX_TOOLKIT_API void __cdecl DisableSpecular();
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

            // Light settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl SetLightEnabled(int whichLight, bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl EnableDefaultLighting() override;

            // Fog settings.
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnabled(bool value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogStart(float value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnd(float value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

            // Vertex color setting.
            DIRECTX_TOOLKIT_API void __cdecl SetVertexColorEnabled(bool value);

            // Texture setting - albedo, normal and specular intensity
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(_In_opt_ ID3D11ShaderResourceView* value);
            DIRECTX_TOOLKIT_API void __cdecl SetNormalTexture(_In_opt_ ID3D11ShaderResourceView* value);
            DIRECTX_TOOLKIT_API void __cdecl SetSpecularTexture(_In_opt_ ID3D11ShaderResourceView* value);

            // Normal compression settings.
            DIRECTX_TOOLKIT_API void __cdecl SetBiasedVertexNormals(bool value);

            // Instancing settings.
            DIRECTX_TOOLKIT_API void __cdecl SetInstancingEnabled(bool value);

        protected:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;

            DIRECTX_TOOLKIT_API NormalMapEffect(_In_ ID3D11Device* device, bool skinningEnabled);

            // Unsupported interface methods.
            DIRECTX_TOOLKIT_API void __cdecl SetLightingEnabled(bool value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetPerPixelLighting(bool value) override;
        };

        class DIRECTX_TOOLKIT_API SkinnedNormalMapEffect : public NormalMapEffect, public IEffectSkinning
        {
        public:
            explicit SkinnedNormalMapEffect(_In_ ID3D11Device* device) :
                NormalMapEffect(device, true)
            {}

            SkinnedNormalMapEffect(SkinnedNormalMapEffect&&) = default;
            SkinnedNormalMapEffect& operator= (SkinnedNormalMapEffect&&) = default;

            SkinnedNormalMapEffect(SkinnedNormalMapEffect const&) = delete;
            SkinnedNormalMapEffect& operator= (SkinnedNormalMapEffect const&) = delete;

            ~SkinnedNormalMapEffect() override;

            // Animation settings.
            void __cdecl SetWeightsPerVertex(int value) override;
            void __cdecl SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) override;
            void __cdecl ResetBoneTransforms() override;
        };

        //------------------------------------------------------------------------------
        // Built-in shader for Physically-Based Rendering (Roughness/Metalness) with Image-based lighting
        class PBREffect : public IEffect, public IEffectMatrices, public IEffectLights
        {
        public:
            DIRECTX_TOOLKIT_API inline explicit PBREffect(_In_ ID3D11Device* device) :
                PBREffect(device, false)
            {}

            DIRECTX_TOOLKIT_API PBREffect(PBREffect&&) noexcept;
            DIRECTX_TOOLKIT_API PBREffect& operator= (PBREffect&&) noexcept;

            PBREffect(PBREffect const&) = delete;
            PBREffect& operator= (PBREffect const&) = delete;

            DIRECTX_TOOLKIT_API ~PBREffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) override;

            DIRECTX_TOOLKIT_API void __cdecl GetVertexShaderBytecode(
                _Out_ void const** pShaderByteCode,
                _Out_ size_t* pByteCodeLength) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Light settings.
            DIRECTX_TOOLKIT_API void __cdecl SetLightEnabled(int whichLight, bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl EnableDefaultLighting() override;

            // PBR Settings.
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetConstantAlbedo(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetConstantMetallic(float value);
            DIRECTX_TOOLKIT_API void __cdecl SetConstantRoughness(float value);

            // Texture settings.
            DIRECTX_TOOLKIT_API void __cdecl SetAlbedoTexture(_In_opt_ ID3D11ShaderResourceView* value);
            DIRECTX_TOOLKIT_API void __cdecl SetNormalTexture(_In_opt_ ID3D11ShaderResourceView* value);
            DIRECTX_TOOLKIT_API void __cdecl SetRMATexture(_In_opt_ ID3D11ShaderResourceView* value);

            DIRECTX_TOOLKIT_API void __cdecl SetEmissiveTexture(_In_opt_ ID3D11ShaderResourceView* value);

            DIRECTX_TOOLKIT_API void __cdecl SetSurfaceTextures(
                _In_opt_ ID3D11ShaderResourceView* albedo,
                _In_opt_ ID3D11ShaderResourceView* normal,
                _In_opt_ ID3D11ShaderResourceView* roughnessMetallicAmbientOcclusion);

            DIRECTX_TOOLKIT_API void __cdecl SetIBLTextures(
                _In_opt_ ID3D11ShaderResourceView* radiance,
                int numRadianceMips,
                _In_opt_ ID3D11ShaderResourceView* irradiance);

            // Normal compression settings.
            DIRECTX_TOOLKIT_API void __cdecl SetBiasedVertexNormals(bool value);

            // Instancing settings.
            DIRECTX_TOOLKIT_API void __cdecl SetInstancingEnabled(bool value);

            // Velocity buffer settings.
            DIRECTX_TOOLKIT_API void __cdecl SetVelocityGeneration(bool value);

            // Render target size, required for velocity buffer output.
            DIRECTX_TOOLKIT_API void __cdecl SetRenderTargetSizeInPixels(int width, int height);

        protected:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;

            DIRECTX_TOOLKIT_API PBREffect(_In_ ID3D11Device* device, bool skinningEnabled);

            // Unsupported interface methods.
            DIRECTX_TOOLKIT_API void __cdecl SetLightingEnabled(bool value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetPerPixelLighting(bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;
        };

        class DIRECTX_TOOLKIT_API SkinnedPBREffect : public PBREffect, public IEffectSkinning
        {
        public:
            explicit SkinnedPBREffect(_In_ ID3D11Device* device) :
                PBREffect(device, true)
            {}

            SkinnedPBREffect(SkinnedPBREffect&&) = default;
            SkinnedPBREffect& operator= (SkinnedPBREffect&&) = default;

            SkinnedPBREffect(SkinnedPBREffect const&) = delete;
            SkinnedPBREffect& operator= (SkinnedPBREffect const&) = delete;

            ~SkinnedPBREffect() override;

            // Animation settings.
            void __cdecl SetWeightsPerVertex(int value) override;
            void __cdecl SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) override;
            void __cdecl ResetBoneTransforms() override;
        };

        //------------------------------------------------------------------------------
        // Built-in shader for debug visualization of normals, tangents, etc.
        class DebugEffect : public IEffect, public IEffectMatrices
        {
        public:
            enum Mode : uint32_t
            {
                Mode_Default = 0,   // Hemispherical ambient lighting
                Mode_Normals,       // RGB normals
                Mode_Tangents,      // RGB tangents
                Mode_BiTangents,    // RGB bi-tangents
            };

            DIRECTX_TOOLKIT_API explicit DebugEffect(_In_ ID3D11Device* device);

            DIRECTX_TOOLKIT_API DebugEffect(DebugEffect&&) noexcept;
            DIRECTX_TOOLKIT_API DebugEffect& operator= (DebugEffect&&) noexcept;

            DebugEffect(DebugEffect const&) = delete;
            DebugEffect& operator= (DebugEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~DebugEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) override;

            DIRECTX_TOOLKIT_API void __cdecl GetVertexShaderBytecode(
                _Out_ void const** pShaderByteCode,
                _Out_ size_t* pByteCodeLength) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Debug Settings.
            DIRECTX_TOOLKIT_API void __cdecl SetMode(Mode debugMode);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetHemisphericalAmbientColor(FXMVECTOR upper, FXMVECTOR lower);
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);

            // Vertex color setting.
            DIRECTX_TOOLKIT_API void __cdecl SetVertexColorEnabled(bool value);

            // Normal compression settings.
            DIRECTX_TOOLKIT_API void __cdecl SetBiasedVertexNormals(bool value);

            // Instancing settings.
            DIRECTX_TOOLKIT_API void __cdecl SetInstancingEnabled(bool value);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };

        //------------------------------------------------------------------------------
        // Abstract interface to factory for sharing effects and texture resources
        class DIRECTX_TOOLKIT_API IEffectFactory
        {
        public:
            virtual ~IEffectFactory() = default;

            IEffectFactory(const IEffectFactory&) = delete;
            IEffectFactory& operator=(const IEffectFactory&) = delete;

            struct DIRECTX_TOOLKIT_API EffectInfo
            {
                const wchar_t*      name;
                bool                perVertexColor;
                bool                enableSkinning;
                bool                enableDualTexture;
                bool                enableNormalMaps;
                bool                biasedVertexNormals;
                float               specularPower;
                float               alpha;
                XMFLOAT3            ambientColor;
                XMFLOAT3            diffuseColor;
                XMFLOAT3            specularColor;
                XMFLOAT3            emissiveColor;
                const wchar_t*      diffuseTexture;
                const wchar_t*      specularTexture;
                const wchar_t*      normalTexture;
                const wchar_t*      emissiveTexture;

                EffectInfo() noexcept :
                    name(nullptr),
                    perVertexColor(false),
                    enableSkinning(false),
                    enableDualTexture(false),
                    enableNormalMaps(false),
                    biasedVertexNormals(false),
                    specularPower(0),
                    alpha(0),
                    ambientColor(0, 0, 0),
                    diffuseColor(0, 0, 0),
                    specularColor(0, 0, 0),
                    emissiveColor(0, 0, 0),
                    diffuseTexture(nullptr),
                    specularTexture(nullptr),
                    normalTexture(nullptr),
                    emissiveTexture(nullptr)
                {}
            };

            virtual std::shared_ptr<IEffect> __cdecl CreateEffect(
                _In_ const EffectInfo& info,
                _In_opt_ ID3D11DeviceContext* deviceContext) = 0;

            virtual void __cdecl CreateTexture(
                _In_z_ const wchar_t* name,
                _In_opt_ ID3D11DeviceContext* deviceContext,
                _Outptr_ ID3D11ShaderResourceView** textureView) = 0;

        protected:
            IEffectFactory() = default;
            IEffectFactory(IEffectFactory&&) = default;
            IEffectFactory& operator=(IEffectFactory&&) = default;
        };


        // Factory for sharing effects and texture resources
        class EffectFactory : public IEffectFactory
        {
        public:
            DIRECTX_TOOLKIT_API explicit EffectFactory(_In_ ID3D11Device* device);

            DIRECTX_TOOLKIT_API EffectFactory(EffectFactory&&) noexcept;
            DIRECTX_TOOLKIT_API EffectFactory& operator= (EffectFactory&&) noexcept;

            EffectFactory(EffectFactory const&) = delete;
            EffectFactory& operator= (EffectFactory const&) = delete;

            DIRECTX_TOOLKIT_API ~EffectFactory() override;

            // IEffectFactory methods.
            DIRECTX_TOOLKIT_API std::shared_ptr<IEffect> __cdecl CreateEffect(
                _In_ const EffectInfo& info,
                _In_opt_ ID3D11DeviceContext* deviceContext) override;
            DIRECTX_TOOLKIT_API void __cdecl CreateTexture(
                _In_z_ const wchar_t* name,
                _In_opt_ ID3D11DeviceContext* deviceContext,
                _Outptr_ ID3D11ShaderResourceView** textureView) override;

            // Settings.
            DIRECTX_TOOLKIT_API void __cdecl ReleaseCache();

            DIRECTX_TOOLKIT_API void __cdecl SetSharing(bool enabled) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl EnableNormalMapEffect(bool enabled) noexcept;
            DIRECTX_TOOLKIT_API void __cdecl EnableForceSRGB(bool forceSRGB) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl SetDirectory(_In_opt_z_ const wchar_t* path) noexcept;

            // Properties.
            DIRECTX_TOOLKIT_API ID3D11Device* GetDevice() const noexcept;

        private:
            // Private implementation.
            class Impl;

            std::shared_ptr<Impl> pImpl;
        };


        // Factory for Physically Based Rendering (PBR)
        class PBREffectFactory : public IEffectFactory
        {
        public:
            DIRECTX_TOOLKIT_API explicit PBREffectFactory(_In_ ID3D11Device* device);

            DIRECTX_TOOLKIT_API PBREffectFactory(PBREffectFactory&&) noexcept;
            DIRECTX_TOOLKIT_API PBREffectFactory& operator= (PBREffectFactory&&) noexcept;

            PBREffectFactory(PBREffectFactory const&) = delete;
            PBREffectFactory& operator= (PBREffectFactory const&) = delete;

            DIRECTX_TOOLKIT_API ~PBREffectFactory() override;

            // IEffectFactory methods.
            DIRECTX_TOOLKIT_API std::shared_ptr<IEffect> __cdecl CreateEffect(
                _In_ const EffectInfo& info,
                _In_opt_ ID3D11DeviceContext* deviceContext) override;
            DIRECTX_TOOLKIT_API void __cdecl CreateTexture(
                _In_z_ const wchar_t* name,
                _In_opt_ ID3D11DeviceContext* deviceContext,
                _Outptr_ ID3D11ShaderResourceView** textureView) override;

            // Settings.
            DIRECTX_TOOLKIT_API void __cdecl ReleaseCache();

            DIRECTX_TOOLKIT_API void __cdecl SetSharing(bool enabled) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl EnableForceSRGB(bool forceSRGB) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl SetDirectory(_In_opt_z_ const wchar_t* path) noexcept;

            // Properties.
            DIRECTX_TOOLKIT_API ID3D11Device* GetDevice() const noexcept;

        private:
            // Private implementation.
            class Impl;

            std::shared_ptr<Impl> pImpl;
        };


        // Factory for sharing Visual Studio Shader Designer (DGSL) shaders and texture resources
        class DGSLEffectFactory : public IEffectFactory
        {
        public:
            DIRECTX_TOOLKIT_API explicit DGSLEffectFactory(_In_ ID3D11Device* device);

            DIRECTX_TOOLKIT_API DGSLEffectFactory(DGSLEffectFactory&&) noexcept;
            DIRECTX_TOOLKIT_API DGSLEffectFactory& operator= (DGSLEffectFactory&&) noexcept;

            DGSLEffectFactory(DGSLEffectFactory const&) = delete;
            DGSLEffectFactory& operator= (DGSLEffectFactory const&) = delete;

            DIRECTX_TOOLKIT_API ~DGSLEffectFactory() override;

            // IEffectFactory methods.
            DIRECTX_TOOLKIT_API std::shared_ptr<IEffect> __cdecl CreateEffect(
                _In_ const EffectInfo& info,
                _In_opt_ ID3D11DeviceContext* deviceContext) override;
            DIRECTX_TOOLKIT_API void __cdecl CreateTexture(
                _In_z_ const wchar_t* name,
                _In_opt_ ID3D11DeviceContext* deviceContext,
                _Outptr_ ID3D11ShaderResourceView** textureView) override;

            // DGSL methods.
            struct DIRECTX_TOOLKIT_API DGSLEffectInfo : public EffectInfo
            {
                static constexpr int BaseTextureOffset = 4;

                const wchar_t* textures[DGSLEffect::MaxTextures - BaseTextureOffset];
                const wchar_t* pixelShader;

                DGSLEffectInfo() noexcept :
                    EffectInfo(),
                    textures{},
                    pixelShader(nullptr)
                {}
            };

            DIRECTX_TOOLKIT_API virtual std::shared_ptr<IEffect> __cdecl CreateDGSLEffect(
                _In_ const DGSLEffectInfo& info,
                _In_opt_ ID3D11DeviceContext* deviceContext);

            DIRECTX_TOOLKIT_API virtual void __cdecl CreatePixelShader(
                _In_z_ const wchar_t* shader,
                _Outptr_ ID3D11PixelShader** pixelShader);

            // Settings.
            DIRECTX_TOOLKIT_API void __cdecl ReleaseCache();

            DIRECTX_TOOLKIT_API void __cdecl SetSharing(bool enabled) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl EnableForceSRGB(bool forceSRGB) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl SetDirectory(_In_opt_z_ const wchar_t* path) noexcept;

            // Properties.
            DIRECTX_TOOLKIT_API ID3D11Device* GetDevice() const noexcept;

        private:
            // Private implementation.
            class Impl;

            std::shared_ptr<Impl> pImpl;
        };
    }
}

#if defined(DIRECTX_TOOLKIT_IMPORT) && defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif // __DIRECTXTK_EFFECTS_H__
