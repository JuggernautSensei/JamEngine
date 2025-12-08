#include "pch.h"

#include "Material.h"

#include "AssetManager.h"
#include "FileUtilities.h"
#include "Hash.h"
#include "MemView.h"
#include "Profile.h"
#include "resources/flatc/jmat_generated.h"

namespace jam
{

[[nodiscard]] static Vec3 ToVec3(const jmat::Vec3& _vec)
{
    return Vec3 { _vec.x(), _vec.y(), _vec.z() };
}

[[nodiscard]] static fs::path ToPath(const flatbuffers::String& _str)
{
    return _str.c_str();
}

[[nodiscard]] static eTextureChannel ToTextureChannel(const jmat::eTextureChannel _channel)
{
    switch (_channel)
    {
        case jmat::eTextureChannel_Red: return eTextureChannel::Red;
        case jmat::eTextureChannel_Green: return eTextureChannel::Green;
        case jmat::eTextureChannel_Blue: return eTextureChannel::Blue;
        case jmat::eTextureChannel_Alpha: return eTextureChannel::Alpha;
        default:
            JAM_ASSERT(0, "Unknown color channel");
            return eTextureChannel::Red;
    }
}

[[nodiscard]] static jmat::Vec3 ToJmatVec3(const Vec3 _vec)
{
    return jmat::Vec3 { _vec.x, _vec.y, _vec.z };
}

[[nodiscard]] static flatbuffers::Offset<flatbuffers::String> ToJmatString(flatbuffers::FlatBufferBuilder& _builder, const fs::path& _path)
{
    std::string nativePath = _path.native();
    return _builder.CreateString(nativePath);
}

[[nodiscard]] static jmat::eTextureChannel ToJmatTextureChannel(const eTextureChannel _channel)
{
    switch (_channel)
    {
        case eTextureChannel::Red: return jmat::eTextureChannel_Red;
        case eTextureChannel::Green: return jmat::eTextureChannel_Green;
        case eTextureChannel::Blue: return jmat::eTextureChannel_Blue;
        case eTextureChannel::Alpha: return jmat::eTextureChannel_Alpha;
        default:
            JAM_ASSERT(0, "Unknown color channel");
            return jmat::eTextureChannel_Red;
    }
}

}   // namespace jam

