#include "pch.h"
#include "Matrix.h"

namespace Jam
{

Mtx4::Mtx4()
{
}

Mtx4::Mtx4(const Vec4 _r0, const Vec4 _r1, const Vec4 _r2, const Vec4 _r3)
    : r0(_r0)
    , r1(_r1)
    , r2(_r2)
    , r3(_r3)
{
}

Mtx4::Mtx4(
    const float _m00,
    const float _m01,
    const float _m02,
    const float _m03,
    const float _m10,
    const float _m11,
    const float _m12,
    const float _m13,
    const float _m20,
    const float _m21,
    const float _m22,
    const float _m23,
    const float _m30,
    const float _m31,
    const float _m32,
    const float _m33)
    : r0(_m00, _m01, _m02, _m03)
    , r1(_m10, _m11, _m12, _m13)
    , r2(_m20, _m21, _m22, _m23)
    , r3(_m30, _m31, _m32, _m33)
{
}

Mtx4 Mtx4::MakeOrtho(
    const float left,
    const float right,
    const float top,
    const float bottom,
    const float nearZ,
    const float farZ)
{
    const float w = right - left;
    const float h = top - bottom;
    const float d = farZ - nearZ;
    JAM_ASSERT(w > 0 && h > 0 && d > 0, "Invalid orthographic projection parameters.");

    return {
        {             2.f / w,                 0.f,        0.f, 0.f },
        {                 0.f,             2.f / h,        0.f, 0.f },
        {                 0.f,                 0.f,    1.f / d, 0.f },
        { -(right + left) / w, -(top + bottom) / h, -nearZ / d, 1.f }
    };
}

Mtx4 Mtx4::MakeOrtho(
    const float _width,
    const float _height,
    const float _nearZ,
    const float _farZ)
{
    return MakeOrtho(-_width * 0.5f, _width * 0.5f, _height * 0.5f, -_height * 0.5f, _nearZ, _farZ);
}

Mtx4 Mtx4::MakePerspectiveFov(
    const float _fovYRad,
    const float _aspect,
    const float _nearZ,
    const float _farZ)
{
    JAM_ASSERT(_fovYRad > 0.f && _fovYRad < kPi, "Invalid field of view.");
    JAM_ASSERT(_aspect > 0.f, "Invalid aspect ratio.");
    JAM_ASSERT(_nearZ > 0.f && _farZ > _nearZ, "Invalid near and far planes.");

    const float f = 1.f / Tan(_fovYRad * 0.5f);
    const float d = _farZ - _nearZ;
    return {
        { f / _aspect, 0.f,                 0.f, 0.f },
        {         0.f,   f,                 0.f, 0.f },
        {         0.f, 0.f,           _farZ / d, 1.f },
        {         0.f, 0.f, -_farZ * _nearZ / d, 0.f }
    };
}

Mtx4 Mtx4::MakeTranslation(
    const Vec3 _translation)
{
    return {
        {            1.f,            0.f,            0.f, 0.f },
        {            0.f,            1.f,            0.f, 0.f },
        {            0.f,            0.f,            1.f, 0.f },
        { _translation.x, _translation.y, _translation.z, 1.f }
    };
}

Mtx4 Mtx4::MakeScale(
    const Vec3 _scale)
{
    JAM_ASSERT(!_scale.IsZeroApprox(), "Scale components cannot be zero.");

    return {
        { _scale.x,      0.f,      0.f, 0.f },
        {      0.f, _scale.y,      0.f, 0.f },
        {      0.f,      0.f, _scale.z, 0.f },
        {      0.f,      0.f,      0.f, 1.f }
    };
}

Mtx4 Mtx4::MakeRotationX(
    const float _angleRad)
{
    const float c = Cos(_angleRad), s = Sin(_angleRad);
    return {
        { 1.f, 0.f, 0.f, 0.f },
        { 0.f,   c,   s, 0.f },
        { 0.f,  -s,   c, 0.f },
        { 0.f, 0.f, 0.f, 1.f }
    };
}

Mtx4 Mtx4::MakeRotationY(
    const float _angleRad)
{
    const float c = Cos(_angleRad), s = Sin(_angleRad);
    return {
        {   c, 0.f,  -s, 0.f },
        { 0.f, 1.f, 0.f, 0.f },
        {   s, 0.f,   c, 0.f },
        { 0.f, 0.f, 0.f, 1.f }
    };
}

Mtx4 Mtx4::MakeRotationZ(
    const float _angleRad)
{
    const float c = Cos(_angleRad), s = Sin(_angleRad);
    return {
        {   c,   s, 0.f, 0.f },
        {  -s,   c, 0.f, 0.f },
        { 0.f, 0.f, 1.f, 0.f },
        { 0.f, 0.f, 0.f, 1.f }
    };
}

Mtx4 Mtx4::FromAxisAngle(
    const Vec3  _axis,
    const float _angleRad)
{
    JAM_ASSERT(!_axis.IsZeroApprox(), "Rotation axis cannot be zero vector.");

    const float c = Cos(_angleRad), s = Sin(_angleRad), t = 1.f - c;
    const Vec3  a = _axis.Normalized();
    const float x = a.x, y = a.y, z = a.z;
    return {
        {     t * x * x + c, t * x * y + z * s, t * x * z - y * s, 0.f },
        { t * x * y - z * s,     t * y * y + c, t * y * z + x * s, 0.f },
        { t * x * z + y * s, t * y * z - x * s,     t * z * z + c, 0.f },
        {               0.f,               0.f,               0.f, 1.f }
    };
}

Mtx4 Mtx4::FromEuler(
    const Vec3 _euler)
{
    const float cx = Cos(_euler.x), sx = Sin(_euler.x);
    const float cy = Cos(_euler.y), sy = Sin(_euler.y);
    const float cz = Cos(_euler.z), sz = Sin(_euler.z);
    const float sxsy = sx * sy, cxsy = cx * sy;

    return {
        {             cy * cz,             cy * sz,     -sy, 0.f },
        { sxsy * cz - cx * sz, sxsy * sz + cx * cz, sx * cy, 0.f },
        { cxsy * cz + sx * sz, cxsy * sz - sx * cz, cx * cy, 0.f },
        {                 0.f,                 0.f,     0.f, 1.f }
    };
}

Mtx4 Mtx4::FromQuaternion(
    const Quat _quat)
{
    JAM_ASSERT(!_quat.IsZeroApprox(), "Rotation quaternion cannot be zero quaternion.");

    const Quat  q  = _quat.Normalized();
    const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    return {
        { 1.f - 2.f * (yy + zz),       2.f * (xy + wz),       2.f * (xz - wy), 0.f },
        {       2.f * (xy - wz), 1.f - 2.f * (xx + zz),       2.f * (yz + wx), 0.f },
        {       2.f * (xz + wy),       2.f * (yz - wx), 1.f - 2.f * (xx + yy), 0.f },
        {                   0.f,                   0.f,                   0.f, 1.f }
    };
}

Mtx4 Mtx4::MakeLookAt(
    const Vec3 _eyePos,
    const Vec3 _focusPoint,
    const Vec3 _up)
{
    JAM_ASSERT(!(_focusPoint - _eyePos).IsZeroApprox(), "Eye and focus point cannot be the same.");
    JAM_ASSERT(!_up.IsZeroApprox(), "Up direction cannot be zero vector.");

    const Vec3  f  = (_focusPoint - _eyePos).Normalized();
    const Vec3  r  = _up.Cross(f).Normalized();
    const Vec3  u  = f.Cross(r);
    const float tx = -r.Dot(_eyePos);
    const float ty = -u.Dot(_eyePos);
    const float tz = -f.Dot(_eyePos);
    return {
        { r.x, u.x, f.x, 0.f },
        { r.y, u.y, f.y, 0.f },
        { r.z, u.z, f.z, 0.f },
        {  tx,  ty,  tz, 1.f }
    };
}

Mtx4 Mtx4::MakeSRT(
    const Vec3 _scale,
    const Quat _rotation,
    const Vec3 _translation)
{
    JAM_ASSERT(!_scale.IsZeroApprox(), "Scale cannot be zero vector.");
    JAM_ASSERT(!_rotation.IsZeroApprox(), "Rotation cannot be zero quaternion.");

    const Quat  q  = _rotation.Normalized();
    const float sx = _scale.x, sy = _scale.y, sz = _scale.z;
    const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    return {
        { sx * (1.f - 2.f * (yy + zz)),         sx * 2.f * (xy + wz),         sx * 2.f * (xz - wy), 0.f },
        {         sy * 2.f * (xy - wz), sy * (1.f - 2.f * (xx + zz)),         sy * 2.f * (yz + wx), 0.f },
        {         sz * 2.f * (xz + wy),         sz * 2.f * (yz - wx), sz * (1.f - 2.f * (xx + yy)), 0.f },
        {               _translation.x,               _translation.y,               _translation.z, 1.f }
    };
}

Mtx4 Mtx4::MakeSRT(Vec3 _scale, Vec3 _euler, Vec3 _translation)
{
    JAM_ASSERT(!_scale.IsZeroApprox(), "Scale cannot be zero vector.");

    const float cx = Cos(_euler.x), sx = Sin(_euler.x);
    const float cy = Cos(_euler.y), sy = Sin(_euler.y);
    const float cz = Cos(_euler.z), sz = Sin(_euler.z);
    const float sxsy = sx * sy, cxsy = cx * sy;

    return {
        {             _scale.x * (cy * cz),             _scale.x * (cy * sz),     _scale.x * (-sy), 0.f },
        { _scale.y * (sxsy * cz - cx * sz), _scale.y * (sxsy * sz + cx * cz), _scale.y * (sx * cy), 0.f },
        { _scale.z * (cxsy * cz + sx * sz), _scale.z * (cxsy * sz - sx * cz), _scale.z * (cx * cy), 0.f },
        {                   _translation.x,                   _translation.y,       _translation.z, 1.f }
    };
}

Mtx4 Mtx4::MakeSRT(Vec3 _scale, Vec3 _forward, Vec3 _up, Vec3 _translation)
{
    JAM_ASSERT(!_scale.IsZeroApprox(), "Scale cannot be zero vector.");
    JAM_ASSERT(!_forward.IsZeroApprox(), "Forward direction cannot be zero vector.");
    JAM_ASSERT(!_up.IsZeroApprox(), "Up direction cannot be zero vector.");

    const Vec3 f = _forward.Normalized();
    const Vec3 r = _up.Cross(f).Normalized();
    const Vec3 u = f.Cross(r);

    return {
        { r.x * _scale.x, r.y * _scale.x, r.z * _scale.x, 0.f },
        { u.x * _scale.y, u.y * _scale.y, u.z * _scale.y, 0.f },
        { f.x * _scale.z, f.y * _scale.z, f.z * _scale.z, 0.f },
        { _translation.x, _translation.y, _translation.z, 1.f }
    };
}

Mtx4 Mtx4::MakeInvSRT(
    const Vec3 _scale,
    const Quat _rotation,
    const Vec3 _translation)
{
    JAM_ASSERT(!_scale.IsZeroApprox(), "Scale cannot be zero vector.");
    JAM_ASSERT(!_rotation.IsZeroApprox(), "Rotation cannot be zero quaternion.");

    const Quat  q  = _rotation.Normalized();
    const float sx = _scale.x, sy = _scale.y, sz = _scale.z;
    const float isx = 1.f / sx, isy = 1.f / sy, isz = 1.f / sz;
    const float tx = _translation.x, ty = _translation.y, tz = _translation.z;

    const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

    const float r00 = 1.f - 2.f * (yy + zz);
    const float r01 = 2.f * (xy + wz);
    const float r02 = 2.f * (xz - wy);
    const float r10 = 2.f * (xy - wz);
    const float r11 = 1.f - 2.f * (xx + zz);
    const float r12 = 2.f * (yz + wx);
    const float r20 = 2.f * (xz + wy);
    const float r21 = 2.f * (yz - wx);
    const float r22 = 1.f - 2.f * (xx + yy);

    return {
        {                               r00 * isx,                               r10 * isy,                               r20 * isz, 0.f },
        {                               r01 * isx,                               r11 * isy,                               r21 * isz, 0.f },
        {                               r02 * isx,                               r12 * isy,                               r22 * isz, 0.f },
        { -(tx * r00 + ty * r01 + tz * r02) * isx, -(tx * r10 + ty * r11 + tz * r12) * isy, -(tx * r20 + ty * r21 + tz * r22) * isz, 1.f }
    };
}

Mtx4 Mtx4::MakeInvSRT(Vec3 _scale, Vec3 _euler, Vec3 _translation)
{
    JAM_ASSERT(!_scale.IsZeroApprox(), "Scale cannot be zero vector.");

    const float cx = Cos(_euler.x), sx = Sin(_euler.x);
    const float cy = Cos(_euler.y), sy = Sin(_euler.y);
    const float cz = Cos(_euler.z), sz = Sin(_euler.z);
    const float sxsy = sx * sy, cxsy = cx * sy;

    const float isx = 1.f / _scale.x, isy = 1.f / _scale.y, isz = 1.f / _scale.z;
    const float tx = _translation.x, ty = _translation.y, tz = _translation.z;

    const float r00 = cy * cz, r01 = cy * sz, r02 = -sy;
    const float r10 = sxsy * cz - cx * sz, r11 = sxsy * sz + cx * cz, r12 = sx * cy;
    const float r20 = cxsy * cz + sx * sz, r21 = cxsy * sz - sx * cz, r22 = cx * cy;

    return {
        {                               r00 * isx,                               r10 * isy,                               r20 * isz, 0.f },
        {                               r01 * isx,                               r11 * isy,                               r21 * isz, 0.f },
        {                               r02 * isx,                               r12 * isy,                               r22 * isz, 0.f },
        { -(tx * r00 + ty * r01 + tz * r02) * isx, -(tx * r10 + ty * r11 + tz * r12) * isy, -(tx * r20 + ty * r21 + tz * r22) * isz, 1.f }
    };
}

Mtx4 Mtx4::MakeInvSRT(Vec3 _scale, Vec3 _forward, Vec3 _up, Vec3 _translation)
{
    JAM_ASSERT(!_scale.IsZeroApprox(), "Scale cannot be zero vector.");
    JAM_ASSERT(!_forward.IsZeroApprox(), "Forward direction cannot be zero vector.");
    JAM_ASSERT(!_up.IsZeroApprox(), "Up direction cannot be zero vector.");

    const Vec3  f   = _forward.Normalized();
    const Vec3  r   = _up.Cross(f).Normalized();
    const Vec3  u   = f.Cross(r);
    const float isx = 1.f / _scale.x, isy = 1.f / _scale.y, isz = 1.f / _scale.z;
    const float tx = _translation.x, ty = _translation.y, tz = _translation.z;
    return {
        {                               r.x * isx,                               u.x * isy,                               f.x * isz, 0.f },
        {                               r.y * isx,                               u.y * isy,                               f.y * isz, 0.f },
        {                               r.z * isx,                               u.z * isy,                               f.z * isz, 0.f },
        { -(tx * r.x + ty * r.y + tz * r.z) * isx, -(tx * u.x + ty * u.y + tz * u.z) * isy, -(tx * f.x + ty * f.y + tz * f.z) * isz, 1.f }
    };
}

void Mtx4::DecomposeSRT(
    Vec3* _pScaleOrNull,
    Quat* _pRotationOrNull,
    Vec3* _pTranslationOrNull) const
{
    const float sx = r0.xyz.Length();
    const float sy = r1.xyz.Length();
    const float sz = r2.xyz.Length();

    if (_pScaleOrNull)
    {
        *_pScaleOrNull = { sx, sy, sz };
    }

    if (_pRotationOrNull)
    {
        if (IsZeroApprox(sx) || IsZeroApprox(sy) || IsZeroApprox(sz))
        {
            *_pRotationOrNull = Quat::kIdentity;
            return;
        }
        const Mtx4 rmtx   = { r0 / sx, r1 / sy, r2 / sz, Vec4::kUnitW };
        *_pRotationOrNull = Quat::FromRotationMatrix(rmtx).Normalized();
    }

    if (_pTranslationOrNull)
    {
        *_pTranslationOrNull = r3.xyz;
    }
}

Vec3 Mtx4::DecomposeScale() const
{
    Vec3 ret = {};
    DecomposeSRT(&ret, nullptr, nullptr);
    return ret;
}

Quat Mtx4::DecomposeRotation() const
{
    Quat ret = {};
    DecomposeSRT(nullptr, &ret, nullptr);
    return ret;
}

Vec3 Mtx4::DecomposeTranslation() const
{
    return r3.xyz;
}

Mtx4 Mtx4::Transpose() const
{
#ifdef JAM_SIMD_SSE
    const __m128 t0 = _mm_unpacklo_ps(m128r0, m128r2);
    const __m128 t1 = _mm_unpackhi_ps(m128r0, m128r2);
    const __m128 t2 = _mm_unpacklo_ps(m128r1, m128r3);
    const __m128 t3 = _mm_unpackhi_ps(m128r1, m128r3);

    Mtx4 ret   = {};
    ret.m128r0 = _mm_unpacklo_ps(t0, t2);
    ret.m128r1 = _mm_unpackhi_ps(t0, t2);
    ret.m128r2 = _mm_unpacklo_ps(t1, t3);
    ret.m128r3 = _mm_unpackhi_ps(t1, t3);
    return ret;
#else
    return {
        { m00, m10, m20, m30 },
        { m01, m11, m21, m31 },
        { m02, m12, m22, m32 },
        { m03, m13, m23, m33 }
    };
#endif
}

Mtx4 Mtx4::Inverse() const
{
#ifdef JAM_SIMD_SSE
    __m128 t1 = _mm_shuffle_ps(m128r0, m128r1, _MM_SHUFFLE(1, 0, 1, 0));
    __m128 t3 = _mm_shuffle_ps(m128r0, m128r1, _MM_SHUFFLE(3, 2, 3, 2));
    __m128 t2 = _mm_shuffle_ps(m128r2, m128r3, _MM_SHUFFLE(1, 0, 1, 0));
    __m128 t4 = _mm_shuffle_ps(m128r2, m128r3, _MM_SHUFFLE(3, 2, 3, 2));

    __m128 mt0 = _mm_shuffle_ps(t1, t2, _MM_SHUFFLE(2, 0, 2, 0));
    __m128 mt1 = _mm_shuffle_ps(t1, t2, _MM_SHUFFLE(3, 1, 3, 1));
    __m128 mt2 = _mm_shuffle_ps(t3, t4, _MM_SHUFFLE(2, 0, 2, 0));
    __m128 mt3 = _mm_shuffle_ps(t3, t4, _MM_SHUFFLE(3, 1, 3, 1));

    __m128 v00 = _mm_shuffle_ps(mt2, mt2, _MM_SHUFFLE(1, 1, 0, 0));
    __m128 v10 = _mm_shuffle_ps(mt3, mt3, _MM_SHUFFLE(3, 2, 3, 2));
    __m128 v01 = _mm_shuffle_ps(mt0, mt0, _MM_SHUFFLE(1, 1, 0, 0));
    __m128 v11 = _mm_shuffle_ps(mt1, mt1, _MM_SHUFFLE(3, 2, 3, 2));
    __m128 v02 = _mm_shuffle_ps(mt2, mt0, _MM_SHUFFLE(2, 0, 2, 0));
    __m128 v12 = _mm_shuffle_ps(mt3, mt1, _MM_SHUFFLE(3, 1, 3, 1));

    __m128 d0 = _mm_mul_ps(v00, v10);
    __m128 d1 = _mm_mul_ps(v01, v11);
    __m128 d2 = _mm_mul_ps(v02, v12);

    v00 = _mm_shuffle_ps(mt2, mt2, _MM_SHUFFLE(3, 2, 3, 2));
    v10 = _mm_shuffle_ps(mt3, mt3, _MM_SHUFFLE(1, 1, 0, 0));
    v01 = _mm_shuffle_ps(mt0, mt0, _MM_SHUFFLE(3, 2, 3, 2));
    v11 = _mm_shuffle_ps(mt1, mt1, _MM_SHUFFLE(1, 1, 0, 0));
    v02 = _mm_shuffle_ps(mt2, mt0, _MM_SHUFFLE(3, 1, 3, 1));
    v12 = _mm_shuffle_ps(mt3, mt1, _MM_SHUFFLE(2, 0, 2, 0));

    d0 = _mm_sub_ps(d0, _mm_mul_ps(v00, v10));
    d1 = _mm_sub_ps(d1, _mm_mul_ps(v01, v11));
    d2 = _mm_sub_ps(d2, _mm_mul_ps(v02, v12));

    v11        = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 1, 3, 1));
    v00        = _mm_shuffle_ps(mt1, mt1, _MM_SHUFFLE(1, 0, 2, 1));
    v10        = _mm_shuffle_ps(v11, d0, _MM_SHUFFLE(0, 3, 0, 2));
    v01        = _mm_shuffle_ps(mt0, mt0, _MM_SHUFFLE(0, 1, 0, 2));
    v11        = _mm_shuffle_ps(v11, d0, _MM_SHUFFLE(2, 1, 2, 1));
    __m128 V13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 3, 3, 1));
    v02        = _mm_shuffle_ps(mt3, mt3, _MM_SHUFFLE(1, 0, 2, 1));
    v12        = _mm_shuffle_ps(V13, d1, _MM_SHUFFLE(0, 3, 0, 2));
    __m128 V03 = _mm_shuffle_ps(mt2, mt2, _MM_SHUFFLE(0, 1, 0, 2));
    V13        = _mm_shuffle_ps(V13, d1, _MM_SHUFFLE(2, 1, 2, 1));

    __m128 C0 = _mm_mul_ps(v00, v10);
    __m128 C2 = _mm_mul_ps(v01, v11);
    __m128 C4 = _mm_mul_ps(v02, v12);
    __m128 C6 = _mm_mul_ps(V03, V13);

    v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(0, 0, 1, 0));
    v00 = _mm_shuffle_ps(mt1, mt1, _MM_SHUFFLE(2, 1, 3, 2));
    v10 = _mm_shuffle_ps(d0, v11, _MM_SHUFFLE(2, 1, 0, 3));
    v01 = _mm_shuffle_ps(mt0, mt0, _MM_SHUFFLE(1, 3, 2, 3));
    v11 = _mm_shuffle_ps(d0, v11, _MM_SHUFFLE(0, 2, 1, 2));
    V13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(2, 2, 1, 0));
    v02 = _mm_shuffle_ps(mt3, mt3, _MM_SHUFFLE(2, 1, 3, 2));
    v12 = _mm_shuffle_ps(d1, V13, _MM_SHUFFLE(2, 1, 0, 3));
    V03 = _mm_shuffle_ps(mt2, mt2, _MM_SHUFFLE(1, 3, 2, 3));
    V13 = _mm_shuffle_ps(d1, V13, _MM_SHUFFLE(0, 2, 1, 2));

    C0 = _mm_sub_ps(C0, _mm_mul_ps(v00, v10));
    C2 = _mm_sub_ps(C2, _mm_mul_ps(v01, v11));
    C4 = _mm_sub_ps(C4, _mm_mul_ps(v02, v12));
    C6 = _mm_sub_ps(C6, _mm_mul_ps(V03, V13));

    v00 = _mm_shuffle_ps(mt1, mt1, _MM_SHUFFLE(0, 3, 0, 3));
    v10 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 0, 2, 2));
    v10 = _mm_shuffle_ps(v10, v10, _MM_SHUFFLE(0, 2, 3, 0));
    v01 = _mm_shuffle_ps(mt0, mt0, _MM_SHUFFLE(2, 0, 3, 1));
    v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 0, 3, 0));
    v11 = _mm_shuffle_ps(v11, v11, _MM_SHUFFLE(2, 1, 0, 3));
    v02 = _mm_shuffle_ps(mt3, mt3, _MM_SHUFFLE(0, 3, 0, 3));
    v12 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 2, 2, 2));
    v12 = _mm_shuffle_ps(v12, v12, _MM_SHUFFLE(0, 2, 3, 0));
    V03 = _mm_shuffle_ps(mt2, mt2, _MM_SHUFFLE(2, 0, 3, 1));
    V13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 2, 3, 0));
    V13 = _mm_shuffle_ps(V13, V13, _MM_SHUFFLE(2, 1, 0, 3));

    v00       = _mm_mul_ps(v00, v10);
    v01       = _mm_mul_ps(v01, v11);
    v02       = _mm_mul_ps(v02, v12);
    V03       = _mm_mul_ps(V03, V13);
    __m128 C1 = _mm_sub_ps(C0, v00);
    C0        = _mm_add_ps(C0, v00);
    __m128 C3 = _mm_add_ps(C2, v01);
    C2        = _mm_sub_ps(C2, v01);
    __m128 C5 = _mm_sub_ps(C4, v02);
    C4        = _mm_add_ps(C4, v02);
    __m128 C7 = _mm_add_ps(C6, V03);
    C6        = _mm_sub_ps(C6, V03);

    C0 = _mm_shuffle_ps(C0, C1, _MM_SHUFFLE(3, 1, 2, 0));
    C2 = _mm_shuffle_ps(C2, C3, _MM_SHUFFLE(3, 1, 2, 0));
    C4 = _mm_shuffle_ps(C4, C5, _MM_SHUFFLE(3, 1, 2, 0));
    C6 = _mm_shuffle_ps(C6, C7, _MM_SHUFFLE(3, 1, 2, 0));
    C0 = _mm_shuffle_ps(C0, C0, _MM_SHUFFLE(3, 1, 2, 0));
    C2 = _mm_shuffle_ps(C2, C2, _MM_SHUFFLE(3, 1, 2, 0));
    C4 = _mm_shuffle_ps(C4, C4, _MM_SHUFFLE(3, 1, 2, 0));
    C6 = _mm_shuffle_ps(C6, C6, _MM_SHUFFLE(3, 1, 2, 0));

    __m128 det  = _mm_mul_ps(C0, mt0);
    __m128 shuf = _mm_shuffle_ps(det, det, _MM_SHUFFLE(2, 3, 0, 1));
    det         = _mm_add_ps(det, shuf);
    shuf        = _mm_movehl_ps(shuf, det);
    det         = _mm_add_ss(det, shuf);
    det         = _mm_shuffle_ps(det, det, _MM_SHUFFLE(0, 0, 0, 0));

    if (IsZeroApprox(_mm_cvtss_f32(det)))
    {
        return kIdentity;
    }

    const __m128 invDet = _mm_div_ps(_mm_set1_ps(1.f), det);

    Mtx4 ret   = Mtx4::kIdentity;
    ret.m128r0 = _mm_mul_ps(C0, invDet);
    ret.m128r1 = _mm_mul_ps(C2, invDet);
    ret.m128r2 = _mm_mul_ps(C4, invDet);
    ret.m128r3 = _mm_mul_ps(C6, invDet);
    return ret;
