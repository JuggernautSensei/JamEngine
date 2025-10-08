#pragma once
#include "Transform.h"

namespace jam
{

enum class eProjection : Int8
{
    Perspective = 0,
    Orthographic
};

struct Camera
{
    JAM_NODISCARD Mat4 CreateProj() const;

    eProjection projection = eProjection::Perspective;   // 투영 방식
    float       nearZ      = 0.1f;                       // 근평면
    float       farZ       = 1000.f;                     // 원평면

    float fovYRad = ToRad(45.f);   // 수직 시야갹 (라디안)
    float width   = 1.f;           // 너비
    float height  = 1.f;           // 높이

    static constexpr float k_nearestZ  = 0.01f;
    static constexpr float k_farthestZ = 10000.f;
};

}   // namespace jam