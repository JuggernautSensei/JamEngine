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

Mtx4 Mtx4::CreateOrtho(
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

Mtx4 Mtx4::CreateOrtho(
    const float _width,
    const float _height,
    const float _nearZ,
    const float _farZ)
{
    return CreateOrtho(-_width * 0.5f, _width * 0.5f, _height * 0.5f, -_height * 0.5f, _nearZ, _farZ);
}

Mtx4 Mtx4::CreatePerspectiveFov(
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

Mtx4 Mtx4::CreateTranslation(
    const Vec3 _translation)
{
    return {
        {            1.f,            0.f,            0.f, 0.f },
        {            0.f,            1.f,            0.f, 0.f },
        {            0.f,            0.f,            1.f, 0.f },
        { _translation.x, _translation.y, _translation.z, 1.f }
    };
}

Mtx4 Mtx4::CreateScale(
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

Mtx4 Mtx4::CreateRotationX(
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

Mtx4 Mtx4::CreateRotationY(
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

Mtx4 Mtx4::CreateRotationZ(
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

Mtx4 Mtx4::CreateLookAt(
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

Mtx4 Mtx4::CreateSRT(
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

Mtx4 Mtx4::CreateSRT(Vec3 _scale, Vec3 _euler, Vec3 _translation)
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

Mtx4 Mtx4::CreateSRT(Vec3 _scale, Vec3 _forward, Vec3 _up, Vec3 _translation)
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

Mtx4 Mtx4::CreateInvSRT(
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

Mtx4 Mtx4::CreateInvSRT(Vec3 _scale, Vec3 _euler, Vec3 _translation)
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

Mtx4 Mtx4::CreateInvSRT(Vec3 _scale, Vec3 _forward, Vec3 _up, Vec3 _translation)
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
        const Mtx4 rMtx   = { r0 / sx, r1 / sy, r2 / sz, Vec4::kUnitW };
        *_pRotationOrNull = Quat::FromRotationMatrix(rMtx).Normalized();
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
    const __m128 t0 = _mm_unpacklo_ps(m128s[0], m128s[2]);
    const __m128 t1 = _mm_unpackhi_ps(m128s[0], m128s[2]);
    const __m128 t2 = _mm_unpacklo_ps(m128s[1], m128s[3]);
    const __m128 t3 = _mm_unpackhi_ps(m128s[1], m128s[3]);

    Mtx4 ret     = {};
    ret.m128s[0] = _mm_unpacklo_ps(t0, t2);
    ret.m128s[1] = _mm_unpackhi_ps(t0, t2);
    ret.m128s[2] = _mm_unpacklo_ps(t1, t3);
    ret.m128s[3] = _mm_unpackhi_ps(t1, t3);
    return ret;
#else
    return {
        { m[0][0], m[1][0], m[2][0], m[3][0] },
        { m[0][1], m[1][1], m[2][1], m[3][1] },
        { m[0][2], m[1][2], m[2][2], m[3][2] },
        { m[0][3], m[1][3], m[2][3], m[3][3] }
    };
#endif
}

Mtx4 Mtx4::Inverse() const
{
#ifdef JAM_SIMD_SSE
    __m128 t1 = _mm_shuffle_ps(m128s[0], m128s[1], _MM_SHUFFLE(1, 0, 1, 0));
    __m128 t3 = _mm_shuffle_ps(m128s[0], m128s[1], _MM_SHUFFLE(3, 2, 3, 2));
    __m128 t2 = _mm_shuffle_ps(m128s[2], m128s[3], _MM_SHUFFLE(1, 0, 1, 0));
    __m128 t4 = _mm_shuffle_ps(m128s[2], m128s[3], _MM_SHUFFLE(3, 2, 3, 2));

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

    Mtx4 ret     = Mtx4::kIdentity;
    ret.m128s[0] = _mm_mul_ps(C0, invDet);
    ret.m128s[1] = _mm_mul_ps(C2, invDet);
    ret.m128s[2] = _mm_mul_ps(C4, invDet);
    ret.m128s[3] = _mm_mul_ps(C6, invDet);
    return ret;
#else
    const float m22m33_m23m32 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
    const float m21m33_m23m31 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
    const float m21m32_m22m31 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
    const float m20m33_m23m30 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
    const float m20m32_m22m30 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
    const float m20m31_m21m30 = m[2][0] * m[3][1] - m[2][1] * m[3][0];

    const float m12m33_m13m32 = m[1][2] * m[3][3] - m[1][3] * m[3][2];
    const float m11m33_m13m31 = m[1][1] * m[3][3] - m[1][3] * m[3][1];
    const float m11m32_m12m31 = m[1][1] * m[3][2] - m[1][2] * m[3][1];
    const float m10m33_m13m30 = m[1][0] * m[3][3] - m[1][3] * m[3][0];
    const float m10m32_m12m30 = m[1][0] * m[3][2] - m[1][2] * m[3][0];
    const float m10m31_m11m30 = m[1][0] * m[3][1] - m[1][1] * m[3][0];

    const float m12m23_m13m22 = m[1][2] * m[2][3] - m[1][3] * m[2][2];
    const float m11m23_m13m21 = m[1][1] * m[2][3] - m[1][3] * m[2][1];
    const float m11m22_m12m21 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
    const float m10m23_m13m20 = m[1][0] * m[2][3] - m[1][3] * m[2][0];
    const float m10m22_m12m20 = m[1][0] * m[2][2] - m[1][2] * m[2][0];
    const float m10m21_m11m20 = m[1][0] * m[2][1] - m[1][1] * m[2][0];

    const float inv00 = (m[1][1] * m22m33_m23m32 - m[1][2] * m21m33_m23m31 + m[1][3] * m21m32_m22m31);
    const float inv10 = -(m[1][0] * m22m33_m23m32 - m[1][2] * m20m33_m23m30 + m[1][3] * m20m32_m22m30);
    const float inv20 = (m[1][0] * m21m33_m23m31 - m[1][1] * m20m33_m23m30 + m[1][3] * m20m31_m21m30);
    const float inv30 = -(m[1][0] * m21m32_m22m31 - m[1][1] * m20m32_m22m30 + m[1][2] * m20m31_m21m30);

    const float det = m[0][0] * inv00 + m[0][1] * inv10 + m[0][2] * inv20 + m[0][3] * inv30;

    if (IsZeroApprox(det))
    {
        return kIdentity;
    }

    const float inv01 = -(m[0][1] * m22m33_m23m32 - m[0][2] * m21m33_m23m31 + m[0][3] * m21m32_m22m31);
    const float inv11 = (m[0][0] * m22m33_m23m32 - m[0][2] * m20m33_m23m30 + m[0][3] * m20m32_m22m30);
    const float inv21 = -(m[0][0] * m21m33_m23m31 - m[0][1] * m20m33_m23m30 + m[0][3] * m20m31_m21m30);
    const float inv31 = (m[0][0] * m21m32_m22m31 - m[0][1] * m20m32_m22m30 + m[0][2] * m20m31_m21m30);

    const float inv02 = (m[0][1] * m12m33_m13m32 - m[0][2] * m11m33_m13m31 + m[0][3] * m11m32_m12m31);
    const float inv12 = -(m[0][0] * m12m33_m13m32 - m[0][2] * m10m33_m13m30 + m[0][3] * m10m32_m12m30);
    const float inv22 = (m[0][0] * m11m33_m13m31 - m[0][1] * m10m33_m13m30 + m[0][3] * m10m31_m11m30);
    const float inv32 = -(m[0][0] * m11m32_m12m31 - m[0][1] * m10m32_m12m30 + m[0][2] * m10m31_m11m30);

    const float inv03 = -(m[0][1] * m12m23_m13m22 - m[0][2] * m11m23_m13m21 + m[0][3] * m11m22_m12m21);
    const float inv13 = (m[0][0] * m12m23_m13m22 - m[0][2] * m10m23_m13m20 + m[0][3] * m10m22_m12m20);
    const float inv23 = -(m[0][0] * m11m23_m13m21 - m[0][1] * m10m23_m13m20 + m[0][3] * m10m21_m11m20);
    const float inv33 = (m[0][0] * m11m22_m12m21 - m[0][1] * m10m22_m12m20 + m[0][2] * m10m21_m11m20);

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
    ret.m128 = _mm_mul_ps(vx, m128s[0]);
    ret.m128 = _mm_fmadd_ps(vy, m128s[1], ret.m128);
    ret.m128 = _mm_fmadd_ps(vz, m128s[2], ret.m128);
    ret.m128 = _mm_fmadd_ps(vw, m128s[3], ret.m128);
    return ret;

#else
    return {
        m[0][0] * _vec.x + m[0][1] * _vec.y + m[0][2] * _vec.z + m[0][3] * _vec.w,
        m[1][0] * _vec.x + m[1][1] * _vec.y + m[1][2] * _vec.z + m[1][3] * _vec.w,
        m[2][0] * _vec.x + m[2][1] * _vec.y + m[2][2] * _vec.z + m[2][3] * _vec.w,
        m[3][0] * _vec.x + m[3][1] * _vec.y + m[3][2] * _vec.z + m[3][3] * _vec.w
    };
#endif
}

Vec3 Mtx4::Mult(
    const Vec3 _point) const
{
#ifdef JAM_SIMD_SSE
    const __m128 vx = _mm_set1_ps(_point.x);
    const __m128 vy = _mm_set1_ps(_point.y);
    const __m128 vz = _mm_set1_ps(_point.z);

    Vec4 ret;
    ret.m128 = _mm_mul_ps(vx, m128s[0]);
    ret.m128 = _mm_fmadd_ps(vy, m128s[1], ret.m128);
    ret.m128 = _mm_fmadd_ps(vz, m128s[2], ret.m128);
    ret.m128 = _mm_add_ps(ret.m128, m128s[3]);

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
    ret.m128 = _mm_mul_ps(vx, m128s[0]);
    ret.m128 = _mm_fmadd_ps(vy, m128s[1], ret.m128);
    ret.m128 = _mm_fmadd_ps(vz, m128s[2], ret.m128);
    return ret.xyz;
#else
    return Mult(Vec4 { _vector, 0.f }).xyz;
#endif
}

Mtx4 Mtx4::operator+(const Mtx4& _other) const
{
#ifdef JAM_SIMD_SSE

    Mtx4 ret = {};
    for (int i = 0; i < 4; ++i)
    {
        ret.m128s[i] = _mm_add_ps(m128s[i], _other.m128s[i]);
    }
    return ret;

#else
    Mtx4 ret = {};
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            ret.m[i][j] = m[i][j] + _other.m[i][j];
        }
    }
    return ret;
#endif
}

Mtx4 Mtx4::operator-(const Mtx4& _other) const
{
#ifdef JAM_SIMD_SSE
    Mtx4 ret = {};
    for (int i = 0; i < 4; ++i)
    {
        ret.m128s[i] = _mm_sub_ps(m128s[i], _other.m128s[i]);
    }
    return ret;
#else
    Mtx4 ret = {};
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            ret.m[i][j] = m[i][j] - _other.m[i][j];
        }
    }
    return ret;
#endif
}

Mtx4 Mtx4::operator*(const Mtx4& _other) const
{
#ifdef JAM_SIMD_SSE

    Mtx4 ret = {};
    for (int i = 0; i < 4; ++i)
    {
        __m128 m  = m128s[i];
        __m128 r0 = _mm_permute_ps(m, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 r1 = _mm_permute_ps(m, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 r2 = _mm_permute_ps(m, _MM_SHUFFLE(2, 2, 2, 2));
        __m128 r3 = _mm_permute_ps(m, _MM_SHUFFLE(3, 3, 3, 3));

        ret.m128s[i] = _mm_mul_ps(r0, _other.m128s[0]);
        ret.m128s[i] = _mm_fmadd_ps(r1, _other.m128s[1], ret.m128s[i]);
        ret.m128s[i] = _mm_fmadd_ps(r2, _other.m128s[2], ret.m128s[i]);
        ret.m128s[i] = _mm_fmadd_ps(r3, _other.m128s[3], ret.m128s[i]);
    }
    return ret;

#else
    Mtx4 ret = {};
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            ret.m[i][j] = m[i][0] * _other.m[0][j]
                        + m[i][1] * _other.m[1][j]
                        + m[i][2] * _other.m[2][j]
                        + m[i][3] * _other.m[3][j];
        }
    }

    return ret;
#endif
}

Mtx4 Mtx4::operator*(const float _scalar) const
{
#ifdef JAM_SIMD_SSE
    const __m128 s   = _mm_set1_ps(_scalar);
    Mtx4         ret = {};
    for (int i = 0; i < 4; ++i)
    {
        ret.m128s[i] = _mm_mul_ps(m128s[i], s);
    }
    return ret;
#else
    Mtx4 ret = {};
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            ret.m[i][j] = m[i][j] * _scalar;
        }
    }
    return ret;
#endif
}

Mtx4& Mtx4::operator+=(const Mtx4& _other)
{
#ifdef JAM_SIMD_SSE
    for (int i = 0; i < 4; ++i)
    {
        m128s[i] = _mm_add_ps(m128s[i], _other.m128s[i]);
    }
#else
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            m[i][j] += _other.m[i][j];
        }
    }
#endif
    return *this;
}

Mtx4& Mtx4::operator-=(const Mtx4& _other)
{
#ifdef JAM_SIMD_SSE
    for (int i = 0; i < 4; ++i)
    {
        m128s[i] = _mm_sub_ps(m128s[i], _other.m128s[i]);
    }
#else
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            m[i][j] -= _other.m[i][j];
        }
    }
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
    for (int i = 0; i < 4; ++i)
    {
        m128s[i] = _mm_mul_ps(m128s[i], s);
    }
#else
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            m[i][j] *= _scalar;
        }
    }
#endif
    return *this;
}

const Mtx4 Mtx4::kIdentity = {
    { 1.f, 0.f, 0.f, 0.f },
    { 0.f, 1.f, 0.f, 0.f },
    { 0.f, 0.f, 1.f, 0.f },
    { 0.f, 0.f, 0.f, 1.f }
};

}   // namespace Jam