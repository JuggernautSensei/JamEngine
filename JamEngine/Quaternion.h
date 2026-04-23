// ReSharper disable CppClangTidyBugproneReservedIdentifier
// ReSharper disable CppClangTidyClangDiagnosticUsedButMarkedUnused
// ReSharper disable CppClangTidyClangDiagnosticNestedAnonTypes
// ReSharper disable CppClangTidyClangDiagnosticReservedIdentifier
#pragma once
#include "Vector.h"

namespace Jam
{

struct Mtx4;

struct AxisAngle
{
    Vec3  axis = Vec3::kZero;
    float rad  = 0.f;
};

struct alignas(16) Quat
{
    Quat();
    Quat(float _x, float _y, float _z, float _w);
    Quat(Vec3 _axis, float _angle);
    explicit Quat(Vec3 _euler);

    [[nodiscard]] static Quat FromAxisAngle(Vec3 _axis, float _angleRad);
    [[nodiscard]] static Quat FromEuler(Vec3 _eulerRad);
    [[nodiscard]] static Quat FromRotationMatrix(const Mtx4& _mtx);
    [[nodiscard]] static Quat FromLookAt(Vec3 _f, Vec3 _u);

    void                    SetEuler(Vec3 _eulerRad);
    [[nodiscard]] Vec3      ToEuler() const;
    void                    SetAxisAngle(Vec3 _axis, float _angle);
    [[nodiscard]] AxisAngle ToAxisAngle() const;

    [[nodiscard]] float Length() const;
    [[nodiscard]] float LengthSq() const;

    void               Normalize();
    [[nodiscard]] Quat Normalized() const;

    [[nodiscard]] Quat Conjugate() const;
    [[nodiscard]] Quat Inverse() const;

    [[nodiscard]] float Dot(Quat _other) const;
    [[nodiscard]] Quat  Lerp(Quat _other, float _t) const;
    [[nodiscard]] Quat  Slerp(Quat _other, float _t) const;
    [[nodiscard]] Vec3  Rotate(Vec3 _v) const;

    [[nodiscard]] bool IsZeroApprox(float _epsilon = kEpsilon) const;
    [[nodiscard]] bool IsEqualApprox(Quat _other, float _epsilon = kEpsilon) const;

    [[nodiscard]] Quat operator*(Quat _other) const;   // q1 * q2 means applying q2 then q1. this is correct mathmatically
    [[nodiscard]] Quat operator*(float _scalar) const;
    [[nodiscard]] Quat operator/(float _scalar) const;
    [[nodiscard]] Quat operator+(Quat _other) const;
    [[nodiscard]] Quat operator-(Quat _other) const;
    [[nodiscard]] Quat operator-() const;

    Quat&               operator*=(Quat _other);   // q1 *= q2 means applying q2 then q1. this is correct mathmaticallys
    [[nodiscard]] Quat& operator*=(float _scalar);
    [[nodiscard]] Quat& operator/=(float _scalar);
    [[nodiscard]] Quat& operator+=(Quat _other);
    [[nodiscard]] Quat& operator-=(Quat _other);

    const static Quat kIdentity;

    union
    {
        struct
        {
            float x, y, z, w;
        };

        struct
        {
            Vec2 xy;
            Vec2 zw;
        };

        struct
        {
            Vec3  xyz;
            float w_;
        };

        struct
        {
            float x_;
            Vec3  yzw;
        };

        Vec4  vec;
        float e[4] = {};

#ifdef JAM_SIMD_SSE
        __m128 m128;
#endif
    };
};

}   // namespace Jam