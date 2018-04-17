//-------------------------------------------------------------------------------------
// SimpleMath.h -- Simplified C++ Math wrapper for DirectXMath
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//-------------------------------------------------------------------------------------

#pragma once

#if !defined(__d3d11_h__) && !defined(__d3d11_x_h__) && !defined(__d3d12_h__) && !defined(__d3d12_x_h__)
#error include d3d11.h or d3d12.h before including SimpleMath.h
#endif

#if !defined(_XBOX_ONE) || !defined(_TITLE)
#include <dxgi1_2.h>
#endif

#include <functional>
#include <assert.h>
#include <memory.h>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>

#ifndef XM_CONSTEXPR
#define XM_CONSTEXPR
#endif


namespace DirectX
{
    namespace SimpleMath
    {
        struct Vector2;
        struct Vector4;
        struct Matrix;
        struct Quaternion;
        struct Plane;

        //------------------------------------------------------------------------------
        // 2D rectangle
        struct Rectangle
        {
            long x;
            long y;
            long width;
            long height;

            // Creators
            Rectangle() throw() : x(0), y(0), width(0), height(0) {}
            XM_CONSTEXPR Rectangle(long ix, long iy, long iw, long ih) : x(ix), y(iy), width(iw), height(ih) {}
            explicit Rectangle(const RECT& rct) : x(rct.left), y(rct.top), width(rct.right - rct.left), height(rct.bottom - rct.top) {}

            Rectangle(const Rectangle&) = default;
            Rectangle& operator=(const Rectangle&) = default;

        #if !defined(_MSC_VER) || _MSC_VER >= 1900
            Rectangle(Rectangle&&) = default;
            Rectangle& operator=(Rectangle&&) = default;
        #endif

            operator RECT() { RECT rct; rct.left = x; rct.top = y; rct.right = (x + width); rct.bottom = (y + height); return rct; }
        #ifdef __cplusplus_winrt
            operator Windows::Foundation::Rect() { return Windows::Foundation::Rect(float(x), float(y), float(width), float(height)); }
        #endif

            // Comparison operators
            bool operator == (const Rectangle& r) const { return (x == r.x) && (y == r.y) && (width == r.width) && (height == r.height); }
            bool operator == (const RECT& rct) const { return (x == rct.left) && (y == rct.top) && (width == (rct.right - rct.left)) && (height == (rct.bottom - rct.top)); }

            bool operator != (const Rectangle& r) const { return (x != r.x) || (y != r.y) || (width != r.width) || (height != r.height); }
            bool operator != (const RECT& rct) const { return (x != rct.left) || (y != rct.top) || (width != (rct.right - rct.left)) || (height != (rct.bottom - rct.top)); }

            // Assignment operators
            Rectangle& operator=(_In_ const RECT& rct) { x = rct.left; y = rct.top; width = (rct.right - rct.left); height = (rct.bottom - rct.top); return *this; }

            // Rectangle operations
            Vector2 Location() const;
            Vector2 Center() const;

            bool IsEmpty() const { return (width == 0 && height == 0 && x == 0 && y == 0); }

            bool Contains(long ix, long iy) const { return (x <= ix) && (ix < (x + width)) && (y <= iy) && (iy < (y + height)); }
            bool Contains(const Vector2& point) const;
            bool Contains(const Rectangle& r) const { return (x <= r.x) && ((r.x + r.width) <= (x + width)) && (y <= r.y) && ((r.y + r.height) <= (y + height)); }
            bool Contains(const RECT& rct) const { return (x <= rct.left) && (rct.right <= (x + width)) && (y <= rct.top) && (rct.bottom <= (y + height)); }

            void Inflate(long horizAmount, long vertAmount);

            bool Intersects(const Rectangle& r) const { return (r.x < (x + width)) && (x < (r.x + r.width)) && (r.y < (y + height)) && (y < (r.y + r.height)); }
            bool Intersects(const RECT& rct) const { return (rct.left < (x + width)) && (x < rct.right) && (rct.top < (y + height)) && (y < rct.bottom); }

            void Offset(long ox, long oy) { x += ox; y += oy; }

            // Static functions
            static Rectangle Intersect(const Rectangle& ra, const Rectangle& rb);
            static RECT Intersect(const RECT& rcta, const RECT& rctb);

            static Rectangle Union(const Rectangle& ra, const Rectangle& rb);
            static RECT Union(const RECT& rcta, const RECT& rctb);
        };

        //------------------------------------------------------------------------------
        // 2D vector
        struct Vector2 : public XMFLOAT2
        {
            Vector2() throw() : XMFLOAT2(0.f, 0.f) {}
            XM_CONSTEXPR explicit Vector2(float x) : XMFLOAT2(x, x) {}
            XM_CONSTEXPR Vector2(float _x, float _y) : XMFLOAT2(_x, _y) {}
            explicit Vector2(_In_reads_(2) const float *pArray) : XMFLOAT2(pArray) {}
            Vector2(FXMVECTOR V) { XMStoreFloat2(this, V); }
            Vector2(const XMFLOAT2& V) { this->x = V.x; this->y = V.y; }
            explicit Vector2(const XMVECTORF32& F) { this->x = F.f[0]; this->y = F.f[1]; }

            Vector2(const Vector2&) = default;
            Vector2& operator=(const Vector2&) = default;

        #if !defined(_MSC_VER) || _MSC_VER >= 1900
            Vector2(Vector2&&) = default;
            Vector2& operator=(Vector2&&) = default;
        #endif

            operator XMVECTOR() const { return XMLoadFloat2(this); }

            // Comparison operators
            bool operator == (const Vector2& V) const;
            bool operator != (const Vector2& V) const;

            // Assignment operators
            Vector2& operator= (const XMVECTORF32& F) { x = F.f[0]; y = F.f[1]; return *this; }
            Vector2& operator+= (const Vector2& V);
            Vector2& operator-= (const Vector2& V);
            Vector2& operator*= (const Vector2& V);
            Vector2& operator*= (float S);
            Vector2& operator/= (float S);

            // Unary operators
            Vector2 operator+ () const { return *this; }
            Vector2 operator- () const { return Vector2(-x, -y); }

            // Vector operations
            bool InBounds(const Vector2& Bounds) const;

            float Length() const;
            float LengthSquared() const;

            float Dot(const Vector2& V) const;
            void Cross(const Vector2& V, Vector2& result) const;
            Vector2 Cross(const Vector2& V) const;

