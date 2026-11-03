//-------------------------------------------------------------------------------------
// SimpleMath.h -- Simplified C++ Math wrapper for DirectXMath
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkId=248929
// https://go.microsoft.com/fwlink/?LinkID=615561
//-------------------------------------------------------------------------------------

#pragma once

#if (defined(_WIN32) || defined(WINAPI_FAMILY)) && !(defined(_XBOX_ONE) && defined(_TITLE)) && !defined(_GAMING_XBOX)
#include <dxgi1_2.h>
#endif

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>

#if (__cplusplus >= 202002L)
#include <compare>
#endif

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>

#ifndef DIRECTX_TOOLKIT_API
#ifdef DIRECTX_TOOLKIT_EXPORT
#ifdef __GNUC__
#define DIRECTX_TOOLKIT_API __attribute__((dllexport))
#else
#define DIRECTX_TOOLKIT_API __declspec(dllexport)
#endif
#elif defined(DIRECTX_TOOLKIT_IMPORT)
#ifdef __GNUC__
#define DIRECTX_TOOLKIT_API __attribute__((dllimport))
#else
#define DIRECTX_TOOLKIT_API __declspec(dllimport)
#endif
#else
#define DIRECTX_TOOLKIT_API
#endif
#endif

#if defined(DIRECTX_TOOLKIT_IMPORT) && defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251 4275)
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
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
        struct DIRECTX_TOOLKIT_API Rectangle
        {
            long x;
            long y;
            long width;
            long height;

            // Creators
            Rectangle() noexcept
                : x(0),
                  y(0),
                  width(0),
                  height(0)
            {}
            constexpr Rectangle(long ix, long iy, long iw, long ih) noexcept
                : x(ix),
                  y(iy),
                  width(iw),
                  height(ih)
            {}
            explicit Rectangle(const RECT& rct) noexcept
                : x(rct.left),
                  y(rct.top),
                  width(rct.right - rct.left),
                  height(rct.bottom - rct.top)
            {}

            Rectangle(Rectangle const&)            = default;
            Rectangle& operator=(Rectangle const&) = default;

            Rectangle(Rectangle&&)            = default;
            Rectangle& operator=(Rectangle&&) = default;

            operator RECT() noexcept
            {
                RECT rct;
                rct.left   = x;
                rct.top    = y;
                rct.right  = (x + width);
                rct.bottom = (y + height);
                return rct;
            }
#ifdef __cplusplus_winrt
            operator Windows::Foundation::Rect() noexcept
            {
                return Windows::Foundation::Rect(float(x), float(y), float(width), float(height));
            }
#endif

            // Comparison operators
#if (__cplusplus >= 202002L)
            bool operator==(Rectangle const&) const    = default;
            auto operator<= > (Rectangle const&) const = default;
#else
            bool operator==(Rectangle const& r) const noexcept
            {
                return (x == r.x) && (y == r.y) && (width == r.width) && (height == r.height);
            }
            bool operator!=(Rectangle const& r) const noexcept
            {
                return (x != r.x) || (y != r.y) || (width != r.width) || (height != r.height);
            }
#endif
            bool operator==(const RECT& rct) const noexcept
            {
                return (x == rct.left) && (y == rct.top) && (width == (rct.right - rct.left)) && (height == (rct.bottom - rct.top));
            }
            bool operator!=(const RECT& rct) const noexcept
            {
                return (x != rct.left) || (y != rct.top) || (width != (rct.right - rct.left)) || (height != (rct.bottom - rct.top));
            }

            // Assignment operators
            Rectangle& operator=(_In_ const RECT& rct) noexcept
            {
                x      = rct.left;
                y      = rct.top;
                width  = (rct.right - rct.left);
                height = (rct.bottom - rct.top);
                return *this;
            }

            // Rectangle operations
            Vector2 Location() const noexcept;
            Vector2 Center() const noexcept;

            bool IsEmpty() const noexcept { return (width == 0 && height == 0 && x == 0 && y == 0); }

            bool Contains(long ix, long iy) const noexcept { return (x <= ix) && (ix < (x + width)) && (y <= iy) && (iy < (y + height)); }
            bool Contains(Vector2 const& point) const noexcept;
            bool Contains(Rectangle const& r) const noexcept
            {
                return (x <= r.x) && ((r.x + r.width) <= (x + width)) && (y <= r.y) && ((r.y + r.height) <= (y + height));
            }
            bool Contains(const RECT& rct) const noexcept
            {
                return (x <= rct.left) && (rct.right <= (x + width)) && (y <= rct.top) && (rct.bottom <= (y + height));
            }

            void Inflate(long horizAmount, long vertAmount) noexcept;

            bool Intersects(Rectangle const& r) const noexcept
            {
                return (r.x < (x + width)) && (x < (r.x + r.width)) && (r.y < (y + height)) && (y < (r.y + r.height));
            }
            bool Intersects(const RECT& rct) const noexcept
            {
                return (rct.left < (x + width)) && (x < rct.right) && (rct.top < (y + height)) && (y < rct.bottom);
            }

            void Offset(long ox, long oy) noexcept
            {
                x += ox;
                y += oy;
            }

            // Static functions
            static Rectangle Intersect(Rectangle const& ra, Rectangle const& rb) noexcept;
            static RECT      Intersect(const RECT& rcta, const RECT& rctb) noexcept;

            static Rectangle Union(Rectangle const& ra, Rectangle const& rb) noexcept;
            static RECT      Union(const RECT& rcta, const RECT& rctb) noexcept;
        };

        //------------------------------------------------------------------------------
        // 2D vector
        struct DIRECTX_TOOLKIT_API Vector2 : public XMFLOAT2
        {
            Vector2() noexcept
                : XMFLOAT2(0.f, 0.f)
            {}
            constexpr explicit Vector2(float ix) noexcept
                : XMFLOAT2(ix, ix)
            {}
            constexpr Vector2(float ix, float iy) noexcept
                : XMFLOAT2(ix, iy)
            {}
            explicit Vector2(_In_reads_(2) const float* pArray) noexcept
                : XMFLOAT2(pArray)
            {}
            Vector2(FXMVECTOR V) noexcept { XMStoreFloat2(this, V); }
            Vector2(const XMFLOAT2& V) noexcept
            {
                this->x = V.x;
                this->y = V.y;
            }
            explicit Vector2(const XMVECTORF32& F) noexcept
            {
                this->x = F.f[0];
                this->y = F.f[1];
            }

            Vector2(Vector2 const&)            = default;
            Vector2& operator=(Vector2 const&) = default;

            Vector2(Vector2&&)            = default;
            Vector2& operator=(Vector2&&) = default;

            operator XMVECTOR() const noexcept { return XMLoadFloat2(this); }

            // Comparison operators
            bool operator==(Vector2 const& V) const noexcept { return ((x == V.x) && (y == V.y)); }
            bool operator!=(Vector2 const& V) const noexcept { return ((x != V.x) || (y != V.y)); }

            // Assignment operators
            Vector2& operator=(const XMVECTORF32& F) noexcept
            {
                x = F.f[0];
                y = F.f[1];
                return *this;
            }
            Vector2& operator+=(Vector2 const& V) noexcept
            {
                x += V.x;
                y += V.y;
                return *this;
            }
            Vector2& operator-=(Vector2 const& V) noexcept
            {
                x -= V.x;
                y -= V.y;
                return *this;
            }
            Vector2& operator*=(Vector2 const& V) noexcept
            {
                x *= V.x;
                y *= V.y;
                return *this;
            }
            Vector2& operator*=(float S) noexcept
            {
                x *= S;
                y *= S;
                return *this;
            }
            Vector2& operator/=(float S) noexcept
            {
                x /= S;
                y /= S;
                return *this;
            }

            // Unary operators
            Vector2 operator+() const noexcept { return *this; }
            Vector2 operator-() const noexcept { return Vector2(-x, -y); }

            // Vector operations
            bool InBounds(Vector2 const& Bounds) const noexcept;

            float Length() const noexcept { return std::sqrt((x * x) + (y * y)); }
            float LengthSquared() const noexcept { return (x * x) + (y * y); }

            float   Dot(Vector2 const& V) const noexcept { return (x * V.x) + (y * V.y); }
            void    Cross(Vector2 const& V, Vector2& result) const noexcept { result.x = result.y = (x * V.y) - (y * V.x); }
            Vector2 Cross(Vector2 const& V) const noexcept
            {
                float c = (x * V.y) - (y * V.x);
                return Vector2(c, c);
            }

            void Normalize() noexcept;
            void Normalize(Vector2& result) const noexcept;

            void Clamp(Vector2 const& vmin, Vector2 const& vmax) noexcept;
            void Clamp(Vector2 const& vmin, Vector2 const& vmax, Vector2& result) const noexcept;

            // Static functions
            static float Distance(Vector2 const& v1, Vector2 const& v2) noexcept;
            static float DistanceSquared(Vector2 const& v1, Vector2 const& v2) noexcept;

            static void    Min(Vector2 const& v1, Vector2 const& v2, Vector2& result) noexcept;
            static Vector2 Min(Vector2 const& v1, Vector2 const& v2) noexcept;

            static void    Max(Vector2 const& v1, Vector2 const& v2, Vector2& result) noexcept;
            static Vector2 Max(Vector2 const& v1, Vector2 const& v2) noexcept;

            static void    Lerp(Vector2 const& v1, Vector2 const& v2, float t, Vector2& result) noexcept;
            static Vector2 Lerp(Vector2 const& v1, Vector2 const& v2, float t) noexcept;

            static void    SmoothStep(Vector2 const& v1, Vector2 const& v2, float t, Vector2& result) noexcept;
            static Vector2 SmoothStep(Vector2 const& v1, Vector2 const& v2, float t) noexcept;

            static void    Barycentric(Vector2 const& v1, Vector2 const& v2, Vector2 const& v3, float f, float g, Vector2& result) noexcept;
            static Vector2 Barycentric(Vector2 const& v1, Vector2 const& v2, Vector2 const& v3, float f, float g) noexcept;

            static void
            CatmullRom(Vector2 const& v1, Vector2 const& v2, Vector2 const& v3, Vector2 const& v4, float t, Vector2& result) noexcept;
            static Vector2 CatmullRom(Vector2 const& v1, Vector2 const& v2, Vector2 const& v3, Vector2 const& v4, float t) noexcept;

            static void
            Hermite(Vector2 const& v1, Vector2 const& t1, Vector2 const& v2, Vector2 const& t2, float t, Vector2& result) noexcept;
            static Vector2 Hermite(Vector2 const& v1, Vector2 const& t1, Vector2 const& v2, Vector2 const& t2, float t) noexcept;

            static void    Reflect(Vector2 const& ivec, Vector2 const& nvec, Vector2& result) noexcept;
            static Vector2 Reflect(Vector2 const& ivec, Vector2 const& nvec) noexcept;

            static void    Refract(Vector2 const& ivec, Vector2 const& nvec, float refractionIndex, Vector2& result) noexcept;
            static Vector2 Refract(Vector2 const& ivec, Vector2 const& nvec, float refractionIndex) noexcept;

            static void    Transform(Vector2 const& v, Quaternion const& quat, Vector2& result) noexcept;
            static Vector2 Transform(Vector2 const& v, Quaternion const& quat) noexcept;

            static void    Transform(Vector2 const& v, Matrix const& m, Vector2& result) noexcept;
            static Vector2 Transform(Vector2 const& v, Matrix const& m) noexcept;
            static void    Transform(_In_reads_(count) const Vector2* varray,
                size_t                                             count,
                Matrix const&                                      m,
                _Out_writes_(count) Vector2*                       resultArray) noexcept;

            static void Transform(Vector2 const& v, Matrix const& m, Vector4& result) noexcept;
            static void Transform(_In_reads_(count) const Vector2* varray,
                size_t                                             count,
                Matrix const&                                      m,
                _Out_writes_(count) Vector4*                       resultArray) noexcept;

            static void    TransformNormal(Vector2 const& v, Matrix const& m, Vector2& result) noexcept;
            static Vector2 TransformNormal(Vector2 const& v, Matrix const& m) noexcept;
            static void    TransformNormal(_In_reads_(count) const Vector2* varray,
                size_t                                                   count,
                Matrix const&                                            m,
                _Out_writes_(count) Vector2*                             resultArray) noexcept;

            // Constants
            static Vector2 const Zero;
            static Vector2 const One;
            static Vector2 const UnitX;
            static Vector2 const UnitY;
        };

        // Binary operators
        DIRECTX_TOOLKIT_API inline Vector2 operator+(Vector2 const& V1, Vector2 const& V2) noexcept
        {
            return Vector2(V1.x + V2.x, V1.y + V2.y);
        }
        DIRECTX_TOOLKIT_API inline Vector2 operator-(Vector2 const& V1, Vector2 const& V2) noexcept
        {
            return Vector2(V1.x - V2.x, V1.y - V2.y);
        }
        DIRECTX_TOOLKIT_API inline Vector2 operator*(Vector2 const& V1, Vector2 const& V2) noexcept
        {
            return Vector2(V1.x * V2.x, V1.y * V2.y);
        }
        DIRECTX_TOOLKIT_API inline Vector2 operator*(Vector2 const& V, float S) noexcept
        {
            return Vector2(V.x * S, V.y * S);
        }
        DIRECTX_TOOLKIT_API inline Vector2 operator*(float S, Vector2 const& V) noexcept
        {
            return Vector2(S * V.x, S * V.y);
        }
        DIRECTX_TOOLKIT_API inline Vector2 operator/(Vector2 const& V1, Vector2 const& V2) noexcept
        {
            return Vector2(V1.x / V2.x, V1.y / V2.y);
        }
        DIRECTX_TOOLKIT_API inline Vector2 operator/(Vector2 const& V, float S) noexcept
        {
            return Vector2(V.x / S, V.y / S);
        }
        DIRECTX_TOOLKIT_API inline Vector2 operator/(float S, Vector2 const& V) noexcept
        {
            return Vector2(S / V.x, S / V.y);
        };

        //------------------------------------------------------------------------------
        // 3D vector
        struct DIRECTX_TOOLKIT_API Vector3 : public XMFLOAT3
        {
            Vector3() noexcept
                : XMFLOAT3(0.f, 0.f, 0.f)
            {}
            constexpr explicit Vector3(float ix) noexcept
                : XMFLOAT3(ix, ix, ix)
            {}
            constexpr Vector3(float ix, float iy, float iz) noexcept
                : XMFLOAT3(ix, iy, iz)
            {}
            explicit Vector3(_In_reads_(3) const float* pArray) noexcept
                : XMFLOAT3(pArray)
            {}
            Vector3(FXMVECTOR V) noexcept { XMStoreFloat3(this, V); }
            Vector3(const XMFLOAT3& V) noexcept
            {
                this->x = V.x;
                this->y = V.y;
                this->z = V.z;
            }
            explicit Vector3(const XMVECTORF32& F) noexcept
            {
                this->x = F.f[0];
                this->y = F.f[1];
                this->z = F.f[2];
            }

            Vector3(Vector3 const&)            = default;
            Vector3& operator=(Vector3 const&) = default;

            Vector3(Vector3&&)            = default;
            Vector3& operator=(Vector3&&) = default;

            operator XMVECTOR() const noexcept { return XMLoadFloat3(this); }

            // Comparison operators
            bool operator==(Vector3 const& V) const noexcept { return ((x == V.x) && (y == V.y) && (z == V.z)); }
            bool operator!=(Vector3 const& V) const noexcept { return ((x != V.x) || (y != V.y) || (z != V.z)); }

            // Assignment operators
            Vector3& operator=(const XMVECTORF32& F) noexcept
            {
                x = F.f[0];
                y = F.f[1];
                z = F.f[2];
                return *this;
            }
            Vector3& operator+=(Vector3 const& V) noexcept
            {
                x += V.x;
                y += V.y;
                z += V.z;
                return *this;
            }
            Vector3& operator-=(Vector3 const& V) noexcept
            {
                x -= V.x;
                y -= V.y;
                z -= V.z;
                return *this;
            }
            Vector3& operator*=(Vector3 const& V) noexcept
            {
                x *= V.x;
                y *= V.y;
                z *= V.z;
                return *this;
            }
            Vector3& operator*=(float S) noexcept
            {
                x *= S;
                y *= S;
                z *= S;
                return *this;
            }
            Vector3& operator/=(float S) noexcept
            {
                x /= S;
                y /= S;
                z /= S;
                return *this;
            }

            // Unary operators
            Vector3 operator+() const noexcept { return *this; }
            Vector3 operator-() const noexcept { return Vector3(-x, -y, -z); }

            // Vector operations
            bool InBounds(Vector3 const& Bounds) const noexcept;

            float Length() const noexcept { return std::sqrt((x * x) + (y * y) + (z * z)); }
            float LengthSquared() const noexcept { return (x * x) + (y * y) + (z * z); }

            float Dot(Vector3 const& V) const noexcept { return (x * V.x) + (y * V.y) + (z * V.z); }
            void  Cross(Vector3 const& V, Vector3& result) const noexcept
            {
                result.x = y * V.z - z * V.y;
                result.y = z * V.x - x * V.z;
                result.z = x * V.y - y * V.x;
            }
            Vector3 Cross(Vector3 const& V) const noexcept { return Vector3(y * V.z - z * V.y, z * V.x - x * V.z, x * V.y - y * V.x); }

            void Normalize() noexcept;
            void Normalize(Vector3& result) const noexcept;

            void Clamp(Vector3 const& vmin, Vector3 const& vmax) noexcept;
            void Clamp(Vector3 const& vmin, Vector3 const& vmax, Vector3& result) const noexcept;

            // Static functions
            static float Distance(Vector3 const& v1, Vector3 const& v2) noexcept;
            static float DistanceSquared(Vector3 const& v1, Vector3 const& v2) noexcept;

            static void    Min(Vector3 const& v1, Vector3 const& v2, Vector3& result) noexcept;
            static Vector3 Min(Vector3 const& v1, Vector3 const& v2) noexcept;

            static void    Max(Vector3 const& v1, Vector3 const& v2, Vector3& result) noexcept;
            static Vector3 Max(Vector3 const& v1, Vector3 const& v2) noexcept;

            static void    Lerp(Vector3 const& v1, Vector3 const& v2, float t, Vector3& result) noexcept;
            static Vector3 Lerp(Vector3 const& v1, Vector3 const& v2, float t) noexcept;

            static void    SmoothStep(Vector3 const& v1, Vector3 const& v2, float t, Vector3& result) noexcept;
            static Vector3 SmoothStep(Vector3 const& v1, Vector3 const& v2, float t) noexcept;

            static void    Barycentric(Vector3 const& v1, Vector3 const& v2, Vector3 const& v3, float f, float g, Vector3& result) noexcept;
            static Vector3 Barycentric(Vector3 const& v1, Vector3 const& v2, Vector3 const& v3, float f, float g) noexcept;

            static void
            CatmullRom(Vector3 const& v1, Vector3 const& v2, Vector3 const& v3, Vector3 const& v4, float t, Vector3& result) noexcept;
            static Vector3 CatmullRom(Vector3 const& v1, Vector3 const& v2, Vector3 const& v3, Vector3 const& v4, float t) noexcept;

            static void
            Hermite(Vector3 const& v1, Vector3 const& t1, Vector3 const& v2, Vector3 const& t2, float t, Vector3& result) noexcept;
            static Vector3 Hermite(Vector3 const& v1, Vector3 const& t1, Vector3 const& v2, Vector3 const& t2, float t) noexcept;

            static void    Reflect(Vector3 const& ivec, Vector3 const& nvec, Vector3& result) noexcept;
            static Vector3 Reflect(Vector3 const& ivec, Vector3 const& nvec) noexcept;

            static void    Refract(Vector3 const& ivec, Vector3 const& nvec, float refractionIndex, Vector3& result) noexcept;
            static Vector3 Refract(Vector3 const& ivec, Vector3 const& nvec, float refractionIndex) noexcept;

            static void    Transform(Vector3 const& v, Quaternion const& quat, Vector3& result) noexcept;
            static Vector3 Transform(Vector3 const& v, Quaternion const& quat) noexcept;

            static void    Transform(Vector3 const& v, Matrix const& m, Vector3& result) noexcept;
            static Vector3 Transform(Vector3 const& v, Matrix const& m) noexcept;
            static void    Transform(_In_reads_(count) const Vector3* varray,
                size_t                                             count,
                Matrix const&                                      m,
                _Out_writes_(count) Vector3*                       resultArray) noexcept;

            static void Transform(Vector3 const& v, Matrix const& m, Vector4& result) noexcept;
            static void Transform(_In_reads_(count) const Vector3* varray,
                size_t                                             count,
                Matrix const&                                      m,
                _Out_writes_(count) Vector4*                       resultArray) noexcept;

            static void    TransformNormal(Vector3 const& v, Matrix const& m, Vector3& result) noexcept;
            static Vector3 TransformNormal(Vector3 const& v, Matrix const& m) noexcept;
            static void    TransformNormal(_In_reads_(count) const Vector3* varray,
                size_t                                                   count,
                Matrix const&                                            m,
                _Out_writes_(count) Vector3*                             resultArray) noexcept;

            // Constants
            static Vector3 const Zero;
            static Vector3 const One;
            static Vector3 const UnitX;
            static Vector3 const UnitY;
            static Vector3 const UnitZ;
            static Vector3 const Up;
            static Vector3 const Down;
            static Vector3 const Right;
            static Vector3 const Left;
            static Vector3 const Forward;
            static Vector3 const Backward;
        };

        // Binary operators
        DIRECTX_TOOLKIT_API inline Vector3 operator+(Vector3 const& V1, Vector3 const& V2) noexcept
        {
            return Vector3(V1.x + V2.x, V1.y + V2.y, V1.z + V2.z);
        }
        DIRECTX_TOOLKIT_API inline Vector3 operator-(Vector3 const& V1, Vector3 const& V2) noexcept
        {
            return Vector3(V1.x - V2.x, V1.y - V2.y, V1.z - V2.z);
        }
        DIRECTX_TOOLKIT_API inline Vector3 operator*(Vector3 const& V1, Vector3 const& V2) noexcept
        {
            return Vector3(V1.x * V2.x, V1.y * V2.y, V1.z * V2.z);
        }
        DIRECTX_TOOLKIT_API inline Vector3 operator*(Vector3 const& V, float S) noexcept
        {
            return Vector3(V.x * S, V.y * S, V.z * S);
        }
        DIRECTX_TOOLKIT_API inline Vector3 operator*(float S, Vector3 const& V) noexcept
        {
            return Vector3(S * V.x, S * V.y, S * V.z);
        }
        DIRECTX_TOOLKIT_API inline Vector3 operator/(Vector3 const& V1, Vector3 const& V2) noexcept
        {
            return Vector3(V1.x / V2.x, V1.y / V2.y, V1.z / V2.z);
        }
        DIRECTX_TOOLKIT_API inline Vector3 operator/(Vector3 const& V, float S) noexcept
        {
            return Vector3(V.x / S, V.y / S, V.z / S);
        }
        DIRECTX_TOOLKIT_API inline Vector3 operator/(float S, Vector3 const& V) noexcept
        {
            return Vector3(S / V.x, S / V.y, S / V.z);
        }

        //------------------------------------------------------------------------------
        // 4D vector
        struct DIRECTX_TOOLKIT_API Vector4 : public XMFLOAT4
        {
            Vector4() noexcept
                : XMFLOAT4(0.f, 0.f, 0.f, 0.f)
            {}
            constexpr explicit Vector4(float ix) noexcept
                : XMFLOAT4(ix, ix, ix, ix)
            {}
            constexpr Vector4(float ix, float iy, float iz, float iw) noexcept
                : XMFLOAT4(ix, iy, iz, iw)
            {}
            explicit Vector4(_In_reads_(4) const float* pArray) noexcept
                : XMFLOAT4(pArray)
            {}
            Vector4(FXMVECTOR V) noexcept { XMStoreFloat4(this, V); }
            Vector4(const XMFLOAT4& V) noexcept
            {
                this->x = V.x;
                this->y = V.y;
                this->z = V.z;
                this->w = V.w;
            }
            explicit Vector4(const XMVECTORF32& F) noexcept
            {
                this->x = F.f[0];
                this->y = F.f[1];
                this->z = F.f[2];
                this->w = F.f[3];
            }

            Vector4(Vector4 const&)            = default;
            Vector4& operator=(Vector4 const&) = default;

            Vector4(Vector4&&)            = default;
            Vector4& operator=(Vector4&&) = default;

            operator XMVECTOR() const noexcept { return XMLoadFloat4(this); }

            // Comparison operators
            bool operator==(Vector4 const& V) const noexcept;
            bool operator!=(Vector4 const& V) const noexcept;

            // Assignment operators
            Vector4& operator=(const XMVECTORF32& F) noexcept
            {
                x = F.f[0];
                y = F.f[1];
                z = F.f[2];
                w = F.f[3];
                return *this;
            }
            Vector4& operator+=(Vector4 const& V) noexcept;
            Vector4& operator-=(Vector4 const& V) noexcept;
            Vector4& operator*=(Vector4 const& V) noexcept;
            Vector4& operator*=(float S) noexcept;
            Vector4& operator/=(float S) noexcept;

            // Unary operators
            Vector4 operator+() const noexcept { return *this; }
            Vector4 operator-() const noexcept { return Vector4(-x, -y, -z, -w); }

            // Vector operations
            bool InBounds(Vector4 const& Bounds) const noexcept;

            float Length() const noexcept;
            float LengthSquared() const noexcept;

            float   Dot(Vector4 const& V) const noexcept;
            void    Cross(Vector4 const& v1, Vector4 const& v2, Vector4& result) const noexcept;
            Vector4 Cross(Vector4 const& v1, Vector4 const& v2) const noexcept;

            void Normalize() noexcept;
            void Normalize(Vector4& result) const noexcept;

            void Clamp(Vector4 const& vmin, Vector4 const& vmax) noexcept;
            void Clamp(Vector4 const& vmin, Vector4 const& vmax, Vector4& result) const noexcept;

            // Static functions
            static float Distance(Vector4 const& v1, Vector4 const& v2) noexcept;
            static float DistanceSquared(Vector4 const& v1, Vector4 const& v2) noexcept;

            static void    Min(Vector4 const& v1, Vector4 const& v2, Vector4& result) noexcept;
            static Vector4 Min(Vector4 const& v1, Vector4 const& v2) noexcept;

            static void    Max(Vector4 const& v1, Vector4 const& v2, Vector4& result) noexcept;
            static Vector4 Max(Vector4 const& v1, Vector4 const& v2) noexcept;

            static void    Lerp(Vector4 const& v1, Vector4 const& v2, float t, Vector4& result) noexcept;
            static Vector4 Lerp(Vector4 const& v1, Vector4 const& v2, float t) noexcept;

            static void    SmoothStep(Vector4 const& v1, Vector4 const& v2, float t, Vector4& result) noexcept;
            static Vector4 SmoothStep(Vector4 const& v1, Vector4 const& v2, float t) noexcept;

            static void    Barycentric(Vector4 const& v1, Vector4 const& v2, Vector4 const& v3, float f, float g, Vector4& result) noexcept;
            static Vector4 Barycentric(Vector4 const& v1, Vector4 const& v2, Vector4 const& v3, float f, float g) noexcept;

            static void
            CatmullRom(Vector4 const& v1, Vector4 const& v2, Vector4 const& v3, Vector4 const& v4, float t, Vector4& result) noexcept;
            static Vector4 CatmullRom(Vector4 const& v1, Vector4 const& v2, Vector4 const& v3, Vector4 const& v4, float t) noexcept;

            static void
            Hermite(Vector4 const& v1, Vector4 const& t1, Vector4 const& v2, Vector4 const& t2, float t, Vector4& result) noexcept;
            static Vector4 Hermite(Vector4 const& v1, Vector4 const& t1, Vector4 const& v2, Vector4 const& t2, float t) noexcept;

            static void    Reflect(Vector4 const& ivec, Vector4 const& nvec, Vector4& result) noexcept;
            static Vector4 Reflect(Vector4 const& ivec, Vector4 const& nvec) noexcept;

            static void    Refract(Vector4 const& ivec, Vector4 const& nvec, float refractionIndex, Vector4& result) noexcept;
            static Vector4 Refract(Vector4 const& ivec, Vector4 const& nvec, float refractionIndex) noexcept;

            static void    Transform(Vector2 const& v, Quaternion const& quat, Vector4& result) noexcept;
            static Vector4 Transform(Vector2 const& v, Quaternion const& quat) noexcept;

            static void    Transform(Vector3 const& v, Quaternion const& quat, Vector4& result) noexcept;
            static Vector4 Transform(Vector3 const& v, Quaternion const& quat) noexcept;

            static void    Transform(Vector4 const& v, Quaternion const& quat, Vector4& result) noexcept;
            static Vector4 Transform(Vector4 const& v, Quaternion const& quat) noexcept;

            static void    Transform(Vector4 const& v, Matrix const& m, Vector4& result) noexcept;
            static Vector4 Transform(Vector4 const& v, Matrix const& m) noexcept;
            static void    Transform(_In_reads_(count) const Vector4* varray,
                size_t                                             count,
                Matrix const&                                      m,
                _Out_writes_(count) Vector4*                       resultArray) noexcept;

            // Constants
            static Vector4 const Zero;
            static Vector4 const One;
            static Vector4 const UnitX;
            static Vector4 const UnitY;
            static Vector4 const UnitZ;
            static Vector4 const UnitW;
        };

        // Binary operators
        DIRECTX_TOOLKIT_API Vector4 operator+(Vector4 const& V1, Vector4 const& V2) noexcept;
        DIRECTX_TOOLKIT_API Vector4 operator-(Vector4 const& V1, Vector4 const& V2) noexcept;
        DIRECTX_TOOLKIT_API Vector4 operator*(Vector4 const& V1, Vector4 const& V2) noexcept;
        DIRECTX_TOOLKIT_API Vector4 operator*(Vector4 const& V, float S) noexcept;
        DIRECTX_TOOLKIT_API Vector4 operator*(float S, Vector4 const& V) noexcept;
        DIRECTX_TOOLKIT_API Vector4 operator/(Vector4 const& V1, Vector4 const& V2) noexcept;
        DIRECTX_TOOLKIT_API Vector4 operator/(Vector4 const& V, float S) noexcept;
        DIRECTX_TOOLKIT_API Vector4 operator/(float S, Vector4 const& V) noexcept;

        //------------------------------------------------------------------------------
        // 4x4 Matrix (assumes right-handed cooordinates)
        struct DIRECTX_TOOLKIT_API Matrix : public XMFLOAT4X4
        {
            Matrix() noexcept
                : XMFLOAT4X4(1.f, 0, 0, 0, 0, 1.f, 0, 0, 0, 0, 1.f, 0, 0, 0, 0, 1.f)
            {}
            constexpr Matrix(float m00,
                float              m01,
                float              m02,
                float              m03,
                float              m10,
                float              m11,
                float              m12,
                float              m13,
                float              m20,
                float              m21,
                float              m22,
                float              m23,
                float              m30,
                float              m31,
                float              m32,
                float              m33) noexcept
                : XMFLOAT4X4(m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33)
            {}
            explicit Matrix(Vector3 const& r0, Vector3 const& r1, Vector3 const& r2) noexcept
                : XMFLOAT4X4(r0.x, r0.y, r0.z, 0, r1.x, r1.y, r1.z, 0, r2.x, r2.y, r2.z, 0, 0, 0, 0, 1.f)
            {}
            explicit Matrix(Vector4 const& r0, Vector4 const& r1, Vector4 const& r2, Vector4 const& r3) noexcept
                : XMFLOAT4X4(r0.x, r0.y, r0.z, r0.w, r1.x, r1.y, r1.z, r1.w, r2.x, r2.y, r2.z, r2.w, r3.x, r3.y, r3.z, r3.w)
            {}
            Matrix(const XMFLOAT4X4& M) noexcept { memcpy(this, &M, sizeof(XMFLOAT4X4)); }
            Matrix(const XMFLOAT3X3& M) noexcept;
            Matrix(const XMFLOAT4X3& M) noexcept;

            explicit Matrix(_In_reads_(16) const float* pArray) noexcept
                : XMFLOAT4X4(pArray)
            {}
            Matrix(CXMMATRIX M) noexcept { XMStoreFloat4x4(this, M); }

            Matrix(Matrix const&)            = default;
            Matrix& operator=(Matrix const&) = default;

            Matrix(Matrix&&)            = default;
            Matrix& operator=(Matrix&&) = default;

            operator XMMATRIX() const noexcept { return XMLoadFloat4x4(this); }

            // Comparison operators
            bool operator==(Matrix const& M) const noexcept;
            bool operator!=(Matrix const& M) const noexcept;

            // Assignment operators
            Matrix& operator=(const XMFLOAT3X3& M) noexcept;
            Matrix& operator=(const XMFLOAT4X3& M) noexcept;
            Matrix& operator+=(Matrix const& M) noexcept;
            Matrix& operator-=(Matrix const& M) noexcept;
            Matrix& operator*=(Matrix const& M) noexcept;
            Matrix& operator*=(float S) noexcept;
            Matrix& operator/=(float S) noexcept;

            Matrix& operator/=(Matrix const& M) noexcept;
            // Element-wise divide

            // Unary operators
            Matrix operator+() const noexcept { return *this; }
            Matrix operator-() const noexcept;

            // Properties
            Vector3 Up() const noexcept { return Vector3(_21, _22, _23); }
            void    Up(Vector3 const& v) noexcept
            {
                _21 = v.x;
                _22 = v.y;
                _23 = v.z;
            }

            Vector3 Down() const noexcept { return Vector3(-_21, -_22, -_23); }
            void    Down(Vector3 const& v) noexcept
            {
                _21 = -v.x;
                _22 = -v.y;
                _23 = -v.z;
            }

            Vector3 Right() const noexcept { return Vector3(_11, _12, _13); }
            void    Right(Vector3 const& v) noexcept
            {
                _11 = v.x;
                _12 = v.y;
                _13 = v.z;
            }

            Vector3 Left() const noexcept { return Vector3(-_11, -_12, -_13); }
            void    Left(Vector3 const& v) noexcept
            {
                _11 = -v.x;
                _12 = -v.y;
                _13 = -v.z;
            }

            Vector3 Forward() const noexcept { return Vector3(-_31, -_32, -_33); }
            void    Forward(Vector3 const& v) noexcept
            {
                _31 = -v.x;
                _32 = -v.y;
                _33 = -v.z;
            }

            Vector3 Backward() const noexcept { return Vector3(_31, _32, _33); }
            void    Backward(Vector3 const& v) noexcept
            {
                _31 = v.x;
                _32 = v.y;
                _33 = v.z;
            }

            Vector3 Translation() const noexcept { return Vector3(_41, _42, _43); }
            void    Translation(Vector3 const& v) noexcept
            {
                _41 = v.x;
                _42 = v.y;
                _43 = v.z;
            }

            // Matrix operations
            bool Decompose(Vector3& scale, Quaternion& rotation, Vector3& translation) noexcept;

            Matrix Transpose() const noexcept;
            void   Transpose(Matrix& result) const noexcept;

            Matrix Invert() const noexcept;
            void   Invert(Matrix& result) const noexcept;

            float Determinant() const noexcept;

            // Computes rotation about y-axis (y), then x-axis (x), then z-axis (z)
            Vector3 ToEuler() const noexcept;

            // Static functions
            static Matrix CreateBillboard(Vector3 const& object,
                Vector3 const&                           cameraPosition,
                Vector3 const&                           cameraUp,
                _In_opt_ const Vector3*                  cameraForward = nullptr) noexcept;

            static Matrix CreateConstrainedBillboard(Vector3 const& object,
                Vector3 const&                                      cameraPosition,
                Vector3 const&                                      rotateAxis,
                _In_opt_ const Vector3*                             cameraForward = nullptr,
                _In_opt_ const Vector3*                             objectForward = nullptr) noexcept;

            static Matrix CreateTranslation(Vector3 const& position) noexcept;
            static Matrix CreateTranslation(float x, float y, float z) noexcept;

            static Matrix CreateScale(Vector3 const& scales) noexcept;
            static Matrix CreateScale(float xs, float ys, float zs) noexcept;
            static Matrix CreateScale(float scale) noexcept;

            static Matrix CreateRotationX(float radians) noexcept;
            static Matrix CreateRotationY(float radians) noexcept;
            static Matrix CreateRotationZ(float radians) noexcept;

            static Matrix CreateFromAxisAngle(Vector3 const& axis, float angle) noexcept;

            static Matrix CreatePerspectiveFieldOfView(float fov, float aspectRatio, float nearPlane, float farPlane) noexcept;
            static Matrix CreatePerspective(float width, float height, float nearPlane, float farPlane) noexcept;
            static Matrix
            CreatePerspectiveOffCenter(float left, float right, float bottom, float top, float nearPlane, float farPlane) noexcept;
            static Matrix CreateOrthographic(float width, float height, float zNearPlane, float zFarPlane) noexcept;
            static Matrix
            CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane) noexcept;

            static Matrix CreateLookAt(Vector3 const& position, Vector3 const& target, Vector3 const& up) noexcept;
            static Matrix CreateWorld(Vector3 const& position, Vector3 const& forward, Vector3 const& up) noexcept;

            static Matrix CreateFromQuaternion(Quaternion const& quat) noexcept;

            // Rotates about y-axis (yaw), then x-axis (pitch), then z-axis (roll)
            static Matrix CreateFromYawPitchRoll(float yaw, float pitch, float roll) noexcept;

            // Rotates about y-axis (angles.y), then x-axis (angles.x), then z-axis (angles.z)
            static Matrix CreateFromYawPitchRoll(Vector3 const& angles) noexcept;

            static Matrix CreateShadow(Vector3 const& lightDir, Plane const& plane) noexcept;

            static Matrix CreateReflection(Plane const& plane) noexcept;

            static void   Lerp(Matrix const& M1, Matrix const& M2, float t, Matrix& result) noexcept;
            static Matrix Lerp(Matrix const& M1, Matrix const& M2, float t) noexcept;

            static void   Transform(Matrix const& M, Quaternion const& rotation, Matrix& result) noexcept;
            static Matrix Transform(Matrix const& M, Quaternion const& rotation) noexcept;

            // Constants
            static Matrix const Identity;
        };

        // Binary operators
        DIRECTX_TOOLKIT_API Matrix operator+(Matrix const& M1, Matrix const& M2) noexcept;
        DIRECTX_TOOLKIT_API Matrix operator-(Matrix const& M1, Matrix const& M2) noexcept;
        DIRECTX_TOOLKIT_API Matrix operator*(Matrix const& M1, Matrix const& M2) noexcept;
        DIRECTX_TOOLKIT_API Matrix operator*(Matrix const& M, float S) noexcept;
        DIRECTX_TOOLKIT_API Matrix operator*(float S, Matrix const& M) noexcept;
        DIRECTX_TOOLKIT_API Matrix operator/(Matrix const& M, float S) noexcept;
        DIRECTX_TOOLKIT_API Matrix operator/(Matrix const& M1, Matrix const& M2) noexcept;
        // Element-wise divide
        DIRECTX_TOOLKIT_API Matrix operator/(float S, Matrix const& M) noexcept;

        //-----------------------------------------------------------------------------
        // Plane
        struct DIRECTX_TOOLKIT_API Plane : public XMFLOAT4
        {
            Plane() noexcept
                : XMFLOAT4(0.f, 1.f, 0.f, 0.f)
            {}
            constexpr Plane(float ix, float iy, float iz, float iw) noexcept
                : XMFLOAT4(ix, iy, iz, iw)
            {}
            Plane(Vector3 const& normal, float d) noexcept
                : XMFLOAT4(normal.x, normal.y, normal.z, d)
            {}
            Plane(Vector3 const& point1, Vector3 const& point2, Vector3 const& point3) noexcept;
            Plane(Vector3 const& point, Vector3 const& normal) noexcept;
            explicit Plane(Vector4 const& v) noexcept
                : XMFLOAT4(v.x, v.y, v.z, v.w)
            {}
            explicit Plane(_In_reads_(4) const float* pArray) noexcept
                : XMFLOAT4(pArray)
            {}
            Plane(FXMVECTOR V) noexcept { XMStoreFloat4(this, V); }
            Plane(const XMFLOAT4& p) noexcept
            {
                this->x = p.x;
                this->y = p.y;
                this->z = p.z;
                this->w = p.w;
            }
            explicit Plane(const XMVECTORF32& F) noexcept
            {
                this->x = F.f[0];
                this->y = F.f[1];
                this->z = F.f[2];
                this->w = F.f[3];
            }

            Plane(Plane const&)            = default;
            Plane& operator=(Plane const&) = default;

            Plane(Plane&&)            = default;
            Plane& operator=(Plane&&) = default;

            operator XMVECTOR() const noexcept { return XMLoadFloat4(this); }

            // Comparison operators
            bool operator==(Plane const& p) const noexcept;
            bool operator!=(Plane const& p) const noexcept;

            // Assignment operators
            Plane& operator=(const XMVECTORF32& F) noexcept
            {
                x = F.f[0];
                y = F.f[1];
                z = F.f[2];
                w = F.f[3];
                return *this;
            }

            // Properties
            Vector3 Normal() const noexcept { return Vector3(x, y, z); }
            void    Normal(Vector3 const& normal) noexcept
            {
                x = normal.x;
                y = normal.y;
                z = normal.z;
            }

            float D() const noexcept { return w; }
            void  D(float d) noexcept { w = d; }

            // Plane operations
            void Normalize() noexcept;
            void Normalize(Plane& result) const noexcept;

            float Dot(Vector4 const& v) const noexcept;
            float DotCoordinate(Vector3 const& position) const noexcept;
            float DotNormal(Vector3 const& normal) const noexcept;

            // Static functions
            static void  Transform(Plane const& plane, Matrix const& M, Plane& result) noexcept;
            static Plane Transform(Plane const& plane, Matrix const& M) noexcept;

            static void  Transform(Plane const& plane, Quaternion const& rotation, Plane& result) noexcept;
            static Plane Transform(Plane const& plane, Quaternion const& rotation) noexcept;
            // Input quaternion must be the inverse transpose of the transformation
        };

        //------------------------------------------------------------------------------
        // Quaternion
        struct DIRECTX_TOOLKIT_API Quaternion : public XMFLOAT4
        {
            Quaternion() noexcept
                : XMFLOAT4(0, 0, 0, 1.f)
            {}
            constexpr Quaternion(float ix, float iy, float iz, float iw) noexcept
                : XMFLOAT4(ix, iy, iz, iw)
            {}
            Quaternion(Vector3 const& v, float scalar) noexcept
                : XMFLOAT4(v.x, v.y, v.z, scalar)
            {}
            explicit Quaternion(Vector4 const& v) noexcept
                : XMFLOAT4(v.x, v.y, v.z, v.w)
            {}
            explicit Quaternion(_In_reads_(4) const float* pArray) noexcept
                : XMFLOAT4(pArray)
            {}
            Quaternion(FXMVECTOR V) noexcept { XMStoreFloat4(this, V); }
            Quaternion(const XMFLOAT4& q) noexcept
            {
                this->x = q.x;
                this->y = q.y;
                this->z = q.z;
                this->w = q.w;
            }
            explicit Quaternion(const XMVECTORF32& F) noexcept
            {
                this->x = F.f[0];
                this->y = F.f[1];
                this->z = F.f[2];
                this->w = F.f[3];
            }

            Quaternion(Quaternion const&)            = default;
            Quaternion& operator=(Quaternion const&) = default;

            Quaternion(Quaternion&&)            = default;
            Quaternion& operator=(Quaternion&&) = default;

            operator XMVECTOR() const noexcept { return XMLoadFloat4(this); }

            // Comparison operators
            bool operator==(Quaternion const& q) const noexcept;
            bool operator!=(Quaternion const& q) const noexcept;

            // Assignment operators
            Quaternion& operator=(const XMVECTORF32& F) noexcept
            {
                x = F.f[0];
                y = F.f[1];
                z = F.f[2];
                w = F.f[3];
                return *this;
            }
            Quaternion& operator+=(Quaternion const& q) noexcept;
            Quaternion& operator-=(Quaternion const& q) noexcept;
            Quaternion& operator*=(Quaternion const& q) noexcept;
            Quaternion& operator*=(float S) noexcept;
            Quaternion& operator/=(Quaternion const& q) noexcept;

            // Unary operators
            Quaternion operator+() const noexcept { return *this; }
            Quaternion operator-() const noexcept { return Quaternion(-x, -y, -z, -w); }

            // Quaternion operations
            float Length() const noexcept;
            float LengthSquared() const noexcept;

            void Normalize() noexcept;
            void Normalize(Quaternion& result) const noexcept;

            void Conjugate() noexcept;
            void Conjugate(Quaternion& result) const noexcept;

            void Inverse(Quaternion& result) const noexcept;

            float Dot(Quaternion const& Q) const noexcept;

            void         RotateTowards(Quaternion const& target, float maxAngle) noexcept;
            void __cdecl RotateTowards(Quaternion const& target, float maxAngle, Quaternion& result) const noexcept;

            // Computes rotation about y-axis (y), then x-axis (x), then z-axis (z)
            Vector3 ToEuler() const noexcept;

            // Static functions
            static Quaternion CreateFromAxisAngle(Vector3 const& axis, float angle) noexcept;

            // Rotates about y-axis (yaw), then x-axis (pitch), then z-axis (roll)
            static Quaternion CreateFromYawPitchRoll(float yaw, float pitch, float roll) noexcept;

            // Rotates about y-axis (angles.y), then x-axis (angles.x), then z-axis (angles.z)
            static Quaternion CreateFromYawPitchRoll(Vector3 const& angles) noexcept;

            static Quaternion CreateFromRotationMatrix(Matrix const& M) noexcept;

            static void       Lerp(Quaternion const& q1, Quaternion const& q2, float t, Quaternion& result) noexcept;
            static Quaternion Lerp(Quaternion const& q1, Quaternion const& q2, float t) noexcept;

            static void       Slerp(Quaternion const& q1, Quaternion const& q2, float t, Quaternion& result) noexcept;
            static Quaternion Slerp(Quaternion const& q1, Quaternion const& q2, float t) noexcept;

            static void       Concatenate(Quaternion const& q1, Quaternion const& q2, Quaternion& result) noexcept;
            static Quaternion Concatenate(Quaternion const& q1, Quaternion const& q2) noexcept;

            static void __cdecl FromToRotation(Vector3 const& fromDir, Vector3 const& toDir, Quaternion& result) noexcept;
            static Quaternion   FromToRotation(Vector3 const& fromDir, Vector3 const& toDir) noexcept;

            static void __cdecl LookRotation(Vector3 const& forward, Vector3 const& up, Quaternion& result) noexcept;
            static Quaternion   LookRotation(Vector3 const& forward, Vector3 const& up) noexcept;

            static float Angle(Quaternion const& q1, Quaternion const& q2) noexcept;

            // Constants
            static Quaternion const Identity;
        };

        // Binary operators
        DIRECTX_TOOLKIT_API Quaternion operator+(Quaternion const& Q1, Quaternion const& Q2) noexcept;
        DIRECTX_TOOLKIT_API Quaternion operator-(Quaternion const& Q1, Quaternion const& Q2) noexcept;
        DIRECTX_TOOLKIT_API Quaternion operator*(Quaternion const& Q1, Quaternion const& Q2) noexcept;
        DIRECTX_TOOLKIT_API Quaternion operator*(Quaternion const& Q, float S) noexcept;
        DIRECTX_TOOLKIT_API Quaternion operator*(float S, Quaternion const& Q) noexcept;
        DIRECTX_TOOLKIT_API Quaternion operator/(Quaternion const& Q1, Quaternion const& Q2) noexcept;

        //------------------------------------------------------------------------------
        // Color
        struct DIRECTX_TOOLKIT_API Color : public XMFLOAT4
        {
            Color() noexcept
                : XMFLOAT4(0, 0, 0, 1.f)
            {}
            constexpr Color(float _r, float _g, float _b) noexcept
                : XMFLOAT4(_r, _g, _b, 1.f)
            {}
            constexpr Color(float _r, float _g, float _b, float _a) noexcept
                : XMFLOAT4(_r, _g, _b, _a)
            {}
            explicit Color(Vector3 const& clr) noexcept
                : XMFLOAT4(clr.x, clr.y, clr.z, 1.f)
            {}
            explicit Color(Vector4 const& clr) noexcept
                : XMFLOAT4(clr.x, clr.y, clr.z, clr.w)
            {}
            explicit Color(_In_reads_(4) const float* pArray) noexcept
                : XMFLOAT4(pArray)
            {}
            Color(FXMVECTOR V) noexcept { XMStoreFloat4(this, V); }
            Color(const XMFLOAT4& c) noexcept
            {
                this->x = c.x;
                this->y = c.y;
                this->z = c.z;
                this->w = c.w;
            }
            explicit Color(const XMVECTORF32& F) noexcept
            {
                this->x = F.f[0];
                this->y = F.f[1];
                this->z = F.f[2];
                this->w = F.f[3];
            }

            // BGRA Direct3D 9 D3DCOLOR packed color
            explicit Color(DirectX::PackedVector::XMCOLOR const& Packed) noexcept;

            // RGBA XNA Game Studio packed color
            explicit Color(DirectX::PackedVector::XMUBYTEN4 const& Packed) noexcept;

            Color(Color const&)            = default;
            Color& operator=(Color const&) = default;

            Color(Color&&)            = default;
            Color& operator=(Color&&) = default;

            operator XMVECTOR() const noexcept { return XMLoadFloat4(this); }
            operator float const*() const noexcept { return reinterpret_cast<float const*>(this); }

            // Comparison operators
            bool operator==(Color const& c) const noexcept;
            bool operator!=(Color const& c) const noexcept;

            // Assignment operators
            Color& operator=(const XMVECTORF32& F) noexcept
            {
                x = F.f[0];
                y = F.f[1];
                z = F.f[2];
                w = F.f[3];
                return *this;
            }
            Color& operator=(DirectX::PackedVector::XMCOLOR const& Packed) noexcept;
            Color& operator=(DirectX::PackedVector::XMUBYTEN4 const& Packed) noexcept;
            Color& operator+=(Color const& c) noexcept;
            Color& operator-=(Color const& c) noexcept;
            Color& operator*=(Color const& c) noexcept;
            Color& operator*=(float S) noexcept;
            Color& operator/=(Color const& c) noexcept;

            // Unary operators
            Color operator+() const noexcept { return *this; }
            Color operator-() const noexcept { return Color(-x, -y, -z, -w); }

            // Properties
            float R() const noexcept { return x; }
            void  R(float r) noexcept { x = r; }

            float G() const noexcept { return y; }
            void  G(float g) noexcept { y = g; }

            float B() const noexcept { return z; }
            void  B(float b) noexcept { z = b; }

            float A() const noexcept { return w; }
            void  A(float a) noexcept { w = a; }

            // Color operations
            DirectX::PackedVector::XMCOLOR   BGRA() const noexcept;
            DirectX::PackedVector::XMUBYTEN4 RGBA() const noexcept;

            Vector3 ToVector3() const noexcept { return Vector3(x, y, z); }
            Vector4 ToVector4() const noexcept { return Vector4(x, y, z, w); }

            void Negate() noexcept;
            void Negate(Color& result) const noexcept;

            void Saturate() noexcept;
            void Saturate(Color& result) const noexcept;

            void Premultiply() noexcept;
            void Premultiply(Color& result) const noexcept;

            void AdjustSaturation(float sat) noexcept;
            void AdjustSaturation(float sat, Color& result) const noexcept;

            void AdjustContrast(float contrast) noexcept;
            void AdjustContrast(float contrast, Color& result) const noexcept;

            // Static functions
            static void  Modulate(Color const& c1, Color const& c2, Color& result) noexcept;
            static Color Modulate(Color const& c1, Color const& c2) noexcept;

            static void  Lerp(Color const& c1, Color const& c2, float t, Color& result) noexcept;
            static Color Lerp(Color const& c1, Color const& c2, float t) noexcept;
        };

        // Binary operators
        DIRECTX_TOOLKIT_API Color operator+(Color const& C1, Color const& C2) noexcept;
        DIRECTX_TOOLKIT_API Color operator-(Color const& C1, Color const& C2) noexcept;
        DIRECTX_TOOLKIT_API Color operator*(Color const& C1, Color const& C2) noexcept;
        DIRECTX_TOOLKIT_API Color operator*(Color const& C, float S) noexcept;
        DIRECTX_TOOLKIT_API Color operator*(float S, Color const& C) noexcept;
        DIRECTX_TOOLKIT_API Color operator/(Color const& C1, Color const& C2) noexcept;

        //------------------------------------------------------------------------------
        // Ray
        class DIRECTX_TOOLKIT_API Ray
        {
        public:
            Vector3 position;
            Vector3 direction;

            Ray() noexcept
                : position(0, 0, 0),
                  direction(0, 0, 1)
            {}
            Ray(Vector3 const& pos, Vector3 const& dir) noexcept
                : position(pos),
                  direction(dir)
            {}

            Ray(Ray const&)            = default;
            Ray& operator=(Ray const&) = default;

            Ray(Ray&&)            = default;
            Ray& operator=(Ray&&) = default;

            // Comparison operators
            bool operator==(Ray const& r) const noexcept;
            bool operator!=(Ray const& r) const noexcept;

            // Ray operations
            bool Intersects(BoundingSphere const& sphere, _Out_ float& Dist) const noexcept;
            bool Intersects(BoundingBox const& box, _Out_ float& Dist) const noexcept;
            bool Intersects(Vector3 const& tri0, Vector3 const& tri1, Vector3 const& tri2, _Out_ float& Dist) const noexcept;
            bool Intersects(Plane const& plane, _Out_ float& Dist) const noexcept;
        };

        //------------------------------------------------------------------------------
        // Viewport
        class DIRECTX_TOOLKIT_API Viewport
        {
        public:
            float x;
            float y;
            float width;
            float height;
            float minDepth;
            float maxDepth;

            Viewport() noexcept
                : x(0.f),
                  y(0.f),
                  width(0.f),
                  height(0.f),
                  minDepth(0.f),
                  maxDepth(1.f)
            {}
            constexpr Viewport(float ix, float iy, float iw, float ih, float iminz = 0.f, float imaxz = 1.f) noexcept
                : x(ix),
                  y(iy),
                  width(iw),
                  height(ih),
                  minDepth(iminz),
                  maxDepth(imaxz)
            {}
            explicit Viewport(const RECT& rct) noexcept
                : x(float(rct.left)),
                  y(float(rct.top)),
                  width(float(rct.right - rct.left)),
                  height(float(rct.bottom - rct.top)),
                  minDepth(0.f),
                  maxDepth(1.f)
            {}

#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
            // Direct3D 11 interop
            explicit Viewport(const D3D11_VIEWPORT& vp) noexcept
                : x(vp.TopLeftX),
                  y(vp.TopLeftY),
                  width(vp.Width),
                  height(vp.Height),
                  minDepth(vp.MinDepth),
                  maxDepth(vp.MaxDepth)
            {}

                                  operator D3D11_VIEWPORT() noexcept { return *reinterpret_cast<const D3D11_VIEWPORT*>(this); }
            const D3D11_VIEWPORT* Get11() const noexcept { return reinterpret_cast<const D3D11_VIEWPORT*>(this); }
            Viewport&             operator=(const D3D11_VIEWPORT& vp) noexcept;
#endif

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
            // Direct3D 12 interop
            explicit Viewport(const D3D12_VIEWPORT& vp) noexcept
                : x(vp.TopLeftX),
                  y(vp.TopLeftY),
                  width(vp.Width),
                  height(vp.Height),
                  minDepth(vp.MinDepth),
                  maxDepth(vp.MaxDepth)
            {}

                                  operator D3D12_VIEWPORT() noexcept { return *reinterpret_cast<const D3D12_VIEWPORT*>(this); }
            const D3D12_VIEWPORT* Get12() const noexcept { return reinterpret_cast<const D3D12_VIEWPORT*>(this); }
            Viewport&             operator=(const D3D12_VIEWPORT& vp) noexcept;
#endif

            Viewport(Viewport const&)            = default;
            Viewport& operator=(Viewport const&) = default;

            Viewport(Viewport&&)            = default;
            Viewport& operator=(Viewport&&) = default;

            // Comparison operators
#if (__cplusplus >= 202002L)
            bool operator==(Viewport const&) const    = default;
            auto operator<= > (Viewport const&) const = default;
#else
            bool operator==(Viewport const& vp) const noexcept;
            bool operator!=(Viewport const& vp) const noexcept;
#endif

            // Assignment operators
            Viewport& operator=(const RECT& rct) noexcept;

            // Viewport operations
            float AspectRatio() const noexcept;

            Vector3 Project(Vector3 const& p, Matrix const& proj, Matrix const& view, Matrix const& world) const noexcept;
            void    Project(Vector3 const& p, Matrix const& proj, Matrix const& view, Matrix const& world, Vector3& result) const noexcept;

            Vector3 Unproject(Vector3 const& p, Matrix const& proj, Matrix const& view, Matrix const& world) const noexcept;
            void Unproject(Vector3 const& p, Matrix const& proj, Matrix const& view, Matrix const& world, Vector3& result) const noexcept;

            // Static methods
#if defined(__dxgi1_2_h__) || defined(__d3d11_x_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
            static RECT __cdecl ComputeDisplayArea(DXGI_SCALING scaling,
                UINT                                            backBufferWidth,
                UINT                                            backBufferHeight,
                int                                             outputWidth,
                int                                             outputHeight) noexcept;
#endif
            static RECT __cdecl ComputeTitleSafeArea(UINT backBufferWidth, UINT backBufferHeight) noexcept;
        };

