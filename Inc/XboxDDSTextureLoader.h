//--------------------------------------------------------------------------------------
// File: XboxDDSTextureLoader.h
//
// Functions for loading a DDS texture using the XBOX extended header and creating a
// Direct3D11.X runtime resource for it via the CreatePlacement APIs
//
// Note these functions will not load standard DDS files. Use the DDSTextureLoader
// module in the DirectXTex package or as part of the DirectXTK library to load
// these files which use standard Direct3D 11 resource creation APIs.
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

#if !defined(_XBOX_ONE) || !defined(_TITLE)
#error This module only supports Xbox One exclusive apps
#endif

#include <d3d11_x.h>

#include <stdint.h>

namespace Xbox
{
    enum DDS_ALPHA_MODE
    {
        DDS_ALPHA_MODE_UNKNOWN       = 0,
        DDS_ALPHA_MODE_STRAIGHT      = 1,
        DDS_ALPHA_MODE_PREMULTIPLIED = 2,
        DDS_ALPHA_MODE_OPAQUE        = 3,
        DDS_ALPHA_MODE_CUSTOM        = 4,
    };

    HRESULT __cdecl CreateDDSTextureFromMemory( _In_ ID3D11DeviceX* d3dDevice,
                                                _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                                _In_ size_t ddsDataSize,
                                                _Outptr_opt_ ID3D11Resource** texture,
                                                _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                                _Outptr_ void** grfxMemory,
                                                _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr, 
                                                _In_ bool forceSRGB = false
                                               );

    HRESULT __cdecl CreateDDSTextureFromFile( _In_ ID3D11DeviceX* d3dDevice,
                                              _In_z_ const wchar_t* szFileName,
                                              _Outptr_opt_ ID3D11Resource** texture,
                                              _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                              _Outptr_ void** grfxMemory,
                                              _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
                                              _In_ bool forceSRGB = false
                                            );
}