            void Normalize();
            void Normalize(Vector2& result) const;

            void Clamp(const Vector2& vmin, const Vector2& vmax);
            void Clamp(const Vector2& vmin, const Vector2& vmax, Vector2& result) const;

            // Static functions
            static float Distance(const Vector2& v1, const Vector2& v2);
            static float DistanceSquared(const Vector2& v1, const Vector2& v2);

            static void Min(const Vector2& v1, const Vector2& v2, Vector2& result);
            static Vector2 Min(const Vector2& v1, const Vector2& v2);

            static void Max(const Vector2& v1, const Vector2& v2, Vector2& result);
            static Vector2 Max(const Vector2& v1, const Vector2& v2);

            static void Lerp(const Vector2& v1, const Vector2& v2, float t, Vector2& result);
            static Vector2 Lerp(const Vector2& v1, const Vector2& v2, float t);

            static void SmoothStep(const Vector2& v1, const Vector2& v2, float t, Vector2& result);
            static Vector2 SmoothStep(const Vector2& v1, const Vector2& v2, float t);

            static void Barycentric(const Vector2& v1, const Vector2& v2, const Vector2& v3, float f, float g, Vector2& result);
            static Vector2 Barycentric(const Vector2& v1, const Vector2& v2, const Vector2& v3, float f, float g);

            static void CatmullRom(const Vector2& v1, const Vector2& v2, const Vector2& v3, const Vector2& v4, float t, Vector2& result);
            static Vector2 CatmullRom(const Vector2& v1, const Vector2& v2, const Vector2& v3, const Vector2& v4, float t);

            static void Hermite(const Vector2& v1, const Vector2& t1, const Vector2& v2, const Vector2& t2, float t, Vector2& result);
            static Vector2 Hermite(const Vector2& v1, const Vector2& t1, const Vector2& v2, const Vector2& t2, float t);

            static void Reflect(const Vector2& ivec, const Vector2& nvec, Vector2& result);
            static Vector2 Reflect(const Vector2& ivec, const Vector2& nvec);

            static void Refract(const Vector2& ivec, const Vector2& nvec, float refractionIndex, Vector2& result);
            static Vector2 Refract(const Vector2& ivec, const Vector2& nvec, float refractionIndex);

            static void Transform(const Vector2& v, const Quaternion& quat, Vector2& result);
            static Vector2 Transform(const Vector2& v, const Quaternion& quat);

            static void Transform(const Vector2& v, const Matrix& m, Vector2& result);
            static Vector2 Transform(const Vector2& v, const Matrix& m);
            static void Transform(_In_reads_(count) const Vector2* varray, size_t count, const Matrix& m, _Out_writes_(count) Vector2* resultArray);

            static void Transform(const Vector2& v, const Matrix& m, Vector4& result);
            static void Transform(_In_reads_(count) const Vector2* varray, size_t count, const Matrix& m, _Out_writes_(count) Vector4* resultArray);

            static void TransformNormal(const Vector2& v, const Matrix& m, Vector2& result);
            static Vector2 TransformNormal(const Vector2& v, const Matrix& m);
            static void TransformNormal(_In_reads_(count) const Vector2* varray, size_t count, const Matrix& m, _Out_writes_(count) Vector2* resultArray);

            // Constants
            static const Vector2 Zero;
            static const Vector2 One;
            static const Vector2 UnitX;
            static const Vector2 UnitY;
        };

        // Binary operators
        Vector2 operator+ (const Vector2& V1, const Vector2& V2);
        Vector2 operator- (const Vector2& V1, const Vector2& V2);
        Vector2 operator* (const Vector2& V1, const Vector2& V2);
        Vector2 operator* (const Vector2& V, float S);
        Vector2 operator/ (const Vector2& V1, const Vector2& V2);
        Vector2 operator* (float S, const Vector2& V);

        //------------------------------------------------------------------------------
        // 3D vector
        struct Vector3 : public XMFLOAT3
        {
            Vector3() throw() : XMFLOAT3(0.f, 0.f, 0.f) {}
            XM_CONSTEXPR explicit Vector3(float x) : XMFLOAT3(x, x, x) {}
            XM_CONSTEXPR Vector3(float _x, float _y, float _z) : XMFLOAT3(_x, _y, _z) {}
            explicit Vector3(_In_reads_(3) const float *pArray) : XMFLOAT3(pArray) {}
            Vector3(FXMVECTOR V) { XMStoreFloat3(this, V); }
            Vector3(const XMFLOAT3& V) { this->x = V.x; this->y = V.y; this->z = V.z; }
            explicit Vector3(const XMVECTORF32& F) { this->x = F.f[0]; this->y = F.f[1]; this->z = F.f[2]; }

            Vector3(const Vector3&) = default;
            Vector3& operator=(const Vector3&) = default;

        #if !defined(_MSC_VER) || _MSC_VER >= 1900
            Vector3(Vector3&&) = default;
            Vector3& operator=(Vector3&&) = default;
        #endif

            operator XMVECTOR() const { return XMLoadFloat3(this); }

            // Comparison operators
            bool operator == (const Vector3& V) const;
            bool operator != (const Vector3& V) const;

            // Assignment operators
            Vector3& operator= (const XMVECTORF32& F) { x = F.f[0]; y = F.f[1]; z = F.f[2]; return *this; }
            Vector3& operator+= (const Vector3& V);
            Vector3& operator-= (const Vector3& V);
            Vector3& operator*= (const Vector3& V);
            Vector3& operator*= (float S);
            Vector3& operator/= (float S);

            // Unary operators
            Vector3 operator+ () const { return *this; }
            Vector3 operator- () const;

            // Vector operations
            bool InBounds(const Vector3& Bounds) const;

            float Length() const;
            float LengthSquared() const;

            float Dot(const Vector3& V) const;
            void Cross(const Vector3& V, Vector3& result) const;
            Vector3 Cross(const Vector3& V) const;

            void Normalize();
            void Normalize(Vector3& result) const;

            void Clamp(const Vector3& vmin, const Vector3& vmax);
            void Clamp(const Vector3& vmin, const Vector3& vmax, Vector3& result) const;

            // Static functions
            static float Distance(const Vector3& v1, const Vector3& v2);
            static float DistanceSquared(const Vector3& v1, const Vector3& v2);

            static void Min(const Vector3& v1, const Vector3& v2, Vector3& result);
            static Vector3 Min(const Vector3& v1, const Vector3& v2);

