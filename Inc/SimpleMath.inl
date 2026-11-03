//-------------------------------------------------------------------------------------
// SimpleMath.inl -- Simplified C++ Math wrapper for DirectXMath
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkId=248929
// https://go.microsoft.com/fwlink/?LinkID=615561
//-------------------------------------------------------------------------------------

#pragma once

/****************************************************************************
 *
 * Rectangle
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Rectangle operations
//------------------------------------------------------------------------------
inline Vector2 Rectangle::Location() const noexcept
{
    return Vector2(float(x), float(y));
}

inline Vector2 Rectangle::Center() const noexcept
{
    return Vector2(float(x) + (float(width) / 2.f), float(y) + (float(height) / 2.f));
}

inline bool Rectangle::Contains(Vector2 const& point) const noexcept
{
    return (float(x) <= point.x) && (point.x < float(x + width)) && (float(y) <= point.y) && (point.y < float(y + height));
}

inline void Rectangle::Inflate(long horizAmount, long vertAmount) noexcept
{
    x -= horizAmount;
    y -= vertAmount;
    width += horizAmount;
    height += vertAmount;
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline Rectangle Rectangle::Intersect(Rectangle const& ra, Rectangle const& rb) noexcept
{
    long const righta = ra.x + ra.width;
    long const rightb = rb.x + rb.width;

    long const bottoma = ra.y + ra.height;
    long const bottomb = rb.y + rb.height;

    long const maxX = ra.x > rb.x ? ra.x : rb.x;
    long const maxY = ra.y > rb.y ? ra.y : rb.y;

    long const minRight  = righta < rightb ? righta : rightb;
    long const minBottom = bottoma < bottomb ? bottoma : bottomb;

    Rectangle result;

    if ((minRight > maxX) && (minBottom > maxY))
    {
        result.x      = maxX;
        result.y      = maxY;
        result.width  = minRight - maxX;
        result.height = minBottom - maxY;
    }
    else
    {
        result.x      = 0;
        result.y      = 0;
        result.width  = 0;
        result.height = 0;
    }

    return result;
}

inline RECT Rectangle::Intersect(const RECT& rcta, const RECT& rctb) noexcept
{
    long const maxX = rcta.left > rctb.left ? rcta.left : rctb.left;
    long const maxY = rcta.top > rctb.top ? rcta.top : rctb.top;

    long const minRight  = rcta.right < rctb.right ? rcta.right : rctb.right;
    long const minBottom = rcta.bottom < rctb.bottom ? rcta.bottom : rctb.bottom;

    RECT result;

    if ((minRight > maxX) && (minBottom > maxY))
    {
        result.left   = maxX;
        result.top    = maxY;
        result.right  = minRight;
        result.bottom = minBottom;
    }
    else
    {
        result.left   = 0;
        result.top    = 0;
        result.right  = 0;
        result.bottom = 0;
    }

    return result;
}

inline Rectangle Rectangle::Union(Rectangle const& ra, Rectangle const& rb) noexcept
{
    long const righta = ra.x + ra.width;
    long const rightb = rb.x + rb.width;

    long const bottoma = ra.y + ra.height;
    long const bottomb = rb.y + rb.height;

    int const minX = ra.x < rb.x ? ra.x : rb.x;
    int const minY = ra.y < rb.y ? ra.y : rb.y;

    int const maxRight  = righta > rightb ? righta : rightb;
    int const maxBottom = bottoma > bottomb ? bottoma : bottomb;

    Rectangle result;
    result.x      = minX;
    result.y      = minY;
    result.width  = maxRight - minX;
    result.height = maxBottom - minY;
    return result;
}

inline RECT Rectangle::Union(const RECT& rcta, const RECT& rctb) noexcept
{
    RECT result;
    result.left   = rcta.left < rctb.left ? rcta.left : rctb.left;
    result.top    = rcta.top < rctb.top ? rcta.top : rctb.top;
    result.right  = rcta.right > rctb.right ? rcta.right : rctb.right;
    result.bottom = rcta.bottom > rctb.bottom ? rcta.bottom : rctb.bottom;
    return result;
}

/****************************************************************************
 *
 * Vector2
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Vector operations
//------------------------------------------------------------------------------

inline bool Vector2::InBounds(Vector2 const& Bounds) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&Bounds);
    return XMVector2InBounds(v1, v2);
}

inline void Vector2::Normalize() noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR X  = XMVector2Normalize(v1);
    XMStoreFloat2(this, X);
}

inline void Vector2::Normalize(Vector2& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR X  = XMVector2Normalize(v1);
    XMStoreFloat2(&result, X);
}

inline void Vector2::Clamp(Vector2 const& vmin, Vector2 const& vmax) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&vmin);
    const XMVECTOR v3 = XMLoadFloat2(&vmax);
    const XMVECTOR X  = XMVectorClamp(v1, v2, v3);
    XMStoreFloat2(this, X);
}

inline void Vector2::Clamp(Vector2 const& vmin, Vector2 const& vmax, Vector2& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&vmin);
    const XMVECTOR v3 = XMLoadFloat2(&vmax);
    const XMVECTOR X  = XMVectorClamp(v1, v2, v3);
    XMStoreFloat2(&result, X);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline float Vector2::Distance(Vector2 const& v1, Vector2 const& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR V  = XMVectorSubtract(x2, x1);
    const XMVECTOR X  = XMVector2Length(V);
    return XMVectorGetX(X);
}

inline float Vector2::DistanceSquared(Vector2 const& v1, Vector2 const& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR V  = XMVectorSubtract(x2, x1);
    const XMVECTOR X  = XMVector2LengthSq(V);
    return XMVectorGetX(X);
}

inline void Vector2::Min(Vector2 const& v1, Vector2 const& v2, Vector2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X  = XMVectorMin(x1, x2);
    XMStoreFloat2(&result, X);
}

inline Vector2 Vector2::Min(Vector2 const& v1, Vector2 const& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X  = XMVectorMin(x1, x2);

    Vector2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Vector2::Max(Vector2 const& v1, Vector2 const& v2, Vector2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X  = XMVectorMax(x1, x2);
    XMStoreFloat2(&result, X);
}

inline Vector2 Vector2::Max(Vector2 const& v1, Vector2 const& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X  = XMVectorMax(x1, x2);

    Vector2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Vector2::Lerp(Vector2 const& v1, Vector2 const& v2, float t, Vector2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X  = XMVectorLerp(x1, x2, t);
    XMStoreFloat2(&result, X);
}

inline Vector2 Vector2::Lerp(Vector2 const& v1, Vector2 const& v2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X  = XMVectorLerp(x1, x2, t);

    Vector2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Vector2::SmoothStep(Vector2 const& v1, Vector2 const& v2, float t, Vector2& result) noexcept
{
    using namespace DirectX;
    t                 = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t); // Clamp value to 0 to 1
    t                 = t * t * (3.f - 2.f * t);
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X  = XMVectorLerp(x1, x2, t);
    XMStoreFloat2(&result, X);
}

inline Vector2 Vector2::SmoothStep(Vector2 const& v1, Vector2 const& v2, float t) noexcept
{
    using namespace DirectX;
    t                 = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t); // Clamp value to 0 to 1
    t                 = t * t * (3.f - 2.f * t);
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X  = XMVectorLerp(x1, x2, t);

    Vector2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Vector2::Barycentric(Vector2 const& v1, Vector2 const& v2, Vector2 const& v3, float f, float g, Vector2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR x3 = XMLoadFloat2(&v3);
    const XMVECTOR X  = XMVectorBaryCentric(x1, x2, x3, f, g);
    XMStoreFloat2(&result, X);
}

inline Vector2 Vector2::Barycentric(Vector2 const& v1, Vector2 const& v2, Vector2 const& v3, float f, float g) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR x3 = XMLoadFloat2(&v3);
    const XMVECTOR X  = XMVectorBaryCentric(x1, x2, x3, f, g);

    Vector2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void
Vector2::CatmullRom(Vector2 const& v1, Vector2 const& v2, Vector2 const& v3, Vector2 const& v4, float t, Vector2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR x3 = XMLoadFloat2(&v3);
    const XMVECTOR x4 = XMLoadFloat2(&v4);
    const XMVECTOR X  = XMVectorCatmullRom(x1, x2, x3, x4, t);
    XMStoreFloat2(&result, X);
}

inline Vector2 Vector2::CatmullRom(Vector2 const& v1, Vector2 const& v2, Vector2 const& v3, Vector2 const& v4, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR x3 = XMLoadFloat2(&v3);
    const XMVECTOR x4 = XMLoadFloat2(&v4);
    const XMVECTOR X  = XMVectorCatmullRom(x1, x2, x3, x4, t);

    Vector2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Vector2::Hermite(Vector2 const& v1, Vector2 const& t1, Vector2 const& v2, Vector2 const& t2, float t, Vector2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&t1);
    const XMVECTOR x3 = XMLoadFloat2(&v2);
    const XMVECTOR x4 = XMLoadFloat2(&t2);
    const XMVECTOR X  = XMVectorHermite(x1, x2, x3, x4, t);
    XMStoreFloat2(&result, X);
}

inline Vector2 Vector2::Hermite(Vector2 const& v1, Vector2 const& t1, Vector2 const& v2, Vector2 const& t2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&t1);
    const XMVECTOR x3 = XMLoadFloat2(&v2);
    const XMVECTOR x4 = XMLoadFloat2(&t2);
    const XMVECTOR X  = XMVectorHermite(x1, x2, x3, x4, t);

    Vector2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Vector2::Reflect(Vector2 const& ivec, Vector2 const& nvec, Vector2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat2(&ivec);
    const XMVECTOR n = XMLoadFloat2(&nvec);
    const XMVECTOR X = XMVector2Reflect(i, n);
    XMStoreFloat2(&result, X);
}

inline Vector2 Vector2::Reflect(Vector2 const& ivec, Vector2 const& nvec) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat2(&ivec);
    const XMVECTOR n = XMLoadFloat2(&nvec);
    const XMVECTOR X = XMVector2Reflect(i, n);

    Vector2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Vector2::Refract(Vector2 const& ivec, Vector2 const& nvec, float refractionIndex, Vector2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat2(&ivec);
    const XMVECTOR n = XMLoadFloat2(&nvec);
    const XMVECTOR X = XMVector2Refract(i, n, refractionIndex);
    XMStoreFloat2(&result, X);
}

inline Vector2 Vector2::Refract(Vector2 const& ivec, Vector2 const& nvec, float refractionIndex) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat2(&ivec);
    const XMVECTOR n = XMLoadFloat2(&nvec);
    const XMVECTOR X = XMVector2Refract(i, n, refractionIndex);

    Vector2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Vector2::Transform(Vector2 const& v, Quaternion const& quat, Vector2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMVECTOR q  = XMLoadFloat4(&quat);
    const XMVECTOR X  = XMVector3Rotate(v1, q);
    XMStoreFloat2(&result, X);
}

inline Vector2 Vector2::Transform(Vector2 const& v, Quaternion const& quat) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMVECTOR q  = XMLoadFloat4(&quat);
    const XMVECTOR X  = XMVector3Rotate(v1, q);

    Vector2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Vector2::Transform(Vector2 const& v, Matrix const& m, Vector2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMMATRIX M  = XMLoadFloat4x4(&m);
    const XMVECTOR X  = XMVector2TransformCoord(v1, M);
    XMStoreFloat2(&result, X);
}

inline Vector2 Vector2::Transform(Vector2 const& v, Matrix const& m) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMMATRIX M  = XMLoadFloat4x4(&m);
    const XMVECTOR X  = XMVector2TransformCoord(v1, M);

    Vector2 result;
    XMStoreFloat2(&result, X);
    return result;
}

_Use_decl_annotations_ inline void Vector2::Transform(Vector2 const* varray, size_t count, Matrix const& m, Vector2* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector2TransformCoordStream(resultArray, sizeof(XMFLOAT2), varray, sizeof(XMFLOAT2), count, M);
}

inline void Vector2::Transform(Vector2 const& v, Matrix const& m, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMMATRIX M  = XMLoadFloat4x4(&m);
    const XMVECTOR X  = XMVector2Transform(v1, M);
    XMStoreFloat4(&result, X);
}

_Use_decl_annotations_ inline void Vector2::Transform(Vector2 const* varray, size_t count, Matrix const& m, Vector4* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector2TransformStream(resultArray, sizeof(XMFLOAT4), varray, sizeof(XMFLOAT2), count, M);
}

inline void Vector2::TransformNormal(Vector2 const& v, Matrix const& m, Vector2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMMATRIX M  = XMLoadFloat4x4(&m);
    const XMVECTOR X  = XMVector2TransformNormal(v1, M);
    XMStoreFloat2(&result, X);
}

inline Vector2 Vector2::TransformNormal(Vector2 const& v, Matrix const& m) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMMATRIX M  = XMLoadFloat4x4(&m);
    const XMVECTOR X  = XMVector2TransformNormal(v1, M);

    Vector2 result;
    XMStoreFloat2(&result, X);
    return result;
}

_Use_decl_annotations_ inline void
Vector2::TransformNormal(Vector2 const* varray, size_t count, Matrix const& m, Vector2* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector2TransformNormalStream(resultArray, sizeof(XMFLOAT2), varray, sizeof(XMFLOAT2), count, M);
}

/****************************************************************************
 *
 * Vector3
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Vector operations
//------------------------------------------------------------------------------

inline bool Vector3::InBounds(Vector3 const& Bounds) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&Bounds);
    return XMVector3InBounds(v1, v2);
}

inline void Vector3::Normalize() noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR X  = XMVector3Normalize(v1);
    XMStoreFloat3(this, X);
}

inline void Vector3::Normalize(Vector3& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR X  = XMVector3Normalize(v1);
    XMStoreFloat3(&result, X);
}

inline void Vector3::Clamp(Vector3 const& vmin, Vector3 const& vmax) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&vmin);
    const XMVECTOR v3 = XMLoadFloat3(&vmax);
    const XMVECTOR X  = XMVectorClamp(v1, v2, v3);
    XMStoreFloat3(this, X);
}

inline void Vector3::Clamp(Vector3 const& vmin, Vector3 const& vmax, Vector3& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&vmin);
    const XMVECTOR v3 = XMLoadFloat3(&vmax);
    const XMVECTOR X  = XMVectorClamp(v1, v2, v3);
    XMStoreFloat3(&result, X);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline float Vector3::Distance(Vector3 const& v1, Vector3 const& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR V  = XMVectorSubtract(x2, x1);
    const XMVECTOR X  = XMVector3Length(V);
    return XMVectorGetX(X);
}

inline float Vector3::DistanceSquared(Vector3 const& v1, Vector3 const& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR V  = XMVectorSubtract(x2, x1);
    const XMVECTOR X  = XMVector3LengthSq(V);
    return XMVectorGetX(X);
}

inline void Vector3::Min(Vector3 const& v1, Vector3 const& v2, Vector3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X  = XMVectorMin(x1, x2);
    XMStoreFloat3(&result, X);
}

inline Vector3 Vector3::Min(Vector3 const& v1, Vector3 const& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X  = XMVectorMin(x1, x2);

    Vector3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Vector3::Max(Vector3 const& v1, Vector3 const& v2, Vector3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X  = XMVectorMax(x1, x2);
    XMStoreFloat3(&result, X);
}

inline Vector3 Vector3::Max(Vector3 const& v1, Vector3 const& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X  = XMVectorMax(x1, x2);

    Vector3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Vector3::Lerp(Vector3 const& v1, Vector3 const& v2, float t, Vector3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X  = XMVectorLerp(x1, x2, t);
    XMStoreFloat3(&result, X);
}

inline Vector3 Vector3::Lerp(Vector3 const& v1, Vector3 const& v2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X  = XMVectorLerp(x1, x2, t);

    Vector3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Vector3::SmoothStep(Vector3 const& v1, Vector3 const& v2, float t, Vector3& result) noexcept
{
    using namespace DirectX;
    t                 = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t); // Clamp value to 0 to 1
    t                 = t * t * (3.f - 2.f * t);
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X  = XMVectorLerp(x1, x2, t);
    XMStoreFloat3(&result, X);
}

inline Vector3 Vector3::SmoothStep(Vector3 const& v1, Vector3 const& v2, float t) noexcept
{
    using namespace DirectX;
    t                 = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t); // Clamp value to 0 to 1
    t                 = t * t * (3.f - 2.f * t);
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X  = XMVectorLerp(x1, x2, t);

    Vector3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Vector3::Barycentric(Vector3 const& v1, Vector3 const& v2, Vector3 const& v3, float f, float g, Vector3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR x3 = XMLoadFloat3(&v3);
    const XMVECTOR X  = XMVectorBaryCentric(x1, x2, x3, f, g);
    XMStoreFloat3(&result, X);
}

inline Vector3 Vector3::Barycentric(Vector3 const& v1, Vector3 const& v2, Vector3 const& v3, float f, float g) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR x3 = XMLoadFloat3(&v3);
    const XMVECTOR X  = XMVectorBaryCentric(x1, x2, x3, f, g);

    Vector3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void
Vector3::CatmullRom(Vector3 const& v1, Vector3 const& v2, Vector3 const& v3, Vector3 const& v4, float t, Vector3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR x3 = XMLoadFloat3(&v3);
    const XMVECTOR x4 = XMLoadFloat3(&v4);
    const XMVECTOR X  = XMVectorCatmullRom(x1, x2, x3, x4, t);
    XMStoreFloat3(&result, X);
}

inline Vector3 Vector3::CatmullRom(Vector3 const& v1, Vector3 const& v2, Vector3 const& v3, Vector3 const& v4, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR x3 = XMLoadFloat3(&v3);
    const XMVECTOR x4 = XMLoadFloat3(&v4);
    const XMVECTOR X  = XMVectorCatmullRom(x1, x2, x3, x4, t);

    Vector3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Vector3::Hermite(Vector3 const& v1, Vector3 const& t1, Vector3 const& v2, Vector3 const& t2, float t, Vector3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&t1);
    const XMVECTOR x3 = XMLoadFloat3(&v2);
    const XMVECTOR x4 = XMLoadFloat3(&t2);
    const XMVECTOR X  = XMVectorHermite(x1, x2, x3, x4, t);
    XMStoreFloat3(&result, X);
}

inline Vector3 Vector3::Hermite(Vector3 const& v1, Vector3 const& t1, Vector3 const& v2, Vector3 const& t2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&t1);
    const XMVECTOR x3 = XMLoadFloat3(&v2);
    const XMVECTOR x4 = XMLoadFloat3(&t2);
    const XMVECTOR X  = XMVectorHermite(x1, x2, x3, x4, t);

    Vector3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Vector3::Reflect(Vector3 const& ivec, Vector3 const& nvec, Vector3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat3(&ivec);
    const XMVECTOR n = XMLoadFloat3(&nvec);
    const XMVECTOR X = XMVector3Reflect(i, n);
    XMStoreFloat3(&result, X);
}

inline Vector3 Vector3::Reflect(Vector3 const& ivec, Vector3 const& nvec) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat3(&ivec);
    const XMVECTOR n = XMLoadFloat3(&nvec);
    const XMVECTOR X = XMVector3Reflect(i, n);

    Vector3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Vector3::Refract(Vector3 const& ivec, Vector3 const& nvec, float refractionIndex, Vector3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat3(&ivec);
    const XMVECTOR n = XMLoadFloat3(&nvec);
    const XMVECTOR X = XMVector3Refract(i, n, refractionIndex);
    XMStoreFloat3(&result, X);
}

inline Vector3 Vector3::Refract(Vector3 const& ivec, Vector3 const& nvec, float refractionIndex) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat3(&ivec);
    const XMVECTOR n = XMLoadFloat3(&nvec);
    const XMVECTOR X = XMVector3Refract(i, n, refractionIndex);

    Vector3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Vector3::Transform(Vector3 const& v, Quaternion const& quat, Vector3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMVECTOR q  = XMLoadFloat4(&quat);
    const XMVECTOR X  = XMVector3Rotate(v1, q);
    XMStoreFloat3(&result, X);
}

inline Vector3 Vector3::Transform(Vector3 const& v, Quaternion const& quat) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMVECTOR q  = XMLoadFloat4(&quat);
    const XMVECTOR X  = XMVector3Rotate(v1, q);

    Vector3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Vector3::Transform(Vector3 const& v, Matrix const& m, Vector3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMMATRIX M  = XMLoadFloat4x4(&m);
    const XMVECTOR X  = XMVector3TransformCoord(v1, M);
    XMStoreFloat3(&result, X);
}

inline Vector3 Vector3::Transform(Vector3 const& v, Matrix const& m) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMMATRIX M  = XMLoadFloat4x4(&m);
    const XMVECTOR X  = XMVector3TransformCoord(v1, M);

    Vector3 result;
    XMStoreFloat3(&result, X);
    return result;
}

_Use_decl_annotations_ inline void Vector3::Transform(Vector3 const* varray, size_t count, Matrix const& m, Vector3* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector3TransformCoordStream(resultArray, sizeof(XMFLOAT3), varray, sizeof(XMFLOAT3), count, M);
}

inline void Vector3::Transform(Vector3 const& v, Matrix const& m, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMMATRIX M  = XMLoadFloat4x4(&m);
    const XMVECTOR X  = XMVector3Transform(v1, M);
    XMStoreFloat4(&result, X);
}

_Use_decl_annotations_ inline void Vector3::Transform(Vector3 const* varray, size_t count, Matrix const& m, Vector4* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector3TransformStream(resultArray, sizeof(XMFLOAT4), varray, sizeof(XMFLOAT3), count, M);
}

inline void Vector3::TransformNormal(Vector3 const& v, Matrix const& m, Vector3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMMATRIX M  = XMLoadFloat4x4(&m);
    const XMVECTOR X  = XMVector3TransformNormal(v1, M);
    XMStoreFloat3(&result, X);
}

inline Vector3 Vector3::TransformNormal(Vector3 const& v, Matrix const& m) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMMATRIX M  = XMLoadFloat4x4(&m);
    const XMVECTOR X  = XMVector3TransformNormal(v1, M);

    Vector3 result;
    XMStoreFloat3(&result, X);
    return result;
}

_Use_decl_annotations_ inline void
Vector3::TransformNormal(Vector3 const* varray, size_t count, Matrix const& m, Vector3* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector3TransformNormalStream(resultArray, sizeof(XMFLOAT3), varray, sizeof(XMFLOAT3), count, M);
}

/****************************************************************************
 *
 * Vector4
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------

inline bool Vector4::operator==(Vector4 const& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&V);
    return XMVector4Equal(v1, v2);
}

inline bool Vector4::operator!=(Vector4 const& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&V);
    return XMVector4NotEqual(v1, v2);
}

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------

inline Vector4& Vector4::operator+=(Vector4 const& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&V);
    const XMVECTOR X  = XMVectorAdd(v1, v2);
    XMStoreFloat4(this, X);
    return *this;
}

inline Vector4& Vector4::operator-=(Vector4 const& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&V);
    const XMVECTOR X  = XMVectorSubtract(v1, v2);
    XMStoreFloat4(this, X);
    return *this;
}

inline Vector4& Vector4::operator*=(Vector4 const& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&V);
    const XMVECTOR X  = XMVectorMultiply(v1, v2);
    XMStoreFloat4(this, X);
    return *this;
}

inline Vector4& Vector4::operator*=(float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR v = XMLoadFloat4(this);
    const XMVECTOR X = XMVectorScale(v, S);
    XMStoreFloat4(this, X);
    return *this;
}

inline Vector4& Vector4::operator/=(float S) noexcept
{
    using namespace DirectX;
    assert(S != 0.0f);
    const XMVECTOR v = XMLoadFloat4(this);
    const XMVECTOR X = XMVectorScale(v, 1.f / S);
    XMStoreFloat4(this, X);
    return *this;
}

//------------------------------------------------------------------------------
// Binary operators
//------------------------------------------------------------------------------

inline Vector4 operator+(Vector4 const& V1, Vector4 const& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&V1);
    const XMVECTOR v2 = XMLoadFloat4(&V2);
    const XMVECTOR X  = XMVectorAdd(v1, v2);
    Vector4        R;
    XMStoreFloat4(&R, X);
    return R;
}

inline Vector4 operator-(Vector4 const& V1, Vector4 const& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&V1);
    const XMVECTOR v2 = XMLoadFloat4(&V2);
    const XMVECTOR X  = XMVectorSubtract(v1, v2);
    Vector4        R;
    XMStoreFloat4(&R, X);
    return R;
}

inline Vector4 operator*(Vector4 const& V1, Vector4 const& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&V1);
    const XMVECTOR v2 = XMLoadFloat4(&V2);
    const XMVECTOR X  = XMVectorMultiply(v1, v2);
    Vector4        R;
    XMStoreFloat4(&R, X);
    return R;
}

inline Vector4 operator*(Vector4 const& V, float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR v = XMLoadFloat4(&V);
    const XMVECTOR X = XMVectorScale(v, S);
    Vector4        R;
    XMStoreFloat4(&R, X);
    return R;
}

inline Vector4 operator*(float S, Vector4 const& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v = XMLoadFloat4(&V);
    const XMVECTOR X = XMVectorScale(v, S);
    Vector4        R;
    XMStoreFloat4(&R, X);
    return R;
}

inline Vector4 operator/(Vector4 const& V1, Vector4 const& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&V1);
    const XMVECTOR v2 = XMLoadFloat4(&V2);
    const XMVECTOR X  = XMVectorDivide(v1, v2);
    Vector4        R;
    XMStoreFloat4(&R, X);
    return R;
}

inline Vector4 operator/(Vector4 const& V, float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR v = XMLoadFloat4(&V);
    const XMVECTOR X = XMVectorScale(v, 1.f / S);
    Vector4        R;
    XMStoreFloat4(&R, X);
    return R;
}

inline Vector4 operator/(float S, Vector4 const& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMVectorReplicate(S);
    const XMVECTOR v2 = XMLoadFloat4(&V);
    const XMVECTOR X  = XMVectorDivide(v1, v2);
    Vector4        R;
    XMStoreFloat4(&R, X);
    return R;
}

//------------------------------------------------------------------------------
// Vector operations
//------------------------------------------------------------------------------

inline bool Vector4::InBounds(Vector4 const& Bounds) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&Bounds);
    return XMVector4InBounds(v1, v2);
}

inline float Vector4::Length() const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR X  = XMVector4Length(v1);
    return XMVectorGetX(X);
}

inline float Vector4::LengthSquared() const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR X  = XMVector4LengthSq(v1);
    return XMVectorGetX(X);
}

inline float Vector4::Dot(Vector4 const& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&V);
    const XMVECTOR X  = XMVector4Dot(v1, v2);
    return XMVectorGetX(X);
}

inline void Vector4::Cross(Vector4 const& v1, Vector4 const& v2, Vector4& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(this);
    const XMVECTOR x2 = XMLoadFloat4(&v1);
    const XMVECTOR x3 = XMLoadFloat4(&v2);
    const XMVECTOR R  = XMVector4Cross(x1, x2, x3);
    XMStoreFloat4(&result, R);
}

inline Vector4 Vector4::Cross(Vector4 const& v1, Vector4 const& v2) const noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(this);
    const XMVECTOR x2 = XMLoadFloat4(&v1);
    const XMVECTOR x3 = XMLoadFloat4(&v2);
    const XMVECTOR R  = XMVector4Cross(x1, x2, x3);

    Vector4 result;
    XMStoreFloat4(&result, R);
    return result;
}

inline void Vector4::Normalize() noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR X  = XMVector4Normalize(v1);
    XMStoreFloat4(this, X);
}

inline void Vector4::Normalize(Vector4& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR X  = XMVector4Normalize(v1);
    XMStoreFloat4(&result, X);
}

inline void Vector4::Clamp(Vector4 const& vmin, Vector4 const& vmax) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&vmin);
    const XMVECTOR v3 = XMLoadFloat4(&vmax);
    const XMVECTOR X  = XMVectorClamp(v1, v2, v3);
    XMStoreFloat4(this, X);
}

inline void Vector4::Clamp(Vector4 const& vmin, Vector4 const& vmax, Vector4& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&vmin);
    const XMVECTOR v3 = XMLoadFloat4(&vmax);
    const XMVECTOR X  = XMVectorClamp(v1, v2, v3);
    XMStoreFloat4(&result, X);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline float Vector4::Distance(Vector4 const& v1, Vector4 const& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR V  = XMVectorSubtract(x2, x1);
    const XMVECTOR X  = XMVector4Length(V);
    return XMVectorGetX(X);
}

inline float Vector4::DistanceSquared(Vector4 const& v1, Vector4 const& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR V  = XMVectorSubtract(x2, x1);
    const XMVECTOR X  = XMVector4LengthSq(V);
    return XMVectorGetX(X);
}

inline void Vector4::Min(Vector4 const& v1, Vector4 const& v2, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X  = XMVectorMin(x1, x2);
    XMStoreFloat4(&result, X);
}

inline Vector4 Vector4::Min(Vector4 const& v1, Vector4 const& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X  = XMVectorMin(x1, x2);

    Vector4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Vector4::Max(Vector4 const& v1, Vector4 const& v2, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X  = XMVectorMax(x1, x2);
    XMStoreFloat4(&result, X);
}

inline Vector4 Vector4::Max(Vector4 const& v1, Vector4 const& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X  = XMVectorMax(x1, x2);

    Vector4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Vector4::Lerp(Vector4 const& v1, Vector4 const& v2, float t, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X  = XMVectorLerp(x1, x2, t);
    XMStoreFloat4(&result, X);
}

inline Vector4 Vector4::Lerp(Vector4 const& v1, Vector4 const& v2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X  = XMVectorLerp(x1, x2, t);

    Vector4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Vector4::SmoothStep(Vector4 const& v1, Vector4 const& v2, float t, Vector4& result) noexcept
{
    using namespace DirectX;
    t                 = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t); // Clamp value to 0 to 1
    t                 = t * t * (3.f - 2.f * t);
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X  = XMVectorLerp(x1, x2, t);
    XMStoreFloat4(&result, X);
}

inline Vector4 Vector4::SmoothStep(Vector4 const& v1, Vector4 const& v2, float t) noexcept
{
    using namespace DirectX;
    t                 = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t); // Clamp value to 0 to 1
    t                 = t * t * (3.f - 2.f * t);
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X  = XMVectorLerp(x1, x2, t);

    Vector4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Vector4::Barycentric(Vector4 const& v1, Vector4 const& v2, Vector4 const& v3, float f, float g, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR x3 = XMLoadFloat4(&v3);
    const XMVECTOR X  = XMVectorBaryCentric(x1, x2, x3, f, g);
    XMStoreFloat4(&result, X);
}

inline Vector4 Vector4::Barycentric(Vector4 const& v1, Vector4 const& v2, Vector4 const& v3, float f, float g) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR x3 = XMLoadFloat4(&v3);
    const XMVECTOR X  = XMVectorBaryCentric(x1, x2, x3, f, g);

    Vector4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void
Vector4::CatmullRom(Vector4 const& v1, Vector4 const& v2, Vector4 const& v3, Vector4 const& v4, float t, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR x3 = XMLoadFloat4(&v3);
    const XMVECTOR x4 = XMLoadFloat4(&v4);
    const XMVECTOR X  = XMVectorCatmullRom(x1, x2, x3, x4, t);
    XMStoreFloat4(&result, X);
}

inline Vector4 Vector4::CatmullRom(Vector4 const& v1, Vector4 const& v2, Vector4 const& v3, Vector4 const& v4, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR x3 = XMLoadFloat4(&v3);
    const XMVECTOR x4 = XMLoadFloat4(&v4);
    const XMVECTOR X  = XMVectorCatmullRom(x1, x2, x3, x4, t);

    Vector4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Vector4::Hermite(Vector4 const& v1, Vector4 const& t1, Vector4 const& v2, Vector4 const& t2, float t, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&t1);
    const XMVECTOR x3 = XMLoadFloat4(&v2);
    const XMVECTOR x4 = XMLoadFloat4(&t2);
    const XMVECTOR X  = XMVectorHermite(x1, x2, x3, x4, t);
    XMStoreFloat4(&result, X);
}

inline Vector4 Vector4::Hermite(Vector4 const& v1, Vector4 const& t1, Vector4 const& v2, Vector4 const& t2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&t1);
    const XMVECTOR x3 = XMLoadFloat4(&v2);
    const XMVECTOR x4 = XMLoadFloat4(&t2);
    const XMVECTOR X  = XMVectorHermite(x1, x2, x3, x4, t);

    Vector4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Vector4::Reflect(Vector4 const& ivec, Vector4 const& nvec, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat4(&ivec);
    const XMVECTOR n = XMLoadFloat4(&nvec);
    const XMVECTOR X = XMVector4Reflect(i, n);
    XMStoreFloat4(&result, X);
}

inline Vector4 Vector4::Reflect(Vector4 const& ivec, Vector4 const& nvec) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat4(&ivec);
    const XMVECTOR n = XMLoadFloat4(&nvec);
    const XMVECTOR X = XMVector4Reflect(i, n);

    Vector4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Vector4::Refract(Vector4 const& ivec, Vector4 const& nvec, float refractionIndex, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat4(&ivec);
    const XMVECTOR n = XMLoadFloat4(&nvec);
    const XMVECTOR X = XMVector4Refract(i, n, refractionIndex);
    XMStoreFloat4(&result, X);
}

inline Vector4 Vector4::Refract(Vector4 const& ivec, Vector4 const& nvec, float refractionIndex) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat4(&ivec);
    const XMVECTOR n = XMLoadFloat4(&nvec);
    const XMVECTOR X = XMVector4Refract(i, n, refractionIndex);

    Vector4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Vector4::Transform(Vector2 const& v, Quaternion const& quat, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMVECTOR q  = XMLoadFloat4(&quat);
    XMVECTOR       X  = XMVector3Rotate(v1, q);
    X                 = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f
    XMStoreFloat4(&result, X);
}

inline Vector4 Vector4::Transform(Vector2 const& v, Quaternion const& quat) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMVECTOR q  = XMLoadFloat4(&quat);
    XMVECTOR       X  = XMVector3Rotate(v1, q);
    X                 = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f

    Vector4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Vector4::Transform(Vector3 const& v, Quaternion const& quat, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMVECTOR q  = XMLoadFloat4(&quat);
    XMVECTOR       X  = XMVector3Rotate(v1, q);
    X                 = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f
    XMStoreFloat4(&result, X);
}

inline Vector4 Vector4::Transform(Vector3 const& v, Quaternion const& quat) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMVECTOR q  = XMLoadFloat4(&quat);
    XMVECTOR       X  = XMVector3Rotate(v1, q);
    X                 = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f

    Vector4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Vector4::Transform(Vector4 const& v, Quaternion const& quat, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&v);
    const XMVECTOR q  = XMLoadFloat4(&quat);
    XMVECTOR       X  = XMVector3Rotate(v1, q);
    X                 = XMVectorSelect(v1, X, g_XMSelect1110); // result.w = v.w
    XMStoreFloat4(&result, X);
}

inline Vector4 Vector4::Transform(Vector4 const& v, Quaternion const& quat) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&v);
    const XMVECTOR q  = XMLoadFloat4(&quat);
    XMVECTOR       X  = XMVector3Rotate(v1, q);
    X                 = XMVectorSelect(v1, X, g_XMSelect1110); // result.w = v.w

    Vector4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Vector4::Transform(Vector4 const& v, Matrix const& m, Vector4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&v);
    const XMMATRIX M  = XMLoadFloat4x4(&m);
    const XMVECTOR X  = XMVector4Transform(v1, M);
    XMStoreFloat4(&result, X);
}

inline Vector4 Vector4::Transform(Vector4 const& v, Matrix const& m) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&v);
    const XMMATRIX M  = XMLoadFloat4x4(&m);
    const XMVECTOR X  = XMVector4Transform(v1, M);

    Vector4 result;
    XMStoreFloat4(&result, X);
    return result;
}

_Use_decl_annotations_ inline void Vector4::Transform(Vector4 const* varray, size_t count, Matrix const& m, Vector4* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector4TransformStream(resultArray, sizeof(XMFLOAT4), varray, sizeof(XMFLOAT4), count, M);
}

/****************************************************************************
 *
 * Matrix
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------

inline bool Matrix::operator==(Matrix const& M) const noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    const XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    const XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    const XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    return (XMVector4Equal(x1, y1) && XMVector4Equal(x2, y2) && XMVector4Equal(x3, y3) && XMVector4Equal(x4, y4)) != 0;
}

inline bool Matrix::operator!=(Matrix const& M) const noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    const XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    const XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    const XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    return (XMVector4NotEqual(x1, y1) || XMVector4NotEqual(x2, y2) || XMVector4NotEqual(x3, y3) || XMVector4NotEqual(x4, y4)) != 0;
}

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------

inline Matrix::Matrix(const XMFLOAT3X3& M) noexcept
{
    _11 = M._11;
    _12 = M._12;
    _13 = M._13;
    _14 = 0.f;
    _21 = M._21;
    _22 = M._22;
    _23 = M._23;
    _24 = 0.f;
    _31 = M._31;
    _32 = M._32;
    _33 = M._33;
    _34 = 0.f;
    _41 = 0.f;
    _42 = 0.f;
    _43 = 0.f;
    _44 = 1.f;
}

inline Matrix::Matrix(const XMFLOAT4X3& M) noexcept
{
    _11 = M._11;
    _12 = M._12;
    _13 = M._13;
    _14 = 0.f;
    _21 = M._21;
    _22 = M._22;
    _23 = M._23;
    _24 = 0.f;
    _31 = M._31;
    _32 = M._32;
    _33 = M._33;
    _34 = 0.f;
    _41 = M._41;
    _42 = M._42;
    _43 = M._43;
    _44 = 1.f;
}

inline Matrix& Matrix::operator=(const XMFLOAT3X3& M) noexcept
{
    _11 = M._11;
    _12 = M._12;
    _13 = M._13;
    _14 = 0.f;
    _21 = M._21;
    _22 = M._22;
    _23 = M._23;
    _24 = 0.f;
    _31 = M._31;
    _32 = M._32;
    _33 = M._33;
    _34 = 0.f;
    _41 = 0.f;
    _42 = 0.f;
    _43 = 0.f;
    _44 = 1.f;
    return *this;
}

inline Matrix& Matrix::operator=(const XMFLOAT4X3& M) noexcept
{
    _11 = M._11;
    _12 = M._12;
    _13 = M._13;
    _14 = 0.f;
    _21 = M._21;
    _22 = M._22;
    _23 = M._23;
    _24 = 0.f;
    _31 = M._31;
    _32 = M._32;
    _33 = M._33;
    _34 = 0.f;
    _41 = M._41;
    _42 = M._42;
    _43 = M._43;
    _44 = 1.f;
    return *this;
}

inline Matrix& Matrix::operator+=(Matrix const& M) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    x1 = XMVectorAdd(x1, y1);
    x2 = XMVectorAdd(x2, y2);
    x3 = XMVectorAdd(x3, y3);
    x4 = XMVectorAdd(x4, y4);

    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
    return *this;
}

inline Matrix& Matrix::operator-=(Matrix const& M) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    x1 = XMVectorSubtract(x1, y1);
    x2 = XMVectorSubtract(x2, y2);
    x3 = XMVectorSubtract(x3, y3);
    x4 = XMVectorSubtract(x4, y4);

    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
    return *this;
}

inline Matrix& Matrix::operator*=(Matrix const& M) noexcept
{
    using namespace DirectX;
    const XMMATRIX M1 = XMLoadFloat4x4(this);
    const XMMATRIX M2 = XMLoadFloat4x4(&M);
    const XMMATRIX X  = XMMatrixMultiply(M1, M2);
    XMStoreFloat4x4(this, X);
    return *this;
}

inline Matrix& Matrix::operator*=(float S) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    x1 = XMVectorScale(x1, S);
    x2 = XMVectorScale(x2, S);
    x3 = XMVectorScale(x3, S);
    x4 = XMVectorScale(x4, S);

    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
    return *this;
}

inline Matrix& Matrix::operator/=(float S) noexcept
{
    using namespace DirectX;
    assert(S != 0.f);
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    float const rs = 1.f / S;

    x1 = XMVectorScale(x1, rs);
    x2 = XMVectorScale(x2, rs);
    x3 = XMVectorScale(x3, rs);
    x4 = XMVectorScale(x4, rs);

    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
    return *this;
}

inline Matrix& Matrix::operator/=(Matrix const& M) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    x1 = XMVectorDivide(x1, y1);
    x2 = XMVectorDivide(x2, y2);
    x3 = XMVectorDivide(x3, y3);
    x4 = XMVectorDivide(x4, y4);

    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
    return *this;
}

//------------------------------------------------------------------------------
// Urnary operators
//------------------------------------------------------------------------------

inline Matrix Matrix::operator-() const noexcept
{
    using namespace DirectX;
    XMVECTOR v1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    XMVECTOR v2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    XMVECTOR v3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    XMVECTOR v4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    v1 = XMVectorNegate(v1);
    v2 = XMVectorNegate(v2);
    v3 = XMVectorNegate(v3);
    v4 = XMVectorNegate(v4);

    Matrix R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), v1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), v2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), v3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), v4);
    return R;
}

//------------------------------------------------------------------------------
// Binary operators
//------------------------------------------------------------------------------

inline Matrix operator+(Matrix const& M1, Matrix const& M2) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._41));

    x1 = XMVectorAdd(x1, y1);
    x2 = XMVectorAdd(x2, y2);
    x3 = XMVectorAdd(x3, y3);
    x4 = XMVectorAdd(x4, y4);

    Matrix R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
    return R;
}

inline Matrix operator-(Matrix const& M1, Matrix const& M2) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._41));

    x1 = XMVectorSubtract(x1, y1);
    x2 = XMVectorSubtract(x2, y2);
    x3 = XMVectorSubtract(x3, y3);
    x4 = XMVectorSubtract(x4, y4);

    Matrix R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
    return R;
}

inline Matrix operator*(Matrix const& M1, Matrix const& M2) noexcept
{
    using namespace DirectX;
    const XMMATRIX m1 = XMLoadFloat4x4(&M1);
    const XMMATRIX m2 = XMLoadFloat4x4(&M2);
    const XMMATRIX X  = XMMatrixMultiply(m1, m2);

    Matrix R;
    XMStoreFloat4x4(&R, X);
    return R;
}

inline Matrix operator*(Matrix const& M, float S) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    x1 = XMVectorScale(x1, S);
    x2 = XMVectorScale(x2, S);
    x3 = XMVectorScale(x3, S);
    x4 = XMVectorScale(x4, S);

    Matrix R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
    return R;
}

inline Matrix operator*(float S, Matrix const& M) noexcept
{
    using namespace DirectX;

    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    x1 = XMVectorScale(x1, S);
    x2 = XMVectorScale(x2, S);
    x3 = XMVectorScale(x3, S);
    x4 = XMVectorScale(x4, S);

    Matrix R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
    return R;
}

inline Matrix operator/(Matrix const& M, float S) noexcept
{
    using namespace DirectX;
    assert(S != 0.f);

    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    float const rs = 1.f / S;

    x1 = XMVectorScale(x1, rs);
    x2 = XMVectorScale(x2, rs);
    x3 = XMVectorScale(x3, rs);
    x4 = XMVectorScale(x4, rs);

    Matrix R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
    return R;
}

inline Matrix operator/(Matrix const& M1, Matrix const& M2) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._41));

    x1 = XMVectorDivide(x1, y1);
    x2 = XMVectorDivide(x2, y2);
    x3 = XMVectorDivide(x3, y3);
    x4 = XMVectorDivide(x4, y4);

    Matrix R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
    return R;
}

inline Matrix operator/(float S, Matrix const& M) noexcept
{
    using namespace DirectX;

    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    XMVECTOR vs = XMVectorReplicate(S);

    x1 = XMVectorDivide(vs, x1);
    x2 = XMVectorDivide(vs, x2);
    x3 = XMVectorDivide(vs, x3);
    x4 = XMVectorDivide(vs, x4);

    Matrix R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
    return R;
}

//------------------------------------------------------------------------------
// Matrix operations
//------------------------------------------------------------------------------

inline bool Matrix::Decompose(Vector3& scale, Quaternion& rotation, Vector3& translation) noexcept
{
    using namespace DirectX;

    XMVECTOR s, r, t;

    if (!XMMatrixDecompose(&s, &r, &t, *this))
        return false;

    XMStoreFloat3(&scale, s);
    XMStoreFloat4(&rotation, r);
    XMStoreFloat3(&translation, t);

    return true;
}

inline Matrix Matrix::Transpose() const noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(this);
    Matrix         R;
    XMStoreFloat4x4(&R, XMMatrixTranspose(M));
    return R;
}

inline void Matrix::Transpose(Matrix& result) const noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(this);
    XMStoreFloat4x4(&result, XMMatrixTranspose(M));
}

inline Matrix Matrix::Invert() const noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(this);
    Matrix         R;
    XMVECTOR       det;
    XMStoreFloat4x4(&R, XMMatrixInverse(&det, M));
    return R;
}

inline void Matrix::Invert(Matrix& result) const noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(this);
    XMVECTOR       det;
    XMStoreFloat4x4(&result, XMMatrixInverse(&det, M));
}

inline float Matrix::Determinant() const noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(this);
    return XMVectorGetX(XMMatrixDeterminant(M));
}

inline Vector3 Matrix::ToEuler() const noexcept
{
    float const cy = sqrtf(_33 * _33 + _31 * _31);
    float const cx = atan2f(-_32, cy);
    if (cy > 16.f * FLT_EPSILON)
    {
        return Vector3(cx, atan2f(_31, _33), atan2f(_12, _22));
    }
    else
    {
        return Vector3(cx, 0.f, atan2f(-_21, _11));
    }
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

_Use_decl_annotations_ inline Matrix Matrix::CreateBillboard(Vector3 const& object,
    Vector3 const&                                                          cameraPosition,
    Vector3 const&                                                          cameraUp,
    Vector3 const*                                                          cameraForward) noexcept
{
    using namespace DirectX;
    const XMVECTOR O = XMLoadFloat3(&object);
    const XMVECTOR C = XMLoadFloat3(&cameraPosition);
    XMVECTOR       Z = XMVectorSubtract(O, C);

    const XMVECTOR N = XMVector3LengthSq(Z);
    if (XMVector3Less(N, g_XMEpsilon))
    {
        if (cameraForward)
        {
            const XMVECTOR F = XMLoadFloat3(cameraForward);
            Z                = XMVectorNegate(F);
        }
        else
            Z = g_XMNegIdentityR2;
    }
    else
    {
        Z = XMVector3Normalize(Z);
    }

    const XMVECTOR up = XMLoadFloat3(&cameraUp);
    XMVECTOR       X  = XMVector3Cross(up, Z);
    X                 = XMVector3Normalize(X);

    const XMVECTOR Y = XMVector3Cross(Z, X);

    XMMATRIX M;
    M.r[0] = X;
    M.r[1] = Y;
    M.r[2] = Z;
    M.r[3] = XMVectorSetW(O, 1.f);

    Matrix R;
    XMStoreFloat4x4(&R, M);
    return R;
}

_Use_decl_annotations_ inline Matrix Matrix::CreateConstrainedBillboard(Vector3 const& object,
    Vector3 const&                                                                     cameraPosition,
    Vector3 const&                                                                     rotateAxis,
    Vector3 const*                                                                     cameraForward,
    Vector3 const*                                                                     objectForward) noexcept
{
    using namespace DirectX;

    static const XMVECTORF32 s_minAngle
        = { { { 0.99825467075f, 0.99825467075f, 0.99825467075f, 0.99825467075f } } }; // 1.0 - XMConvertToRadians( 0.1f );

    const XMVECTOR O       = XMLoadFloat3(&object);
    const XMVECTOR C       = XMLoadFloat3(&cameraPosition);
    XMVECTOR       faceDir = XMVectorSubtract(O, C);

    const XMVECTOR N = XMVector3LengthSq(faceDir);
    if (XMVector3Less(N, g_XMEpsilon))
    {
        if (cameraForward)
        {
            const XMVECTOR F = XMLoadFloat3(cameraForward);
            faceDir          = XMVectorNegate(F);
        }
        else
            faceDir = g_XMNegIdentityR2;
    }
    else
    {
        faceDir = XMVector3Normalize(faceDir);
    }

    const XMVECTOR Y = XMLoadFloat3(&rotateAxis);
    XMVECTOR       X, Z;

    XMVECTOR dot = XMVectorAbs(XMVector3Dot(Y, faceDir));
    if (XMVector3Greater(dot, s_minAngle))
    {
        if (objectForward)
        {
            Z   = XMLoadFloat3(objectForward);
            dot = XMVectorAbs(XMVector3Dot(Y, Z));
            if (XMVector3Greater(dot, s_minAngle))
            {
                dot = XMVectorAbs(XMVector3Dot(Y, g_XMNegIdentityR2));
                Z   = (XMVector3Greater(dot, s_minAngle)) ? g_XMIdentityR0 : g_XMNegIdentityR2;
            }
        }
        else
        {
            dot = XMVectorAbs(XMVector3Dot(Y, g_XMNegIdentityR2));
            Z   = (XMVector3Greater(dot, s_minAngle)) ? g_XMIdentityR0 : g_XMNegIdentityR2;
        }

        X = XMVector3Cross(Y, Z);
        X = XMVector3Normalize(X);

        Z = XMVector3Cross(X, Y);
        Z = XMVector3Normalize(Z);
    }
    else
    {
        X = XMVector3Cross(Y, faceDir);
        X = XMVector3Normalize(X);

        Z = XMVector3Cross(X, Y);
        Z = XMVector3Normalize(Z);
    }

    XMMATRIX M;
    M.r[0] = X;
    M.r[1] = Y;
    M.r[2] = Z;
    M.r[3] = XMVectorSetW(O, 1.f);

    Matrix R;
    XMStoreFloat4x4(&R, M);
    return R;
}

inline Matrix Matrix::CreateTranslation(Vector3 const& position) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixTranslation(position.x, position.y, position.z));
    return R;
}

inline Matrix Matrix::CreateTranslation(float x, float y, float z) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixTranslation(x, y, z));
    return R;
}

inline Matrix Matrix::CreateScale(Vector3 const& scales) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixScaling(scales.x, scales.y, scales.z));
    return R;
}

inline Matrix Matrix::CreateScale(float xs, float ys, float zs) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixScaling(xs, ys, zs));
    return R;
}

inline Matrix Matrix::CreateScale(float scale) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixScaling(scale, scale, scale));
    return R;
}

inline Matrix Matrix::CreateRotationX(float radians) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixRotationX(radians));
    return R;
}

inline Matrix Matrix::CreateRotationY(float radians) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixRotationY(radians));
    return R;
}

inline Matrix Matrix::CreateRotationZ(float radians) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixRotationZ(radians));
    return R;
}

inline Matrix Matrix::CreateFromAxisAngle(Vector3 const& axis, float angle) noexcept
{
    using namespace DirectX;
    Matrix         R;
    const XMVECTOR a = XMLoadFloat3(&axis);
    XMStoreFloat4x4(&R, XMMatrixRotationAxis(a, angle));
    return R;
}

inline Matrix Matrix::CreatePerspectiveFieldOfView(float fov, float aspectRatio, float nearPlane, float farPlane) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixPerspectiveFovRH(fov, aspectRatio, nearPlane, farPlane));
    return R;
}

inline Matrix Matrix::CreatePerspective(float width, float height, float nearPlane, float farPlane) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixPerspectiveRH(width, height, nearPlane, farPlane));
    return R;
}

inline Matrix Matrix::CreatePerspectiveOffCenter(float left, float right, float bottom, float top, float nearPlane, float farPlane) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixPerspectiveOffCenterRH(left, right, bottom, top, nearPlane, farPlane));
    return R;
}

inline Matrix Matrix::CreateOrthographic(float width, float height, float zNearPlane, float zFarPlane) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixOrthographicRH(width, height, zNearPlane, zFarPlane));
    return R;
}

inline Matrix
Matrix::CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixOrthographicOffCenterRH(left, right, bottom, top, zNearPlane, zFarPlane));
    return R;
}

inline Matrix Matrix::CreateLookAt(Vector3 const& eye, Vector3 const& target, Vector3 const& up) noexcept
{
    using namespace DirectX;
    Matrix         R;
    const XMVECTOR eyev    = XMLoadFloat3(&eye);
    const XMVECTOR targetv = XMLoadFloat3(&target);
    const XMVECTOR upv     = XMLoadFloat3(&up);
    XMStoreFloat4x4(&R, XMMatrixLookAtRH(eyev, targetv, upv));
    return R;
}

inline Matrix Matrix::CreateWorld(Vector3 const& position, Vector3 const& forward, Vector3 const& up) noexcept
{
    using namespace DirectX;
    const XMVECTOR zaxis = XMVector3Normalize(XMVectorNegate(XMLoadFloat3(&forward)));
    XMVECTOR       yaxis = XMLoadFloat3(&up);
    const XMVECTOR xaxis = XMVector3Normalize(XMVector3Cross(yaxis, zaxis));
    yaxis                = XMVector3Cross(zaxis, xaxis);

    Matrix R;
    XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._11), xaxis);
    XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._21), yaxis);
    XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._31), zaxis);
    R._14 = R._24 = R._34 = 0.f;
    R._41                 = position.x;
    R._42                 = position.y;
    R._43                 = position.z;
    R._44                 = 1.f;
    return R;
}

inline Matrix Matrix::CreateFromQuaternion(Quaternion const& rotation) noexcept
{
    using namespace DirectX;
    const XMVECTOR quatv = XMLoadFloat4(&rotation);
    Matrix         R;
    XMStoreFloat4x4(&R, XMMatrixRotationQuaternion(quatv));
    return R;
}

inline Matrix Matrix::CreateFromYawPitchRoll(float yaw, float pitch, float roll) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixRotationRollPitchYaw(pitch, yaw, roll));
    return R;
}

inline Matrix Matrix::CreateFromYawPitchRoll(Vector3 const& angles) noexcept
{
    using namespace DirectX;
    Matrix R;
    XMStoreFloat4x4(&R, XMMatrixRotationRollPitchYawFromVector(angles));
    return R;
}

inline Matrix Matrix::CreateShadow(Vector3 const& lightDir, Plane const& plane) noexcept
{
    using namespace DirectX;
    const XMVECTOR light  = XMLoadFloat3(&lightDir);
    const XMVECTOR planev = XMLoadFloat4(&plane);
    Matrix         R;
    XMStoreFloat4x4(&R, XMMatrixShadow(planev, light));
    return R;
}

inline Matrix Matrix::CreateReflection(Plane const& plane) noexcept
{
    using namespace DirectX;
    const XMVECTOR planev = XMLoadFloat4(&plane);
    Matrix         R;
    XMStoreFloat4x4(&R, XMMatrixReflect(planev));
    return R;
}

inline void Matrix::Lerp(Matrix const& M1, Matrix const& M2, float t, Matrix& result) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._41));

    x1 = XMVectorLerp(x1, y1, t);
    x2 = XMVectorLerp(x2, y2, t);
    x3 = XMVectorLerp(x3, y3, t);
    x4 = XMVectorLerp(x4, y4, t);

    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._41), x4);
}

inline Matrix Matrix::Lerp(Matrix const& M1, Matrix const& M2, float t) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._41));

    x1 = XMVectorLerp(x1, y1, t);
    x2 = XMVectorLerp(x2, y2, t);
    x3 = XMVectorLerp(x3, y3, t);
    x4 = XMVectorLerp(x4, y4, t);

    Matrix result;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._41), x4);
    return result;
}

inline void Matrix::Transform(Matrix const& M, Quaternion const& rotation, Matrix& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR quatv = XMLoadFloat4(&rotation);

    const XMMATRIX M0 = XMLoadFloat4x4(&M);
    const XMMATRIX M1 = XMMatrixRotationQuaternion(quatv);

    XMStoreFloat4x4(&result, XMMatrixMultiply(M0, M1));
}

inline Matrix Matrix::Transform(Matrix const& M, Quaternion const& rotation) noexcept
{
    using namespace DirectX;
    const XMVECTOR quatv = XMLoadFloat4(&rotation);

    const XMMATRIX M0 = XMLoadFloat4x4(&M);
    const XMMATRIX M1 = XMMatrixRotationQuaternion(quatv);

    Matrix result;
    XMStoreFloat4x4(&result, XMMatrixMultiply(M0, M1));
    return result;
}

/****************************************************************************
 *
 * Plane
 *
 ****************************************************************************/

