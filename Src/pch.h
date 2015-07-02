//--------------------------------------------------------------------------------------
// File: pch.h
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

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

#if !defined(NOMINMAX)
#define NOMINMAX
#endif

#ifndef _WIN32_WINNT_WIN10
#define _WIN32_WINNT_WIN10 0x0A00
#endif

// VS 2010/2012 do not support =default =delete
#ifndef DIRECTX_CTOR_DEFAULT
#if defined(_MSC_VER) && (_MSC_VER < 1800)
#define DIRECTX_CTOR_DEFAULT {}
#define DIRECTX_CTOR_DELETE ;
#else
#define DIRECTX_CTOR_DEFAULT =default;
#define DIRECTX_CTOR_DELETE =delete;
#endif
#endif

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#define DCOMMON_H_INCLUDED
#else
#include <d3d11_1.h>
#endif

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
#include <Windows.UI.Core.h>
#endif

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>

#include <algorithm>
#include <array>
#include <exception>
#include <malloc.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// VS 2010's stdint.h conflicts with intsafe.h
#pragma warning(push)
#pragma warning(disable : 4005)
#include <stdint.h>
#include <intsafe.h>
#pragma warning(pop)

#include <wrl.h>

namespace DirectX
{
    #if (DIRECTX_MATH_VERSION < 305) && !defined(XM_CALLCONV)
    #define XM_CALLCONV __fastcall
    typedef const XMVECTOR& HXMVECTOR;
    typedef const XMMATRIX& FXMMATRIX;
    #endif
}