#else
    const float m22m33_m23m32 = m22 * m33 - m23 * m32;
    const float m21m33_m23m31 = m21 * m33 - m23 * m31;
    const float m21m32_m22m31 = m21 * m32 - m22 * m31;
    const float m20m33_m23m30 = m20 * m33 - m23 * m30;
    const float m20m32_m22m30 = m20 * m32 - m22 * m30;
    const float m20m31_m21m30 = m20 * m31 - m21 * m30;

    const float m12m33_m13m32 = m12 * m33 - m13 * m32;
    const float m11m33_m13m31 = m11 * m33 - m13 * m31;
    const float m11m32_m12m31 = m11 * m32 - m12 * m31;
    const float m10m33_m13m30 = m10 * m33 - m13 * m30;
    const float m10m32_m12m30 = m10 * m32 - m12 * m30;
    const float m10m31_m11m30 = m10 * m31 - m11 * m30;

    const float m12m23_m13m22 = m12 * m23 - m13 * m22;
    const float m11m23_m13m21 = m11 * m23 - m13 * m21;
    const float m11m22_m12m21 = m11 * m22 - m12 * m21;
    const float m10m23_m13m20 = m10 * m23 - m13 * m20;
    const float m10m22_m12m20 = m10 * m22 - m12 * m20;
    const float m10m21_m11m20 = m10 * m21 - m11 * m20;

    const float inv00 = (m11 * m22m33_m23m32 - m12 * m21m33_m23m31 + m13 * m21m32_m22m31);
    const float inv10 = -(m10 * m22m33_m23m32 - m12 * m20m33_m23m30 + m13 * m20m32_m22m30);
    const float inv20 = (m10 * m21m33_m23m31 - m11 * m20m33_m23m30 + m13 * m20m31_m21m30);
    const float inv30 = -(m10 * m21m32_m22m31 - m11 * m20m32_m22m30 + m12 * m20m31_m21m30);

    const float det = m00 * inv00 + m01 * inv10 + m02 * inv20 + m03 * inv30;

    if (IsZeroApprox(det))
    {
        return kIdentity;
    }

    const float inv01 = -(m01 * m22m33_m23m32 - m02 * m21m33_m23m31 + m03 * m21m32_m22m31);
    const float inv11 = (m00 * m22m33_m23m32 - m02 * m20m33_m23m30 + m03 * m20m32_m22m30);
    const float inv21 = -(m00 * m21m33_m23m31 - m01 * m20m33_m23m30 + m03 * m20m31_m21m30);
    const float inv31 = (m00 * m21m32_m22m31 - m01 * m20m32_m22m30 + m02 * m20m31_m21m30);

    const float inv02 = (m01 * m12m33_m13m32 - m02 * m11m33_m13m31 + m03 * m11m32_m12m31);
    const float inv12 = -(m00 * m12m33_m13m32 - m02 * m10m33_m13m30 + m03 * m10m32_m12m30);
    const float inv22 = (m00 * m11m33_m13m31 - m01 * m10m33_m13m30 + m03 * m10m31_m11m30);
    const float inv32 = -(m00 * m11m32_m12m31 - m01 * m10m32_m12m30 + m02 * m10m31_m11m30);

    const float inv03 = -(m01 * m12m23_m13m22 - m02 * m11m23_m13m21 + m03 * m11m22_m12m21);
    const float inv13 = (m00 * m12m23_m13m22 - m02 * m10m23_m13m20 + m03 * m10m22_m12m20);
    const float inv23 = -(m00 * m11m23_m13m21 - m01 * m10m23_m13m20 + m03 * m10m21_m11m20);
    const float inv33 = (m00 * m11m22_m12m21 - m01 * m10m22_m12m20 + m02 * m10m21_m11m20);

    const float invDet = 1.f / det;
    return {
        { inv00 * invDet, inv01 * invDet, inv02 * invDet, inv03 * invDet },
        { inv10 * invDet, inv11 * invDet, inv12 * invDet, inv13 * invDet },
        { inv20 * invDet, inv21 * invDet, inv22 * invDet, inv23 * invDet },
        { inv30 * invDet, inv31 * invDet, inv32 * invDet, inv33 * invDet }
    };
