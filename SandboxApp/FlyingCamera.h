#pragma once
#include "../JamEngine/InfiniteDragger.h"

struct FlyingCamera
{
    void                      Update(float _dt);
    [[nodiscard]] Vec3        GetEyePos() const;
    [[nodiscard]] Vec3        GetForward() const;
    [[nodiscard]] const Mtx4& GetProj() const;
    [[nodiscard]] const Mtx4& GetInvProj() const;
    [[nodiscard]] const Mtx4& GetView() const;
    [[nodiscard]] const Mtx4& GetInvView() const;
    [[nodiscard]] Mtx4        CalcViewProj() const;
    [[nodiscard]] Mtx4        CalcInvViewProj() const;
    void                      SetAspectRatioFromBackBuffer();

    Camera    camera    = {};
    Transform transform = {};

    float dRot  = 0.0025f;
    float dMove = 10.f;
    Vec3  euler = Vec3::kZero;

    InfiniteDragger dragger { eInfiniteDraggerFlags::Right };
};