inline Plane::Plane(Vector3 const& point1, Vector3 const& point2, Vector3 const& point3) noexcept
{
    using namespace DirectX;
    const XMVECTOR P0 = XMLoadFloat3(&point1);
    const XMVECTOR P1 = XMLoadFloat3(&point2);
    const XMVECTOR P2 = XMLoadFloat3(&point3);
    XMStoreFloat4(this, XMPlaneFromPoints(P0, P1, P2));
}

inline Plane::Plane(Vector3 const& point, Vector3 const& normal) noexcept
{
    using namespace DirectX;
    const XMVECTOR P = XMLoadFloat3(&point);
    const XMVECTOR N = XMLoadFloat3(&normal);
    XMStoreFloat4(this, XMPlaneFromPointNormal(P, N));
}

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------

inline bool Plane::operator==(Plane const& p) const noexcept
{
    using namespace DirectX;
    const XMVECTOR p1 = XMLoadFloat4(this);
    const XMVECTOR p2 = XMLoadFloat4(&p);
    return XMPlaneEqual(p1, p2);
}

inline bool Plane::operator!=(Plane const& p) const noexcept
{
    using namespace DirectX;
    const XMVECTOR p1 = XMLoadFloat4(this);
    const XMVECTOR p2 = XMLoadFloat4(&p);
    return XMPlaneNotEqual(p1, p2);
}

