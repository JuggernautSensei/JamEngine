// ReSharper disable CppClangTidyBugproneReservedIdentifier
// ReSharper disable CppClangTidyClangDiagnosticUsedButMarkedUnused
// ReSharper disable CppClangTidyClangDiagnosticNestedAnonTypes
// ReSharper disable CppClangTidyClangDiagnosticReservedIdentifier
// ReSharper disable CppClangTidyBugproneNarrowingConversions
#pragma once

namespace Jam
{

struct Vec2i
{
    constexpr Vec2i() = default;
    constexpr Vec2i(const Int32 _x, const Int32 _y)
        : x(_x)
        , y(_y)

    {
    }

    constexpr explicit Vec2i(const Int32 _xy)
        : x(_xy)
        , y(_xy)

    {
    }

    [[nodiscard]] constexpr Vec2i Abs() const

    {
        return { x > 0 ? x : -x, y > 0 ? y : -y };
    }

    [[nodiscard]] constexpr Vec2i Min(const Vec2i _other) const

    {
        return { Jam::Min(x, _other.x), Jam::Min(y, _other.y) };
    }

    [[nodiscard]] constexpr Vec2i Max(const Vec2i _other) const

    {
        return { Jam::Max(x, _other.x), Jam::Max(y, _other.y) };
    }

    [[nodiscard]] constexpr Vec2i Clamp(const Vec2i _min, const Vec2i _max) const

    {
        return { Jam::Clamp(x, _min.x, _max.x), Jam::Clamp(y, _min.y, _max.y) };
    }

    constexpr Vec2i& operator+=(const Vec2i _other)

    {
        x += _other.x;
        y += _other.y;
        return *this;
    }

    constexpr Vec2i& operator-=(const Vec2i _other)

    {
        x -= _other.x;
        y -= _other.y;
        return *this;
    }

    constexpr Vec2i& operator*=(const Vec2i _other)

    {
        x *= _other.x;
        y *= _other.y;
        return *this;
    }

    constexpr Vec2i& operator/=(const Vec2i _other)

    {
        x /= _other.x;
        y /= _other.y;
        return *this;
    }

    constexpr Vec2i& operator*=(const Int32 _scalar)

    {
        x *= _scalar;
        y *= _scalar;
        return *this;
    }

    constexpr Vec2i& operator/=(const Int32 _scalar)

    {
        x /= _scalar;
        y /= _scalar;
        return *this;
    }

    [[nodiscard]] constexpr Vec2i operator+(const Vec2i _other) const

    {
        return { x + _other.x, y + _other.y };
    }

    [[nodiscard]] constexpr Vec2i operator-(const Vec2i _other) const

    {
        return { x - _other.x, y - _other.y };
    }

    [[nodiscard]] constexpr Vec2i operator*(const Vec2i _other) const

    {
        return { x * _other.x, y * _other.y };
    }

    [[nodiscard]] constexpr Vec2i operator/(const Vec2i _other) const

    {
        return { x / _other.x, y / _other.y };
    }

    [[nodiscard]] constexpr Vec2i operator*(const Int32 _scalar) const

    {
        return { x * _scalar, y * _scalar };
    }

    [[nodiscard]] constexpr Vec2i operator/(const Int32 _scalar) const

    {
        return { x / _scalar, y / _scalar };
    }

    [[nodiscard]] constexpr Vec2i operator-() const

    {
        return { -x, -y };
    }

    [[nodiscard]] constexpr bool operator==(const Vec2i _other) const

    {
        return x == _other.x && y == _other.y;
    }

    [[nodiscard]] constexpr bool operator!=(const Vec2i _other) const

    {
        return !(*this == _other);
    }

    const static Vec2i kZero;
    const static Vec2i kOne;
    const static Vec2i kUnitX;
    const static Vec2i kUnitY;

    union
    {
        struct
        {
            Int32 x, y;
        };

        struct
        {
            Int32 u, v;
        };

        struct
        {
            Int32 width, height;
        };

        Int32 e[2] = {};
    };
};

struct Vec2
{
    constexpr Vec2() = default;
    constexpr Vec2(const float _x, const float _y)
        : x(_x)
        , y(_y)

    {
    }

    constexpr explicit Vec2(const float _xy)
        : x(_xy)
        , y(_xy)

    {
    }

    [[nodiscard]] constexpr Vec2 Lerp(const Vec2 _other, const float _t) const

    {
        return { Jam::Lerp(x, _other.x, _t), Jam::Lerp(y, _other.y, _t) };
    }

    [[nodiscard]] constexpr Vec2 Abs() const

    {
        return { Jam::Abs(x), Jam::Abs(y) };
    }

    [[nodiscard]] float Length() const

    {
        return Sqrt(x * x + y * y);
    }

    [[nodiscard]] constexpr float LengthSq() const

    {
        return x * x + y * y;
    }

    [[nodiscard]] float Distance(const Vec2 _other) const

    {
        return (*this - _other).Length();
    }

    [[nodiscard]] constexpr float DistanceSq(const Vec2 _other) const

    {
        return (*this - _other).LengthSq();
    }

    [[nodiscard]] constexpr float Dot(const Vec2 _other) const

    {
        return x * _other.x + y * _other.y;
    }

    [[nodiscard]] constexpr float Cross(const Vec2 _other) const

    {
        return x * _other.y - y * _other.x;
    }

    void Normalize()

    {
        const float invLen = 1.f / Sqrt(LengthSq());
        x *= invLen;
        y *= invLen;
    }

    [[nodiscard]] Vec2 Normalized() const

    {
        Vec2 v = *this;
        v.Normalize();
        return v;
    }

    [[nodiscard]] constexpr Vec2 Min(const Vec2 _other) const

    {
        return { Jam::Min(x, _other.x), Jam::Min(y, _other.y) };
    }

    [[nodiscard]] constexpr Vec2 Max(const Vec2 _other) const

