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

#if defined(_XBOX_ONE) && defined(_TITLE) && MONOLITHIC
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
#endif

#include <DirectXMath.h>
#include <memory>

#pragma warning(push)
#pragma warning(disable : 4481)
// VS 2010 considers 'override' to be a extension, but it's part of C++11 as of VS 2012

namespace DirectX
{
    #if (DIRECTXMATH_VERSION < 305) && !defined(XM_CALLCONV)
    #define XM_CALLCONV __fastcall
    typedef const XMVECTOR& HXMVECTOR;
    typedef const XMMATRIX& FXMMATRIX;
    #endif

    //----------------------------------------------------------------------------------
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

        virtual void XM_CALLCONV SetWorld(FXMMATRIX value) = 0;
        virtual void XM_CALLCONV SetView(FXMMATRIX value) = 0;
        virtual void XM_CALLCONV SetProjection(FXMMATRIX value) = 0;
    };


    // Abstract interface for effects which support directional lighting.
    class IEffectLights
    {
    public:
        virtual ~IEffectLights() { }

        virtual void SetLightingEnabled(bool value) = 0;
        virtual void SetPerPixelLighting(bool value) = 0;
        virtual void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) = 0;

        virtual void SetLightEnabled(int whichLight, bool value) = 0;
        virtual void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) = 0;
        virtual void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) = 0;
        virtual void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) = 0;

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
        virtual void XM_CALLCONV SetFogColor(FXMVECTOR value) = 0;
    };


    // Abstract interface for effects which support skinning
    class IEffectSkinning
    {
    public:
        virtual ~IEffectSkinning() { } 

        virtual void SetWeightsPerVertex(int value) = 0;
        virtual void SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) = 0;
        virtual void ResetBoneTransforms() = 0;

        static const int MaxBones = 72;
    };


    //----------------------------------------------------------------------------------
    // Built-in shader supports optional texture mapping, vertex coloring, directional lighting, and fog.
    class BasicEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog
    {
    public:
        explicit BasicEffect(_In_ ID3D11Device* device);
        BasicEffect(BasicEffect&& moveFrom);
        BasicEffect& operator= (BasicEffect&& moveFrom);
        virtual ~BasicEffect();

        // IEffect methods.
        void Apply(_In_ ID3D11DeviceContext* deviceContext) override;

        void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;

        // Material settings.
        void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
        void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
        void XM_CALLCONV SetSpecularColor(FXMVECTOR value);
        void SetSpecularPower(float value);
        void DisableSpecular();
        void SetAlpha(float value);
        
        // Light settings.
        void SetLightingEnabled(bool value) override;
        void SetPerPixelLighting(bool value) override;
        void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

        void SetLightEnabled(int whichLight, bool value) override;
        void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

        void EnableDefaultLighting() override;

        // Fog settings.
        void SetFogEnabled(bool value) override;
        void SetFogStart(float value) override;
        void SetFogEnd(float value) override;
        void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

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
        void Apply(_In_ ID3D11DeviceContext* deviceContext) override;

        void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;

        // Material settings.
        void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
        void SetAlpha(float value);
        
        // Fog settings.
        void SetFogEnabled(bool value) override;
        void SetFogStart(float value) override;
        void SetFogEnd(float value) override;
        void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

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
        void Apply(_In_ ID3D11DeviceContext* deviceContext) override;

        void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;

        // Material settings.
        void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
        void SetAlpha(float value);
        
        // Fog settings.
        void SetFogEnabled(bool value) override;
        void SetFogStart(float value) override;
        void SetFogEnd(float value) override;
        void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

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
        void Apply(_In_ ID3D11DeviceContext* deviceContext) override;

        void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;

        // Material settings.
        void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
        void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
        void SetAlpha(float value);
        
        // Light settings.
        void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

        void SetLightEnabled(int whichLight, bool value) override;
        void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;

        void EnableDefaultLighting() override;

        // Fog settings.
        void SetFogEnabled(bool value) override;
        void SetFogStart(float value) override;
        void SetFogEnd(float value) override;
        void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

        // Texture setting.
        void SetTexture(_In_opt_ ID3D11ShaderResourceView* value);

        // Environment map settings.
        void SetEnvironmentMap(_In_opt_ ID3D11ShaderResourceView* value);
        void SetEnvironmentMapAmount(float value);
        void XM_CALLCONV SetEnvironmentMapSpecular(FXMVECTOR value);
        void SetFresnelFactor(float value);
        
    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Unsupported interface methods.
        void SetLightingEnabled(bool value) override;
        void SetPerPixelLighting(bool value) override;
        void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

        // Prevent copying.
        EnvironmentMapEffect(EnvironmentMapEffect const&);
        EnvironmentMapEffect& operator= (EnvironmentMapEffect const&);
    };



    // Built-in shader supports skinned animation.
    class SkinnedEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog, public IEffectSkinning
    {
    public:
        explicit SkinnedEffect(_In_ ID3D11Device* device);
        SkinnedEffect(SkinnedEffect&& moveFrom);
        SkinnedEffect& operator= (SkinnedEffect&& moveFrom);
        virtual ~SkinnedEffect();

        // IEffect methods.
        void Apply(_In_ ID3D11DeviceContext* deviceContext) override;

        void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;

        // Material settings.
        void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
        void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
        void XM_CALLCONV SetSpecularColor(FXMVECTOR value);
        void SetSpecularPower(float value);
        void DisableSpecular();
        void SetAlpha(float value);
        
        // Light settings.
        void SetPerPixelLighting(bool value) override;
        void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

        void SetLightEnabled(int whichLight, bool value) override;
        void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

        void EnableDefaultLighting() override;

        // Fog settings.
        void SetFogEnabled(bool value) override;
        void SetFogStart(float value) override;
        void SetFogEnd(float value) override;
        void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

        // Texture setting.
        void SetTexture(_In_opt_ ID3D11ShaderResourceView* value);
        
        // Animation settings.
        void SetWeightsPerVertex(int value) override;
        void SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) override;
        void ResetBoneTransforms() override;

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Unsupported interface method.
        void SetLightingEnabled(bool value) override;

        // Prevent copying.
        SkinnedEffect(SkinnedEffect const&);
        SkinnedEffect& operator= (SkinnedEffect const&);
    };

    

    //----------------------------------------------------------------------------------
    // Built-in effect for Visual Studio Shader Designer (DGSL) shaders
    class DGSLEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectSkinning
    {
    public:
        explicit DGSLEffect( _In_ ID3D11Device* device, _In_opt_ ID3D11PixelShader* pixelShader = nullptr,
                             _In_ bool enableSkinning = false );
        DGSLEffect(DGSLEffect&& moveFrom);
        DGSLEffect& operator= (DGSLEffect&& moveFrom);
        virtual ~DGSLEffect();

        // IEffect methods.
        void Apply(_In_ ID3D11DeviceContext* deviceContext) override;

        void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;

        // Material settings.
        void XM_CALLCONV SetAmbientColor(FXMVECTOR value);
        void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
        void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
        void XM_CALLCONV SetSpecularColor(FXMVECTOR value);
        void SetSpecularPower(float value);
        void DisableSpecular();
        void SetAlpha(float value);

        // Additional settings.
        void XM_CALLCONV SetUVTransform(FXMMATRIX value);
        void SetViewport( float width, float height );
        void SetTime( float time );
        void SetAlphaDiscardEnable(bool value);

        // Light settings.
        void SetLightingEnabled(bool value) override;
        void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

        void SetLightEnabled(int whichLight, bool value) override;
        void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

        void EnableDefaultLighting() override;

        static const int MaxDirectionalLights = 4;

        // Vertex color setting.
        void SetVertexColorEnabled(bool value);

        // Texture settings.
        void SetTextureEnabled(bool value);
        void SetTexture(_In_opt_ ID3D11ShaderResourceView* value);
        void SetTexture(int whichTexture, _In_opt_ ID3D11ShaderResourceView* value);

        static const int MaxTextures = 8;

        // Animation setting.
        void SetWeightsPerVertex(int value) override;
        void SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) override;
        void ResetBoneTransforms() override;

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Unsupported interface methods.
        void SetPerPixelLighting(bool value) override;

        // Prevent copying.
        DGSLEffect(DGSLEffect const&);
        DGSLEffect& operator= (DGSLEffect const&);
    };



    //----------------------------------------------------------------------------------
    // Abstract interface to factory for sharing effects and texture resources
    class IEffectFactory
    {
    public:
        virtual ~IEffectFactory() {}

        struct EffectInfo
        {
            const WCHAR*        name;
            bool                perVertexColor;
            bool                enableSkinning;
            float               specularPower;
            float               alpha;
            DirectX::XMFLOAT3   ambientColor;
            DirectX::XMFLOAT3   diffuseColor;
            DirectX::XMFLOAT3   specularColor;
            DirectX::XMFLOAT3   emissiveColor;
            const WCHAR*        texture;

            EffectInfo() { memset( this, 0, sizeof(EffectInfo) ); };
        };

        virtual std::shared_ptr<IEffect> CreateEffect( _In_ const EffectInfo& info, _In_opt_ ID3D11DeviceContext* deviceContext ) = 0;

        virtual void CreateTexture( _In_z_ const WCHAR* name, _In_opt_ ID3D11DeviceContext* deviceContext, _Outptr_ ID3D11ShaderResourceView** textureView ) = 0;
    };


    // Factory for sharing effects and texture resources
    class EffectFactory : public IEffectFactory
    {
    public:
        explicit EffectFactory(_In_ ID3D11Device* device);
        EffectFactory(EffectFactory&& moveFrom);
        EffectFactory& operator= (EffectFactory&& moveFrom);
        virtual ~EffectFactory();

        // IEffectFactory methods.
        virtual std::shared_ptr<IEffect> CreateEffect( _In_ const EffectInfo& info, _In_opt_ ID3D11DeviceContext* deviceContext ) override;
        virtual void CreateTexture( _In_z_ const WCHAR* name, _In_opt_ ID3D11DeviceContext* deviceContext, _Outptr_ ID3D11ShaderResourceView** textureView ) override;

        // Settings.
        void ReleaseCache();

        void SetSharing( bool enabled );

        void SetDirectory( _In_opt_z_ const WCHAR* path );

    private:
        // Private implementation.
        class Impl;

        std::shared_ptr<Impl> pImpl;

        // Prevent copying.
        EffectFactory(EffectFactory const&);
        EffectFactory& operator= (EffectFactory const&);
    };


    // Factory for sharing Visual Studio Shader Designer (DGSL) shaders and texture resources
    class DGSLEffectFactory : public IEffectFactory
    {
    public:
        explicit DGSLEffectFactory(_In_ ID3D11Device* device);
        DGSLEffectFactory(DGSLEffectFactory&& moveFrom);
        DGSLEffectFactory& operator= (DGSLEffectFactory&& moveFrom);
        virtual ~DGSLEffectFactory();

        // IEffectFactory methods.
        virtual std::shared_ptr<IEffect> CreateEffect( _In_ const EffectInfo& info, _In_opt_ ID3D11DeviceContext* deviceContext ) override;
        virtual void CreateTexture( _In_z_ const WCHAR* name, _In_opt_ ID3D11DeviceContext* deviceContext, _Outptr_ ID3D11ShaderResourceView** textureView ) override;

        // DGSL methods.
        struct DGSLEffectInfo : public EffectInfo
        {
            const WCHAR*        textures[7];
            const WCHAR*        pixelShader;

            DGSLEffectInfo() { memset( this, 0, sizeof(DGSLEffectInfo) ); };
        };

        virtual std::shared_ptr<IEffect> CreateDGSLEffect( _In_ const DGSLEffectInfo& info, _In_opt_ ID3D11DeviceContext* deviceContext );

        virtual void CreatePixelShader( _In_z_ const WCHAR* shader, _Outptr_ ID3D11PixelShader** pixelShader );

        // Settings.
        void ReleaseCache();

        void SetSharing( bool enabled );

        void SetDirectory( _In_opt_z_ const WCHAR* path );

    private:
        // Private implementation.
        class Impl;

        std::shared_ptr<Impl> pImpl;

        // Prevent copying.
        DGSLEffectFactory(DGSLEffectFactory const&);
        DGSLEffectFactory& operator= (DGSLEffectFactory const&);
    };

}

#pragma warning(pop)