            static void Max(const Vector3& v1, const Vector3& v2, Vector3& result);
            static Vector3 Max(const Vector3& v1, const Vector3& v2);

            static void Lerp(const Vector3& v1, const Vector3& v2, float t, Vector3& result);
            static Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t);

            static void SmoothStep(const Vector3& v1, const Vector3& v2, float t, Vector3& result);
            static Vector3 SmoothStep(const Vector3& v1, const Vector3& v2, float t);

            static void Barycentric(const Vector3& v1, const Vector3& v2, const Vector3& v3, float f, float g, Vector3& result);
            static Vector3 Barycentric(const Vector3& v1, const Vector3& v2, const Vector3& v3, float f, float g);

            static void CatmullRom(const Vector3& v1, const Vector3& v2, const Vector3& v3, const Vector3& v4, float t, Vector3& result);
            static Vector3 CatmullRom(const Vector3& v1, const Vector3& v2, const Vector3& v3, const Vector3& v4, float t);

            static void Hermite(const Vector3& v1, const Vector3& t1, const Vector3& v2, const Vector3& t2, float t, Vector3& result);
            static Vector3 Hermite(const Vector3& v1, const Vector3& t1, const Vector3& v2, const Vector3& t2, float t);

            static void Reflect(const Vector3& ivec, const Vector3& nvec, Vector3& result);
            static Vector3 Reflect(const Vector3& ivec, const Vector3& nvec);

            static void Refract(const Vector3& ivec, const Vector3& nvec, float refractionIndex, Vector3& result);
            static Vector3 Refract(const Vector3& ivec, const Vector3& nvec, float refractionIndex);

            static void Transform(const Vector3& v, const Quaternion& quat, Vector3& result);
            static Vector3 Transform(const Vector3& v, const Quaternion& quat);

            static void Transform(const Vector3& v, const Matrix& m, Vector3& result);
            static Vector3 Transform(const Vector3& v, const Matrix& m);
            static void Transform(_In_reads_(count) const Vector3* varray, size_t count, const Matrix& m, _Out_writes_(count) Vector3* resultArray);

            static void Transform(const Vector3& v, const Matrix& m, Vector4& result);
            static void Transform(_In_reads_(count) const Vector3* varray, size_t count, const Matrix& m, _Out_writes_(count) Vector4* resultArray);

            static void TransformNormal(const Vector3& v, const Matrix& m, Vector3& result);
            static Vector3 TransformNormal(const Vector3& v, const Matrix& m);
            static void TransformNormal(_In_reads_(count) const Vector3* varray, size_t count, const Matrix& m, _Out_writes_(count) Vector3* resultArray);

            // Constants
            static const Vector3 Zero;
            static const Vector3 One;
            static const Vector3 UnitX;
            static const Vector3 UnitY;
            static const Vector3 UnitZ;
            static const Vector3 Up;
            static const Vector3 Down;
            static const Vector3 Right;
            static const Vector3 Left;
            static const Vector3 Forward;
            static const Vector3 Backward;
        };

        // Binary operators
        Vector3 operator+ (const Vector3& V1, const Vector3& V2);
        Vector3 operator- (const Vector3& V1, const Vector3& V2);
        Vector3 operator* (const Vector3& V1, const Vector3& V2);
        Vector3 operator* (const Vector3& V, float S);
        Vector3 operator/ (const Vector3& V1, const Vector3& V2);
        Vector3 operator* (float S, const Vector3& V);

        //------------------------------------------------------------------------------
        // 4D vector
        struct Vector4 : public XMFLOAT4
        {
            Vector4() throw() : XMFLOAT4(0.f, 0.f, 0.f, 0.f) {}
            XM_CONSTEXPR explicit Vector4(float x) : XMFLOAT4(x, x, x, x) {}
            XM_CONSTEXPR Vector4(float _x, float _y, float _z, float _w) : XMFLOAT4(_x, _y, _z, _w) {}
            explicit Vector4(_In_reads_(4) const float *pArray) : XMFLOAT4(pArray) {}
            Vector4(FXMVECTOR V) { XMStoreFloat4(this, V); }
            Vector4(const XMFLOAT4& V) { this->x = V.x; this->y = V.y; this->z = V.z; this->w = V.w; }
            explicit Vector4(const XMVECTORF32& F) { this->x = F.f[0]; this->y = F.f[1]; this->z = F.f[2]; this->w = F.f[3]; }

            Vector4(const Vector4&) = default;
            Vector4& operator=(const Vector4&) = default;

        #if !defined(_MSC_VER) || _MSC_VER >= 1900
            Vector4(Vector4&&) = default;
            Vector4& operator=(Vector4&&) = default;
        #endif

            operator XMVECTOR() const { return XMLoadFloat4(this); }

            // Comparison operators
            bool operator == (const Vector4& V) const;
            bool operator != (const Vector4& V) const;

            // Assignment operators
            Vector4& operator= (const XMVECTORF32& F) { x = F.f[0]; y = F.f[1]; z = F.f[2]; w = F.f[3]; return *this; }
            Vector4& operator+= (const Vector4& V);
            Vector4& operator-= (const Vector4& V);
            Vector4& operator*= (const Vector4& V);
            Vector4& operator*= (float S);
            Vector4& operator/= (float S);

            // Unary operators
            Vector4 operator+ () const { return *this; }
            Vector4 operator- () const;

            // Vector operations
            bool InBounds(const Vector4& Bounds) const;

            float Length() const;
            float LengthSquared() const;

            float Dot(const Vector4& V) const;
            void Cross(const Vector4& v1, const Vector4& v2, Vector4& result) const;
            Vector4 Cross(const Vector4& v1, const Vector4& v2) const;

            void Normalize();
            void Normalize(Vector4& result) const;

            void Clamp(const Vector4& vmin, const Vector4& vmax);
            void Clamp(const Vector4& vmin, const Vector4& vmax, Vector4& result) const;

            // Static functions
            static float Distance(const Vector4& v1, const Vector4& v2);
            static float DistanceSquared(const Vector4& v1, const Vector4& v2);

            static void Min(const Vector4& v1, const Vector4& v2, Vector4& result);
            static Vector4 Min(const Vector4& v1, const Vector4& v2);

            static void Max(const Vector4& v1, const Vector4& v2, Vector4& result);
            static Vector4 Max(const Vector4& v1, const Vector4& v2);