//------------------------------------------------------------------------------
// Plane operations
//------------------------------------------------------------------------------

inline void Plane::Normalize() noexcept
{
    using namespace DirectX;
    const XMVECTOR p = XMLoadFloat4(this);
    XMStoreFloat4(this, XMPlaneNormalize(p));
}

inline void Plane::Normalize(Plane& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR p = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMPlaneNormalize(p));
}

inline float Plane::Dot(Vector4 const& v) const noexcept
{
    using namespace DirectX;
    const XMVECTOR p  = XMLoadFloat4(this);
    const XMVECTOR v0 = XMLoadFloat4(&v);
    return XMVectorGetX(XMPlaneDot(p, v0));
}

inline float Plane::DotCoordinate(Vector3 const& position) const noexcept
{
    using namespace DirectX;
    const XMVECTOR p  = XMLoadFloat4(this);
    const XMVECTOR v0 = XMLoadFloat3(&position);
    return XMVectorGetX(XMPlaneDotCoord(p, v0));
}

inline float Plane::DotNormal(Vector3 const& normal) const noexcept
{
    using namespace DirectX;
    const XMVECTOR p  = XMLoadFloat4(this);
    const XMVECTOR n0 = XMLoadFloat3(&normal);
    return XMVectorGetX(XMPlaneDotNormal(p, n0));
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline void Plane::Transform(Plane const& plane, Matrix const& M, Plane& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR p  = XMLoadFloat4(&plane);
    const XMMATRIX m0 = XMLoadFloat4x4(&M);
    XMStoreFloat4(&result, XMPlaneTransform(p, m0));
}

inline Plane Plane::Transform(Plane const& plane, Matrix const& M) noexcept
{
    using namespace DirectX;
    const XMVECTOR p  = XMLoadFloat4(&plane);
    const XMMATRIX m0 = XMLoadFloat4x4(&M);

    Plane result;
    XMStoreFloat4(&result, XMPlaneTransform(p, m0));
    return result;
}

inline void Plane::Transform(Plane const& plane, Quaternion const& rotation, Plane& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR p = XMLoadFloat4(&plane);
    const XMVECTOR q = XMLoadFloat4(&rotation);
    XMVECTOR       X = XMVector3Rotate(p, q);
    X                = XMVectorSelect(p, X, g_XMSelect1110); // result.d = plane.d
    XMStoreFloat4(&result, X);
}

inline Plane Plane::Transform(Plane const& plane, Quaternion const& rotation) noexcept
{
    using namespace DirectX;
    const XMVECTOR p = XMLoadFloat4(&plane);
    const XMVECTOR q = XMLoadFloat4(&rotation);
    XMVECTOR       X = XMVector3Rotate(p, q);
    X                = XMVectorSelect(p, X, g_XMSelect1110); // result.d = plane.d

    Plane result;
    XMStoreFloat4(&result, X);
    return result;
}

/****************************************************************************
 *
 * Quaternion
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------

inline bool Quaternion::operator==(Quaternion const& q) const noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    const XMVECTOR q2 = XMLoadFloat4(&q);
    return XMQuaternionEqual(q1, q2);
}

inline bool Quaternion::operator!=(Quaternion const& q) const noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    const XMVECTOR q2 = XMLoadFloat4(&q);
    return XMQuaternionNotEqual(q1, q2);
}

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------

inline Quaternion& Quaternion::operator+=(Quaternion const& q) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    const XMVECTOR q2 = XMLoadFloat4(&q);
    XMStoreFloat4(this, XMVectorAdd(q1, q2));
    return *this;
}

inline Quaternion& Quaternion::operator-=(Quaternion const& q) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    const XMVECTOR q2 = XMLoadFloat4(&q);
    XMStoreFloat4(this, XMVectorSubtract(q1, q2));
    return *this;
}

inline Quaternion& Quaternion::operator*=(Quaternion const& q) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    const XMVECTOR q2 = XMLoadFloat4(&q);
    XMStoreFloat4(this, XMQuaternionMultiply(q1, q2));
    return *this;
}

inline Quaternion& Quaternion::operator*=(float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    XMStoreFloat4(this, XMVectorScale(q, S));
    return *this;
}

inline Quaternion& Quaternion::operator/=(Quaternion const& q) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    XMVECTOR       q2 = XMLoadFloat4(&q);
    q2                = XMQuaternionInverse(q2);
    XMStoreFloat4(this, XMQuaternionMultiply(q1, q2));
    return *this;
}

//------------------------------------------------------------------------------
// Binary operators
//------------------------------------------------------------------------------

inline Quaternion operator+(Quaternion const& Q1, Quaternion const& Q2) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(&Q1);
    const XMVECTOR q2 = XMLoadFloat4(&Q2);

    Quaternion R;
    XMStoreFloat4(&R, XMVectorAdd(q1, q2));
    return R;
}

inline Quaternion operator-(Quaternion const& Q1, Quaternion const& Q2) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(&Q1);
    const XMVECTOR q2 = XMLoadFloat4(&Q2);

    Quaternion R;
    XMStoreFloat4(&R, XMVectorSubtract(q1, q2));
    return R;
}

inline Quaternion operator*(Quaternion const& Q1, Quaternion const& Q2) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(&Q1);
    const XMVECTOR q2 = XMLoadFloat4(&Q2);

    Quaternion R;
    XMStoreFloat4(&R, XMQuaternionMultiply(q1, q2));
    return R;
}

inline Quaternion operator*(Quaternion const& Q, float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(&Q);

    Quaternion R;
    XMStoreFloat4(&R, XMVectorScale(q, S));
    return R;
}

inline Quaternion operator*(float S, Quaternion const& Q) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(&Q);

    Quaternion R;
    XMStoreFloat4(&R, XMVectorScale(q1, S));
    return R;
}

inline Quaternion operator/(Quaternion const& Q1, Quaternion const& Q2) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(&Q1);
    XMVECTOR       q2 = XMLoadFloat4(&Q2);
    q2                = XMQuaternionInverse(q2);

    Quaternion R;
    XMStoreFloat4(&R, XMQuaternionMultiply(q1, q2));
    return R;
}

//------------------------------------------------------------------------------
// Quaternion operations
//------------------------------------------------------------------------------

inline float Quaternion::Length() const noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    return XMVectorGetX(XMQuaternionLength(q));
}

inline float Quaternion::LengthSquared() const noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    return XMVectorGetX(XMQuaternionLengthSq(q));
}

inline void Quaternion::Normalize() noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    XMStoreFloat4(this, XMQuaternionNormalize(q));
}

inline void Quaternion::Normalize(Quaternion& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMQuaternionNormalize(q));
}

inline void Quaternion::Conjugate() noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    XMStoreFloat4(this, XMQuaternionConjugate(q));
}

inline void Quaternion::Conjugate(Quaternion& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMQuaternionConjugate(q));
}

inline void Quaternion::Inverse(Quaternion& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMQuaternionInverse(q));
}

inline float Quaternion::Dot(Quaternion const& q) const noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    const XMVECTOR q2 = XMLoadFloat4(&q);
    return XMVectorGetX(XMQuaternionDot(q1, q2));
}

inline void Quaternion::RotateTowards(Quaternion const& target, float maxAngle) noexcept
{
    RotateTowards(target, maxAngle, *this);
}

inline Vector3 Quaternion::ToEuler() const noexcept
{
    float const xx = x * x;
    float const yy = y * y;
    float const zz = z * z;

    float const m31 = 2.f * x * z + 2.f * y * w;
    float const m32 = 2.f * y * z - 2.f * x * w;
    float const m33 = 1.f - 2.f * xx - 2.f * yy;

    float const cy = sqrtf(m33 * m33 + m31 * m31);
    float const cx = atan2f(-m32, cy);
    if (cy > 16.f * FLT_EPSILON)
    {
        float const m12 = 2.f * x * y + 2.f * z * w;
        float const m22 = 1.f - 2.f * xx - 2.f * zz;

        return Vector3(cx, atan2f(m31, m33), atan2f(m12, m22));
    }
    else
    {
        float const m11 = 1.f - 2.f * yy - 2.f * zz;
        float const m21 = 2.f * x * y - 2.f * z * w;

        return Vector3(cx, 0.f, atan2f(-m21, m11));
    }
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline Quaternion Quaternion::CreateFromAxisAngle(Vector3 const& axis, float angle) noexcept
{
    using namespace DirectX;
    const XMVECTOR a = XMLoadFloat3(&axis);

    Quaternion R;
    XMStoreFloat4(&R, XMQuaternionRotationAxis(a, angle));
    return R;
}

inline Quaternion Quaternion::CreateFromYawPitchRoll(float yaw, float pitch, float roll) noexcept
{
    using namespace DirectX;
    Quaternion R;
    XMStoreFloat4(&R, XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));
    return R;
}

inline Quaternion Quaternion::CreateFromYawPitchRoll(Vector3 const& angles) noexcept
{
    using namespace DirectX;
    Quaternion R;
    XMStoreFloat4(&R, XMQuaternionRotationRollPitchYawFromVector(angles));
    return R;
}

inline Quaternion Quaternion::CreateFromRotationMatrix(Matrix const& M) noexcept
{
    using namespace DirectX;
    const XMMATRIX M0 = XMLoadFloat4x4(&M);

    Quaternion R;
    XMStoreFloat4(&R, XMQuaternionRotationMatrix(M0));
    return R;
}

inline void Quaternion::Lerp(Quaternion const& q1, Quaternion const& q2, float t, Quaternion& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);

    const XMVECTOR dot = XMVector4Dot(Q0, Q1);

    XMVECTOR R;
    if (XMVector4GreaterOrEqual(dot, XMVectorZero()))
    {
        R = XMVectorLerp(Q0, Q1, t);
    }
    else
    {
        const XMVECTOR tv  = XMVectorReplicate(t);
        const XMVECTOR t1v = XMVectorReplicate(1.f - t);
        const XMVECTOR X0  = XMVectorMultiply(Q0, t1v);
        const XMVECTOR X1  = XMVectorMultiply(Q1, tv);
        R                  = XMVectorSubtract(X0, X1);
    }

    XMStoreFloat4(&result, XMQuaternionNormalize(R));
}

inline Quaternion Quaternion::Lerp(Quaternion const& q1, Quaternion const& q2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);

    const XMVECTOR dot = XMVector4Dot(Q0, Q1);

    XMVECTOR R;
    if (XMVector4GreaterOrEqual(dot, XMVectorZero()))
    {
        R = XMVectorLerp(Q0, Q1, t);
    }
    else
    {
        const XMVECTOR tv  = XMVectorReplicate(t);
        const XMVECTOR t1v = XMVectorReplicate(1.f - t);
        const XMVECTOR X0  = XMVectorMultiply(Q0, t1v);
        const XMVECTOR X1  = XMVectorMultiply(Q1, tv);
        R                  = XMVectorSubtract(X0, X1);
    }

    Quaternion result;
    XMStoreFloat4(&result, XMQuaternionNormalize(R));
    return result;
}

inline void Quaternion::Slerp(Quaternion const& q1, Quaternion const& q2, float t, Quaternion& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);
    XMStoreFloat4(&result, XMQuaternionSlerp(Q0, Q1, t));
}

inline Quaternion Quaternion::Slerp(Quaternion const& q1, Quaternion const& q2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);

    Quaternion result;
    XMStoreFloat4(&result, XMQuaternionSlerp(Q0, Q1, t));
    return result;
}

inline void Quaternion::Concatenate(Quaternion const& q1, Quaternion const& q2, Quaternion& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);
    XMStoreFloat4(&result, XMQuaternionMultiply(Q1, Q0));
}

inline Quaternion Quaternion::Concatenate(Quaternion const& q1, Quaternion const& q2) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);

    Quaternion result;
    XMStoreFloat4(&result, XMQuaternionMultiply(Q1, Q0));
    return result;
}

inline Quaternion Quaternion::FromToRotation(Vector3 const& fromDir, Vector3 const& toDir) noexcept
{
    Quaternion result;
    FromToRotation(fromDir, toDir, result);
    return result;
}

inline Quaternion Quaternion::LookRotation(Vector3 const& forward, Vector3 const& up) noexcept
{
    Quaternion result;
    LookRotation(forward, up, result);
    return result;
}

inline float Quaternion::Angle(Quaternion const& q1, Quaternion const& q2) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);

    // We can use the conjugate here instead of inverse assuming q1 & q2 are normalized.
    XMVECTOR R = XMQuaternionMultiply(XMQuaternionConjugate(Q0), Q1);

    float const rs = XMVectorGetW(R);
    R              = XMVector3Length(R);
    return 2.f * atan2f(XMVectorGetX(R), rs);
}

/****************************************************************************
 *
 * Color
 *
 ****************************************************************************/

inline Color::Color(DirectX::PackedVector::XMCOLOR const& Packed) noexcept
{
    using namespace DirectX;
    XMStoreFloat4(this, PackedVector::XMLoadColor(&Packed));
}

inline Color::Color(DirectX::PackedVector::XMUBYTEN4 const& Packed) noexcept
{
    using namespace DirectX;
    XMStoreFloat4(this, PackedVector::XMLoadUByteN4(&Packed));
}

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------
inline bool Color::operator==(Color const& c) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(this);
    const XMVECTOR c2 = XMLoadFloat4(&c);
    return XMColorEqual(c1, c2);
}