    {
        return { Jam::Max(x, _other.x), Jam::Max(y, _other.y) };
    }

    [[nodiscard]] constexpr Vec2 Clamp(const Vec2 _min, const Vec2 _max) const

    {
        return { Jam::Clamp(x, _min.x, _max.x), Jam::Clamp(y, _min.y, _max.y) };
    }

    [[nodiscard]] constexpr Vec2 Rcp() const

    {
        return { 1.f / x, 1.f / y };
    }

    [[nodiscard]] constexpr Vec2 RcpSafe() const

    {
        float xx = IsZeroApprox(x) ? 0.f : 1.f / x;
        float yy = IsZeroApprox(y) ? 0.f : 1.f / y;
        return { xx, yy };
    }

    [[nodiscard]] constexpr Vec2 Reflect(const Vec2 _normal) const

    {
        return *this - _normal * (2.f * Dot(_normal));
    }

    [[nodiscard]] Vec2 Refract(const Vec2 _normal, const float _eta) const

    {
        const float nDotV = Dot(_normal);
        const float k     = 1.f - _eta * _eta * (1.f - nDotV * nDotV);
        if (k < 0.f)

        {
            return Vec2::kZero;
        }
        return *this * _eta + _normal * (_eta * nDotV - Sqrt(k));
    }

    [[nodiscard]] constexpr Vec2 Project(const Vec2 _onto) const

    {
        return _onto * (Dot(_onto) / _onto.LengthSq());
    }

    [[nodiscard]] float Angle(const Vec2 _other) const

    {
        return Acos(Jam::Clamp(Dot(_other) / (Length() * _other.Length()), -1.f, 1.f));
    }

    [[nodiscard]] constexpr bool IsZeroApprox(const float _epsilon = kEpsilon) const

    {
        return Jam::IsZeroApprox(x, _epsilon) && Jam::IsZeroApprox(y, _epsilon);
    }

    [[nodiscard]] constexpr bool IsEqualApprox(const Vec2 _other, const float _epsilon = kEpsilon) const

    {
        return Jam::IsEqualApprox(x, _other.x, _epsilon) && Jam::IsEqualApprox(y, _other.y, _epsilon);
    }

    constexpr Vec2& operator+=(const Vec2 _other)

    {
        x += _other.x;
        y += _other.y;
        return *this;
    }

    constexpr Vec2& operator-=(const Vec2 _other)

    {
        x -= _other.x;
        y -= _other.y;
        return *this;
    }

    constexpr Vec2& operator*=(const Vec2 _other)

    {
        x *= _other.x;
        y *= _other.y;
        return *this;
    }

    constexpr Vec2& operator/=(const Vec2 _other)

    {
        x /= _other.x;
        y /= _other.y;
        return *this;
    }

    constexpr Vec2& operator*=(const float _scalar)

    {
        x *= _scalar;
        y *= _scalar;
        return *this;
    }

    constexpr Vec2& operator/=(const float _scalar)

    {
        x /= _scalar;
        y /= _scalar;
        return *this;
    }

    [[nodiscard]] constexpr Vec2 operator+(const Vec2 _other) const

    {
        return { x + _other.x, y + _other.y };
    }

    [[nodiscard]] constexpr Vec2 operator-(const Vec2 _other) const

    {
        return { x - _other.x, y - _other.y };
    }

    [[nodiscard]] constexpr Vec2 operator*(const Vec2 _other) const

    {
        return { x * _other.x, y * _other.y };
    }

    [[nodiscard]] constexpr Vec2 operator/(const Vec2 _other) const

    {
        return { x / _other.x, y / _other.y };
    }

    [[nodiscard]] constexpr Vec2 operator*(const float _scalar) const

    {
        return { x * _scalar, y * _scalar };
    }

    [[nodiscard]] constexpr Vec2 operator/(const float _scalar) const

    {
        return { x / _scalar, y / _scalar };
    }

    [[nodiscard]] constexpr Vec2 operator-() const

    {
        return { -x, -y };
    }

    const static Vec2 kZero;
    const static Vec2 kOne;
    const static Vec2 kUnitX;
    const static Vec2 kUnitY;
    const static Vec2 kRight;
    const static Vec2 kUp;

    union

    {
        struct
        {
            float x, y;
        };

        struct
        {
            float u, v;
        };

        struct
        {
            float width, height;
        };

        float e[2] = {};
    };
};

struct Vec3i
{
    constexpr Vec3i() = default;
    constexpr Vec3i(const Int32 _x, const Int32 _y, const Int32 _z)
        : x(_x)
        , y(_y)
        , z(_z)
    {
    }

    constexpr explicit Vec3i(const Int32 _xyz)
        : x(_xyz)
        , y(_xyz)
        , z(_xyz)
    {
    }

    constexpr Vec3i(const Vec2i _xy, const Int32 _z)
        : x(_xy.x)
        , y(_xy.y)
        , z(_z)
    {
    }

    [[nodiscard]] constexpr Vec3i Abs() const
    {
        return { x > 0 ? x : -x, y > 0 ? y : -y, z > 0 ? z : -z };
    }

    [[nodiscard]] constexpr Vec3i Min(const Vec3i _other) const
    {
        return { Jam::Min(x, _other.x), Jam::Min(y, _other.y), Jam::Min(z, _other.z) };
    }

    [[nodiscard]] constexpr Vec3i Max(const Vec3i _other) const
    {
        return { Jam::Max(x, _other.x), Jam::Max(y, _other.y), Jam::Max(z, _other.z) };
    }

    [[nodiscard]] constexpr Vec3i Clamp(const Vec3i _min, const Vec3i _max) const
    {
        return { Jam::Clamp(x, _min.x, _max.x), Jam::Clamp(y, _min.y, _max.y), Jam::Clamp(z, _min.z, _max.z) };
    }