            static void Lerp(const Vector4& v1, const Vector4& v2, float t, Vector4& result);
            static Vector4 Lerp(const Vector4& v1, const Vector4& v2, float t);

            static void SmoothStep(const Vector4& v1, const Vector4& v2, float t, Vector4& result);
            static Vector4 SmoothStep(const Vector4& v1, const Vector4& v2, float t);

            static void Barycentric(const Vector4& v1, const Vector4& v2, const Vector4& v3, float f, float g, Vector4& result);
            static Vector4 Barycentric(const Vector4& v1, const Vector4& v2, const Vector4& v3, float f, float g);

            static void CatmullRom(const Vector4& v1, const Vector4& v2, const Vector4& v3, const Vector4& v4, float t, Vector4& result);
            static Vector4 CatmullRom(const Vector4& v1, const Vector4& v2, const Vector4& v3, const Vector4& v4, float t);

            static void Hermite(const Vector4& v1, const Vector4& t1, const Vector4& v2, const Vector4& t2, float t, Vector4& result);
            static Vector4 Hermite(const Vector4& v1, const Vector4& t1, const Vector4& v2, const Vector4& t2, float t);

            static void Reflect(const Vector4& ivec, const Vector4& nvec, Vector4& result);
            static Vector4 Reflect(const Vector4& ivec, const Vector4& nvec);

            static void Refract(const Vector4& ivec, const Vector4& nvec, float refractionIndex, Vector4& result);
            static Vector4 Refract(const Vector4& ivec, const Vector4& nvec, float refractionIndex);

            static void Transform(const Vector2& v, const Quaternion& quat, Vector4& result);
            static Vector4 Transform(const Vector2& v, const Quaternion& quat);

            static void Transform(const Vector3& v, const Quaternion& quat, Vector4& result);
            static Vector4 Transform(const Vector3& v, const Quaternion& quat);

            static void Transform(const Vector4& v, const Quaternion& quat, Vector4& result);
            static Vector4 Transform(const Vector4& v, const Quaternion& quat);

            static void Transform(const Vector4& v, const Matrix& m, Vector4& result);
            static Vector4 Transform(const Vector4& v, const Matrix& m);
            static void Transform(_In_reads_(count) const Vector4* varray, size_t count, const Matrix& m, _Out_writes_(count) Vector4* resultArray);

            // Constants
            static const Vector4 Zero;
            static const Vector4 One;
            static const Vector4 UnitX;
            static const Vector4 UnitY;
            static const Vector4 UnitZ;
            static const Vector4 UnitW;
        };

        // Binary operators
        Vector4 operator+ (const Vector4& V1, const Vector4& V2);
        Vector4 operator- (const Vector4& V1, const Vector4& V2);
        Vector4 operator* (const Vector4& V1, const Vector4& V2);
        Vector4 operator* (const Vector4& V, float S);
        Vector4 operator/ (const Vector4& V1, const Vector4& V2);
        Vector4 operator* (float S, const Vector4& V);

        //------------------------------------------------------------------------------
        // 4x4 Matrix (assumes right-handed cooordinates)
        struct Matrix : public XMFLOAT4X4
        {
            Matrix() throw()
                : XMFLOAT4X4(1.f, 0, 0, 0,
                            0, 1.f, 0, 0,
                            0, 0, 1.f, 0,
                            0, 0, 0, 1.f) {}
            XM_CONSTEXPR Matrix(float m00, float m01, float m02, float m03,
                                float m10, float m11, float m12, float m13,
                                float m20, float m21, float m22, float m23,
                                float m30, float m31, float m32, float m33)
                : XMFLOAT4X4(m00, m01, m02, m03,
                             m10, m11, m12, m13,
                             m20, m21, m22, m23,
                             m30, m31, m32, m33) {}
            explicit Matrix(const Vector3& r0, const Vector3& r1, const Vector3& r2)
                : XMFLOAT4X4(r0.x, r0.y, r0.z, 0,
                             r1.x, r1.y, r1.z, 0,
                             r2.x, r2.y, r2.z, 0,
                             0, 0, 0, 1.f) {}
            explicit Matrix(const Vector4& r0, const Vector4& r1, const Vector4& r2, const Vector4& r3)
                : XMFLOAT4X4(r0.x, r0.y, r0.z, r0.w,
                             r1.x, r1.y, r1.z, r1.w,
                             r2.x, r2.y, r2.z, r2.w,
                             r3.x, r3.y, r3.z, r3.w) {}
            Matrix(const XMFLOAT4X4& M) { memcpy_s(this, sizeof(float) * 16, &M, sizeof(XMFLOAT4X4)); }
            Matrix(const XMFLOAT3X3& M);
            Matrix(const XMFLOAT4X3& M);

            explicit Matrix(_In_reads_(16) const float *pArray) : XMFLOAT4X4(pArray) {}
            Matrix(CXMMATRIX M) { XMStoreFloat4x4(this, M); }

            Matrix(const Matrix&) = default;
            Matrix& operator=(const Matrix&) = default;

        #if !defined(_MSC_VER) || _MSC_VER >= 1900
            Matrix(Matrix&&) = default;
            Matrix& operator=(Matrix&&) = default;
        #endif

            operator XMMATRIX() const { return XMLoadFloat4x4(this); }

            // Comparison operators
            bool operator == (const Matrix& M) const;
            bool operator != (const Matrix& M) const;

            // Assignment operators
            Matrix& operator= (const XMFLOAT3X3& M);
            Matrix& operator= (const XMFLOAT4X3& M);
            Matrix& operator+= (const Matrix& M);
            Matrix& operator-= (const Matrix& M);
            Matrix& operator*= (const Matrix& M);
            Matrix& operator*= (float S);
            Matrix& operator/= (float S);

            Matrix& operator/= (const Matrix& M);
                // Element-wise divide

            // Unary operators
            Matrix operator+ () const { return *this; }
            Matrix operator- () const;

            // Properties
            Vector3 Up() const { return Vector3(_21, _22, _23); }
            void Up(const Vector3& v) { _21 = v.x; _22 = v.y; _23 = v.z; }

            Vector3 Down() const { return Vector3(-_21, -_22, -_23); }
            void Down(const Vector3& v) { _21 = -v.x; _22 = -v.y; _23 = -v.z; }