inline bool Color::operator!=(Color const& c) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(this);
    const XMVECTOR c2 = XMLoadFloat4(&c);
    return XMColorNotEqual(c1, c2);
}

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------

inline Color& Color::operator=(DirectX::PackedVector::XMCOLOR const& Packed) noexcept
{
    using namespace DirectX;
    XMStoreFloat4(this, PackedVector::XMLoadColor(&Packed));
    return *this;
}

inline Color& Color::operator=(DirectX::PackedVector::XMUBYTEN4 const& Packed) noexcept
{
    using namespace DirectX;
    XMStoreFloat4(this, PackedVector::XMLoadUByteN4(&Packed));
    return *this;
}

inline Color& Color::operator+=(Color const& c) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(this);
    const XMVECTOR c2 = XMLoadFloat4(&c);
    XMStoreFloat4(this, XMVectorAdd(c1, c2));
    return *this;
}

inline Color& Color::operator-=(Color const& c) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(this);
    const XMVECTOR c2 = XMLoadFloat4(&c);
    XMStoreFloat4(this, XMVectorSubtract(c1, c2));
    return *this;
}

inline Color& Color::operator*=(Color const& c) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(this);
    const XMVECTOR c2 = XMLoadFloat4(&c);
    XMStoreFloat4(this, XMVectorMultiply(c1, c2));
    return *this;
}

