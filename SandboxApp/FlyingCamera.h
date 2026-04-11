#pragma once
#include "../JamEngine/InfiniteDragger.h"

struct FlyingCamera
{
    void Update(float _dt);

    float           rotSpeed  = 0.002f;
    float           moveSpeed = 3.f;
    Camera          cmr       = {};
    Transform       trans     = {};
    float           yawRad    = 0.f;
    float           pitchRad  = 0.f;
    InfiniteDragger dragger { eInfiniteDraggerFlags::Right };
};