            Vector3 Right() const { return Vector3(_11, _12, _13); }
            void Right(const Vector3& v) { _11 = v.x; _12 = v.y; _13 = v.z; }

            Vector3 Left() const { return Vector3(-_11, -_12, -_13); }
            void Left(const Vector3& v) { _11 = -v.x; _12 = -v.y; _13 = -v.z; }

            Vector3 Forward() const { return Vector3(-_31, -_32, -_33); }
            void Forward(const Vector3& v) { _31 = -v.x; _32 = -v.y; _33 = -v.z; }

            Vector3 Backward() const { return Vector3(_31, _32, _33); }
            void Backward(const Vector3& v) { _31 = v.x; _32 = v.y; _33 = v.z; }

            Vector3 Translation() const { return Vector3(_41, _42, _43); }
            void Translation(const Vector3& v) { _41 = v.x; _42 = v.y; _43 = v.z; }

            // Matrix operations
            bool Decompose(Vector3& scale, Quaternion& rotation, Vector3& translation);

            Matrix Transpose() const;
            void Transpose(Matrix& result) const;

            Matrix Invert() const;
            void Invert(Matrix& result) const;

            float Determinant() const;

            // Static functions
            static Matrix CreateBillboard(const Vector3& object, const Vector3& cameraPosition, const Vector3& cameraUp, _In_opt_ const Vector3* cameraForward = nullptr);

            static Matrix CreateConstrainedBillboard(const Vector3& object, const Vector3& cameraPosition, const Vector3& rotateAxis,
                                                     _In_opt_ const Vector3* cameraForward = nullptr, _In_opt_ const Vector3* objectForward = nullptr);

            static Matrix CreateTranslation(const Vector3& position);
            static Matrix CreateTranslation(float x, float y, float z);

            static Matrix CreateScale(const Vector3& scales);
            static Matrix CreateScale(float xs, float ys, float zs);
            static Matrix CreateScale(float scale);

            static Matrix CreateRotationX(float radians);
            static Matrix CreateRotationY(float radians);
            static Matrix CreateRotationZ(float radians);

            static Matrix CreateFromAxisAngle(const Vector3& axis, float angle);

            static Matrix CreatePerspectiveFieldOfView(float fov, float aspectRatio, float nearPlane, float farPlane);
            static Matrix CreatePerspective(float width, float height, float nearPlane, float farPlane);
            static Matrix CreatePerspectiveOffCenter(float left, float right, float bottom, float top, float nearPlane, float farPlane);
            static Matrix CreateOrthographic(float width, float height, float zNearPlane, float zFarPlane);
            static Matrix CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane);

            static Matrix CreateLookAt(const Vector3& position, const Vector3& target, const Vector3& up);
            static Matrix CreateWorld(const Vector3& position, const Vector3& forward, const Vector3& up);

            static Matrix CreateFromQuaternion(const Quaternion& quat);

            static Matrix CreateFromYawPitchRoll(float yaw, float pitch, float roll);

            static Matrix CreateShadow(const Vector3& lightDir, const Plane& plane);

            static Matrix CreateReflection(const Plane& plane);

            static void Lerp(const Matrix& M1, const Matrix& M2, float t, Matrix& result);
            static Matrix Lerp(const Matrix& M1, const Matrix& M2, float t);

            static void Transform(const Matrix& M, const Quaternion& rotation, Matrix& result);
            static Matrix Transform(const Matrix& M, const Quaternion& rotation);