inline Color& Color::operator*=(float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(this, XMVectorScale(c, S));
    return *this;
}

inline Color& Color::operator/=(Color const& c) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(this);
    const XMVECTOR c2 = XMLoadFloat4(&c);
    XMStoreFloat4(this, XMVectorDivide(c1, c2));
    return *this;
}

//------------------------------------------------------------------------------
// Binary operators
//------------------------------------------------------------------------------

inline Color operator+(Color const& C1, Color const& C2) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(&C1);
    const XMVECTOR c2 = XMLoadFloat4(&C2);
    Color          R;
    XMStoreFloat4(&R, XMVectorAdd(c1, c2));
    return R;
}

inline Color operator-(Color const& C1, Color const& C2) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(&C1);
    const XMVECTOR c2 = XMLoadFloat4(&C2);
    Color          R;
    XMStoreFloat4(&R, XMVectorSubtract(c1, c2));
    return R;
}

inline Color operator*(Color const& C1, Color const& C2) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(&C1);
    const XMVECTOR c2 = XMLoadFloat4(&C2);
    Color          R;
    XMStoreFloat4(&R, XMVectorMultiply(c1, c2));
    return R;
}

inline Color operator*(Color const& C, float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(&C);
    Color          R;
    XMStoreFloat4(&R, XMVectorScale(c, S));
    return R;
}