#include "SimpleMath.inl"

    } // namespace SimpleMath

} // namespace DirectX

//------------------------------------------------------------------------------
// Support for SimpleMath and Standard C++ Library containers
namespace std
{

    template<>
    struct less<DirectX::SimpleMath::Rectangle>
    {
        bool operator()(DirectX::SimpleMath::Rectangle const& r1, DirectX::SimpleMath::Rectangle const& r2) const noexcept
        {
            return ((r1.x < r2.x) || ((r1.x == r2.x) && (r1.y < r2.y)) || ((r1.x == r2.x) && (r1.y == r2.y) && (r1.width < r2.width))
                    || ((r1.x == r2.x) && (r1.y == r2.y) && (r1.width == r2.width) && (r1.height < r2.height)));
        }
    };

    template<>
    struct less<DirectX::SimpleMath::Vector2>
    {
        bool operator()(DirectX::SimpleMath::Vector2 const& V1, DirectX::SimpleMath::Vector2 const& V2) const noexcept
        {
            return ((V1.x < V2.x) || ((V1.x == V2.x) && (V1.y < V2.y)));
        }
    };

    template<>
    struct less<DirectX::SimpleMath::Vector3>
    {
        bool operator()(DirectX::SimpleMath::Vector3 const& V1, DirectX::SimpleMath::Vector3 const& V2) const noexcept
        {
            return ((V1.x < V2.x) || ((V1.x == V2.x) && (V1.y < V2.y)) || ((V1.x == V2.x) && (V1.y == V2.y) && (V1.z < V2.z)));
        }
    };

