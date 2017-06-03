//--------------------------------------------------------------------------------------
// File: PostProcess.h
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

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
#endif

#include <DirectXMath.h>
#include <memory>
#include <functional>


namespace DirectX
{
    //----------------------------------------------------------------------------------
    // Abstract interface representing a post-process pass
    class IPostProcess
    {
    public:
        virtual ~IPostProcess() { }

        virtual void __cdecl Process(_In_ ID3D11DeviceContext* deviceContext, _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) = 0;
    };


    //----------------------------------------------------------------------------------
    // Basic post-process
    class BasicPostProcess : public IPostProcess
    {
    public:
        enum Effect
        {
            Copy,
            Monochrome,
            DownScale_2x2,
            DownScale_4x4,
            // TODO - GaussBlur_5x5,
            // TODO - Bloom,
            // TODO - SampleLuminance,
            Effect_Max
        };

        explicit BasicPostProcess(_In_ ID3D11Device* device);
        BasicPostProcess(BasicPostProcess&& moveFrom);
        BasicPostProcess& operator= (BasicPostProcess&& moveFrom);

        BasicPostProcess(BasicPostProcess const&) = delete;
        BasicPostProcess& operator= (BasicPostProcess const&) = delete;

        virtual ~BasicPostProcess();

        // IPostProcess methods.
        void __cdecl Process(_In_ ID3D11DeviceContext* deviceContext, _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) override;

        // Mode control
        void __cdecl Set(Effect fx);

        // Properties
        void __cdecl SetSourceTexture(_In_opt_ ID3D11ShaderResourceView* value);

        // TODO - void SetBloomParameters(bool horizontal, float size = 3.f, float brigthness = 2.f);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };

    // TODO - DualPostProcess (Merge, Add, BrightPassFilter, AdaptLuminance)

    // TODO - ToneMapPostProcess (Reinhard, Reinhard_Filmic, ST2084)

    // TODO - ComputePostProcess (FXAA)
}