    constexpr Vec3i& operator+=(const Vec3i _other)
    {
        x += _other.x;
        y += _other.y;
        z += _other.z;
        return *this;
    }

    constexpr Vec3i& operator-=(const Vec3i _other)
    {
        x -= _other.x;
        y -= _other.y;
        z -= _other.z;
        return *this;
    }

    constexpr Vec3i& operator*=(const Vec3i _other)
    {
        x *= _other.x;
        y *= _other.y;
        z *= _other.z;
        return *this;
    }

    constexpr Vec3i& operator/=(const Vec3i _other)
    {
        x /= _other.x;
        y /= _other.y;
        z /= _other.z;
        return *this;
    }

    constexpr Vec3i& operator*=(const Int32 _scalar)
    {
        x *= _scalar;
        y *= _scalar;
        z *= _scalar;
        return *this;
    }

    constexpr Vec3i& operator/=(const Int32 _scalar)
    {
        x /= _scalar;
        y /= _scalar;
        z /= _scalar;
        return *this;
    }

    [[nodiscard]] constexpr Vec3i operator+(const Vec3i _other) const
    {
        return { x + _other.x, y + _other.y, z + _other.z };
    }

    [[nodiscard]] constexpr Vec3i operator-(const Vec3i _other) const
    {
        return { x - _other.x, y - _other.y, z - _other.z };
    }

    [[nodiscard]] constexpr Vec3i operator*(const Vec3i _other) const
    {
        return { x * _other.x, y * _other.y, z * _other.z };
    }

    [[nodiscard]] constexpr Vec3i operator/(const Vec3i _other) const
    {
        return { x / _other.x, y / _other.y, z / _other.z };
    }

    [[nodiscard]] constexpr Vec3i operator*(const Int32 _scalar) const
    {
        return { x * _scalar, y * _scalar, z * _scalar };
    }

    [[nodiscard]] constexpr Vec3i operator/(const Int32 _scalar) const
    {
        return { x / _scalar, y / _scalar, z / _scalar };
    }

    [[nodiscard]] constexpr Vec3i operator-() const
    {
        return { -x, -y, -z };
    }

    [[nodiscard]] constexpr bool operator==(const Vec3i _other) const
    {
        return x == _other.x && y == _other.y && z == _other.z;
    }

    [[nodiscard]] constexpr bool operator!=(const Vec3i _other) const
    {
        return !(*this == _other);
    }

    const static Vec3i kZero;
    const static Vec3i kOne;
    const static Vec3i kUnitX;
    const static Vec3i kUnitY;
    const static Vec3i kUnitZ;

    union
    {
        struct
        {
            Int32 x, y, z;
        };

        struct
        {
            Int32 r, g, b;
        };

        struct
        {
            Int32 width, height, depth;
        };

        struct
        {
            Vec2i xy;
            Int32 z_;
        };

        struct
        {
            Int32 x_;
            Vec2i yz;
        };

        struct
        {
            Vec2i rg;
            Int32 b_;
        };

        struct
        {
            Int32 r_;
            Vec2i gb;
        };

        struct
        {
            Vec2i widthHeight;
            Int32 depth_;
        };

        struct
        {
            Int32 width_;
            Vec2i heightDepth;
        };

        Int32 e[3] = {};
    };
};

struct Vec3
{
    constexpr Vec3() = default;
    constexpr Vec3(const float _x, const float _y, const float _z)
        : x(_x)
        , y(_y)
        , z(_z)

    {
    }

    constexpr explicit Vec3(const float _xyz)
        : x(_xyz)
        , y(_xyz)
        , z(_xyz)

    {
    }

    constexpr Vec3(const Vec2 _xy, const float _z)
        : x(_xy.x)
        , y(_xy.y)
        , z(_z)

    {
    }

    [[nodiscard]] constexpr Vec3 Lerp(const Vec3 _other, const float _t) const

    {
        return { Jam::Lerp(x, _other.x, _t), Jam::Lerp(y, _other.y, _t), Jam::Lerp(z, _other.z, _t) };
    }

    [[nodiscard]] constexpr Vec3 Abs() const

    {
        return { Jam::Abs(x), Jam::Abs(y), Jam::Abs(z) };
    }

    [[nodiscard]] float Length() const

    {
        return Sqrt(x * x + y * y + z * z);
    }

    [[nodiscard]] constexpr float LengthSq() const

    {
        return x * x + y * y + z * z;
    }

    [[nodiscard]] float Distance(const Vec3 _other) const

    {
        return (*this - _other).Length();
    }

    [[nodiscard]] constexpr float DistanceSq(const Vec3 _other) const

    {
        return (*this - _other).LengthSq();
    }

    [[nodiscard]] constexpr float Dot(const Vec3 _other) const

    {
        return x * _other.x + y * _other.y + z * _other.z;
    }

    void Normalize()
    {
        const float lenSq  = LengthSq();
        const float invLen = RSqrt(lenSq);
        x *= invLen;
        y *= invLen;
        z *= invLen;
    }

    [[nodiscard]] Vec3 Normalized() const
    {
        const float lenSq  = LengthSq();
        const float invLen = RSqrt(lenSq);
        return { x * invLen, y * invLen, z * invLen };
    }

    [[nodiscard]] constexpr Vec3 Cross(const Vec3 _other) const
    {
        return {
            y * _other.z - z * _other.y,
            z * _other.x - x * _other.z,
            x * _other.y - y * _other.x,
        };
    }

    [[nodiscard]] constexpr Vec3 Min(const Vec3 _other) const

    {
        return { Jam::Min(x, _other.x), Jam::Min(y, _other.y), Jam::Min(z, _other.z) };
    }

    [[nodiscard]] constexpr Vec3 Max(const Vec3 _other) const

    {
        return { Jam::Max(x, _other.x), Jam::Max(y, _other.y), Jam::Max(z, _other.z) };
    }