inline Color operator*(float S, Color const& C) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(&C);
    Color          R;
    XMStoreFloat4(&R, XMVectorScale(c1, S));
    return R;
}

inline Color operator/(Color const& C1, Color const& C2) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(&C1);
    const XMVECTOR c2 = XMLoadFloat4(&C2);
    Color          R;
    XMStoreFloat4(&R, XMVectorDivide(c1, c2));
    return R;
}

//------------------------------------------------------------------------------
// Color operations
//------------------------------------------------------------------------------

inline DirectX::PackedVector::XMCOLOR Color::BGRA() const noexcept
{
    using namespace DirectX;
    const XMVECTOR        clr = XMLoadFloat4(this);
    PackedVector::XMCOLOR Packed;
    PackedVector::XMStoreColor(&Packed, clr);
    return Packed;
}

inline DirectX::PackedVector::XMUBYTEN4 Color::RGBA() const noexcept
{
    using namespace DirectX;
    const XMVECTOR          clr = XMLoadFloat4(this);
    PackedVector::XMUBYTEN4 Packed;
    PackedVector::XMStoreUByteN4(&Packed, clr);
    return Packed;
}

inline void Color::Negate() noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(this, XMColorNegative(c));
}

inline void Color::Negate(Color& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMColorNegative(c));
}