    template<>
    struct less<DirectX::SimpleMath::Vector4>
    {
        bool operator()(DirectX::SimpleMath::Vector4 const& V1, DirectX::SimpleMath::Vector4 const& V2) const noexcept
        {
            return ((V1.x < V2.x) || ((V1.x == V2.x) && (V1.y < V2.y)) || ((V1.x == V2.x) && (V1.y == V2.y) && (V1.z < V2.z))
                    || ((V1.x == V2.x) && (V1.y == V2.y) && (V1.z == V2.z) && (V1.w < V2.w)));
        }
    };

    template<>
    struct less<DirectX::SimpleMath::Matrix>
    {
        bool operator()(DirectX::SimpleMath::Matrix const& M1, DirectX::SimpleMath::Matrix const& M2) const noexcept
        {
            if (M1._11 != M2._11)
                return M1._11 < M2._11;
            if (M1._12 != M2._12)
                return M1._12 < M2._12;
            if (M1._13 != M2._13)
                return M1._13 < M2._13;
            if (M1._14 != M2._14)
                return M1._14 < M2._14;
            if (M1._21 != M2._21)
                return M1._21 < M2._21;
            if (M1._22 != M2._22)
                return M1._22 < M2._22;
            if (M1._23 != M2._23)
                return M1._23 < M2._23;
            if (M1._24 != M2._24)
                return M1._24 < M2._24;
            if (M1._31 != M2._31)
                return M1._31 < M2._31;
            if (M1._32 != M2._32)
                return M1._32 < M2._32;
            if (M1._33 != M2._33)
                return M1._33 < M2._33;
            if (M1._34 != M2._34)
                return M1._34 < M2._34;
            if (M1._41 != M2._41)
                return M1._41 < M2._41;
            if (M1._42 != M2._42)
                return M1._42 < M2._42;
            if (M1._43 != M2._43)
                return M1._43 < M2._43;
            if (M1._44 != M2._44)
                return M1._44 < M2._44;

            return false;
        }
    };