    [[nodiscard]] constexpr Vec3 Clamp(const Vec3 _min, const Vec3 _max) const

    {
        return { Jam::Clamp(x, _min.x, _max.x), Jam::Clamp(y, _min.y, _max.y), Jam::Clamp(z, _min.z, _max.z) };
    }

    [[nodiscard]] constexpr Vec3 Rcp() const

    {
        return { 1.f / x, 1.f / y, 1.f / z };
    }

    [[nodiscard]] constexpr Vec3 RcpSafe() const

    {
        float xx = IsZeroApprox(x) ? 0.f : 1.f / x;
        float yy = IsZeroApprox(y) ? 0.f : 1.f / y;
        float zz = IsZeroApprox(z) ? 0.f : 1.f / z;
        return { xx, yy, zz };
    }

    [[nodiscard]] constexpr Vec3 Reflect(const Vec3 _normal) const

    {
        return *this - _normal * (2.f * Dot(_normal));
    }

    [[nodiscard]] Vec3 Refract(const Vec3 _normal, const float _eta) const

    {
        const float dotNV = Dot(_normal);
        const float k     = 1.f - _eta * _eta * (1.f - dotNV * dotNV);
        if (k < 0.f)

        {
            return Vec3::kZero;
        }
        return *this * _eta + _normal * (_eta * dotNV - Sqrt(k));
    }

    [[nodiscard]] constexpr Vec3 Project(const Vec3 _onto) const

    {
        return _onto * (Dot(_onto) / _onto.LengthSq());
    }

    [[nodiscard]] float Angle(const Vec3 _other) const

    {
        return Acos(Jam::Clamp(Dot(_other) / (Length() * _other.Length()), -1.f, 1.f));
    }

    [[nodiscard]] constexpr bool IsZeroApprox(const float _epsilon = kEpsilon) const

    {
        return Jam::IsZeroApprox(x, _epsilon) && Jam::IsZeroApprox(y, _epsilon) && Jam::IsZeroApprox(z, _epsilon);
    }

    [[nodiscard]] constexpr bool IsEqualApprox(const Vec3 _other, const float _epsilon = kEpsilon) const

    {
        return Jam::IsEqualApprox(x, _other.x, _epsilon) && Jam::IsEqualApprox(y, _other.y, _epsilon) && Jam::IsEqualApprox(z, _other.z, _epsilon);
    }

    constexpr Vec3& operator+=(const Vec3 _other)

    {
        x += _other.x;
        y += _other.y;
        z += _other.z;
        return *this;
    }

    constexpr Vec3& operator-=(const Vec3 _other)

    {
        x -= _other.x;
        y -= _other.y;
        z -= _other.z;
        return *this;
    }

    constexpr Vec3& operator*=(const Vec3 _other)

    {
        x *= _other.x;
        y *= _other.y;
        z *= _other.z;
        return *this;
    }

    constexpr Vec3& operator/=(const Vec3 _other)

    {
        x /= _other.x;
        y /= _other.y;
        z /= _other.z;
        return *this;
    }

    constexpr Vec3& operator*=(const float _scalar)

    {
        x *= _scalar;
        y *= _scalar;
        z *= _scalar;
        return *this;
    }

    constexpr Vec3& operator/=(const float _scalar)

    {
        x /= _scalar;
        y /= _scalar;
        z /= _scalar;
        return *this;
    }

    [[nodiscard]] constexpr Vec3 operator+(const Vec3 _other) const

    {
        return { x + _other.x, y + _other.y, z + _other.z };
    }

    [[nodiscard]] constexpr Vec3 operator-(const Vec3 _other) const

    {
        return { x - _other.x, y - _other.y, z - _other.z };
    }

    [[nodiscard]] constexpr Vec3 operator*(const Vec3 _other) const

    {
        return { x * _other.x, y * _other.y, z * _other.z };
    }

    [[nodiscard]] constexpr Vec3 operator/(const Vec3 _other) const

    {
        return { x / _other.x, y / _other.y, z / _other.z };
    }

    [[nodiscard]] constexpr Vec3 operator*(const float _scalar) const
    {
        return { x * _scalar, y * _scalar, z * _scalar };
    }

    [[nodiscard]] constexpr Vec3 operator/(const float _scalar) const

    {
        return { x / _scalar, y / _scalar, z / _scalar };
    }

    [[nodiscard]] constexpr Vec3 operator-() const

    {
        return { -x, -y, -z };
    }

    const static Vec3 kZero;
    const static Vec3 kOne;
    const static Vec3 kUnitX;
    const static Vec3 kUnitY;
    const static Vec3 kUnitZ;
    const static Vec3 kRight;
    const static Vec3 kUp;
    const static Vec3 kForward;

    union

    {
        struct

        {
            float x, y, z;
        };

        struct

        {
            float r, g, b;
        };

        struct

        {
            float u, v, w;
        };

        struct

        {
            float pitch, yaw, roll;
        };

        struct

        {
            Vec2  xy;
            float z_;
        };

        struct

        {
            float x_;
            Vec2  yz;
        };

        struct

        {
            Vec2  uv;
            float w_;
        };

        struct

        {
            float u_;
            Vec2  vw;
        };

        struct

        {
            Vec2  rg;
            float b_;
        };

        struct

        {
            float r_;
            Vec2  gb;
        };

        struct

        {
            Vec2  pitchYaw;
            float roll_;
        };

        struct

        {
            float pitch_;
            Vec2  yawRoll;
        };

        float e[3] = {};
    };
};

struct Vec4i
{
    constexpr Vec4i() = default;
    constexpr Vec4i(const Int32 _x, const Int32 _y, const Int32 _z, const Int32 _w)
        : x(_x)
        , y(_y)
        , z(_z)
        , w(_w)
    {
    }

    constexpr explicit Vec4i(const Int32 _xyzw)
        : x(_xyzw)
        , y(_xyzw)
        , z(_xyzw)
        , w(_xyzw)
    {
    }

