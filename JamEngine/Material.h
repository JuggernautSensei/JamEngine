#pragma once
#include "IAsset.h"
#include "ImageTexture.h"

namespace jam
{

struct MaterialParams
{
    // phong properties
    Vec3  ambientColor  = Vec3::One;
    Vec3  diffuseColor  = Vec3::One;
    Vec3  specularColor = Vec3::One;
    float shininess     = 32.0f;

    // PBR properties
    Vec3  albedoColor = Vec3::One;
    float roughness   = 1.0f;
    float metallic    = 1.0f;

    // emissive properties
    Vec3  emissiveColor     = Vec3::Zero;
    float emissiveIntensity = 1.0f;

    // alpha properties
    float opacity        = 1.0f;
    bool  bIsTransparent = false;

    // misc
    bool  bDoubleSide       = false;
    bool  bFlipedYNormalMap = false;
    float lightMapIntencity = 1.0f;
    float aoMapIntencity    = 1.0f;
    float bumpMapScaling    = 1.0f;

    // textures
    std::shared_ptr<ImageTexture> albedoMap    = nullptr;
    std::shared_ptr<ImageTexture> normalMap    = nullptr;
    std::shared_ptr<ImageTexture> metallicMap  = nullptr;
    std::shared_ptr<ImageTexture> roughnessMap = nullptr;
    std::shared_ptr<ImageTexture> aoMap        = nullptr;
    std::shared_ptr<ImageTexture> emissiveMap  = nullptr;
    std::shared_ptr<ImageTexture> lightMap     = nullptr;
    std::shared_ptr<ImageTexture> bumpMap      = nullptr;
};

class Material : public IAsset
{
public:
    eErrorCode Load(const fs::path& _path) override;
    void       Unload() override;
    eErrorCode Save(const fs::path& _path) const override;

    JAM_NODISCARD bool  IsLoaded() const override;
    JAM_NODISCARD const fs::path& GetPath() const override;
    JAM_NODISCARD eAssetType      GetType() const override;
    JAM_NODISCARD UInt32          GetHash() const override;
    JAM_NODISCARD std::string_view GetName() const override;

    void                                SetParams(const MaterialParams& _params);
    JAM_NODISCARD const MaterialParams& GetParams() const;

    static constexpr std::string_view k_name      = NameOf<Material>();
    static constexpr UInt32           k_hash      = HashOf<Material>();
    static constexpr std::string_view k_extension = ".jmat";
    static constexpr eAssetType       k_type      = eAssetType::Material;
    static constexpr Version          k_version { 1, 0, 0 };

private:
    fs::path       m_path;
    bool           m_bIsLoaded = false;
    MaterialParams m_params;
};

}   // namespace jam