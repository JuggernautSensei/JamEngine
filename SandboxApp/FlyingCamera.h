#pragma once
#include "../JamEngine/InfiniteDragger.h"

struct FlyingCamera
{
    void                      Update(float _dt);
    [[nodiscard]] Vec3        GetEyePos() const;
    [[nodiscard]] const Mtx4& GetProj() const;
    [[nodiscard]] Mtx4        CalcView() const;
    [[nodiscard]] Mtx4        CalcViewProj() const;
    void                      FitAspectRatioAsBackBufferResolution();

    Camera    camera    = {};
    Transform transform = {};

    float dRot  = 0.0025f;
    float dMove = 10.f;
    Vec3  euler = Vec3::kZero;

    InfiniteDragger dragger { eInfiniteDraggerFlags::Right };
};