    constexpr Vec4i(const Vec2i _xy, const Int32 _z, const Int32 _w)
        : x(_xy.x)
        , y(_xy.y)
        , z(_z)
        , w(_w)
    {
    }

    constexpr Vec4i(const Vec3i _xyz, const Int32 _w)
        : x(_xyz.x)
        , y(_xyz.y)
        , z(_xyz.z)
        , w(_w)
    {
    }

    [[nodiscard]] constexpr Vec4i Abs() const
    {
        return { x > 0 ? x : -x, y > 0 ? y : -y, z > 0 ? z : -z, w > 0 ? w : -w };
    }

    [[nodiscard]] constexpr Vec4i Min(const Vec4i _other) const
    {
        return { Jam::Min(x, _other.x), Jam::Min(y, _other.y), Jam::Min(z, _other.z), Jam::Min(w, _other.w) };
    }

    [[nodiscard]] constexpr Vec4i Max(const Vec4i _other) const
    {
        return { Jam::Max(x, _other.x), Jam::Max(y, _other.y), Jam::Max(z, _other.z), Jam::Max(w, _other.w) };
    }

    [[nodiscard]] constexpr Vec4i Clamp(const Vec4i _min, const Vec4i _max) const
    {
        return { Jam::Clamp(x, _min.x, _max.x), Jam::Clamp(y, _min.y, _max.y), Jam::Clamp(z, _min.z, _max.z), Jam::Clamp(w, _min.w, _max.w) };
    }

    constexpr Vec4i& operator+=(const Vec4i _other)
    {
        x += _other.x;
        y += _other.y;
        z += _other.z;
        w += _other.w;
        return *this;
    }

    constexpr Vec4i& operator-=(const Vec4i _other)
    {
        x -= _other.x;
        y -= _other.y;
        z -= _other.z;
        w -= _other.w;
        return *this;
    }

    constexpr Vec4i& operator*=(const Vec4i _other)
    {
        x *= _other.x;
        y *= _other.y;
        z *= _other.z;
        w *= _other.w;
        return *this;
    }

    constexpr Vec4i& operator/=(const Vec4i _other)
    {
        x /= _other.x;
        y /= _other.y;
        z /= _other.z;
        w /= _other.w;
        return *this;
    }

    constexpr Vec4i& operator*=(const Int32 _scalar)
    {
        x *= _scalar;
        y *= _scalar;
        z *= _scalar;
        w *= _scalar;
        return *this;
    }

    constexpr Vec4i& operator/=(const Int32 _scalar)
    {
        x /= _scalar;
        y /= _scalar;
        z /= _scalar;
        w /= _scalar;
        return *this;
    }

    [[nodiscard]] constexpr Vec4i operator+(const Vec4i _other) const
    {
        return { x + _other.x, y + _other.y, z + _other.z, w + _other.w };
    }

    [[nodiscard]] constexpr Vec4i operator-(const Vec4i _other) const
    {
        return { x - _other.x, y - _other.y, z - _other.z, w - _other.w };
    }

    [[nodiscard]] constexpr Vec4i operator*(const Vec4i _other) const
    {
        return { x * _other.x, y * _other.y, z * _other.z, w * _other.w };
    }

    [[nodiscard]] constexpr Vec4i operator/(const Vec4i _other) const
    {
        return { x / _other.x, y / _other.y, z / _other.z, w / _other.w };
    }

    [[nodiscard]] constexpr Vec4i operator*(const Int32 _scalar) const
    {
        return { x * _scalar, y * _scalar, z * _scalar, w * _scalar };
    }

    [[nodiscard]] constexpr Vec4i operator/(const Int32 _scalar) const
    {
        return { x / _scalar, y / _scalar, z / _scalar, w / _scalar };
    }

    [[nodiscard]] constexpr Vec4i operator-() const
    {
        return { -x, -y, -z, -w };
    }

    [[nodiscard]] constexpr bool operator==(const Vec4i _other) const
    {
        return x == _other.x && y == _other.y && z == _other.z && w == _other.w;
    }

    [[nodiscard]] constexpr bool operator!=(const Vec4i _other) const
    {
        return !(*this == _other);
    }

    const static Vec4i kZero;
    const static Vec4i kOne;
    const static Vec4i kUnitX;
    const static Vec4i kUnitY;
    const static Vec4i kUnitZ;
    const static Vec4i kUnitW;

    union
    {
        struct
        {
            Int32 x, y, z, w;
        };

        struct
        {
            Int32 r, g, b, a;
        };

        struct
        {
            Vec2i xy;
            Vec2i zw;
        };

        struct
        {
            Int32 x_;
            Vec2i yz;
            Int32 w_;
        };

        struct
        {
            Vec3i xyz;
            Int32 w__;
        };

        struct
        {
            Int32 x__;
            Vec3i yzw;
        };

        struct
        {
            Vec2i rg;
            Vec2i ba;
        };

        struct
        {
            Vec3i rgb;
            Int32 a_;
        };

        Int32 e[4] = {};
    };
};

struct alignas(16) Vec4
{
    constexpr Vec4() = default;
    constexpr Vec4(const float _x, const float _y, const float _z, const float _w)
        : x(_x)
        , y(_y)
        , z(_z)
        , w(_w)

    {
    }

    constexpr explicit Vec4(const float _xyzw)
        : x(_xyzw)
        , y(_xyzw)
        , z(_xyzw)
        , w(_xyzw)

    {
    }

    constexpr Vec4(const Vec2 _xy, const float _z, const float _w)
        : x(_xy.x)
        , y(_xy.y)
        , z(_z)
        , w(_w)

    {
    }

    constexpr Vec4(const Vec3 _xyz, const float _w)
        : x(_xyz.x)
        , y(_xyz.y)
        , z(_xyz.z)
        , w(_w)

