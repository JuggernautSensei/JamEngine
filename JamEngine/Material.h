#pragma once

namespace jam
{

struct Material
{
    // phong
    Vec3  ambientColor  = Vec3::One;
    Vec3  diffuseColor  = Vec3::One;
    Vec3  specularColor = Vec3::One;
    float shininess     = 32.f;

    // pbr
    Vec3  albedoColor = Vec3::One;
    float metallic    = 0.f;
    float roughness   = 1.f;
    float ao          = 1.f;   // ambient occlusion
    float emissive    = 0.f;   // emissive intensity

    // misc
    Vec3  emissiveColor     = Vec3::Zero;   // emissive color
    float emissiveScale     = 1.f;
    float displacementScale = 0.f;   // for tessellation
};

}