#pragma once
#include "../JamEngine/InfiniteDragger.h"

struct FlyingCamera
{
    void Update(float _dt);

    Camera    cmr   = {};
    Transform trans = {};

    float dRot  = 0.002f;
    float dMove = 3.f;
    Vec3  euler = Vec3::kZero;

    InfiniteDragger dragger { eInfiniteDraggerFlags::Right };
};