            // Constants
            static const Matrix Identity;
        };

        // Binary operators
        Matrix operator+ (const Matrix& M1, const Matrix& M2);
        Matrix operator- (const Matrix& M1, const Matrix& M2);
        Matrix operator* (const Matrix& M1, const Matrix& M2);
        Matrix operator* (const Matrix& M, float S);
        Matrix operator/ (const Matrix& M, float S);
        Matrix operator/ (const Matrix& M1, const Matrix& M2);
            // Element-wise divide
        Matrix operator* (float S, const Matrix& M);


        //-----------------------------------------------------------------------------
        // Plane
        struct Plane : public XMFLOAT4
        {
            Plane() throw() : XMFLOAT4(0.f, 1.f, 0.f, 0.f) {}
            XM_CONSTEXPR Plane(float _x, float _y, float _z, float _w) : XMFLOAT4(_x, _y, _z, _w) {}
            Plane(const Vector3& normal, float d) : XMFLOAT4(normal.x, normal.y, normal.z, d) {}
            Plane(const Vector3& point1, const Vector3& point2, const Vector3& point3);
            Plane(const Vector3& point, const Vector3& normal);
            explicit Plane(const Vector4& v) : XMFLOAT4(v.x, v.y, v.z, v.w) {}
            explicit Plane(_In_reads_(4) const float *pArray) : XMFLOAT4(pArray) {}
            Plane(FXMVECTOR V) { XMStoreFloat4(this, V); }
            Plane(const XMFLOAT4& p) { this->x = p.x; this->y = p.y; this->z = p.z; this->w = p.w; }
            explicit Plane(const XMVECTORF32& F) { this->x = F.f[0]; this->y = F.f[1]; this->z = F.f[2]; this->w = F.f[3]; }

            Plane(const Plane&) = default;
            Plane& operator=(const Plane&) = default;

        #if !defined(_MSC_VER) || _MSC_VER >= 1900
            Plane(Plane&&) = default;
            Plane& operator=(Plane&&) = default;
        #endif

            operator XMVECTOR() const { return XMLoadFloat4(this); }

            // Comparison operators
            bool operator == (const Plane& p) const;
            bool operator != (const Plane& p) const;

            // Assignment operators
            Plane& operator= (const XMVECTORF32& F) { x = F.f[0]; y = F.f[1]; z = F.f[2]; w = F.f[3]; return *this; }

            // Properties
            Vector3 Normal() const { return Vector3(x, y, z); }
            void Normal(const Vector3& normal) { x = normal.x; y = normal.y; z = normal.z; }

            float D() const { return w; }
            void D(float d) { w = d; }

            // Plane operations
            void Normalize();
            void Normalize(Plane& result) const;

            float Dot(const Vector4& v) const;
            float DotCoordinate(const Vector3& position) const;
            float DotNormal(const Vector3& normal) const;

            // Static functions
            static void Transform(const Plane& plane, const Matrix& M, Plane& result);
            static Plane Transform(const Plane& plane, const Matrix& M);

            static void Transform(const Plane& plane, const Quaternion& rotation, Plane& result);
            static Plane Transform(const Plane& plane, const Quaternion& rotation);
                // Input quaternion must be the inverse transpose of the transformation
        };

        //------------------------------------------------------------------------------
        // Quaternion
        struct Quaternion : public XMFLOAT4
        {
            Quaternion() throw() : XMFLOAT4(0, 0, 0, 1.f) {}
            XM_CONSTEXPR Quaternion(float _x, float _y, float _z, float _w) : XMFLOAT4(_x, _y, _z, _w) {}
            Quaternion(const Vector3& v, float scalar) : XMFLOAT4(v.x, v.y, v.z, scalar) {}
            explicit Quaternion(const Vector4& v) : XMFLOAT4(v.x, v.y, v.z, v.w) {}
            explicit Quaternion(_In_reads_(4) const float *pArray) : XMFLOAT4(pArray) {}
            Quaternion(FXMVECTOR V) { XMStoreFloat4(this, V); }
            Quaternion(const XMFLOAT4& q) { this->x = q.x; this->y = q.y; this->z = q.z; this->w = q.w; }
            explicit Quaternion(const XMVECTORF32& F) { this->x = F.f[0]; this->y = F.f[1]; this->z = F.f[2]; this->w = F.f[3]; }

            Quaternion(const Quaternion&) = default;
            Quaternion& operator=(const Quaternion&) = default;

        #if !defined(_MSC_VER) || _MSC_VER >= 1900
            Quaternion(Quaternion&&) = default;
            Quaternion& operator=(Quaternion&&) = default;
        #endif

            operator XMVECTOR() const { return XMLoadFloat4(this); }

            // Comparison operators
            bool operator == (const Quaternion& q) const;
            bool operator != (const Quaternion& q) const;

            // Assignment operators
            Quaternion& operator= (const XMVECTORF32& F) { x = F.f[0]; y = F.f[1]; z = F.f[2]; w = F.f[3]; return *this; }
            Quaternion& operator+= (const Quaternion& q);
            Quaternion& operator-= (const Quaternion& q);
            Quaternion& operator*= (const Quaternion& q);
            Quaternion& operator*= (float S);
            Quaternion& operator/= (const Quaternion& q);

            // Unary operators
            Quaternion operator+ () const { return *this; }
            Quaternion operator- () const;

            // Quaternion operations
            float Length() const;
            float LengthSquared() const;

            void Normalize();
            void Normalize(Quaternion& result) const;

            void Conjugate();
            void Conjugate(Quaternion& result) const;

            void Inverse(Quaternion& result) const;

            float Dot(const Quaternion& Q) const;

            // Static functions
            static Quaternion CreateFromAxisAngle(const Vector3& axis, float angle);
            static Quaternion CreateFromYawPitchRoll(float yaw, float pitch, float roll);
            static Quaternion CreateFromRotationMatrix(const Matrix& M);

            static void Lerp(const Quaternion& q1, const Quaternion& q2, float t, Quaternion& result);
            static Quaternion Lerp(const Quaternion& q1, const Quaternion& q2, float t);

            static void Slerp(const Quaternion& q1, const Quaternion& q2, float t, Quaternion& result);
            static Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float t);

            static void Concatenate(const Quaternion& q1, const Quaternion& q2, Quaternion& result);
            static Quaternion Concatenate(const Quaternion& q1, const Quaternion& q2);

            // Constants
            static const Quaternion Identity;
        };

        // Binary operators
        Quaternion operator+ (const Quaternion& Q1, const Quaternion& Q2);
        Quaternion operator- (const Quaternion& Q1, const Quaternion& Q2);
        Quaternion operator* (const Quaternion& Q1, const Quaternion& Q2);
        Quaternion operator* (const Quaternion& Q, float S);
        Quaternion operator/ (const Quaternion& Q1, const Quaternion& Q2);
        Quaternion operator* (float S, const Quaternion& Q);

        //------------------------------------------------------------------------------
        // Color
        struct Color : public XMFLOAT4
        {
            Color() throw() : XMFLOAT4(0, 0, 0, 1.f) {}
            XM_CONSTEXPR Color(float _r, float _g, float _b) : XMFLOAT4(_r, _g, _b, 1.f) {}
            XM_CONSTEXPR Color(float _r, float _g, float _b, float _a) : XMFLOAT4(_r, _g, _b, _a) {}
            explicit Color(const Vector3& clr) : XMFLOAT4(clr.x, clr.y, clr.z, 1.f) {}
            explicit Color(const Vector4& clr) : XMFLOAT4(clr.x, clr.y, clr.z, clr.w) {}
            explicit Color(_In_reads_(4) const float *pArray) : XMFLOAT4(pArray) {}
            Color(FXMVECTOR V) { XMStoreFloat4(this, V); }
            Color(const XMFLOAT4& c) { this->x = c.x; this->y = c.y; this->z = c.z; this->w = c.w; }
            explicit Color(const XMVECTORF32& F) { this->x = F.f[0]; this->y = F.f[1]; this->z = F.f[2]; this->w = F.f[3]; }

            explicit Color(const DirectX::PackedVector::XMCOLOR& Packed);
                // BGRA Direct3D 9 D3DCOLOR packed color

            explicit Color(const DirectX::PackedVector::XMUBYTEN4& Packed);
                // RGBA XNA Game Studio packed color

            Color(const Color&) = default;
            Color& operator=(const Color&) = default;

        #if !defined(_MSC_VER) || _MSC_VER >= 1900
            Color(Color&&) = default;
            Color& operator=(Color&&) = default;
        #endif

            operator XMVECTOR() const { return XMLoadFloat4(this); }
            operator const float*() const { return reinterpret_cast<const float*>(this); }

            // Comparison operators
            bool operator == (const Color& c) const;
            bool operator != (const Color& c) const;

            // Assignment operators
            Color& operator= (const XMVECTORF32& F) { x = F.f[0]; y = F.f[1]; z = F.f[2]; w = F.f[3]; return *this; }
            Color& operator= (const DirectX::PackedVector::XMCOLOR& Packed);
            Color& operator= (const DirectX::PackedVector::XMUBYTEN4& Packed);
            Color& operator+= (const Color& c);
            Color& operator-= (const Color& c);
            Color& operator*= (const Color& c);
            Color& operator*= (float S);
            Color& operator/= (const Color& c);

            // Unary operators
            Color operator+ () const { return *this; }
            Color operator- () const;

            // Properties
            float R() const { return x; }
            void R(float r) { x = r; }

            float G() const { return y; }
            void G(float g) { y = g; }

            float B() const { return z; }
            void B(float b) { z = b; }

            float A() const { return w; }
            void A(float a) { w = a; }

            // Color operations
            DirectX::PackedVector::XMCOLOR BGRA() const;
            DirectX::PackedVector::XMUBYTEN4 RGBA() const;

            Vector3 ToVector3() const;
            Vector4 ToVector4() const;

            void Negate();
            void Negate(Color& result) const;

            void Saturate();
            void Saturate(Color& result) const;

            void Premultiply();
            void Premultiply(Color& result) const;

            void AdjustSaturation(float sat);
            void AdjustSaturation(float sat, Color& result) const;

            void AdjustContrast(float contrast);
            void AdjustContrast(float contrast, Color& result) const;

            // Static functions
            static void Modulate(const Color& c1, const Color& c2, Color& result);
            static Color Modulate(const Color& c1, const Color& c2);

            static void Lerp(const Color& c1, const Color& c2, float t, Color& result);
            static Color Lerp(const Color& c1, const Color& c2, float t);
        };

        // Binary operators
        Color operator+ (const Color& C1, const Color& C2);
        Color operator- (const Color& C1, const Color& C2);
        Color operator* (const Color& C1, const Color& C2);
        Color operator* (const Color& C, float S);
        Color operator/ (const Color& C1, const Color& C2);
        Color operator* (float S, const Color& C);

        //------------------------------------------------------------------------------
        // Ray
        class Ray
        {
        public:
            Vector3 position;
            Vector3 direction;

            Ray() throw() : position(0, 0, 0), direction(0, 0, 1) {}
            Ray(const Vector3& pos, const Vector3& dir) : position(pos), direction(dir) {}

            Ray(const Ray&) = default;
            Ray& operator=(const Ray&) = default;

        #if !defined(_MSC_VER) || _MSC_VER >= 1900
            Ray(Ray&&) = default;
            Ray& operator=(Ray&&) = default;
        #endif

            // Comparison operators
            bool operator == (const Ray& r) const;
            bool operator != (const Ray& r) const;

            // Ray operations
            bool Intersects(const BoundingSphere& sphere, _Out_ float& Dist) const;
            bool Intersects(const BoundingBox& box, _Out_ float& Dist) const;
            bool Intersects(const Vector3& tri0, const Vector3& tri1, const Vector3& tri2, _Out_ float& Dist) const;
            bool Intersects(const Plane& plane, _Out_ float& Dist) const;
        };

        //------------------------------------------------------------------------------
        // Viewport
        class Viewport
        {
        public:
            float x;
            float y;
            float width;
            float height;
            float minDepth;
            float maxDepth;

            Viewport() throw() :
                x(0.f), y(0.f), width(0.f), height(0.f), minDepth(0.f), maxDepth(1.f) {}
            XM_CONSTEXPR Viewport(float ix, float iy, float iw, float ih, float iminz = 0.f, float imaxz = 1.f) :
                x(ix), y(iy), width(iw), height(ih), minDepth(iminz), maxDepth(imaxz) {}
            explicit Viewport(const RECT& rct) :
                x(float(rct.left)), y(float(rct.top)),
                width(float(rct.right - rct.left)),
                height(float(rct.bottom - rct.top)),
                minDepth(0.f), maxDepth(1.f) {}

        #if defined(__d3d11_h__) || defined(__d3d11_x_h__)
            // Direct3D 11 interop
            explicit Viewport(const D3D11_VIEWPORT& vp) :
                x(vp.TopLeftX), y(vp.TopLeftY),
                width(vp.Width), height(vp.Height),
                minDepth(vp.MinDepth), maxDepth(vp.MaxDepth) {}

            operator D3D11_VIEWPORT() { return *reinterpret_cast<const D3D11_VIEWPORT*>(this); }
            const D3D11_VIEWPORT* Get11() const { return reinterpret_cast<const D3D11_VIEWPORT*>(this); }
            Viewport& operator= (const D3D11_VIEWPORT& vp);
        #endif

        #if defined(__d3d12_h__) || defined(__d3d12_x_h__)
            // Direct3D 12 interop
            explicit Viewport(const D3D12_VIEWPORT& vp) :
                x(vp.TopLeftX), y(vp.TopLeftY),
                width(vp.Width), height(vp.Height),
                minDepth(vp.MinDepth), maxDepth(vp.MaxDepth) {}

            operator D3D12_VIEWPORT() { return *reinterpret_cast<const D3D12_VIEWPORT*>(this); }
            const D3D12_VIEWPORT* Get12() const { return reinterpret_cast<const D3D12_VIEWPORT*>(this); }
            Viewport& operator= (const D3D12_VIEWPORT& vp);
        #endif

            Viewport(const Viewport&) = default;
            Viewport& operator=(const Viewport&) = default;

        #if !defined(_MSC_VER) || _MSC_VER >= 1900
            Viewport(Viewport&&) = default;
            Viewport& operator=(Viewport&&) = default;
        #endif

            // Comparison operators
            bool operator == (const Viewport& vp) const;
            bool operator != (const Viewport& vp) const;

            // Assignment operators
            Viewport& operator= (const RECT& rct);

            // Viewport operations
            float AspectRatio() const;

            Vector3 Project(const Vector3& p, const Matrix& proj, const Matrix& view, const Matrix& world) const;
            void Project(const Vector3& p, const Matrix& proj, const Matrix& view, const Matrix& world, Vector3& result) const;

            Vector3 Unproject(const Vector3& p, const Matrix& proj, const Matrix& view, const Matrix& world) const;
            void Unproject(const Vector3& p, const Matrix& proj, const Matrix& view, const Matrix& world, Vector3& result) const;

            // Static methods
            static RECT __cdecl ComputeDisplayArea(DXGI_SCALING scaling, UINT backBufferWidth, UINT backBufferHeight, int outputWidth, int outputHeight);
            static RECT __cdecl ComputeTitleSafeArea(UINT backBufferWidth, UINT backBufferHeight);
        };

    #include "SimpleMath.inl"

    } // namespace SimpleMath

} // namespace DirectX

