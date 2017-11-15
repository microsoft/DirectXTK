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

// VS 2013 related Off by default warnings
#pragma warning(disable : 4619 4616 4350 4351 4472 4640 5038)
// C4619/4616 #pragma warning warnings
// C4350 behavior change
// C4351 behavior change; warning removed in later versions
// C4472 'X' is a native enum: add an access specifier (private/public) to declare a WinRT enum
// C4640 construction of local static object is not thread-safe
// C5038 can't use strictly correct initialization order due to Dev12 initialization limitations

// Off by default warnings
#pragma warning(disable : 4061 4265 4365 4571 4623 4625 4626 4668 4710 4711 4746 4774 4820 4987 5026 5027 5031 5032)
// C4061 enumerator 'X' in switch of enum 'X' is not explicitly handled by a case label
// C4265 class has virtual functions, but destructor is not virtual
// C4365 signed/unsigned mismatch
// C4571 behavior change
// C4623 default constructor was implicitly defined as deleted
// C4625 copy constructor was implicitly defined as deleted
// C4626 assignment operator was implicitly defined as deleted
// C4668 not defined as a preprocessor macro
// C4710 function not inlined
// C4711 selected for automatic inline expansion
// C4746 volatile access of '<expression>' is subject to /volatile:<iso|ms> setting
// C4774 format string expected in argument 3 is not a string literal
// C4820 padding added after data member
// C4987 nonstandard extension used
// C5026 move constructor was implicitly defined as deleted
// C5027 move assignment operator was implicitly defined as deleted
// C5031/5032 push/pop mismatches in windows headers

// Windows 8.1 SDK related Off by default warnings
#pragma warning(disable : 4471 4917 4986 5029)
// C4471 forward declaration of an unscoped enumeration must have an underlying type
// C4917 a GUID can only be associated with a class, interface or namespace
// C4986 exception specification does not match previous declaration
// C5029 nonstandard extension used

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include <windows.h>

#ifndef _WIN32_WINNT_WIN10
#define _WIN32_WINNT_WIN10 0x0A00
#endif

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#define DCOMMON_H_INCLUDED
#else
#include <d3d11_1.h>
#endif

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
#pragma warning(push)
#pragma warning(disable: 4471)
#include <Windows.UI.Core.h>
#pragma warning(pop)
#endif

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>

#include <algorithm>
#include <array>
#include <exception>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4702)
#include <functional>
#pragma warning(pop)

#include <malloc.h>
#include <stdint.h>

#pragma warning(push)
#pragma warning(disable : 4467)
#include <wrl.h>
#pragma warning(pop)

#include <wincodec.h>
