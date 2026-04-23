// ReSharper disable CppClangTidyBugproneReservedIdentifier
// ReSharper disable CppClangTidyClangDiagnosticUsedButMarkedUnused
// ReSharper disable CppClangTidyClangDiagnosticNestedAnonTypes
// ReSharper disable CppClangTidyClangDiagnosticReservedIdentifier
#include "pch.h"
#include "Quaternion.h"

#include <DirectXTK/SimpleMath.h>

#include "Matrix.h"

namespace Jam
{

const Quat Quat::kIdentity = { 0.f, 0.f, 0.f, 1.f };

Quat::Quat()
{
}

Quat::Quat(const float _x, const float _y, const float _z, const float _w)
    : x(_x)
    , y(_y)
    , z(_z)
    , w(_w)
{
}

Quat::Quat(const Vec3 _axis, const float _angle)
{
    SetAxisAngle(_axis, _angle);
}

Quat::Quat(const Vec3 _euler)
{
    SetEuler(_euler);
}

Quat Quat::FromAxisAngle(const Vec3 _axis, const float _angleRad)
{
    return { _axis, _angleRad };
}

Quat Quat::FromEuler(Vec3 _eulerRad)
{
    Quat q = {};
    q.SetEuler(_eulerRad);
    return q;
}

Quat Quat::FromRotationMatrix(const Mtx4& _mtx)
{
    const float m00   = _mtx.m[0][0];
    const float m11   = _mtx.m[1][1];
    const float m22   = _mtx.m[2][2];
    const float trace = m00 + m11 + m22;

    Quat ret = {};
    if (trace > 0.f)
    {
        const float s    = Sqrt(trace + 1.f) * 2.f;
        const float invS = 1.f / s;

        ret.x = (_mtx.m[1][2] - _mtx.m[2][1]) * invS;
        ret.y = (_mtx.m[2][0] - _mtx.m[0][2]) * invS;
        ret.z = (_mtx.m[0][1] - _mtx.m[1][0]) * invS;
        ret.w = s * 0.25f;
    }
    else if (m00 > m11 && m00 > m22)
    {
        const float s    = Sqrt(1.f + m00 - m11 - m22) * 2.f;
        const float invS = 1.f / s;

        ret.x = s * 0.25f;
        ret.y = (_mtx.m[0][1] + _mtx.m[1][0]) * invS;
        ret.z = (_mtx.m[2][0] + _mtx.m[0][2]) * invS;
        ret.w = (_mtx.m[1][2] - _mtx.m[2][1]) * invS;
    }
    else if (m11 > m22)
    {
        const float s    = Sqrt(1.f + m11 - m00 - m22) * 2.f;
        const float invS = 1.f / s;

        ret.x = (_mtx.m[0][1] + _mtx.m[1][0]) * invS;
        ret.y = s * 0.25f;
        ret.z = (_mtx.m[1][2] + _mtx.m[2][1]) * invS;
        ret.w = (_mtx.m[2][0] - _mtx.m[0][2]) * invS;
    }
    else
    {
        const float s    = Sqrt(1.f + m22 - m00 - m11) * 2.f;
        const float invS = 1.f / s;

        ret.x = (_mtx.m[2][0] + _mtx.m[0][2]) * invS;
        ret.y = (_mtx.m[1][2] + _mtx.m[2][1]) * invS;
        ret.z = s * 0.25f;
        ret.w = (_mtx.m[0][1] - _mtx.m[1][0]) * invS;
    }

    return ret;
}

Quat Quat::FromLookAt(Vec3 _forward, Vec3 _up)
{
    const Vec3 f = _forward.Normalized();
    const Vec3 r = _up.Cross(f).Normalized();
    const Vec3 u = f.Cross(r);

    Mtx4 mtx    = Mtx4::kIdentity;
    mtx.m[0][0] = r.x;
    mtx.m[0][1] = u.x;
    mtx.m[0][2] = f.x;
    mtx.m[1][0] = r.y;
    mtx.m[1][1] = u.y;
    mtx.m[1][2] = f.y;
    mtx.m[2][0] = r.z;
    mtx.m[2][1] = u.z;
    mtx.m[2][2] = f.z;

    return FromRotationMatrix(mtx);
}

void Quat::SetAxisAngle(const Vec3 _axis, const float _angle)
{
    const Vec3  n       = _axis.Normalized();
    const float half    = _angle * 0.5f;
    const float sinHalf = Sin(half);

    xyz = n * sinHalf;
    w   = Cos(half);
}

void Quat::SetEuler(Vec3 _eulerRad)
{
    const float cy = Cos(_eulerRad.y * 0.5f);
    const float sy = Sin(_eulerRad.y * 0.5f);
    const float cx = Cos(_eulerRad.x * 0.5f);
    const float sx = Sin(_eulerRad.x * 0.5f);
    const float cz = Cos(_eulerRad.z * 0.5f);
    const float sz = Sin(_eulerRad.z * 0.5f);

    x = cy * sx * cz + sy * cx * sz;
    y = sy * cx * cz - cy * sx * sz;
    z = cy * cx * sz - sy * sx * cz;
    w = cy * cx * cz + sy * sx * sz;
}

Vec3 Quat::ToEuler() const
{
    Vec3 euler = {};

    float sx = 2.f * (w * x - y * z);
    sx       = Clamp(sx, -1.f, 1.f);
    euler.x  = Asin(sx);

    const float sy = 2.f * (w * y + x * z);
    const float cy = 1.f - 2.f * (x * x + y * y);
    euler.y        = Atan2(sy, cy);

    const float sz = 2.f * (w * z + x * y);
    const float cz = 1.f - 2.f * (x * x + z * z);
    euler.z        = Atan2(sz, cz);

    return euler;
}

AxisAngle Quat::ToAxisAngle() const
{
    AxisAngle aa = {};
    aa.rad       = 2.f * Acos(w);
    aa.axis      = xyz.Normalized();
    return aa;
}

float Quat::Length() const
{
    return Sqrt(LengthSq());
}

float Quat::LengthSq() const
{
    return vec.LengthSq();
}

void Quat::Normalize()
{
    vec.Normalize();
}

Quat Quat::Normalized() const
{
    Quat ret = *this;
    ret.Normalize();
    return ret;
}

Quat Quat::Conjugate() const
{
    return { -x, -y, -z, w };
}

Quat Quat::Inverse() const
{
    return Conjugate();
}

float Quat::Dot(const Quat _other) const
{
    return vec.Dot(_other.vec);
}

Quat Quat::Lerp(const Quat _other, const float _t) const
{
    Quat  q   = _other;
    float dot = Dot(q);
    if (dot < 0.f)
    {
        q = -q;
    }

    Quat ret = Quat::kIdentity;
    ret.vec  = vec.Lerp(q.vec, _t);
    return ret;
}

Quat Quat::Slerp(const Quat _other, const float _t) const
{
    Quat  q   = _other;
    float dot = Dot(q);
    if (dot < 0.f)
    {
        q   = -q;
        dot = -dot;
    }

    dot                  = Clamp(dot, -1.f, 1.f);
    const float theta    = Acos(dot);
    const float sinTheta = Sin(theta);

    if (sinTheta < kEpsilon)
    {
        return Lerp(q, _t);
    }

    const float w0 = Sin((1.f - _t) * theta) / sinTheta;
    const float w1 = Sin(_t * theta) / sinTheta;

#ifdef JAM_SIMD_SSE
    const __m128 w0v   = _mm_set1_ps(w0);
    const __m128 w1v   = _mm_set1_ps(w1);
    __m128       batch = _mm_mul_ps(m128, w0v);
    batch              = _mm_fmadd_ps(q.m128, w1v, batch);

    Quat ret = {};
    _mm_store_ps(&ret.x, batch);
    return ret;
#else
    Quat ret = {};
    ret.x    = x * w0 + q.x * w1;
    ret.y    = y * w0 + q.y * w1;
    ret.z    = z * w0 + q.z * w1;
    ret.w    = w * w0 + q.w * w1;
    return ret;
#endif
}

Vec3 Quat::Rotate(const Vec3 _v) const
{
    const Vec3 qv = { x, y, z };
    const Vec3 t  = qv.Cross(_v) * 2.f;
    return _v + t * w + qv.Cross(t);
}

bool Quat::IsZeroApprox(const float _epsilon) const
{
    return vec.IsZeroApprox(_epsilon);
}

bool Quat::IsEqualApprox(const Quat _other, const float _epsilon) const
{
    return vec.IsEqualApprox(_other.vec, _epsilon);
}
Quat Quat::operator*(const Quat _other) const
{
#ifdef JAM_SIMD_SSE

    __m128 w1 = _mm_set1_ps(w);
    __m128 x1 = _mm_set1_ps(x);
    __m128 y1 = _mm_set1_ps(y);
    __m128 z1 = _mm_set1_ps(z);

    __m128 w_part = _mm_mul_ps(w1, _other.m128);
    __m128 x_perm = _mm_permute_ps(_other.m128, _MM_SHUFFLE(0, 1, 2, 3));
    __m128 x_part = _mm_mul_ps(x1, x_perm);
    x_part        = _mm_xor_ps(x_part, _mm_set_ps(-0.f, 0.f, -0.f, 0.f));

    __m128 y_perm = _mm_permute_ps(_other.m128, _MM_SHUFFLE(1, 0, 3, 2));
    __m128 y_part = _mm_mul_ps(y1, y_perm);
    y_part        = _mm_xor_ps(y_part, _mm_set_ps(-0.f, -0.f, 0.f, 0.f));

    __m128 z_perm = _mm_permute_ps(_other.m128, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 z_part = _mm_mul_ps(z1, z_perm);
    z_part        = _mm_xor_ps(z_part, _mm_set_ps(-0.f, 0.f, 0.f, -0.f));

    __m128 ret = w_part;
    ret        = _mm_add_ps(ret, x_part);
    ret        = _mm_add_ps(ret, y_part);
    ret        = _mm_add_ps(ret, z_part);

    Quat result;
    result.m128 = ret;
    return result;

#else
    const float xx = w * _other.x + x * _other.w + y * _other.z - z * _other.y;
    const float yy = w * _other.y - x * _other.z + y * _other.w + z * _other.x;
    const float zz = w * _other.z + x * _other.y - y * _other.x + z * _other.w;
    const float ww = w * _other.w - x * _other.x - y * _other.y - z * _other.z;
    return { xx, yy, zz, ww };
#endif
}
Quat Quat::operator*(const float _scalar) const
{
    Quat q = *this;
    q.vec  = vec * _scalar;
    return q;
}

Quat Quat::operator/(const float _scalar) const
{
    Quat q = *this;
    q.vec  = vec / _scalar;
    return q;
}

Quat Quat::operator+(const Quat _other) const
{
    Quat q = *this;
    q.vec  = q.vec + _other.vec;
    return q;
}

Quat Quat::operator-(const Quat _other) const
{
    Quat q = *this;
    q.vec  = q.vec - _other.vec;
    return q;
}

Quat Quat::operator-() const
{
    Quat q = *this;
    q.vec  = -q.vec;
    return q;
}

Quat& Quat::operator*=(
    const Quat _other)
{
    *this = *this * _other;
    return *this;
}

Quat& Quat::operator*=(const float _scalar)
{
    vec *= _scalar;
    return *this;
}

Quat& Quat::operator/=(const float _scalar)
{
    vec /= _scalar;
    return *this;
}

Quat& Quat::operator+=(const Quat _other)
{
    vec += _other.vec;
    return *this;
}

Quat& Quat::operator-=(const Quat _other)
{
    vec -= _other.vec;
    return *this;
}

}   // namespace Jam