//------------------------------------------------------------------------------
// Support for SimpleMath and Standard C++ Library containers
namespace std
{

    template<> struct less<DirectX::SimpleMath::Rectangle>
    {
        bool operator()(const DirectX::SimpleMath::Rectangle& r1, const DirectX::SimpleMath::Rectangle& r2) const
        {
            return ((r1.x < r2.x)
                    || ((r1.x == r2.x) && (r1.y < r2.y))
                    || ((r1.x == r2.x) && (r1.y == r2.y) && (r1.width < r2.width))
                    || ((r1.x == r2.x) && (r1.y == r2.y) && (r1.width == r2.width) && (r1.height < r2.height)));
        }
    };

    template<> struct less<DirectX::SimpleMath::Vector2>
    {
        bool operator()(const DirectX::SimpleMath::Vector2& V1, const DirectX::SimpleMath::Vector2& V2) const
        {
            return ((V1.x < V2.x) || ((V1.x == V2.x) && (V1.y < V2.y)));
        }
    };

    template<> struct less<DirectX::SimpleMath::Vector3>
    {
        bool operator()(const DirectX::SimpleMath::Vector3& V1, const DirectX::SimpleMath::Vector3& V2) const
        {
            return ((V1.x < V2.x)
                    || ((V1.x == V2.x) && (V1.y < V2.y))
                    || ((V1.x == V2.x) && (V1.y == V2.y) && (V1.z < V2.z)));
        }
    };