inline void Color::Saturate() noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(this, XMVectorSaturate(c));
}

inline void Color::Saturate(Color& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMVectorSaturate(c));
}

inline void Color::Premultiply() noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMVECTOR       a = XMVectorSplatW(c);
    a                = XMVectorSelect(g_XMIdentityR3, a, g_XMSelect1110);
    XMStoreFloat4(this, XMVectorMultiply(c, a));
}

inline void Color::Premultiply(Color& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMVECTOR       a = XMVectorSplatW(c);
    a                = XMVectorSelect(g_XMIdentityR3, a, g_XMSelect1110);
    XMStoreFloat4(&result, XMVectorMultiply(c, a));
}

inline void Color::AdjustSaturation(float sat) noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(this, XMColorAdjustSaturation(c, sat));
}

inline void Color::AdjustSaturation(float sat, Color& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMColorAdjustSaturation(c, sat));
}

inline void Color::AdjustContrast(float contrast) noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(this, XMColorAdjustContrast(c, contrast));
}

inline void Color::AdjustContrast(float contrast, Color& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMColorAdjustContrast(c, contrast));
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline void Color::Modulate(Color const& c1, Color const& c2, Color& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR C0 = XMLoadFloat4(&c1);
    const XMVECTOR C1 = XMLoadFloat4(&c2);
    XMStoreFloat4(&result, XMColorModulate(C0, C1));
}