    {
    }

    [[nodiscard]] Vec4 Lerp(const Vec4 _other, const float _t) const
    {
#ifdef JAM_SIMD_SSE
        const __m128 t  = _mm_set1_ps(_t);
        const __m128 om = _mm_set1_ps(1.f - _t);

        Vec4 ret = kZero;
        ret.m128 = _mm_mul_ps(m128, om);
        ret.m128 = _mm_fmadd_ps(_other.m128, t, ret.m128);
        return ret;
#else
        const float om = 1.f - _t;
        return *this * om + _other * _t;
#endif
    }

    [[nodiscard]] Vec4 Abs() const
    {
#ifdef JAM_SIMD_SSE
        static const __m128 kMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));

        Vec4 ret = kZero;
        ret.m128 = _mm_and_ps(m128, kMask);
        return ret;
#else
        return { Jam::Abs(x), Jam::Abs(y), Jam::Abs(z), Jam::Abs(w) };
#endif
    }

    [[nodiscard]] float LengthSq() const
    {
#ifdef JAM_SIMD_SSE
        __m128       sq    = _mm_mul_ps(m128, m128);
        const __m128 perm1 = _mm_permute_ps(sq, _MM_SHUFFLE(2, 3, 0, 1));
        sq                 = _mm_add_ps(sq, perm1);
        const __m128 perm2 = _mm_movehl_ps(sq, sq);
        sq                 = _mm_add_ss(sq, perm2);
        return _mm_cvtss_f32(sq);
#else
        return x * x + y * y + z * z + w * w;
#endif
    }

    [[nodiscard]] float Length() const

    {
        return Sqrt(LengthSq());
    }

    [[nodiscard]] float Distance(const Vec4 _other) const

    {
        return (*this - _other).Length();
    }

    [[nodiscard]] float DistanceSq(const Vec4 _other) const

    {
        return (*this - _other).LengthSq();
    }

    [[nodiscard]] float Dot(const Vec4 _other) const
    {
#ifdef JAM_SIMD_SSE
        __m128       mul   = _mm_mul_ps(m128, _other.m128);
        const __m128 shuf1 = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));
        mul                = _mm_add_ps(mul, shuf1);
        const __m128 shuf2 = _mm_movehl_ps(mul, mul);
        mul                = _mm_add_ss(mul, shuf2);
        return _mm_cvtss_f32(mul);
#else
        return x * _other.x + y * _other.y + z * _other.z + w * _other.w;
#endif
    }

    void Normalize()
    {
#ifdef JAM_SIMD_SSE
        __m128 sq = _mm_mul_ps(m128, m128);

        const __m128 shuf1 = _mm_shuffle_ps(sq, sq, _MM_SHUFFLE(2, 3, 0, 1));
        sq                 = _mm_add_ps(sq, shuf1);
        const __m128 shuf2 = _mm_movehl_ps(sq, sq);
        __m128       lenSq = _mm_add_ss(sq, shuf2);
        lenSq              = _mm_shuffle_ps(lenSq, lenSq, _MM_SHUFFLE(0, 0, 0, 0));

        const __m128 len  = _mm_sqrt_ps(lenSq);
        const __m128 zero = _mm_setzero_ps();
        const __m128 mask = _mm_cmpneq_ps(len, zero);
        m128              = _mm_and_ps(_mm_div_ps(m128, len), mask);
#else
        const float invLen = RSqrt(LengthSq());
        x *= invLen;
        y *= invLen;
        z *= invLen;
        w *= invLen;
#endif
    }

    [[nodiscard]] Vec4 Normalized() const
    {
        Vec4 ret = *this;
        ret.Normalize();
        return ret;
    }

    [[nodiscard]] Vec4 Cross(const Vec4 _other) const
    {
#ifdef JAM_SIMD_SSE
        const __m128 v0_yzxw = _mm_permute_ps(m128, _MM_SHUFFLE(3, 0, 2, 1));
        const __m128 v1_zxyw = _mm_permute_ps(_other.m128, _MM_SHUFFLE(3, 1, 0, 2));
        const __m128 v0_zxyw = _mm_permute_ps(m128, _MM_SHUFFLE(3, 1, 0, 2));
        const __m128 v1_yzxw = _mm_permute_ps(_other.m128, _MM_SHUFFLE(3, 0, 2, 1));
        __m128       result  = _mm_fmsub_ps(v0_yzxw, v1_zxyw, _mm_mul_ps(v0_zxyw, v1_yzxw));

        static const __m128 kMask = _mm_castsi128_ps(_mm_setr_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000));

        Vec4 ret = kZero;
        ret.m128 = _mm_and_ps(result, kMask);
        return ret;
#else
        return { y * _other.z - z * _other.y, z * _other.x - x * _other.z, x * _other.y - y * _other.x, 0.f };
