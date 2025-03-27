//--------------------------------------------------------------------------------------
// File: PostProcess.h
//
// Copyright (c) Microsoft Corporation.
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

#include <cstdint>
#include <memory>
#include <functional>

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


namespace DirectX
{
    inline namespace DX11
    {
        //------------------------------------------------------------------------------
        // Abstract interface representing a post-process pass
        class DIRECTX_TOOLKIT_API IPostProcess
        {
        public:
            virtual ~IPostProcess() = default;

            IPostProcess(const IPostProcess&) = delete;
            IPostProcess& operator=(const IPostProcess&) = delete;

            virtual void __cdecl Process(
                _In_ ID3D11DeviceContext* deviceContext,
                _In_ std::function<void __cdecl()> setCustomState = nullptr) = 0;

        protected:
            IPostProcess() = default;
            IPostProcess(IPostProcess&&) = default;
            IPostProcess& operator=(IPostProcess&&) = default;
        };


        //------------------------------------------------------------------------------
        // Basic post-process
        class BasicPostProcess : public IPostProcess
        {
        public:
            enum Effect : uint32_t
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

            DIRECTX_TOOLKIT_API explicit BasicPostProcess(_In_ ID3D11Device* device);

            DIRECTX_TOOLKIT_API BasicPostProcess(BasicPostProcess&&) noexcept;
            DIRECTX_TOOLKIT_API BasicPostProcess& operator= (BasicPostProcess&&) noexcept;

            BasicPostProcess(BasicPostProcess const&) = delete;
            BasicPostProcess& operator= (BasicPostProcess const&) = delete;

            DIRECTX_TOOLKIT_API ~BasicPostProcess() override;

            // IPostProcess methods.
            DIRECTX_TOOLKIT_API void __cdecl Process(
                _In_ ID3D11DeviceContext* deviceContext,
                _In_ std::function<void __cdecl()> setCustomState = nullptr) override;

            // Shader control
            DIRECTX_TOOLKIT_API void __cdecl SetEffect(Effect fx);

            // Properties
            DIRECTX_TOOLKIT_API void __cdecl SetSourceTexture(_In_opt_ ID3D11ShaderResourceView* value);

            // Sets multiplier for GaussianBlur_5x5
            DIRECTX_TOOLKIT_API void __cdecl SetGaussianParameter(float multiplier);

            // Sets parameters for BloomExtract
            DIRECTX_TOOLKIT_API void __cdecl SetBloomExtractParameter(float threshold);

            // Sets parameters for BloomBlur
            DIRECTX_TOOLKIT_API void __cdecl SetBloomBlurParameters(bool horizontal, float size, float brightness);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };


        //------------------------------------------------------------------------------
        // Dual-texure post-process
        class DualPostProcess : public IPostProcess
        {
        public:
            enum Effect : uint32_t
            {
                Merge,
                BloomCombine,
                Effect_Max
            };

            DIRECTX_TOOLKIT_API explicit DualPostProcess(_In_ ID3D11Device* device);

            DIRECTX_TOOLKIT_API DualPostProcess(DualPostProcess&&) noexcept;
            DIRECTX_TOOLKIT_API DualPostProcess& operator= (DualPostProcess&&) noexcept;

            DualPostProcess(DualPostProcess const&) = delete;
            DualPostProcess& operator= (DualPostProcess const&) = delete;

            DIRECTX_TOOLKIT_API ~DualPostProcess() override;

            // IPostProcess methods.
            DIRECTX_TOOLKIT_API void __cdecl Process(
                _In_ ID3D11DeviceContext* deviceContext,
                _In_ std::function<void __cdecl()> setCustomState = nullptr) override;

            // Shader control
            DIRECTX_TOOLKIT_API void __cdecl SetEffect(Effect fx);

            // Properties
            DIRECTX_TOOLKIT_API void __cdecl SetSourceTexture(_In_opt_ ID3D11ShaderResourceView* value);
            DIRECTX_TOOLKIT_API void __cdecl SetSourceTexture2(_In_opt_ ID3D11ShaderResourceView* value);

            // Sets parameters for Merge
            DIRECTX_TOOLKIT_API void __cdecl SetMergeParameters(float weight1, float weight2);

            // Sets parameters for BloomCombine
            DIRECTX_TOOLKIT_API void __cdecl SetBloomCombineParameters(float bloom, float base, float bloomSaturation, float baseSaturation);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };


        //------------------------------------------------------------------------------
        // Tone-map post-process
        class ToneMapPostProcess : public IPostProcess
        {
        public:
            // Tone-mapping operator
            enum Operator : uint32_t
            {
                None,               // Pass-through
                Saturate,           // Clamp [0,1]
                Reinhard,           // x/(1+x)
                ACESFilmic,
                Operator_Max
            };

            // Electro-Optical Transfer Function (EOTF)
            enum TransferFunction : uint32_t
            {
                Linear,             // Pass-through
                SRGB,               // sRGB (Rec.709 and approximate sRGB display curve)
                ST2084,             // HDR10 (Rec.2020 color primaries and ST.2084 display curve)
                TransferFunction_Max
            };

            // Color Rotation Transform for HDR10
            enum ColorPrimaryRotation : uint32_t
            {
                HDTV_to_UHDTV,       // Rec.709 to Rec.2020
                DCI_P3_D65_to_UHDTV, // DCI-P3-D65 (a.k.a Display P3 or P3D65) to Rec.2020
                HDTV_to_DCI_P3_D65,  // Rec.709 to DCI-P3-D65 (a.k.a Display P3 or P3D65)
            };

            DIRECTX_TOOLKIT_API explicit ToneMapPostProcess(_In_ ID3D11Device* device);

            DIRECTX_TOOLKIT_API ToneMapPostProcess(ToneMapPostProcess&&) noexcept;
            DIRECTX_TOOLKIT_API ToneMapPostProcess& operator= (ToneMapPostProcess&&) noexcept;

            ToneMapPostProcess(ToneMapPostProcess const&) = delete;
            ToneMapPostProcess& operator= (ToneMapPostProcess const&) = delete;

            DIRECTX_TOOLKIT_API ~ToneMapPostProcess() override;

            // IPostProcess methods.
            DIRECTX_TOOLKIT_API void __cdecl Process(
                _In_ ID3D11DeviceContext* deviceContext,
                _In_ std::function<void __cdecl()> setCustomState = nullptr) override;

            // Shader control
            DIRECTX_TOOLKIT_API void __cdecl SetOperator(Operator op);

            DIRECTX_TOOLKIT_API void __cdecl SetTransferFunction(TransferFunction func);

        #if defined(_XBOX_ONE) && defined(_TITLE)
            // Uses Multiple Render Targets to generate both HDR10 and GameDVR SDR signals
            DIRECTX_TOOLKIT_API void __cdecl SetMRTOutput(bool value = true);
        #endif

            // Properties
            DIRECTX_TOOLKIT_API void __cdecl SetHDRSourceTexture(_In_opt_ ID3D11ShaderResourceView* value);

            // Sets the Color Rotation Transform for HDR10 signal output
            DIRECTX_TOOLKIT_API void __cdecl SetColorRotation(ColorPrimaryRotation value);
            DIRECTX_TOOLKIT_API void __cdecl SetColorRotation(CXMMATRIX value);

            // Sets exposure value for LDR tonemap operators
            DIRECTX_TOOLKIT_API void __cdecl SetExposure(float exposureValue);

            // Sets ST.2084 parameter for how bright white should be in nits
            DIRECTX_TOOLKIT_API void __cdecl SetST2084Parameter(float paperWhiteNits);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };
    }
}
