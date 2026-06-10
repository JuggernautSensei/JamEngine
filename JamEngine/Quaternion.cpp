// ReSharper disable CppClangTidyBugproneReservedIdentifier
// ReSharper disable CppClangTidyClangDiagnosticUsedButMarkedUnused
// ReSharper disable CppClangTidyClangDiagnosticNestedAnonTypes
// ReSharper disable CppClangTidyClangDiagnosticReservedIdentifier
#include "pch.h"
#include "Quaternion.h"
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

Quat Quat::FromAxisAngle(const Vec3 _axis, const float _angleRad)
{
    Quat q = {};
    q.SetAxisAngle(_axis, _angleRad);
    return q;
}

Quat Quat::FromEuler(Vec3 _eulerRad)
{
    Quat q = {};
    q.SetEuler(_eulerRad);
    return q;
}

Quat Quat::FromRotationMatrix(const Mtx4& _mtx)
{
    const float m00   = _mtx.m00;
    const float m11   = _mtx.m11;
    const float m22   = _mtx.m22;
    const float trace = m00 + m11 + m22;

    Quat ret = {};
    if (trace > 0.f)
    {
        const float s    = Sqrt(trace + 1.f) * 2.f;
        const float invS = 1.f / s;

        ret.x = (_mtx.m12 - _mtx.m21) * invS;
        ret.y = (_mtx.m20 - _mtx.m02) * invS;
        ret.z = (_mtx.m01 - _mtx.m10) * invS;
        ret.w = s * 0.25f;
    }
    else if (m00 > m11 && m00 > m22)
    {
        const float s    = Sqrt(1.f + m00 - m11 - m22) * 2.f;
        const float invS = 1.f / s;

        ret.x = s * 0.25f;
        ret.y = (_mtx.m01 + _mtx.m10) * invS;
        ret.z = (_mtx.m20 + _mtx.m02) * invS;
        ret.w = (_mtx.m12 - _mtx.m21) * invS;
    }
    else if (m11 > m22)
    {
        const float s    = Sqrt(1.f + m11 - m00 - m22) * 2.f;
        const float invS = 1.f / s;

        ret.x = (_mtx.m01 + _mtx.m10) * invS;
        ret.y = s * 0.25f;
        ret.z = (_mtx.m12 + _mtx.m21) * invS;
        ret.w = (_mtx.m20 - _mtx.m02) * invS;
    }
    else
    {
        const float s    = Sqrt(1.f + m22 - m00 - m11) * 2.f;
        const float invS = 1.f / s;

        ret.x = (_mtx.m20 + _mtx.m02) * invS;
        ret.y = (_mtx.m12 + _mtx.m21) * invS;
        ret.z = s * 0.25f;
        ret.w = (_mtx.m01 - _mtx.m10) * invS;
    }

    return ret;
}

Quat Quat::FromLookAt(Vec3 _position, Vec3 _point, Vec3 _up)
{
    return FromLookTo(_point - _position, _up);
}

Quat Quat::FromLookTo(Vec3 _forward, Vec3 _up)
{
    const Vec3 f = _forward.Normalized();
    const Vec3 r = _up.Cross(f).Normalized();
    const Vec3 u = f.Cross(r);

    Mtx4 mtx = Mtx4::kIdentity;
    mtx.m00  = r.x;
    mtx.m01  = u.x;
    mtx.m02  = f.x;
    mtx.m10  = r.y;
    mtx.m11  = u.y;
    mtx.m12  = f.y;
    mtx.m20  = r.z;
    mtx.m21  = u.z;
    mtx.m22  = f.z;

    return FromRotationMatrix(mtx);
}

Quat Quat::FromToRotation(Vec3 _from, Vec3 _to)
{
    const Vec3 f = _from.Normalized();
    const Vec3 t = _to.Normalized();

    const float dot = f.Dot(t);
    if (dot < -1.f + kEpsilon)
    {
        Vec3 perp = Vec3::kRight.Cross(f);
        if (perp.LengthSq() < kEpsilon)
        {
            perp = Vec3::kUp.Cross(f);
        }
        return FromAxisAngle(perp.Normalized(), kPi);
    }

    Quat q = {};
    q.x    = f.y * t.z - f.z * t.y;
    q.y    = f.z * t.x - f.x * t.z;
    q.z    = f.x * t.y - f.y * t.x;
    q.w    = 1.f + dot;
    q.Normalize();
    return q;
}

