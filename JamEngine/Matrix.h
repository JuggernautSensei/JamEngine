// ReSharper disable CppClangTidyBugproneReservedIdentifier
// ReSharper disable CppClangTidyClangDiagnosticUsedButMarkedUnused
// ReSharper disable CppClangTidyClangDiagnosticNestedAnonTypes
// ReSharper disable CppClangTidyClangDiagnosticReservedIdentifier
#pragma once
#include "Quaternion.h"
#include "Vector.h"

namespace Jam
{

struct alignas(16) Mtx4
{
    Mtx4();
    Mtx4(Vec4 _r0, Vec4 _r1, Vec4 _r2, Vec4 _r3);
    Mtx4(float _m00, float _m01, float _m02, float _m03, float _m10, float _m11, float _m12, float _m13, float _m20, float _m21, float _m22, float _m23, float _m30, float _m31, float _m32, float _m33);

    [[nodiscard]] static Mtx4 MakeOrtho(float left, float right, float top, float bottom, float nearZ, float farZ);
    [[nodiscard]] static Mtx4 MakeOrtho(float _width, float _height, float _nearZ, float _farZ);
    [[nodiscard]] static Mtx4 MakePerspectiveFov(float _fovYRad, float _aspect, float _nearZ, float _farZ);

    [[nodiscard]] static Mtx4 MakeTranslation(Vec3 _translation);
    [[nodiscard]] static Mtx4 MakeScale(Vec3 _scale);
    [[nodiscard]] static Mtx4 MakeRotationX(float _angleRad);
    [[nodiscard]] static Mtx4 MakeRotationY(float _angleRad);
    [[nodiscard]] static Mtx4 MakeRotationZ(float _angleRad);
    [[nodiscard]] static Mtx4 FromAxisAngle(Vec3 _axis, float _angleRad);
    [[nodiscard]] static Mtx4 FromEuler(Vec3 _euler);
    [[nodiscard]] static Mtx4 FromQuaternion(Quat _quat);
    [[nodiscard]] static Mtx4 MakeLookAt(Vec3 _eyePos, Vec3 _focusPoint, Vec3 _up);
    [[nodiscard]] static Mtx4 MakeSRT(Vec3 _scale, Quat _rotation, Vec3 _translation);
    [[nodiscard]] static Mtx4 MakeSRT(Vec3 _scale, Vec3 _euler, Vec3 _translation);
    [[nodiscard]] static Mtx4 MakeSRT(Vec3 _scale, Vec3 _forward, Vec3 _up, Vec3 _translation);
    [[nodiscard]] static Mtx4 MakeInvSRT(Vec3 _scale, Quat _rotation, Vec3 _translation);
    [[nodiscard]] static Mtx4 MakeInvSRT(Vec3 _scale, Vec3 _euler, Vec3 _translation);
    [[nodiscard]] static Mtx4 MakeInvSRT(Vec3 _scale, Vec3 _forward, Vec3 _up, Vec3 _translation);

    void               DecomposeSRT(Vec3* _pScaleOrNull, Quat* _pRotationOrNull, Vec3* _pTranslationOrNull) const;
    [[nodiscard]] Vec3 DecomposeScale() const;
    [[nodiscard]] Quat DecomposeRotation() const;
    [[nodiscard]] Vec3 DecomposeTranslation() const;

    [[nodiscard]] Mtx4 Transpose() const;
    [[nodiscard]] Mtx4 Inverse() const;

    [[nodiscard]] Vec4         Mult(Vec4 _vec) const;
    [[nodiscard]] Vec3         MultPoint(Vec3 _point) const;
    [[nodiscard]] Vec3         MultVector(Vec3 _vector) const;
    [[nodiscard]] float*       GetPtr();
    [[nodiscard]] const float* GetPtr() const;

    [[nodiscard]] Mtx4 operator+(const Mtx4& _other) const;
    [[nodiscard]] Mtx4 operator-(const Mtx4& _other) const;
    [[nodiscard]] Mtx4 operator*(const Mtx4& _other) const;
    [[nodiscard]] Mtx4 operator*(float _scalar) const;

    Mtx4& operator+=(const Mtx4& _other);
    Mtx4& operator-=(const Mtx4& _other);
    Mtx4& operator*=(const Mtx4& _other);
    Mtx4& operator*=(float _scalar);

    [[nodiscard]] Vec4& operator[](SizeT _index);
    [[nodiscard]] Vec4  operator[](SizeT _index) const;

    const static Mtx4 kIdentity;

    union
    {
        struct
        {
            Vec4 r0;
            Vec4 r1;
            Vec4 r2;
            Vec4 r3;
        };

        struct
        {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
            float m30, m31, m32, m33;
        };

        Array<Vec4, 4>   rows;
        Array<float, 16> m = {};

#ifdef JAM_SIMD_SSE

        struct
        {
            __m128 m128r0;
            __m128 m128r1;
            __m128 m128r2;
            __m128 m128r3;
        };

        Array<__m128, 4> m128Rows;

#endif
    };
};

}   // namespace Jam
