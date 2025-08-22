#pragma once

namespace jam
{

class TextureAsset;

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
    Vec3  emissiveColor = Vec3::Zero;   // emissive color
    float emissiveScale = 1.f;          // emissive scale

    // texture
    std::optional<Ref<TextureAsset>> albedoTexture;      // albedo texture
    std::optional<Ref<TextureAsset>> normalTexture;      // normal texture
    std::optional<Ref<TextureAsset>> metallicTexture;    // metallic texture
    std::optional<Ref<TextureAsset>> roughnessTexture;   // roughness texture
    std::optional<Ref<TextureAsset>> aoTexture;          // ambient occlusion texture
    std::optional<Ref<TextureAsset>> emissiveTexture;    // emissive texture
    std::optional<Ref<TextureAsset>> lightmapTexture;    // lightmap texture
};

}   // namespace jam