#endif
}

Vec4 Mtx4::Mult(Vec4 _vec) const
{
#ifdef JAM_SIMD_SSE
    const __m128 vx = _mm_permute_ps(_vec.m128, _MM_SHUFFLE(0, 0, 0, 0));
    const __m128 vy = _mm_permute_ps(_vec.m128, _MM_SHUFFLE(1, 1, 1, 1));
    const __m128 vz = _mm_permute_ps(_vec.m128, _MM_SHUFFLE(2, 2, 2, 2));
    const __m128 vw = _mm_permute_ps(_vec.m128, _MM_SHUFFLE(3, 3, 3, 3));

    Vec4 ret;
    ret.m128 = _mm_mul_ps(vx, m128r0);
    ret.m128 = _mm_fmadd_ps(vy, m128r1, ret.m128);
    ret.m128 = _mm_fmadd_ps(vz, m128r2, ret.m128);
    ret.m128 = _mm_fmadd_ps(vw, m128r3, ret.m128);
    return ret;
#else
    return {
        m00 * _vec.x + m01 * _vec.y + m02 * _vec.z + m03 * _vec.w,
        m10 * _vec.x + m11 * _vec.y + m12 * _vec.z + m13 * _vec.w,
        m20 * _vec.x + m21 * _vec.y + m22 * _vec.z + m23 * _vec.w,
        m30 * _vec.x + m31 * _vec.y + m32 * _vec.z + m33 * _vec.w
    };
#endif
}