namespace jam
{

void Material::LoadFromMemory(const fs::path& _virtualPath)
{
    Unload();
    m_path   = _virtualPath;
    m_bValid = true;

    // create sort key
    if (_virtualPath.empty())
    {
        // use random value
        m_sortKey = MakeRandom<UInt16>(0, k_uint16Max);
    }
    else
    {
        // hash based on virtual path
        const std::string_view strPath = _virtualPath.native();

        MurmurHash64 hasher;
        hasher.Begin();
        hasher.Add(strPath.data(), strPath.size());

        const UInt64 hash = hasher.End();
        m_sortKey         = hash & 0xFFFF;   // use lower 16 bits
    }
}

bool Material::Load(const fs::path& _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    Unload();

    // load .jmat
    std::optional<std::vector<Byte>> optBin = LoadBinaryFromFile(_path);
    if (!optBin)
    {
        return false;
    }
    const std::vector<Byte>& buf = *optBin;

    // verify
    flatbuffers::Verifier verifier { buf.data(), buf.size() };
    if (!jmat::VerifyMaterialBuffer(verifier))
    {
        SetErrorString(R"(Material file is corrupted or invalid: "{}")", _path.native());
        return false;
    }

    // parse material
    const jmat::Material* pFbsMat = jmat::GetMaterial(buf.data());
    if (!pFbsMat)
    {
        SetErrorString(R"(Failed to parse material file: "{}")", _path.native());
        return false;
    }

    // parse properties
    bool bValid = true;
    bValid &= !!pFbsMat->ambient_color();
    bValid &= !!pFbsMat->diffuse_color();
    bValid &= !!pFbsMat->specular_color();
    bValid &= !!pFbsMat->albedo_color();
    bValid &= !!pFbsMat->emissive_color();
    if (!bValid)
    {
        return false;
    }

    // material properties
    m_ambientColor                   = ToVec3(*pFbsMat->ambient_color());
    m_diffuseColor                   = ToVec3(*pFbsMat->diffuse_color());
    m_specularColor                  = ToVec3(*pFbsMat->specular_color());
    m_shininess                      = pFbsMat->shininess();
    m_albedoColor                    = ToVec3(*pFbsMat->albedo_color());
    m_roughness                      = pFbsMat->roughness();
    m_metallic                       = pFbsMat->metallic();
    m_roughnessTextureChannel        = ToTextureChannel(pFbsMat->roughness_texture_channel());
    m_metallicTextureChannel         = ToTextureChannel(pFbsMat->metallic_texture_channel());
    m_emissiveColor                  = ToVec3(*pFbsMat->emissive_color());
    m_emissiveIntensity              = pFbsMat->emissive_intensity();
    m_bAlphaCutout                   = pFbsMat->b_alpha_cutout();
    m_bDoubleSided                   = pFbsMat->b_double_sided();
    m_bNormalTextureFliped           = pFbsMat->b_normal_texture_fliped();
    m_lightTextureIntensity          = pFbsMat->light_texture_intensity();
    m_emissiveTextureIntensity       = pFbsMat->emissive_texture_intensity();
    m_vertexDisplacementTextureScale = pFbsMat->vertex_displacement_scale();

    Ref<ImageTexture>* const imgTexes[] = {
        &m_pAlbedoTexture,
        &m_pNormalTexture,
        &m_pMetallicTexture,
        &m_pRoughnessTexture,
        &m_pAoTexture,
        &m_pEmissiveTexture,
        &m_pLightTexture,
        &m_pVertexDisplacementTexture,
    };

    const flatbuffers::String* const texPaths[] = {
        pFbsMat->albedo_texture_path(),
        pFbsMat->normal_texture_path(),
        pFbsMat->metallic_texture_path(),
        pFbsMat->roughness_texture_path(),
        pFbsMat->ao_texture_path(),
        pFbsMat->emissive_texture_path(),
        pFbsMat->light_texture_path(),
        pFbsMat->vertex_displacement_texture_path(),
    };

    for (size_t i = 0; i < std::size(imgTexes); ++i)
    {
        if (texPaths[i])
        {
            fs::path path = ToPath(*texPaths[i]);
            if (std::optional<Ref<ImageTexture>> opt = AssetManager::GetOrLoadImageTexture(path))
            {
                *imgTexes[i] = opt.value();
            }
            else
            {
                Log::Warn("Failed to load texture: {} (Error: {})", path.native(), GetErrorString());
            }
        }
    }

    // compute sort key
    {
        std::string_view strPath = _path.native();

        MurmurHash64 hasher;
        hasher.Begin();
        hasher.Add(strPath.data(), strPath.size());
        m_sortKey = static_cast<UInt16>(hasher.End());
    }

    // init done
    m_path   = _path;
    m_bValid = true;
    return true;
}

void Material::Unload()
{
    m_path.clear();
    m_sortKey = 0;
    m_bValid  = false;

    m_ambientColor  = Vec3::One;
    m_diffuseColor  = Vec3::One;
    m_specularColor = Vec3::One;
    m_shininess     = 32.0f;

    m_albedoColor = Vec3::One;
    m_roughness   = 1.f;
    m_metallic    = 1.f;

    m_emissiveColor     = Vec3::Zero;
    m_emissiveIntensity = 1.f;

    m_bAlphaCutout                   = false;
    m_bDoubleSided                   = false;
    m_bNormalTextureFliped           = false;
    m_lightTextureIntensity          = 1.f;
    m_emissiveTextureIntensity       = 1.f;
    m_vertexDisplacementTextureScale = 1.f;

    m_metallicTextureChannel  = eTextureChannel::Red;
    m_roughnessTextureChannel = eTextureChannel::Red;

    m_pAlbedoTexture             = nullptr;
    m_pNormalTexture             = nullptr;
    m_pMetallicTexture           = nullptr;
    m_pRoughnessTexture          = nullptr;
    m_pAoTexture                 = nullptr;
    m_pEmissiveTexture           = nullptr;
    m_pLightTexture              = nullptr;
    m_pVertexDisplacementTexture = nullptr;
}

bool Material::Save(const fs::path& _path) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    JAM_ASSERT(IsValid(), "Cannot save invalid material");

    flatbuffers::FlatBufferBuilder builder { 1024 };