#endif
    }

    [[nodiscard]] Vec4 Min(const Vec4 _other) const

    {
#ifdef JAM_SIMD_SSE
        Vec4 ret = kZero;
        ret.m128 = _mm_min_ps(m128, _other.m128);
        return ret;
#else
        return { Jam::Min(x, _other.x), Jam::Min(y, _other.y), Jam::Min(z, _other.z), Jam::Min(w, _other.w) };
#endif
    }

    [[nodiscard]] Vec4 Max(const Vec4 _other) const

    {
#ifdef JAM_SIMD_SSE
        Vec4 ret = kZero;
        ret.m128 = _mm_max_ps(m128, _other.m128);
        return ret;
#else
        return { Jam::Max(x, _other.x), Jam::Max(y, _other.y), Jam::Max(z, _other.z), Jam::Max(w, _other.w) };
#endif
    }

    [[nodiscard]] Vec4 Clamp(const Vec4 _min, const Vec4 _max) const

    {
#ifdef JAM_SIMD_SSE
        Vec4 ret = kZero;
        ret.m128 = _mm_min_ps(_mm_max_ps(m128, _min.m128), _max.m128);
        return ret;
#else
        return { Jam::Clamp(x, _min.x, _max.x), Jam::Clamp(y, _min.y, _max.y), Jam::Clamp(z, _min.z, _max.z), Jam::Clamp(w, _min.w, _max.w) };
#endif
    }

    [[nodiscard]] Vec4 Rcp() const

    {
#ifdef JAM_SIMD_SSE
        Vec4 ret = kZero;
        ret.m128 = _mm_rcp_ps(m128);
        return ret;
#else
        return { 1.f / x, 1.f / y, 1.f / z, 1.f / w };
#endif
    }

    [[nodiscard]] Vec4 Project(const Vec4 _onto) const
    {
        const float scalar = Dot(_onto) / _onto.LengthSq();
        return _onto * scalar;
    }

    [[nodiscard]] float Angle(const Vec4 _other) const
    {
        return Acos(Jam::Clamp(Dot(_other) / (Length() * _other.Length()), -1.f, 1.f));
    }

    [[nodiscard]] bool IsZeroApprox(const float _epsilon = kEpsilon) const
    {
#ifdef JAM_SIMD_SSE
        const __m128        eps      = _mm_set1_ps(_epsilon);
        static const __m128 kAbsMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
        const __m128        abs_v    = _mm_and_ps(m128, kAbsMask);
        const __m128        cmp      = _mm_cmple_ps(abs_v, eps);
        return (_mm_movemask_ps(cmp) == 0x0F);
#else
        return Jam::IsZeroApprox(x, _epsilon) && Jam::IsZeroApprox(y, _epsilon)
            && Jam::IsZeroApprox(z, _epsilon) && Jam::IsZeroApprox(w, _epsilon);
#endif
    }

    [[nodiscard]] bool IsEqualApprox(const Vec4 _other, const float _epsilon = kEpsilon) const
    {
#ifdef JAM_SIMD_SSE
        const __m128        eps      = _mm_set1_ps(_epsilon);
        static const __m128 kAbsMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
        const __m128        abs_diff = _mm_and_ps(_mm_sub_ps(m128, _other.m128), kAbsMask);
        const __m128        cmp      = _mm_cmple_ps(abs_diff, eps);
        return (_mm_movemask_ps(cmp) == 0x0F);
#else
        return Jam::IsEqualApprox(x, _other.x, _epsilon) && Jam::IsEqualApprox(y, _other.y, _epsilon)
            && Jam::IsEqualApprox(z, _other.z, _epsilon) && Jam::IsEqualApprox(w, _other.w, _epsilon);
#endif
    }

    [[nodiscard]] Vec4 Reflect(const Vec4 _normal) const

    {
#ifdef JAM_SIMD_SSE
        const __m128 dot2 = _mm_set1_ps(2.f * Dot(_normal));
        Vec4         ret  = kZero;
        ret.m128          = _mm_sub_ps(m128, _mm_mul_ps(_normal.m128, dot2));
        return ret;
#else
        const float dot2 = 2.f * Dot(_normal);
        return *this - _normal * dot2;
#endif
    }

    [[nodiscard]] Vec4 Refract(const Vec4 _normal, const float _eta) const
    {
        const float dotNV = Dot(_normal);
        const float k     = 1.f - _eta * _eta * (1.f - dotNV * dotNV);
        if (k < 0.f)

        {
            return Vec4::kZero;
        }

#ifdef JAM_SIMD_SSE
        const __m128 eta_val = _mm_set1_ps(_eta);
        const __m128 term    = _mm_set1_ps(_eta * dotNV - Sqrt(k));

        Vec4 ret = kZero;
        ret.m128 = _mm_add_ps(_mm_mul_ps(m128, eta_val), _mm_mul_ps(_normal.m128, term));
        return ret;
#else
        const float etaDotK = _eta * dotNV - Sqrt(k);
        return *this * _eta + _normal * etaDotK;
#endif
    }

    Vec4& operator+=(const Vec4 _other)
    {
#ifdef JAM_SIMD_SSE
        m128 = _mm_add_ps(m128, _other.m128);
        return *this;
#else
        x += _other.x;
        y += _other.y;
        z += _other.z;
        w += _other.w;
        return *this;
#endif
    }

    Vec4& operator-=(const Vec4 _other)
    {
#ifdef JAM_SIMD_SSE
        m128 = _mm_sub_ps(m128, _other.m128);
        return *this;
#else
        x -= _other.x;
        y -= _other.y;
        z -= _other.z;
        w -= _other.w;
        return *this;
#endif
    }

    Vec4& operator*=(const Vec4 _other)
    {
#ifdef JAM_SIMD_SSE
        m128 = _mm_mul_ps(m128, _other.m128);
        return *this;
#else
        x *= _other.x;
        y *= _other.y;
        z *= _other.z;
        w *= _other.w;
        return *this;
#endif
    }

    Vec4& operator/=(const Vec4 _other)
    {
#ifdef JAM_SIMD_SSE
        m128 = _mm_div_ps(m128, _other.m128);
        return *this;
#else
        x /= _other.x;
        y /= _other.y;
        z /= _other.z;
        w /= _other.w;
        return *this;
#endif
    }

    Vec4& operator*=(const float _scalar)
    {
#ifdef JAM_SIMD_SSE
        m128 = _mm_mul_ps(m128, _mm_set1_ps(_scalar));
        return *this;
#else
        x *= _scalar;
        y *= _scalar;
        z *= _scalar;
        w *= _scalar;
        return *this;
#endif
    }

    Vec4& operator/=(const float _scalar)
    {
#ifdef JAM_SIMD_SSE
        m128 = _mm_div_ps(m128, _mm_set1_ps(_scalar));
        return *this;
#else
        x /= _scalar;
        y /= _scalar;
        z /= _scalar;
        w /= _scalar;
        return *this;
#endif
    }

    [[nodiscard]] Vec4 operator+(const Vec4 _other) const
    {
#ifdef JAM_SIMD_SSE
        Vec4 ret = kZero;
        ret.m128 = _mm_add_ps(m128, _other.m128);
        return ret;
#else
        return { x + _other.x, y + _other.y, z + _other.z, w + _other.w };
#endif
    }

    [[nodiscard]] Vec4 operator-(const Vec4 _other) const
    {
#ifdef JAM_SIMD_SSE
        Vec4 ret = kZero;
        ret.m128 = _mm_sub_ps(m128, _other.m128);
        return ret;
#else
        return { x - _other.x, y - _other.y, z - _other.z, w - _other.w };
#endif
    }

    [[nodiscard]] Vec4 operator*(const Vec4 _other) const
    {
#ifdef JAM_SIMD_SSE
        Vec4 ret = kZero;
        ret.m128 = _mm_mul_ps(m128, _other.m128);
        return ret;
#else
        return { x * _other.x, y * _other.y, z * _other.z, w * _other.w };
#endif
    }

    [[nodiscard]] Vec4 operator/(const Vec4 _other) const
    {
#ifdef JAM_SIMD_SSE
        Vec4 ret = kZero;
        ret.m128 = _mm_div_ps(m128, _other.m128);
        return ret;
#else
        return { x / _other.x, y / _other.y, z / _other.z, w / _other.w };
#endif
    }

    [[nodiscard]] Vec4 operator*(const float _scalar) const
    {
#ifdef JAM_SIMD_SSE
        Vec4 ret = kZero;
        ret.m128 = _mm_mul_ps(m128, _mm_set1_ps(_scalar));
        return ret;
#else
        return { x * _scalar, y * _scalar, z * _scalar, w * _scalar };
#endif
    }

    [[nodiscard]] Vec4 operator/(const float _scalar) const
    {
#ifdef JAM_SIMD_SSE
        Vec4 ret = kZero;
        ret.m128 = _mm_div_ps(m128, _mm_set1_ps(_scalar));
        return ret;
#else
        return { x / _scalar, y / _scalar, z / _scalar, w / _scalar };
#endif
    }

    [[nodiscard]] Vec4 operator-() const
    {
#ifdef JAM_SIMD_SSE
        Vec4 ret = kZero;
        ret.m128 = _mm_sub_ps(_mm_setzero_ps(), m128);
        return ret;
#else
        return { -x, -y, -z, -w };
#endif
    }

    const static Vec4 kZero;
    const static Vec4 kOne;
    const static Vec4 kUnitX;
    const static Vec4 kUnitY;
    const static Vec4 kUnitZ;
    const static Vec4 kUnitW;
    const static Vec4 kRight;
    const static Vec4 kUp;
    const static Vec4 kForward;

    union
    {
        struct
        {
            float x, y, z, w;
        };

        struct
        {
            float r, g, b, a;
        };

        struct
        {
            Vec2 xy;
            Vec2 zw;
        };

        struct
        {
            float x_;
            Vec2  yz;
            float w_;
        };

        struct
        {
            Vec3  xyz;
            float w__;
        };

        struct
        {
            float x__;
            Vec3  yzw;
        };

        struct
        {
            Vec2 rg;
            Vec2 ba;
        };

        struct
        {
            Vec3  rgb;
            float a_;
        };

        float e[4] = {};

#ifdef JAM_SIMD_SSE
        __m128 m128;
#endif
    };
};

