//--------------------------------------------------------------------------------------
// File: PostProcess.h
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
#endif

#include <memory>
#include <functional>

#include <DirectXMath.h>


namespace DirectX
{
    //----------------------------------------------------------------------------------
    // Abstract interface representing a post-process pass
    class IPostProcess
    {
    public:
        virtual ~IPostProcess() = default;

        IPostProcess(const IPostProcess&) = delete;
        IPostProcess& operator=(const IPostProcess&) = delete;

        IPostProcess(IPostProcess&&) = delete;
        IPostProcess& operator=(IPostProcess&&) = delete;

        virtual void __cdecl Process(_In_ ID3D11DeviceContext* deviceContext,
            _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) = 0;

    protected:
        IPostProcess() = default;
    };


    //----------------------------------------------------------------------------------
    // Basic post-process
    class BasicPostProcess : public IPostProcess
    {
    public:
        enum Effect : unsigned int
        {
            Copy,
            Monochrome,
            Sepia,
            DownScale_2x2,
            DownScale_4x4,
            GaussianBlur_5x5,
            BloomExtract,
            BloomBlur,
            Effect_Max
        };

        explicit BasicPostProcess(_In_ ID3D11Device* device);
        BasicPostProcess(BasicPostProcess&& moveFrom) noexcept;
        BasicPostProcess& operator= (BasicPostProcess&& moveFrom) noexcept;

        BasicPostProcess(BasicPostProcess const&) = delete;
        BasicPostProcess& operator= (BasicPostProcess const&) = delete;

        ~BasicPostProcess() override;

        // IPostProcess methods.
        void __cdecl Process(
            _In_ ID3D11DeviceContext* deviceContext,
            _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) override;

        // Shader control
        void __cdecl SetEffect(Effect fx);

        // Properties
        void __cdecl SetSourceTexture(_In_opt_ ID3D11ShaderResourceView* value);

        // Sets multiplier for GaussianBlur_5x5
        void __cdecl SetGaussianParameter(float multiplier);

        // Sets parameters for BloomExtract
        void __cdecl SetBloomExtractParameter(float threshold);

        // Sets parameters for BloomBlur
        void __cdecl SetBloomBlurParameters(bool horizontal, float size, float brightness);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };


    //----------------------------------------------------------------------------------
    // Dual-texure post-process
    class DualPostProcess : public IPostProcess
    {
    public:
        enum Effect : unsigned int
        {
            Merge,
            BloomCombine,
            Effect_Max
        };

        explicit DualPostProcess(_In_ ID3D11Device* device);
        DualPostProcess(DualPostProcess&& moveFrom) noexcept;
        DualPostProcess& operator= (DualPostProcess&& moveFrom) noexcept;

        DualPostProcess(DualPostProcess const&) = delete;
        DualPostProcess& operator= (DualPostProcess const&) = delete;

        ~DualPostProcess() override;

        // IPostProcess methods.
        void __cdecl Process(_In_ ID3D11DeviceContext* deviceContext,
            _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) override;

        // Shader control
        void __cdecl SetEffect(Effect fx);

        // Properties
        void __cdecl SetSourceTexture(_In_opt_ ID3D11ShaderResourceView* value);
        void __cdecl SetSourceTexture2(_In_opt_ ID3D11ShaderResourceView* value);

        // Sets parameters for Merge
        void __cdecl SetMergeParameters(float weight1, float weight2);

        // Sets parameters for BloomCombine
        void __cdecl SetBloomCombineParameters(float bloom, float base, float bloomSaturation, float baseSaturation);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };


    //----------------------------------------------------------------------------------
    // Tone-map post-process
    class ToneMapPostProcess : public IPostProcess
    {
    public:
        // Tone-mapping operator
        enum Operator : unsigned int
        {
            None,               // Pass-through
            Saturate,           // Clamp [0,1]
            Reinhard,           // x/(1+x)
            ACESFilmic,
            Operator_Max
        };

        // Electro-Optical Transfer Function (EOTF)
        enum TransferFunction : unsigned int
        {
            Linear,             // Pass-through
            SRGB,               // sRGB (Rec.709 and approximate sRGB display curve)
            ST2084,             // HDR10 (Rec.2020 color primaries and ST.2084 display curve)
            TransferFunction_Max
        };

        explicit ToneMapPostProcess(_In_ ID3D11Device* device);
        ToneMapPostProcess(ToneMapPostProcess&& moveFrom) noexcept;
        ToneMapPostProcess& operator= (ToneMapPostProcess&& moveFrom) noexcept;

        ToneMapPostProcess(ToneMapPostProcess const&) = delete;
        ToneMapPostProcess& operator= (ToneMapPostProcess const&) = delete;

        ~ToneMapPostProcess() override;

        // IPostProcess methods.
        void __cdecl Process(_In_ ID3D11DeviceContext* deviceContext,
            _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) override;

        // Shader control
        void __cdecl SetOperator(Operator op);

        void __cdecl SetTransferFunction(TransferFunction func);

    #if defined(_XBOX_ONE) && defined(_TITLE)
        // Uses Multiple Render Targets to generate both HDR10 and GameDVR SDR signals
        void __cdecl SetMRTOutput(bool value = true);
    #endif

        // Properties
        void __cdecl SetHDRSourceTexture(_In_opt_ ID3D11ShaderResourceView* value);

        // Sets exposure value for LDR tonemap operators
        void SetExposure(float exposureValue);

        // Sets ST.2084 parameter for how bright white should be in nits
        void SetST2084Parameter(float paperWhiteNits);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };
}