    template<>
    struct less<DirectX::SimpleMath::Plane>
    {
        bool operator()(DirectX::SimpleMath::Plane const& P1, DirectX::SimpleMath::Plane const& P2) const noexcept
        {
            return ((P1.x < P2.x) || ((P1.x == P2.x) && (P1.y < P2.y)) || ((P1.x == P2.x) && (P1.y == P2.y) && (P1.z < P2.z))
                    || ((P1.x == P2.x) && (P1.y == P2.y) && (P1.z == P2.z) && (P1.w < P2.w)));
        }
    };

    template<>
    struct less<DirectX::SimpleMath::Quaternion>
    {
        bool operator()(DirectX::SimpleMath::Quaternion const& Q1, DirectX::SimpleMath::Quaternion const& Q2) const noexcept
        {
            return ((Q1.x < Q2.x) || ((Q1.x == Q2.x) && (Q1.y < Q2.y)) || ((Q1.x == Q2.x) && (Q1.y == Q2.y) && (Q1.z < Q2.z))
                    || ((Q1.x == Q2.x) && (Q1.y == Q2.y) && (Q1.z == Q2.z) && (Q1.w < Q2.w)));
        }
    };

    template<>
    struct less<DirectX::SimpleMath::Color>
    {
        bool operator()(DirectX::SimpleMath::Color const& C1, DirectX::SimpleMath::Color const& C2) const noexcept
        {
            return ((C1.x < C2.x) || ((C1.x == C2.x) && (C1.y < C2.y)) || ((C1.x == C2.x) && (C1.y == C2.y) && (C1.z < C2.z))
                    || ((C1.x == C2.x) && (C1.y == C2.y) && (C1.z == C2.z) && (C1.w < C2.w)));
        }
    };

