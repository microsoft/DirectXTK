//--------------------------------------------------------------------------------------
// File: ScreenGrab.h
//
// Function for capturing a 2D texture and saving it to a file (aka a 'screenshot'
// when used on a Direct3D Render Target).
//
// Note these functions are useful as a light-weight runtime screen grabber. For
// full-featured texture capture, DDS writer, and texture processing pipeline,
// see the 'Texconv' sample and the 'DirectXTex' library.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
#endif

#include <ocidl.h>

#include <functional>
#include <stdint.h>


namespace DirectX
{
    HRESULT __cdecl SaveDDSTextureToFile(
        _In_ ID3D11DeviceContext* pContext,
        _In_ ID3D11Resource* pSource,
        _In_z_ const wchar_t* fileName);

    HRESULT __cdecl SaveWICTextureToFile(
        _In_ ID3D11DeviceContext* pContext,
        _In_ ID3D11Resource* pSource,
        _In_ REFGUID guidContainerFormat,
        _In_z_ const wchar_t* fileName,
        _In_opt_ const GUID* targetFormat = nullptr,
        _In_opt_ std::function<void __cdecl(IPropertyBag2*)> setCustomProps = nullptr);
}