// ReSharper disable CppClangTidyBugproneReservedIdentifier
// ReSharper disable CppClangTidyClangDiagnosticUsedButMarkedUnused
// ReSharper disable CppClangTidyClangDiagnosticNestedAnonTypes
// ReSharper disable CppClangTidyClangDiagnosticReservedIdentifier
#include "pch.h"
#include "Quaternion.h"
#include "Matrix.h"

namespace Jam
{

namespace
{

    constexpr float kAlmostOne = 1.f - kEpsilon;

}

const Quat Quat::kIdentity = { 0.f, 0.f, 0.f, 1.f };

Quat::Quat()
{
}

Quat::Quat(
    const float _x,
    const float _y,
    const float _z,
    const float _w)
    : x(_x)
    , y(_y)
    , z(_z)
    , w(_w)
{
}

Quat::Quat(
    const Vec4 _xywz)
    : vec(_xywz)
{
}

Quat Quat::FromAxisAngle(
    const Vec3  _axis,
    const float _rad)
{
    JAM_ASSERT(_axis.IsNormalized(), "Axis must be normalized");

    const float half    = _rad * 0.5f;
    const float sinHalf = Sin(half);
    const float cosHalf = Cos(half);
    return { _axis.x * sinHalf, _axis.y * sinHalf, _axis.z * sinHalf, cosHalf };
}

Quat Quat::FromEuler(
    const Vec3 _euler)
{
    // Y - X - Z order (yaw-pitch-roll)
    const float cy = Cos(_euler.y * 0.5f);
    const float sy = Sin(_euler.y * 0.5f);
    const float cx = Cos(_euler.x * 0.5f);
    const float sx = Sin(_euler.x * 0.5f);
    const float cz = Cos(_euler.z * 0.5f);
    const float sz = Sin(_euler.z * 0.5f);
    return {
        cy * sx * cz + sy * cx * sz,
        sy * cx * cz - cy * sx * sz,
        cy * cx * sz - sy * sx * cz,
        cy * cx * cz + sy * sx * sz
    };
}

Quat Quat::FromRotationMatrix(
    const Mtx4& _mtx)
{
    const float m00   = _mtx.m00;
    const float m11   = _mtx.m11;
    const float m22   = _mtx.m22;
    const float trace = m00 + m11 + m22;

    if (trace > 0.f)
    {
        const float s    = Sqrt(trace + 1.f) * 2.f;
        const float invS = 1.f / s;
        return {
            (_mtx.m21 - _mtx.m12) * invS,
            (_mtx.m02 - _mtx.m20) * invS,
            (_mtx.m10 - _mtx.m01) * invS,
            s * 0.25f
        };
    }
    else if (m00 > m11 && m00 > m22)
    {
        const float s    = Sqrt(1.f + m00 - m11 - m22) * 2.f;
        const float invS = 1.f / s;
        return {
            s * 0.25f,
            (_mtx.m01 + _mtx.m10) * invS,
            (_mtx.m20 + _mtx.m02) * invS,
            (_mtx.m21 - _mtx.m12) * invS
        };
    }
    else if (m11 > m22)
    {
        const float s    = Sqrt(1.f + m11 - m00 - m22) * 2.f;
        const float invS = 1.f / s;
        return {
            (_mtx.m01 + _mtx.m10) * invS,
            s * 0.25f,
            (_mtx.m12 + _mtx.m21) * invS,
            (_mtx.m02 - _mtx.m20) * invS
        };
    }
    else
    {
        const float s    = Sqrt(1.f + m22 - m00 - m11) * 2.f;
        const float invS = 1.f / s;
        return {
            (_mtx.m20 + _mtx.m02) * invS,
            (_mtx.m12 + _mtx.m21) * invS,
            s * 0.25f,
            (_mtx.m10 - _mtx.m01) * invS
        };
    }
}

Quat Quat::FromLookAt(
    const Vec3 _position,
    const Vec3 _focusPoint,
    const Vec3 _up)
{
    const Vec3 f   = (_focusPoint - _position).Normalized();
    const Mtx4 mtx = Mtx4::MakeLookTo(Vec3::kZero, f, _up);
    return FromRotationMatrix(mtx);
}

Quat Quat::FromLookTo(
    const Vec3 _forward,
    const Vec3 _up)
{
    const Mtx4 mtx = Mtx4::MakeLookTo(Vec3::kZero, _forward, _up);
    return FromRotationMatrix(mtx);
}

Quat Quat::FromToRotation(
    const Vec3 _from,
    const Vec3 _to)
{
    JAM_ASSERT(_from.IsNormalized() && _to.IsNormalized(), "FromToRotation requires normalized vectors.");

    const float dot = _from.Dot(_to);
    if (Abs(dot) > kAlmostOne)
    {
        if (dot >= 0.f)
        {
            return kIdentity;   // vectors are already aligned
        }

        // 아무 수직 벡터만 있으면 됨.
        // x요소가 xyz 중에 가장 작으면 x축과 외적, 아니면 y축과 외적. 휴리스틱
        const Vec3 perp = _from.Cross((Abs(_from.x) <= Abs(_from.y) && Abs(_from.x) <= Abs(_from.z)) ? Vec3::kRight : Vec3::kUp).Normalized();
        return FromAxisAngle(perp, kPi);
    }

    const Vec3  c    = _from.Cross(_to);
    const float s    = Sqrt((1.f + dot) * 2.f);
    const float invS = 1.f / s;
    return { c.x * invS, c.y * invS, c.z * invS, s * 0.5f };
}

AxisAngle Quat::ToAxisAngle() const
{
    JAM_ASSERT(IsNormalized(), "ToAxisAngle requires normalized quaternion.");

    if (Abs(w) > kAlmostOne)
    {
        return {};
    }

    const float invSinH = 1.f / Sqrt(1.f - w * w);
    const Vec3  axis    = xyz * invSinH;
    return { axis, 2.f * Acos(w) };
}

Vec3 Quat::ToEuler() const
{
    JAM_ASSERT(IsNormalized(), "ToEuler requires normalized quaternion.");

    // Y - X - Z order (yaw-pitch-roll)
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

bool Quat::IsNormalized() const
{
    return vec.IsNormalized();
}

Quat Quat::Conjugate() const
{
    return { -x, -y, -z, w };
}

Quat Quat::Inverse() const
{
    JAM_ASSERT(IsNormalized(), "Inverse requires normalized quaternion. Use Conjugate for non-normalized quaternions.");
    return Conjugate();
}

float Quat::Dot(const Quat _other) const
{
    return vec.Dot(_other.vec);
}

Quat Quat::Slerp(
    Quat        _q,
    const float _t) const
{
    JAM_ASSERT(IsNormalized() && _q.IsNormalized(), "Slerp requires normalized quaternions.");

    float cosom = Dot(_q);
    if (cosom < 0.f)
    {
        cosom  = -cosom;
        _q.vec = -_q.vec;
    }

    if ((1.f - cosom) > kEpsilon)
    {
        const float omega  = Acos(cosom);
        const float sinom  = Sin(omega);
        const float invSin = 1.f / sinom;
        const float scale0 = Sin((1.f - _t) * omega) * invSin;
        const float scale1 = Sin(_t * omega) * invSin;
        return *this * scale0 + _q * scale1;
    }
    else
    {
        const float scale0 = 1.f - _t;
        const float scale1 = _t;
        return *this * scale0 + _q * scale1;
    }
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
        ret.xyz       = v * s;
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
        ret.xyz       = v * s;
    }

    return ret;
}

Quat Quat::Pow(
    const float _t) const
{
    return (Log() * _t).Exp();
}

float Quat::Angle(
    const Quat _other) const
{
    JAM_ASSERT(IsNormalized() && _other.IsNormalized(), "Angle requires normalized quaternions.");

    const float dot = Clamp(Abs(Dot(_other)), 0.f, 1.f);
    return 2.f * Acos(dot);
}

Quat Quat::SquadSetup(
    const Quat _q0,
    const Quat _q1,
    const Quat _q2)
{
    JAM_ASSERT(_q0.IsNormalized() && _q1.IsNormalized() && _q2.IsNormalized(), "SquadSetup requires normalized quaternions.");

    const Quat invQ1 = _q1.Inverse();
    const Quat log01 = (invQ1 * _q0).Log();
    const Quat log12 = (invQ1 * _q2).Log();
    const Quat inner = (log01 + log12) * -0.25f;
    return (_q1 * inner.Exp());
}

Quat Quat::Squad(
    const Quat  _q1,
    const Quat  _q2,
    const Quat  _s1,
    const Quat  _s2,
    const float _t)
{
    JAM_ASSERT(_q1.IsNormalized() && _q2.IsNormalized() && _s1.IsNormalized() && _s2.IsNormalized(), "Squad requires normalized quaternions.");

    const float blend  = 2.f * _t * (1.f - _t);
    const Quat  slerpA = _q1.Slerp(_q2, _t);
    const Quat  slerpB = _s1.Slerp(_s2, _t);
    return slerpA.Slerp(slerpB, blend);
}

Vec3 Quat::Rotate(
    const Vec3 _v) const
{
    JAM_ASSERT(IsNormalized(), "Rotate requires normalized quaternion.");

    const Vec3 qv = { x, y, z };
    const Vec3 t  = qv.Cross(_v) * 2.f;
    return _v + t * w + qv.Cross(t);
}

bool Quat::IsZeroApprox() const
{
    return vec.IsZeroApprox();
}

bool Quat::IsEqualApprox(
    const Quat _other) const
{
    return vec.IsEqualApprox(_other.vec);
}

float* Quat::GetPtr()
{
    return e.data();
}

const float* Quat::GetPtr() const
{
    return e.data();
}

Quat Quat::operator*(
    const Quat _other) const
{
    const float xx = w * _other.x + x * _other.w + y * _other.z - z * _other.y;
    const float yy = w * _other.y - x * _other.z + y * _other.w + z * _other.x;
    const float zz = w * _other.z + x * _other.y - y * _other.x + z * _other.w;
    const float ww = w * _other.w - x * _other.x - y * _other.y - z * _other.z;
    return { xx, yy, zz, ww };
}

Quat Quat::operator*(
    const float _scalar) const
{
    Quat q = *this;
    q.vec  = vec * _scalar;
    return q;
}

Quat Quat::operator/(
    const float _scalar) const
{
    Quat q = *this;
    q.vec  = vec / _scalar;
    return q;
}

Quat Quat::operator+(
    const Quat _other) const
{
    Quat q = *this;
    q.vec  = q.vec + _other.vec;
    return q;
}

Quat Quat::operator-(
    const Quat _other) const
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

Quat& Quat::operator*=(
    const float _scalar)
{
    vec *= _scalar;
    return *this;
}

Quat& Quat::operator/=(
    const float _scalar)
{
    vec /= _scalar;
    return *this;
}

Quat& Quat::operator+=(
    const Quat _other)
{
    vec += _other.vec;
    return *this;
}

Quat& Quat::operator-=(
    const Quat _other)
{
    vec -= _other.vec;
    return *this;
}

float& Quat::operator[](
    const SizeT _index)
{
    return e[_index];
}

float Quat::operator[](
    const SizeT _index) const
{
    return e[_index];
}

}   // namespace Jam