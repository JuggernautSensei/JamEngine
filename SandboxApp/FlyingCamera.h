#pragma once
#include "../JamEngine/InfiniteDragger.h"

struct FlyingCamera
{
    void                      Update(float _dt);
    [[nodiscard]] Vec3        GetEyePos() const;
    [[nodiscard]] const Mtx4& GetProj() const;
    [[nodiscard]] Mtx4        CalcViewProj() const;
    [[nodiscard]] Mtx4        CalcView() const;

    Camera    cmr   = {};
    Transform trans = {};

    float dRot  = 0.002f;
    float dMove = 3.f;
    Vec3  euler = Vec3::kZero;

    InfiniteDragger dragger { eInfiniteDraggerFlags::Right };
};