    template<> struct less<DirectX::SimpleMath::Vector4>
    {
        bool operator()(const DirectX::SimpleMath::Vector4& V1, const DirectX::SimpleMath::Vector4& V2) const
        {
            return ((V1.x < V2.x)
                    || ((V1.x == V2.x) && (V1.y < V2.y))
                    || ((V1.x == V2.x) && (V1.y == V2.y) && (V1.z < V2.z))
                    || ((V1.x == V2.x) && (V1.y == V2.y) && (V1.z == V2.z) && (V1.w < V2.w)));
        }
    };

    template<> struct less<DirectX::SimpleMath::Matrix>
    {
        bool operator()(const DirectX::SimpleMath::Matrix& M1, const DirectX::SimpleMath::Matrix& M2) const
        {
            if (M1._11 != M2._11) return M1._11 < M2._11;
            if (M1._12 != M2._12) return M1._12 < M2._12;
            if (M1._13 != M2._13) return M1._13 < M2._13;
            if (M1._14 != M2._14) return M1._14 < M2._14;
            if (M1._21 != M2._21) return M1._21 < M2._21;
            if (M1._22 != M2._22) return M1._22 < M2._22;
            if (M1._23 != M2._23) return M1._23 < M2._23;
            if (M1._24 != M2._24) return M1._24 < M2._24;
            if (M1._31 != M2._31) return M1._31 < M2._31;
            if (M1._32 != M2._32) return M1._32 < M2._32;
            if (M1._33 != M2._33) return M1._33 < M2._33;
            if (M1._34 != M2._34) return M1._34 < M2._34;
            if (M1._41 != M2._41) return M1._41 < M2._41;
            if (M1._42 != M2._42) return M1._42 < M2._42;
            if (M1._43 != M2._43) return M1._43 < M2._43;
            if (M1._44 != M2._44) return M1._44 < M2._44;

            return false;
        }
    };

    template<> struct less<DirectX::SimpleMath::Plane>
    {
        bool operator()(const DirectX::SimpleMath::Plane& P1, const DirectX::SimpleMath::Plane& P2) const
        {
            return ((P1.x < P2.x)
                    || ((P1.x == P2.x) && (P1.y < P2.y))
                    || ((P1.x == P2.x) && (P1.y == P2.y) && (P1.z < P2.z))
                    || ((P1.x == P2.x) && (P1.y == P2.y) && (P1.z == P2.z) && (P1.w < P2.w)));
        }
    };

    template<> struct less<DirectX::SimpleMath::Quaternion>
    {
        bool operator()(const DirectX::SimpleMath::Quaternion& Q1, const DirectX::SimpleMath::Quaternion& Q2) const
        {
            return ((Q1.x < Q2.x)
                    || ((Q1.x == Q2.x) && (Q1.y < Q2.y))
                    || ((Q1.x == Q2.x) && (Q1.y == Q2.y) && (Q1.z < Q2.z))
                    || ((Q1.x == Q2.x) && (Q1.y == Q2.y) && (Q1.z == Q2.z) && (Q1.w < Q2.w)));
        }
    };

    template<> struct less<DirectX::SimpleMath::Color>
    {
        bool operator()(const DirectX::SimpleMath::Color& C1, const DirectX::SimpleMath::Color& C2) const
        {
            return ((C1.x < C2.x)
                    || ((C1.x == C2.x) && (C1.y < C2.y))
                    || ((C1.x == C2.x) && (C1.y == C2.y) && (C1.z < C2.z))
                    || ((C1.x == C2.x) && (C1.y == C2.y) && (C1.z == C2.z) && (C1.w < C2.w)));
        }
    };

    template<> struct less<DirectX::SimpleMath::Ray>
    {
        bool operator()(const DirectX::SimpleMath::Ray& R1, const DirectX::SimpleMath::Ray& R2) const
        {
            if (R1.position.x != R2.position.x) return R1.position.x < R2.position.x;
            if (R1.position.y != R2.position.y) return R1.position.y < R2.position.y;
            if (R1.position.z != R2.position.z) return R1.position.z < R2.position.z;

            if (R1.direction.x != R2.direction.x) return R1.direction.x < R2.direction.x;
            if (R1.direction.y != R2.direction.y) return R1.direction.y < R2.direction.y;
            if (R1.direction.z != R2.direction.z) return R1.direction.z < R2.direction.z;

            return false;
        }
    };

    template<> struct less<DirectX::SimpleMath::Viewport>
    {
        bool operator()(const DirectX::SimpleMath::Viewport& vp1, const DirectX::SimpleMath::Viewport& vp2) const
        {
            if (vp1.x != vp2.x) return (vp1.x < vp2.x);
            if (vp1.y != vp2.y) return (vp1.y < vp2.y);

            if (vp1.width != vp2.width) return (vp1.width < vp2.width);
            if (vp1.height != vp2.height) return (vp1.height < vp2.height);

            if (vp1.minDepth != vp2.minDepth) return (vp1.minDepth < vp2.minDepth);
            if (vp1.maxDepth != vp2.maxDepth) return (vp1.maxDepth < vp2.maxDepth);

            return false;
        }
    };

} // namespace std
