//-------------------------------------------------------------------------------------
// SimpleMath.cpp -- Simplified C++ Math wrapper for DirectXMath
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//  
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleMath.h"

namespace DirectX
{

namespace SimpleMath
{

#if defined(_MSC_VER) && (_MSC_VER < 1800)
    const Vector2 Vector2::Zero(0.f, 0.f);
    const Vector2 Vector2::One(1.f, 1.f);
    const Vector2 Vector2::UnitX(1.f, 0.f);
    const Vector2 Vector2::UnitY(0.f, 1.f);

    const Vector3 Vector3::Zero(0.f, 0.f, 0.f);
    const Vector3 Vector3::One(1.f, 1.f, 1.f);
    const Vector3 Vector3::UnitX(1.f, 0.f, 0.f);
    const Vector3 Vector3::UnitY(0.f, 1.f, 0.f);
    const Vector3 Vector3::UnitZ(0.f, 0.f, 1.f);
    const Vector3 Vector3::Up(0.f, 1.f, 0.f);
    const Vector3 Vector3::Down(0.f, -1.f, 0.f);
    const Vector3 Vector3::Right(1.f, 0.f, 0.f);
    const Vector3 Vector3::Left(-1.f, 0.f, 0.f);
    const Vector3 Vector3::Forward(0.f, 0.f, -1.f);
    const Vector3 Vector3::Backward(0.f, 0.f, 1.f);

    const Vector4 Vector4::Zero(0.f, 0.f, 0.f, 0.f);
    const Vector4 Vector4::One(1.f, 1.f, 1.f, 1.f);
    const Vector4 Vector4::UnitX(1.f, 0.f, 0.f, 0.f);
    const Vector4 Vector4::UnitY(0.f, 1.f, 0.f, 0.f);
    const Vector4 Vector4::UnitZ(0.f, 0.f, 1.f, 0.f);
    const Vector4 Vector4::UnitW(0.f, 0.f, 0.f, 1.f);

    const Matrix Matrix::Identity(1.f, 0.f, 0.f, 0.f,
                                  0.f, 1.f, 0.f, 0.f,
                                  0.f, 0.f, 1.f, 0.f,
                                  0.f, 0.f, 0.f, 1.f);

    const Quaternion Quaternion::Identity(0.f, 0.f, 0.f, 1.f);
#else
    const Vector2 Vector2::Zero = { 0.f, 0.f };
    const Vector2 Vector2::One = { 1.f, 1.f };
    const Vector2 Vector2::UnitX = { 1.f, 0.f };
    const Vector2 Vector2::UnitY = { 0.f, 1.f };

    const Vector3 Vector3::Zero = { 0.f, 0.f, 0.f };
    const Vector3 Vector3::One = { 1.f, 1.f, 1.f };
    const Vector3 Vector3::UnitX = { 1.f, 0.f, 0.f };
    const Vector3 Vector3::UnitY = { 0.f, 1.f, 0.f };
    const Vector3 Vector3::UnitZ = { 0.f, 0.f, 1.f };
    const Vector3 Vector3::Up = { 0.f, 1.f, 0.f };
    const Vector3 Vector3::Down = { 0.f, -1.f, 0.f };
    const Vector3 Vector3::Right = { 1.f, 0.f, 0.f };
    const Vector3 Vector3::Left = { -1.f, 0.f, 0.f };
    const Vector3 Vector3::Forward = { 0.f, 0.f, -1.f };
    const Vector3 Vector3::Backward = { 0.f, 0.f, 1.f };

    const Vector4 Vector4::Zero = { 0.f, 0.f, 0.f, 0.f };
    const Vector4 Vector4::One = { 1.f, 1.f, 1.f, 1.f };
    const Vector4 Vector4::UnitX = { 1.f, 0.f, 0.f, 0.f };
    const Vector4 Vector4::UnitY = { 0.f, 1.f, 0.f, 0.f };
    const Vector4 Vector4::UnitZ = { 0.f, 0.f, 1.f, 0.f };
    const Vector4 Vector4::UnitW = { 0.f, 0.f, 0.f, 1.f };

    const Matrix Matrix::Identity = { 1.f, 0.f, 0.f, 0.f,
                                      0.f, 1.f, 0.f, 0.f,
                                      0.f, 0.f, 1.f, 0.f,
                                      0.f, 0.f, 0.f, 1.f };

    const Quaternion Quaternion::Identity = { 0.f, 0.f, 0.f, 1.f };
#endif
}

}