    template<>
    struct less<DirectX::SimpleMath::Ray>
    {
        bool operator()(DirectX::SimpleMath::Ray const& R1, DirectX::SimpleMath::Ray const& R2) const noexcept
        {
            if (R1.position.x != R2.position.x)
                return R1.position.x < R2.position.x;
            if (R1.position.y != R2.position.y)
                return R1.position.y < R2.position.y;
            if (R1.position.z != R2.position.z)
                return R1.position.z < R2.position.z;

            if (R1.direction.x != R2.direction.x)
                return R1.direction.x < R2.direction.x;
            if (R1.direction.y != R2.direction.y)
                return R1.direction.y < R2.direction.y;
            if (R1.direction.z != R2.direction.z)
                return R1.direction.z < R2.direction.z;

            return false;
        }
    };

    template<>
    struct less<DirectX::SimpleMath::Viewport>
    {
        bool operator()(DirectX::SimpleMath::Viewport const& vp1, DirectX::SimpleMath::Viewport const& vp2) const noexcept
        {
            if (vp1.x != vp2.x)
                return (vp1.x < vp2.x);
            if (vp1.y != vp2.y)
                return (vp1.y < vp2.y);

            if (vp1.width != vp2.width)
                return (vp1.width < vp2.width);
            if (vp1.height != vp2.height)
                return (vp1.height < vp2.height);

            if (vp1.minDepth != vp2.minDepth)
                return (vp1.minDepth < vp2.minDepth);
            if (vp1.maxDepth != vp2.maxDepth)
                return (vp1.maxDepth < vp2.maxDepth);

            return false;
        }
    };

} // namespace std

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#if defined(DIRECTX_TOOLKIT_IMPORT) && defined(_MSC_VER)
#pragma warning(pop)
#endif
