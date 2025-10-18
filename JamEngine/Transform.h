#pragma once
#include "MathUtilities.h"

namespace jam
{

struct Transform
{
    JAM_NODISCARD Vec3 Forward() const { return Vec3::Transform(Vec3::Forward, rotate); }
    JAM_NODISCARD Vec3 Backward() const { return Vec3::Transform(Vec3::Backward, rotate); }
    JAM_NODISCARD Vec3 Right() const { return Vec3::Transform(Vec3::Right, rotate); }
    JAM_NODISCARD Vec3 Left() const { return Vec3::Transform(Vec3::Left, rotate); }
    JAM_NODISCARD Vec3 Up() const { return Vec3::Transform(Vec3::Up, rotate); }
    JAM_NODISCARD Vec3 Down() const { return Vec3::Transform(Vec3::Down, rotate); }

    JAM_NODISCARD Mat4 CreateWorld() const;
    JAM_NODISCARD Mat4 CreateView() const;

    Transform ToParent(const Transform& _parentTransform) const;     // 로컬 공간의 변환을 부모 공간에서 해석
    Transform FromParent(const Transform& _parentTransform) const;   // 부모 공간의 변환을 로컬 공간에서 해석

    JAM_NODISCARD Transform Inverse() const;

    Vec3 translate = Vec3::Zero;
    Quat rotate    = Quat::Identity;
    Vec3 scale     = Vec3::One;
};

}   // namespace jam