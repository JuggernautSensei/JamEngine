#pragma once
#include "GfxBase.h"

namespace jam
{

struct AssimpMesh
{
    std::string name;

    std::vector<Vec3> positions  = {};
    std::vector<Vec3> normals    = {};
    std::vector<Vec3> tangents   = {};
    std::vector<Vec3> bitangents = {};
    std::vector<Vec3> texCoords0 = {};
    std::vector<Vec3> texCoords1 = {};
    std::vector<Vec3> texCoords2 = {};
    std::vector<Vec3> texCoords3 = {};
    std::vector<Vec4> colors0    = {};
    std::vector<Vec4> colors1    = {};
    std::vector<Vec4> colors2    = {};
    std::vector<Vec4> colors3    = {};

    std::vector<Byte> indices     = {};
    eIndexFormat      indexFormat = eIndexFormat::U16;
};

struct AssimpMaterial
{
    std::string name;

    Vec3  ambientColor  = Vec3::One;
    Vec3  diffuseColor  = Vec3::One;
    Vec3  specularColor = Vec3::One;
    float shininess     = 0.f;

    Vec3  albedoColor = Vec3::One;
    float roughness   = 1.f;
    float metallic    = 1.f;

    float opacity           = 1.0f;
    bool  bDoubleSide       = false;
    Vec3  emissiveColor     = Vec3::Zero;
    float emissiveIntensity = 1.f;
    float bumpScaling       = 1.0f;
};

}   // namespace jam