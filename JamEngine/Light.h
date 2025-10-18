#pragma once

namespace jam
{

enum class eLightType : Int8
{
    Point,
    Spot
};

struct Light
{
    eLightType type             = eLightType::Point;
    Vec3       radiance         = Vec3(1.f, 1.f, 1.f);
    float      intensity        = 1.f;
    float      fallOffStart     = 1.f;
    float      fallOffEnd       = 10.f;
    float      innerConeHalfRad = ToRad(15.f);
    float      outerConeHalfRad = ToRad(30.f);

    // shadow
    bool  bCastShadow   = false;
};

}   // namespace jam