Vec3 Mtx4::MultPoint(
    const Vec3 _point) const
{
#ifdef JAM_SIMD_SSE
    const __m128 vx = _mm_set1_ps(_point.x);
    const __m128 vy = _mm_set1_ps(_point.y);
    const __m128 vz = _mm_set1_ps(_point.z);

    Vec4 ret;
    ret.m128 = _mm_mul_ps(vx, m128r0);
    ret.m128 = _mm_fmadd_ps(vy, m128r1, ret.m128);
    ret.m128 = _mm_fmadd_ps(vz, m128r2, ret.m128);
    ret.m128 = _mm_add_ps(ret.m128, m128r3);

    const __m128 w = _mm_permute_ps(ret.m128, _MM_SHUFFLE(3, 3, 3, 3));
    ret.m128       = _mm_div_ps(ret.m128, w);
    return ret.xyz;
#else
    const Vec4 r = Mult({ _point, 1.f });
    if (IsZeroApprox(r.w))
    {
        return r.xyz;
    }
    const float invW = 1.f / r.w;
    return r.xyz * invW;
#endif
}

Vec3 Mtx4::MultVector(
    const Vec3 _vector) const
{
#ifdef JAM_SIMD_SSE
    const __m128 vx = _mm_set1_ps(_vector.x);
    const __m128 vy = _mm_set1_ps(_vector.y);
    const __m128 vz = _mm_set1_ps(_vector.z);

    Vec4 ret;
    ret.m128 = _mm_mul_ps(vx, m128r0);
    ret.m128 = _mm_fmadd_ps(vy, m128r1, ret.m128);
    ret.m128 = _mm_fmadd_ps(vz, m128r2, ret.m128);
    return ret.xyz;
#else
    return Mult(Vec4 { _vector, 0.f }).xyz;
#endif
}

