#pragma once

namespace jam
{

struct PointLight
{
    Vec3  radiance     = Vec3(1.f, 1.f, 1.f);
    float intensity    = 1.f;
    float fallOffStart = 1.f;
    float fallOffEnd   = 10.f;
    bool  bCastShadow  = false;
};

struct SpotLight
{
    Vec3  radiance         = Vec3(1.f, 1.f, 1.f);
    float intensity        = 1.f;
    float fallOffStart     = 1.f;
    float fallOffEnd       = 10.f;
    float innerConeHalfRad = ToRad(15.f);
    float outerConeHalfRad = ToRad(30.f);
    bool  bCastShadow      = false;
};

}   // namespace jam