[[nodiscard]] constexpr Vec2i operator*(const Int32 _scalar, const Vec2i _vec)
{
    return _vec * _scalar;
}

[[nodiscard]] constexpr Vec2i operator/(const Int32 _scalar, const Vec2i _vec)
{
    return { _scalar / _vec.x, _scalar / _vec.y };
}

[[nodiscard]] constexpr Vec2 operator*(const float _scalar, const Vec2 _vec)
{
    return _vec * _scalar;
}

[[nodiscard]] constexpr Vec2 operator/(const float _scalar, const Vec2 _vec)
{
    return { _scalar / _vec.x, _scalar / _vec.y };
}

[[nodiscard]] constexpr Vec3i operator*(const Int32 _scalar, const Vec3i _vec)
{
    return _vec * _scalar;
}

[[nodiscard]] constexpr Vec3i operator/(const Int32 _scalar, const Vec3i _vec)
{
    return { _scalar / _vec.x, _scalar / _vec.y, _scalar / _vec.z };
}

[[nodiscard]] constexpr Vec3 operator*(const float _scalar, const Vec3 _vec)
{
    return _vec * _scalar;
}

[[nodiscard]] constexpr Vec3 operator/(const float _scalar, const Vec3 _vec)
{
    return { _scalar / _vec.x, _scalar / _vec.y, _scalar / _vec.z };
}

[[nodiscard]] constexpr Vec4i operator*(const Int32 _scalar, const Vec4i _vec)
{
    return _vec * _scalar;
}

[[nodiscard]] constexpr Vec4i operator/(const Int32 _scalar, const Vec4i _vec)
{
    return { _scalar / _vec.x, _scalar / _vec.y, _scalar / _vec.z, _scalar / _vec.w };
}
[[nodiscard]] inline Vec4 operator*(const float _scalar, const Vec4 _vec)
{
    return _vec * _scalar;
}

[[nodiscard]] inline Vec4 operator/(const float _scalar, const Vec4 _vec)
{
#ifdef JAM_SIMD_SSE
    const __m128 numer = _mm_set1_ps(_scalar);

    Vec4 result = Vec4::kZero;
    result.m128 = _mm_div_ps(numer, _vec.m128);
    return result;
#else
    return { _scalar / _vec.x, _scalar / _vec.y, _scalar / _vec.z, _scalar / _vec.w };
#endif
}

}   // namespace Jam