float* Mtx4::GetPtr()
{
    return &m00;
}

const float* Mtx4::GetPtr() const
{
    return &m00;
}

Mtx4 Mtx4::operator+(const Mtx4& _other) const
{
#ifdef JAM_SIMD_SSE
    Mtx4 ret   = {};
    ret.m128r0 = _mm_add_ps(m128r0, _other.m128r0);
    ret.m128r1 = _mm_add_ps(m128r1, _other.m128r1);
    ret.m128r2 = _mm_add_ps(m128r2, _other.m128r2);
    ret.m128r3 = _mm_add_ps(m128r3, _other.m128r3);
    return ret;
#else
    return {
        { m00 + _other.m00, m01 + _other.m01, m02 + _other.m02, m03 + _other.m03 },
        { m10 + _other.m10, m11 + _other.m11, m12 + _other.m12, m13 + _other.m13 },
        { m20 + _other.m20, m21 + _other.m21, m22 + _other.m22, m23 + _other.m23 },
        { m30 + _other.m30, m31 + _other.m31, m32 + _other.m32, m33 + _other.m33 }
    };
#endif
}

Mtx4 Mtx4::operator-(const Mtx4& _other) const
{
#ifdef JAM_SIMD_SSE
    Mtx4 ret   = {};
    ret.m128r0 = _mm_sub_ps(m128r0, _other.m128r0);
    ret.m128r1 = _mm_sub_ps(m128r1, _other.m128r1);
    ret.m128r2 = _mm_sub_ps(m128r2, _other.m128r2);
    ret.m128r3 = _mm_sub_ps(m128r3, _other.m128r3);
    return ret;
#else
    return {
        { m00 - _other.m00, m01 - _other.m01, m02 - _other.m02, m03 - _other.m03 },
        { m10 - _other.m10, m11 - _other.m11, m12 - _other.m12, m13 - _other.m13 },
        { m20 - _other.m20, m21 - _other.m21, m22 - _other.m22, m23 - _other.m23 },
        { m30 - _other.m30, m31 - _other.m31, m32 - _other.m32, m33 - _other.m33 }
    };
#endif
}

