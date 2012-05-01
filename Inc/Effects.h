//--------------------------------------------------------------------------------------
// File: Effects.h
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>


namespace DirectX
{
    // Abstract interface representing any effect which can be applied onto a D3D device context.
    class IEffect
    {
    public:
        virtual ~IEffect() { }

        virtual void Apply(_In_ ID3D11DeviceContext* deviceContext) = 0;

        virtual void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) = 0;
    };


    // Abstract interface for effects with world, view, and projection matrices.
    class IEffectMatrices
    {
    public:
        virtual ~IEffectMatrices() { }

        virtual void SetWorld(CXMMATRIX value) = 0;
        virtual void SetView(CXMMATRIX value) = 0;
        virtual void SetProjection(CXMMATRIX value) = 0;
    };


    // Abstract interface for effects which support directional lighting.
    class IEffectLights
    {
    public:
        virtual ~IEffectLights() { }

        virtual void SetLightingEnabled(bool value) = 0;
        virtual void SetPerPixelLighting(bool value) = 0;
        virtual void SetAmbientLightColor(FXMVECTOR value) = 0;

        virtual void SetLightEnabled(int whichLight, bool value) = 0;
        virtual void SetLightDirection(int whichLight, FXMVECTOR value) = 0;
        virtual void SetLightDiffuseColor(int whichLight, FXMVECTOR value) = 0;
        virtual void SetLightSpecularColor(int whichLight, FXMVECTOR value) = 0;

        virtual void EnableDefaultLighting() = 0;

        static const int MaxDirectionalLights = 3;
    };


    // Abstract interface for effects which support fog.
    class IEffectFog
    {
    public:
        virtual ~IEffectFog() { }

        virtual void SetFogEnabled(bool value) = 0;
        virtual void SetFogStart(float value) = 0;
        virtual void SetFogEnd(float value) = 0;
        virtual void SetFogColor(FXMVECTOR value) = 0;
    };



    // Built-in shader supports optional texture mapping, vertex coloring, directional lighting, and fog.
    class BasicEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog
    {
    public:
        explicit BasicEffect(_In_ ID3D11Device* device);
        BasicEffect(BasicEffect&& moveFrom);
        BasicEffect& operator= (BasicEffect&& moveFrom);
        virtual ~BasicEffect();

        // IEffect methods.
        void Apply(_In_ ID3D11DeviceContext* deviceContext);

        void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength);

        // Camera settings.
        void SetWorld(CXMMATRIX value);
        void SetView(CXMMATRIX value);
        void SetProjection(CXMMATRIX value);

        // Material settings.
        void SetDiffuseColor(FXMVECTOR value);
        void SetEmissiveColor(FXMVECTOR value);
        void SetSpecularColor(FXMVECTOR value);
        void SetSpecularPower(float value);
        void SetAlpha(float value);
        
        // Light settings.
        void SetLightingEnabled(bool value);
        void SetPerPixelLighting(bool value);
        void SetAmbientLightColor(FXMVECTOR value);

        void SetLightEnabled(int whichLight, bool value);
        void SetLightDirection(int whichLight, FXMVECTOR value);
        void SetLightDiffuseColor(int whichLight, FXMVECTOR value);
        void SetLightSpecularColor(int whichLight, FXMVECTOR value);

        void EnableDefaultLighting();

        // Fog settings.
        void SetFogEnabled(bool value);
        void SetFogStart(float value);
        void SetFogEnd(float value);
        void SetFogColor(FXMVECTOR value);

        // Vertex color setting.
        void SetVertexColorEnabled(bool value);

        // Texture setting.
        void SetTextureEnabled(bool value);
        void SetTexture(_In_opt_ ID3D11ShaderResourceView* value);
        
    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Prevent copying.
        BasicEffect(BasicEffect const&);
        BasicEffect& operator= (BasicEffect const&);
    };



    // Built-in shader supports per-pixel alpha testing.
    class AlphaTestEffect : public IEffect, public IEffectMatrices, public IEffectFog
    {
    public:
        explicit AlphaTestEffect(_In_ ID3D11Device* device);
        AlphaTestEffect(AlphaTestEffect&& moveFrom);
        AlphaTestEffect& operator= (AlphaTestEffect&& moveFrom);
        virtual ~AlphaTestEffect();

        // IEffect methods.
        void Apply(_In_ ID3D11DeviceContext* deviceContext);

        void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength);

        // Camera settings.
        void SetWorld(CXMMATRIX value);
        void SetView(CXMMATRIX value);
        void SetProjection(CXMMATRIX value);

        // Material settings.
        void SetDiffuseColor(FXMVECTOR value);
        void SetAlpha(float value);
        
        // Fog settings.
        void SetFogEnabled(bool value);
        void SetFogStart(float value);
        void SetFogEnd(float value);
        void SetFogColor(FXMVECTOR value);

        // Vertex color setting.
        void SetVertexColorEnabled(bool value);

        // Texture setting.
        void SetTexture(_In_opt_ ID3D11ShaderResourceView* value);
        
        // Alpha test settings.
        void SetAlphaFunction(D3D11_COMPARISON_FUNC value);
        void SetReferenceAlpha(int value);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Prevent copying.
        AlphaTestEffect(AlphaTestEffect const&);
        AlphaTestEffect& operator= (AlphaTestEffect const&);
    };



    // Built-in shader supports two layer multitexturing (eg. for lightmaps or detail textures).
    class DualTextureEffect : public IEffect, public IEffectMatrices, public IEffectFog
    {
    public:
        explicit DualTextureEffect(_In_ ID3D11Device* device);
        DualTextureEffect(DualTextureEffect&& moveFrom);
        DualTextureEffect& operator= (DualTextureEffect&& moveFrom);
        ~DualTextureEffect();

        // IEffect methods.
        void Apply(_In_ ID3D11DeviceContext* deviceContext);

        void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength);

        // Camera settings.
        void SetWorld(CXMMATRIX value);
        void SetView(CXMMATRIX value);
        void SetProjection(CXMMATRIX value);

        // Material settings.
        void SetDiffuseColor(FXMVECTOR value);
        void SetAlpha(float value);
        
        // Fog settings.
        void SetFogEnabled(bool value);
        void SetFogStart(float value);
        void SetFogEnd(float value);
        void SetFogColor(FXMVECTOR value);

        // Vertex color setting.
        void SetVertexColorEnabled(bool value);

        // Texture settings.
        void SetTexture(_In_opt_ ID3D11ShaderResourceView* value);
        void SetTexture2(_In_opt_ ID3D11ShaderResourceView* value);
        
    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Prevent copying.
        DualTextureEffect(DualTextureEffect const&);
        DualTextureEffect& operator= (DualTextureEffect const&);
    };



    // Built-in shader supports cubic environment mapping.
    class EnvironmentMapEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog
    {
    public:
        explicit EnvironmentMapEffect(_In_ ID3D11Device* device);
        EnvironmentMapEffect(EnvironmentMapEffect&& moveFrom);
        EnvironmentMapEffect& operator= (EnvironmentMapEffect&& moveFrom);
        virtual ~EnvironmentMapEffect();

        // IEffect methods.
        void Apply(_In_ ID3D11DeviceContext* deviceContext);

        void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength);

        // Camera settings.
        void SetWorld(CXMMATRIX value);
        void SetView(CXMMATRIX value);
        void SetProjection(CXMMATRIX value);

        // Material settings.
        void SetDiffuseColor(FXMVECTOR value);
        void SetEmissiveColor(FXMVECTOR value);
        void SetAlpha(float value);
        
        // Light settings.
        void SetAmbientLightColor(FXMVECTOR value);

        void SetLightEnabled(int whichLight, bool value);
        void SetLightDirection(int whichLight, FXMVECTOR value);
        void SetLightDiffuseColor(int whichLight, FXMVECTOR value);

        void EnableDefaultLighting();

        // Fog settings.
        void SetFogEnabled(bool value);
        void SetFogStart(float value);
        void SetFogEnd(float value);
        void SetFogColor(FXMVECTOR value);

        // Texture setting.
        void SetTexture(_In_opt_ ID3D11ShaderResourceView* value);

        // Environment map settings.
        void SetEnvironmentMap(_In_opt_ ID3D11ShaderResourceView* value);
        void SetEnvironmentMapAmount(float value);
        void SetEnvironmentMapSpecular(FXMVECTOR value);
        void SetFresnelFactor(float value);
        
    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Unsupported interface methods.
        void SetLightingEnabled(bool value);
        void SetPerPixelLighting(bool value);
        void SetLightSpecularColor(int whichLight, FXMVECTOR value);

        // Prevent copying.
        EnvironmentMapEffect(EnvironmentMapEffect const&);
        EnvironmentMapEffect& operator= (EnvironmentMapEffect const&);
    };



    // Built-in shader supports skinned animation.
    class SkinnedEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog
    {
    public:
        explicit SkinnedEffect(_In_ ID3D11Device* device);
        SkinnedEffect(SkinnedEffect&& moveFrom);
        SkinnedEffect& operator= (SkinnedEffect&& moveFrom);
        virtual ~SkinnedEffect();

        // IEffect methods.
        void Apply(_In_ ID3D11DeviceContext* deviceContext);

        void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength);

        // Camera settings.
        void SetWorld(CXMMATRIX value);
        void SetView(CXMMATRIX value);
        void SetProjection(CXMMATRIX value);

        // Material settings.
        void SetDiffuseColor(FXMVECTOR value);
        void SetEmissiveColor(FXMVECTOR value);
        void SetSpecularColor(FXMVECTOR value);
        void SetSpecularPower(float value);
        void SetAlpha(float value);
        
        // Light settings.
        void SetPerPixelLighting(bool value);
        void SetAmbientLightColor(FXMVECTOR value);

        void SetLightEnabled(int whichLight, bool value);
        void SetLightDirection(int whichLight, FXMVECTOR value);
        void SetLightDiffuseColor(int whichLight, FXMVECTOR value);
        void SetLightSpecularColor(int whichLight, FXMVECTOR value);

        void EnableDefaultLighting();

        // Fog settings.
        void SetFogEnabled(bool value);
        void SetFogStart(float value);
        void SetFogEnd(float value);
        void SetFogColor(FXMVECTOR value);

        // Texture setting.
        void SetTexture(_In_opt_ ID3D11ShaderResourceView* value);
        
        // Animation settings.
        void SetWeightsPerVertex(int value);
        void SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count);

        static const int MaxBones = 72;

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Unsupported interface method.
        void SetLightingEnabled(bool value);

        // Prevent copying.
        SkinnedEffect(SkinnedEffect const&);
        SkinnedEffect& operator= (SkinnedEffect const&);
    };
}