    // colors
    jmat::Vec3 fbsAmbientColor  = ToJmatVec3(m_ambientColor);
    jmat::Vec3 fbsDiffuseColor  = ToJmatVec3(m_diffuseColor);
    jmat::Vec3 fbsSpecularColor = ToJmatVec3(m_specularColor);
    jmat::Vec3 fbsAlbedoColor   = ToJmatVec3(m_albedoColor);
    jmat::Vec3 fbsEmissiveColor = ToJmatVec3(m_emissiveColor);

    // channels
    jmat::eTextureChannel fbsMetallicTextureChannel  = ToJmatTextureChannel(m_metallicTextureChannel);
    jmat::eTextureChannel fbsRoughnessTextureChannel = ToJmatTextureChannel(m_roughnessTextureChannel);

    // textures
    flatbuffers::Offset<flatbuffers::String> fbsAlbedoTexturePath             = m_pAlbedoTexture ? ToJmatString(builder, m_pAlbedoTexture->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsNormalTexturePath             = m_pNormalTexture ? ToJmatString(builder, m_pNormalTexture->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsMetallicTexturePath           = m_pMetallicTexture ? ToJmatString(builder, m_pMetallicTexture->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsRoughnessTexturePath          = m_pRoughnessTexture ? ToJmatString(builder, m_pRoughnessTexture->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsAOTexturePath                 = m_pAoTexture ? ToJmatString(builder, m_pAoTexture->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsEmissiveTexturePath           = m_pEmissiveTexture ? ToJmatString(builder, m_pEmissiveTexture->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsLightTexturePath              = m_pLightTexture ? ToJmatString(builder, m_pLightTexture->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsVertexDisplacementTexturePath = m_pVertexDisplacementTexture ? ToJmatString(builder, m_pVertexDisplacementTexture->GetPath()) : 0;

    auto fbsMaterial = jmat::CreateMaterial(
        builder,
        &fbsAmbientColor,
        &fbsDiffuseColor,
        &fbsSpecularColor,
        m_shininess,
        &fbsAlbedoColor,
        m_roughness,
        m_metallic,
        &fbsEmissiveColor,
        m_emissiveIntensity,
        m_bAlphaCutout,
        m_bDoubleSided,
        m_bNormalTextureFliped,
        m_lightTextureIntensity,
        m_emissiveTextureIntensity,
        m_vertexDisplacementTextureScale,
        fbsMetallicTextureChannel,
        fbsRoughnessTextureChannel,
        fbsAlbedoTexturePath,
        fbsNormalTexturePath,
        fbsMetallicTexturePath,
        fbsRoughnessTexturePath,
        fbsAOTexturePath,
        fbsEmissiveTexturePath,
        fbsLightTexturePath,
        fbsVertexDisplacementTexturePath);

    builder.Finish(fbsMaterial);

    // 파일 쓰기
    return SaveBinaryToFile(_path, { builder.GetBufferPointer(), builder.GetSize() });
}

bool Material::IsValid() const
{
    return m_bValid;
}

const fs::path& Material::GetPath() const
{
    return m_path;
}

eAssetType Material::GetAssetType() const
{
    return k_type;
}

UInt32 Material::GetTypeHash() const
{
    return k_hash;
}

std::string_view Material::GetTypeName() const
{
    return k_name;
}

UInt16 Material::GetSortKey() const
{
    return m_sortKey;
}

void Material::SetAmbientColor(const Vec3 _color)
{
    m_ambientColor = _color;
}

Vec3 Material::GetAmbientColor() const
{
    return m_ambientColor;
}

void Material::SetAlphaCutout(const bool _alphaCutout)
{
    m_bAlphaCutout = _alphaCutout;
}

bool Material::IsAlphaCutout() const
{
    return m_bAlphaCutout;
}

void Material::SetDiffuseColor(const Vec3 _color)
{
    m_diffuseColor = _color;
}

Vec3 Material::GetDiffuseColor() const
{
    return m_diffuseColor;
}

void Material::SetSpecularColor(const Vec3 _color)
{
    m_specularColor = _color;
}

Vec3 Material::GetSpecularColor() const
{
    return m_specularColor;
}

void Material::SetShininess(const float _shininess)
{
    m_shininess = _shininess;
}

float Material::GetShininess() const
{
    return m_shininess;
}

void Material::SetAlbedoColor(const Vec3 _color)
{
    m_albedoColor = _color;
}

Vec3 Material::GetAlbedoColor() const
{
    return m_albedoColor;
}

void Material::SetRoughness(const float _roughness)
{
    m_roughness = _roughness;
}

float Material::GetRoughness() const
{
    return m_roughness;
}

void Material::SetMetallic(const float _metallic)
{
    m_metallic = _metallic;
}

float Material::GetMetallic() const
{
    return m_metallic;
}

void Material::SetEmissiveColor(const Vec3 _color)
{
    m_emissiveColor = _color;
}

Vec3 Material::GetEmissiveColor() const
{
    return m_emissiveColor;
}

void Material::SetEmissiveIntensity(const float _intensity)
{
    m_emissiveIntensity = _intensity;
}

float Material::GetEmissiveIntensity() const
{
    return m_emissiveIntensity;
}

void Material::SetEmissiveTextureIntensity(const float _intensity)
{
    m_emissiveTextureIntensity = _intensity;
}

float Material::GetEmissiveTextureIntensity() const
{
    return m_emissiveTextureIntensity;
}

void Material::SetDoubleSide(const bool _doubleSide)
{
    m_bDoubleSided = _doubleSide;
}

bool Material::IsDoubleSide() const
{
    return m_bDoubleSided;
}

void Material::SetFlipYNormalTexture(const bool _flipY)
{
    m_bNormalTextureFliped = _flipY;
}

bool Material::IsFlipYNormalTexture() const
{
    return m_bNormalTextureFliped;
}

void Material::SetLightTextureIntensity(const float _intensity)
{
    m_lightTextureIntensity = _intensity;
}

float Material::GetLightTextureIntensity() const
{
    return m_lightTextureIntensity;
}

void Material::SetVertexDisplacementTextureScale(const float _scale)
{
    m_vertexDisplacementTextureScale = _scale;
}

float Material::GetVertexDisplacementTextureScale() const
{
    return m_vertexDisplacementTextureScale;
}

void Material::SetAlbedoTexture(const Ref<ImageTexture>& _texture)
{
    m_pAlbedoTexture = _texture;
}

const Ref<ImageTexture>& Material::GetAlbedoTexture() const
{
    return m_pAlbedoTexture;
}

void Material::SetNormalTexture(const Ref<ImageTexture>& _texture)
{
    m_pNormalTexture = _texture;
}

const Ref<ImageTexture>& Material::GetNormalTexture() const
{
    return m_pNormalTexture;
}

void Material::SetMetallicTexture(const Ref<ImageTexture>& _texture)
{
    m_pMetallicTexture = _texture;
}

void Material::SetMetallicTextureChannel(const eTextureChannel _channel)
{
    m_metallicTextureChannel = _channel;
}

eTextureChannel Material::GetMetallicTextureChannel() const
{
    return m_metallicTextureChannel;
}

void Material::SetRoughnessTextureChannel(const eTextureChannel _channel)
{
    m_roughnessTextureChannel = _channel;
}

eTextureChannel Material::GetRoughnessTextureChannel() const
{
    return m_roughnessTextureChannel;
}

const Ref<ImageTexture>& Material::GetMetallicTexture() const
{
    return m_pMetallicTexture;
}

void Material::SetRoughnessTexture(const Ref<ImageTexture>& _texture)
{
    m_pRoughnessTexture = _texture;
}

const Ref<ImageTexture>& Material::GetRoughnessTexture() const
{
    return m_pRoughnessTexture;
}

void Material::SetAoTexture(const Ref<ImageTexture>& _texture)
{
    m_pAoTexture = _texture;
}

const Ref<ImageTexture>& Material::GetAoTexture() const
{
    return m_pAoTexture;
}

void Material::SetEmissiveTexture(const Ref<ImageTexture>& _texture)
{
    m_pEmissiveTexture = _texture;
}

const Ref<ImageTexture>& Material::GetEmissiveTexture() const
{
    return m_pEmissiveTexture;
}

void Material::SetLightTexture(const Ref<ImageTexture>& _texture)
{
    m_pLightTexture = _texture;
}

const Ref<ImageTexture>& Material::GetLightTexture() const
{
    return m_pLightTexture;
}

void Material::SetVertexDisplacementTexture(const Ref<ImageTexture>& _texture)
{
    m_pVertexDisplacementTexture = _texture;
}

const Ref<ImageTexture>& Material::GetVertexDisplacementTexture() const
{
    return m_pVertexDisplacementTexture;
}

}   // namespace jam