inline Color Color::Modulate(Color const& c1, Color const& c2) noexcept
{
    using namespace DirectX;
    const XMVECTOR C0 = XMLoadFloat4(&c1);
    const XMVECTOR C1 = XMLoadFloat4(&c2);

    Color result;
    XMStoreFloat4(&result, XMColorModulate(C0, C1));
    return result;
}

inline void Color::Lerp(Color const& c1, Color const& c2, float t, Color& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR C0 = XMLoadFloat4(&c1);
    const XMVECTOR C1 = XMLoadFloat4(&c2);
    XMStoreFloat4(&result, XMVectorLerp(C0, C1, t));
}

inline Color Color::Lerp(Color const& c1, Color const& c2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR C0 = XMLoadFloat4(&c1);
    const XMVECTOR C1 = XMLoadFloat4(&c2);

    Color result;
    XMStoreFloat4(&result, XMVectorLerp(C0, C1, t));
    return result;
}

/****************************************************************************
 *
 * Ray
 *
 ****************************************************************************/

//-----------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------
inline bool Ray::operator==(Ray const& r) const noexcept
{
    using namespace DirectX;
    const XMVECTOR r1p = XMLoadFloat3(&position);
    const XMVECTOR r2p = XMLoadFloat3(&r.position);
    const XMVECTOR r1d = XMLoadFloat3(&direction);
    const XMVECTOR r2d = XMLoadFloat3(&r.direction);
    return XMVector3Equal(r1p, r2p) && XMVector3Equal(r1d, r2d);
}

inline bool Ray::operator!=(Ray const& r) const noexcept
{
    using namespace DirectX;
    const XMVECTOR r1p = XMLoadFloat3(&position);
    const XMVECTOR r2p = XMLoadFloat3(&r.position);
    const XMVECTOR r1d = XMLoadFloat3(&direction);
    const XMVECTOR r2d = XMLoadFloat3(&r.direction);
    return XMVector3NotEqual(r1p, r2p) && XMVector3NotEqual(r1d, r2d);
}

//-----------------------------------------------------------------------------
// Ray operators
//------------------------------------------------------------------------------

inline bool Ray::Intersects(BoundingSphere const& sphere, _Out_ float& Dist) const noexcept
{
    return sphere.Intersects(position, direction, Dist);
}

inline bool Ray::Intersects(BoundingBox const& box, _Out_ float& Dist) const noexcept
{
    return box.Intersects(position, direction, Dist);
}

inline bool Ray::Intersects(Vector3 const& tri0, Vector3 const& tri1, Vector3 const& tri2, _Out_ float& Dist) const noexcept
{
    return DirectX::TriangleTests::Intersects(position, direction, tri0, tri1, tri2, Dist);
}

inline bool Ray::Intersects(Plane const& plane, _Out_ float& Dist) const noexcept
{
    using namespace DirectX;

    const XMVECTOR p   = XMLoadFloat4(&plane);
    const XMVECTOR dir = XMLoadFloat3(&direction);

    const XMVECTOR nd = XMPlaneDotNormal(p, dir);

    if (XMVector3LessOrEqual(XMVectorAbs(nd), g_RayEpsilon))
    {
        Dist = 0.f;
        return false;
    }
    else
    {
        // t = -(dot(n,origin) + D) / dot(n,dir)
        const XMVECTOR pos = XMLoadFloat3(&position);
        XMVECTOR       v   = XMPlaneDotNormal(p, pos);
        v                  = XMVectorAdd(v, XMVectorSplatW(p));
        v                  = XMVectorDivide(v, nd);
        float dist         = -XMVectorGetX(v);
        if (dist < 0)
        {
            Dist = 0.f;
            return false;
        }
        else
        {
            Dist = dist;
            return true;
        }
    }
}

/****************************************************************************
 *
 * Viewport
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------

#if (__cplusplus < 202002L)
inline bool Viewport::operator==(Viewport const& vp) const noexcept
{
    return (x == vp.x && y == vp.y && width == vp.width && height == vp.height && minDepth == vp.minDepth && maxDepth == vp.maxDepth);
}

inline bool Viewport::operator!=(Viewport const& vp) const noexcept
{
    return (x != vp.x || y != vp.y || width != vp.width || height != vp.height || minDepth != vp.minDepth || maxDepth != vp.maxDepth);
}
#endif

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------

inline Viewport& Viewport::operator=(const RECT& rct) noexcept
{
    x        = float(rct.left);
    y        = float(rct.top);
    width    = float(rct.right - rct.left);
    height   = float(rct.bottom - rct.top);
    minDepth = 0.f;
    maxDepth = 1.f;
    return *this;
}

#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
inline Viewport& Viewport::operator=(const D3D11_VIEWPORT& vp) noexcept
{
    x        = vp.TopLeftX;
    y        = vp.TopLeftY;
    width    = vp.Width;
    height   = vp.Height;
    minDepth = vp.MinDepth;
    maxDepth = vp.MaxDepth;
    return *this;
}
#endif

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
inline Viewport& Viewport::operator=(const D3D12_VIEWPORT& vp) noexcept
{
    x        = vp.TopLeftX;
    y        = vp.TopLeftY;
    width    = vp.Width;
    height   = vp.Height;
    minDepth = vp.MinDepth;
    maxDepth = vp.MaxDepth;
    return *this;
}
#endif

//------------------------------------------------------------------------------
// Viewport operations
//------------------------------------------------------------------------------

inline float Viewport::AspectRatio() const noexcept
{
    if (width == 0.f || height == 0.f)
        return 0.f;

    return (width / height);
}

inline Vector3 Viewport::Project(Vector3 const& p, Matrix const& proj, Matrix const& view, Matrix const& world) const noexcept
{
    using namespace DirectX;
    XMVECTOR       v          = XMLoadFloat3(&p);
    const XMMATRIX projection = XMLoadFloat4x4(&proj);
    v                         = XMVector3Project(v, x, y, width, height, minDepth, maxDepth, projection, view, world);
    Vector3 result;
    XMStoreFloat3(&result, v);
    return result;
}

inline void Viewport::Project(Vector3 const& p, Matrix const& proj, Matrix const& view, Matrix const& world, Vector3& result) const noexcept
{
    using namespace DirectX;
    XMVECTOR       v          = XMLoadFloat3(&p);
    const XMMATRIX projection = XMLoadFloat4x4(&proj);
    v                         = XMVector3Project(v, x, y, width, height, minDepth, maxDepth, projection, view, world);
    XMStoreFloat3(&result, v);
}

inline Vector3 Viewport::Unproject(Vector3 const& p, Matrix const& proj, Matrix const& view, Matrix const& world) const noexcept
{
    using namespace DirectX;
    XMVECTOR       v          = XMLoadFloat3(&p);
    const XMMATRIX projection = XMLoadFloat4x4(&proj);
    v                         = XMVector3Unproject(v, x, y, width, height, minDepth, maxDepth, projection, view, world);
    Vector3 result;
    XMStoreFloat3(&result, v);
    return result;
}

inline void
Viewport::Unproject(Vector3 const& p, Matrix const& proj, Matrix const& view, Matrix const& world, Vector3& result) const noexcept
{
    using namespace DirectX;
    XMVECTOR       v          = XMLoadFloat3(&p);
    const XMMATRIX projection = XMLoadFloat4x4(&proj);
    v                         = XMVector3Unproject(v, x, y, width, height, minDepth, maxDepth, projection, view, world);
    XMStoreFloat3(&result, v);
}