void Quat::SetAxisAngle(const Vec3 _axis, const float _angle)
{
    const Vec3  n       = _axis.Normalized();
    const float half    = _angle * 0.5f;
    const float sinHalf = Sin(half);

    x = n.x * sinHalf;
    y = n.y * sinHalf;
    z = n.z * sinHalf;
    w = Cos(half);
}

AxisAngle Quat::ToAxisAngle() const
{
    if (Abs(w) > 1.f - kEpsilon)
    {
        return {};
    }

    AxisAngle   ret     = {};
    const float invSinH = 1.f / Sqrt(1.f - w * w);
    ret.rad             = 2.f * Acos(w);
    ret.axis            = Vec3 { x, y, z } * invSinH;
    ret.axis.Normalize();
    return ret;
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
    const float sinX = Clamp(2.f * (w * x + y * z), -1.f, 1.f);
    const float sinY = 2.f * (w * y - x * z);
    const float cosY = 1.f - 2.f * (x * x + y * y);
    const float sinZ = 2.f * (w * z - x * y);
    const float cosZ = 1.f - 2.f * (x * x + z * z);
    return { Asin(sinX), Atan2(sinY, cosY), Atan2(sinZ, cosZ) };
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

bool Quat::IsNormalized(const float _epsilon) const
{
    return Abs(LengthSq() - 1.f) < _epsilon;
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

    Quat ret = {};
    ret.vec  = vec.Lerp(q.vec, _t);
    ret.Normalize();
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
    const __m128 v0    = _mm_load_ps(&x);
    const __m128 v1    = _mm_load_ps(&q.x);
    const __m128 w0v   = _mm_set1_ps(w0);
    const __m128 w1v   = _mm_set1_ps(w1);
    const __m128 batch = _mm_add_ps(_mm_mul_ps(v0, w0v), _mm_mul_ps(v1, w1v));

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

Quat Quat::Exp() const
{
    const Vec3  v    = { x, y, z };
    const float vLen = v.Length();
    const float expW = Jam::Exp(w);

    Quat ret = {};
    ret.w    = expW * Cos(vLen);

    if (vLen > kEpsilon)
    {
        const float s = expW * Sin(vLen) / vLen;
        ret.x         = v.x * s;
        ret.y         = v.y * s;
        ret.z         = v.z * s;
    }

    return ret;
}

Quat Quat::Log() const
{
    const float qLen = Length();
    const Vec3  v    = { x, y, z };
    const float vLen = v.Length();

    Quat ret = {};
    ret.w    = Jam::Log(qLen);

    if (vLen > kEpsilon)
    {
        const float s = Acos(Clamp(w / qLen, -1.f, 1.f)) / vLen;
        ret.x         = v.x * s;
        ret.y         = v.y * s;
        ret.z         = v.z * s;
    }

    return ret;
}

Quat Quat::Pow(const float _t) const
{
    return (Log() * _t).Exp();
}

float Quat::Angle(const Quat _other) const
{
    const float dot = Clamp(Abs(Dot(_other)), 0.f, 1.f);
    return 2.f * Acos(dot);
}

Quat Quat::SquadSetup(const Quat _q0, const Quat _q1, const Quat _q2)
{
    const Quat invQ1 = _q1.Inverse();
    const Quat log01 = (invQ1 * _q0).Log();
    const Quat log12 = (invQ1 * _q2).Log();
    const Quat inner = (log01 + log12) * -0.25f;
    return (_q1 * inner.Exp()).Normalized();
}

Quat Quat::Squad(const Quat _q1, const Quat _q2, const Quat _s1, const Quat _s2, const float _t)
{
    const float blend  = 2.f * _t * (1.f - _t);
    const Quat  slerpA = _q1.Slerp(_q2, _t);
    const Quat  slerpB = _s1.Slerp(_s2, _t);
    return slerpA.Slerp(slerpB, blend);
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

float* Quat::GetPtr()
{
    return e.data();
}

const float* Quat::GetPtr() const
{
    return e.data();
}

Quat Quat::operator*(const Quat _other) const
{
    const float xx = w * _other.x + x * _other.w + y * _other.z - z * _other.y;
    const float yy = w * _other.y - x * _other.z + y * _other.w + z * _other.x;
    const float zz = w * _other.z + x * _other.y - y * _other.x + z * _other.w;
    const float ww = w * _other.w - x * _other.x - y * _other.y - z * _other.z;
    return { xx, yy, zz, ww };
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

Quat& Quat::operator*=(const Quat _other)
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

float& Quat::operator[](const SizeT _index)
{
    return e[_index];
}

float Quat::operator[](const SizeT _index) const
{
    return e[_index];
}

}   // namespace Jam