Mtx4 Mtx4::operator*(const Mtx4& _other) const
{
#ifdef JAM_SIMD_SSE
    Mtx4 ret = {};

    {
        const __m128 r0 = _mm_permute_ps(m128r0, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 r1 = _mm_permute_ps(m128r0, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 r2 = _mm_permute_ps(m128r0, _MM_SHUFFLE(2, 2, 2, 2));
        const __m128 r3 = _mm_permute_ps(m128r0, _MM_SHUFFLE(3, 3, 3, 3));
        ret.m128r0      = _mm_fmadd_ps(r3, _other.m128r3, _mm_fmadd_ps(r2, _other.m128r2, _mm_fmadd_ps(r1, _other.m128r1, _mm_mul_ps(r0, _other.m128r0))));
    }
    {
        const __m128 r0 = _mm_permute_ps(m128r1, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 r1 = _mm_permute_ps(m128r1, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 r2 = _mm_permute_ps(m128r1, _MM_SHUFFLE(2, 2, 2, 2));
        const __m128 r3 = _mm_permute_ps(m128r1, _MM_SHUFFLE(3, 3, 3, 3));
        ret.m128r1      = _mm_fmadd_ps(r3, _other.m128r3, _mm_fmadd_ps(r2, _other.m128r2, _mm_fmadd_ps(r1, _other.m128r1, _mm_mul_ps(r0, _other.m128r0))));
    }
    {
        const __m128 r0 = _mm_permute_ps(m128r2, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 r1 = _mm_permute_ps(m128r2, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 r2 = _mm_permute_ps(m128r2, _MM_SHUFFLE(2, 2, 2, 2));
        const __m128 r3 = _mm_permute_ps(m128r2, _MM_SHUFFLE(3, 3, 3, 3));
        ret.m128r2      = _mm_fmadd_ps(r3, _other.m128r3, _mm_fmadd_ps(r2, _other.m128r2, _mm_fmadd_ps(r1, _other.m128r1, _mm_mul_ps(r0, _other.m128r0))));
    }
    {
        const __m128 r0 = _mm_permute_ps(m128r3, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 r1 = _mm_permute_ps(m128r3, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 r2 = _mm_permute_ps(m128r3, _MM_SHUFFLE(2, 2, 2, 2));
        const __m128 r3 = _mm_permute_ps(m128r3, _MM_SHUFFLE(3, 3, 3, 3));
        ret.m128r3      = _mm_fmadd_ps(r3, _other.m128r3, _mm_fmadd_ps(r2, _other.m128r2, _mm_fmadd_ps(r1, _other.m128r1, _mm_mul_ps(r0, _other.m128r0))));
    }
    return ret;
#else
    return {
        { r0.x * _other.m00 + r0.y * _other.m10 + r0.z * _other.m20 + r0.w * _other.m30,
         r0.x * _other.m01 + r0.y * _other.m11 + r0.z * _other.m21 + r0.w * _other.m31,
         r0.x * _other.m02 + r0.y * _other.m12 + r0.z * _other.m22 + r0.w * _other.m32,
         r0.x * _other.m03 + r0.y * _other.m13 + r0.z * _other.m23 + r0.w * _other.m33 },
        { r1.x * _other.m00 + r1.y * _other.m10 + r1.z * _other.m20 + r1.w * _other.m30,
         r1.x * _other.m01 + r1.y * _other.m11 + r1.z * _other.m21 + r1.w * _other.m31,
         r1.x * _other.m02 + r1.y * _other.m12 + r1.z * _other.m22 + r1.w * _other.m32,
         r1.x * _other.m03 + r1.y * _other.m13 + r1.z * _other.m23 + r1.w * _other.m33 },
        { r2.x * _other.m00 + r2.y * _other.m10 + r2.z * _other.m20 + r2.w * _other.m30,
         r2.x * _other.m01 + r2.y * _other.m11 + r2.z * _other.m21 + r2.w * _other.m31,
         r2.x * _other.m02 + r2.y * _other.m12 + r2.z * _other.m22 + r2.w * _other.m32,
         r2.x * _other.m03 + r2.y * _other.m13 + r2.z * _other.m23 + r2.w * _other.m33 },
        { r3.x * _other.m00 + r3.y * _other.m10 + r3.z * _other.m20 + r3.w * _other.m30,
         r3.x * _other.m01 + r3.y * _other.m11 + r3.z * _other.m21 + r3.w * _other.m31,
         r3.x * _other.m02 + r3.y * _other.m12 + r3.z * _other.m22 + r3.w * _other.m32,
         r3.x * _other.m03 + r3.y * _other.m13 + r3.z * _other.m23 + r3.w * _other.m33 }
    };
#endif
}

Mtx4 Mtx4::operator*(const float _scalar) const
{
#ifdef JAM_SIMD_SSE
    const __m128 s = _mm_set1_ps(_scalar);

    Mtx4 ret   = {};
    ret.m128r0 = _mm_mul_ps(m128r0, s);
    ret.m128r1 = _mm_mul_ps(m128r1, s);
    ret.m128r2 = _mm_mul_ps(m128r2, s);
    ret.m128r3 = _mm_mul_ps(m128r3, s);
    return ret;
#else
    return {
        { m00 * _scalar, m01 * _scalar, m02 * _scalar, m03 * _scalar },
        { m10 * _scalar, m11 * _scalar, m12 * _scalar, m13 * _scalar },
        { m20 * _scalar, m21 * _scalar, m22 * _scalar, m23 * _scalar },
        { m30 * _scalar, m31 * _scalar, m32 * _scalar, m33 * _scalar }
    };
#endif
}

Mtx4& Mtx4::operator+=(const Mtx4& _other)
{
#ifdef JAM_SIMD_SSE
    m128r0 = _mm_add_ps(m128r0, _other.m128r0);
    m128r1 = _mm_add_ps(m128r1, _other.m128r1);
    m128r2 = _mm_add_ps(m128r2, _other.m128r2);
    m128r3 = _mm_add_ps(m128r3, _other.m128r3);
#else
    m00 += _other.m00;
    m01 += _other.m01;
    m02 += _other.m02;
    m03 += _other.m03;
    m10 += _other.m10;
    m11 += _other.m11;
    m12 += _other.m12;
    m13 += _other.m13;
    m20 += _other.m20;
    m21 += _other.m21;
    m22 += _other.m22;
    m23 += _other.m23;
    m30 += _other.m30;
    m31 += _other.m31;
    m32 += _other.m32;
    m33 += _other.m33;
#endif
    return *this;
}

Mtx4& Mtx4::operator-=(const Mtx4& _other)
{
#ifdef JAM_SIMD_SSE
    m128r0 = _mm_sub_ps(m128r0, _other.m128r0);
    m128r1 = _mm_sub_ps(m128r1, _other.m128r1);
    m128r2 = _mm_sub_ps(m128r2, _other.m128r2);
    m128r3 = _mm_sub_ps(m128r3, _other.m128r3);
#else
    m00 -= _other.m00;
    m01 -= _other.m01;
    m02 -= _other.m02;
    m03 -= _other.m03;
    m10 -= _other.m10;
    m11 -= _other.m11;
    m12 -= _other.m12;
    m13 -= _other.m13;
    m20 -= _other.m20;
    m21 -= _other.m21;
    m22 -= _other.m22;
    m23 -= _other.m23;
    m30 -= _other.m30;
    m31 -= _other.m31;
    m32 -= _other.m32;
    m33 -= _other.m33;
#endif
    return *this;
}

Mtx4& Mtx4::operator*=(const Mtx4& _other)
{
    *this = *this * _other;
    return *this;
}

Mtx4& Mtx4::operator*=(const float _scalar)
{
#ifdef JAM_SIMD_SSE
    const __m128 s = _mm_set1_ps(_scalar);
    m128r0         = _mm_mul_ps(m128r0, s);
    m128r1         = _mm_mul_ps(m128r1, s);
    m128r2         = _mm_mul_ps(m128r2, s);
    m128r3         = _mm_mul_ps(m128r3, s);
#else
    m00 *= _scalar;
    m01 *= _scalar;
    m02 *= _scalar;
    m03 *= _scalar;
    m10 *= _scalar;
    m11 *= _scalar;
    m12 *= _scalar;
    m13 *= _scalar;
    m20 *= _scalar;
    m21 *= _scalar;
    m22 *= _scalar;
    m23 *= _scalar;
    m30 *= _scalar;
    m31 *= _scalar;
    m32 *= _scalar;
    m33 *= _scalar;
#endif
    return *this;
}

Vec4& Mtx4::operator[](const SizeT _index)
{
    JAM_ASSERT(_index < 4, "Mtx4 row index out of range.");
    return (&r0)[_index];
}

Vec4 Mtx4::operator[](const SizeT _index) const
{
    JAM_ASSERT(_index < 4, "Mtx4 row index out of range.");
    return (&r0)[_index];
}

const Mtx4 Mtx4::kIdentity = {
    { 1.f, 0.f, 0.f, 0.f },
    { 0.f, 1.f, 0.f, 0.f },
    { 0.f, 0.f, 1.f, 0.f },
    { 0.f, 0.f, 0.f, 1.f }
};

}   // namespace Jam