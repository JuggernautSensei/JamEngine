#include "pch.h"

#include "Material.h"

#include "AssetManager.h"
#include "FileIOUtilities.h"
#include "MemView.h"
#include "Profile.h"
#include "resources/flatc/jmat_generated.h"

namespace jam
{

[[nodiscard]] static Vec3 Vec3From(
    const jmat::Vec3& _vec)
{
    return { _vec.x(), _vec.y(), _vec.z() };
}

[[nodiscard]] static fs::path PathFrom(
    const flatbuffers::String& _str)
{
    return _str.c_str();
}

[[nodiscard]] static eTextureChannel TextureChannelFrom(
    const jmat::eTextureChannel _channel)
{
    switch (_channel)
    {
        case jmat::eTextureChannel_Red:
            return eTextureChannel::Red;
        case jmat::eTextureChannel_Green:
            return eTextureChannel::Green;
        case jmat::eTextureChannel_Blue:
            return eTextureChannel::Blue;
        case jmat::eTextureChannel_Alpha:
            return eTextureChannel::Alpha;
        default:
            JAM_ASSERT(0, "Unknown color channel");
            return eTextureChannel::Red;
    }
}

[[nodiscard]] static jmat::Vec3 JmatVec3From(
    const Vec3 _vec)
{
    return { _vec.x, _vec.y, _vec.z };
}

[[nodiscard]] static flatbuffers::Offset<flatbuffers::String> JmatStringOffsetFrom(
    flatbuffers::FlatBufferBuilder& _builder,
    const Ref<ImageTexture>&        _pTextureOrNull)
{
    if (_pTextureOrNull)
    {
        const fs::path& path = _pTextureOrNull->GetPath();
        return _builder.CreateString(path.native());
    }
    else
    {
        return 0;
    }
}

[[nodiscard]] static jmat::eTextureChannel JmatTextureChannelFrom(
    const eTextureChannel _channel)
{
    switch (_channel)
    {
        case eTextureChannel::Red:
            return jmat::eTextureChannel_Red;
        case eTextureChannel::Green:
            return jmat::eTextureChannel_Green;
        case eTextureChannel::Blue:
            return jmat::eTextureChannel_Blue;
        case eTextureChannel::Alpha:
            return jmat::eTextureChannel_Alpha;
        default:
            JAM_ASSERT(0, "Unknown color channel");
            return jmat::eTextureChannel_Red;
    }
}

[[nodiscard]] static Ref<ImageTexture> TryImageTextureFrom(   /// << return nullptr if failed
    const flatbuffers::String* _pStrOrNull)
{
    if (!_pStrOrNull)
    {
        return nullptr;
    }

    const fs::path path = PathFrom(*_pStrOrNull);
    if (const Optional<Ref<ImageTexture>> optTex = AssetManager::GetOrLoadImageTexture(path))
    {
        return optTex.value();
    }
    else
    {
        Log::Warn("failed to load texture: {} (error: {})", path.native(), GlobalBuffer::GetLastError());
        return nullptr;
    }
}

}   // namespace jam

//////////////////////////////////////////////////////////////////////

namespace jam
{

void Material::LoadEmpty(
    const fs::path& _virtualPath)
{
    CleanUp_();

    // create sort key
    UInt16 sortKey;
    if (_virtualPath.empty())
    {
        sortKey = MakeRandom<UInt16>(0, 0xFFFF);   // use random value
    }
    else
    {
        // hash based on virtual path
        const StringView path = _virtualPath.native();
        const UInt64     hash = HashOf(path);
        sortKey               = static_cast<UInt16>(hash);   // use lower 16 bits
    }

    m_path    = _virtualPath;
    m_sortKey = sortKey;
    m_bValid  = true;
}

bool Material::Load(
    const fs::path& _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    CleanUp_();

    // load file
    const Optional<Vector<Byte>> optData = LoadBinaryFile(_path);
    if (!optData)
    {
        return false;
    }
    const Vector<Byte>& data = optData.value();

    // verify
    flatbuffers::Verifier verifier = { data.data(), data.size() };
    if (!jmat::VerifyMaterialBuffer(verifier))
    {
        GlobalBuffer::SetLastError(R"(Material file is corrupted or invalid: "{}")", _path.native());
        return false;
    }

    // parse
    const jmat::Material* pJmatMat = jmat::GetMaterial(data.data());
    if (!pJmatMat)
    {
        GlobalBuffer::SetLastError(R"(Failed to parse material file: "{}")", _path.native());
        return false;
    }

    // parse
    bool bValid = true;
    bValid &= (pJmatMat->ambient_color() != nullptr);
    bValid &= (pJmatMat->diffuse_color() != nullptr);
    bValid &= (pJmatMat->specular_color() != nullptr);
    bValid &= (pJmatMat->albedo_color() != nullptr);
    bValid &= (pJmatMat->emissive_color() != nullptr);
    if (!bValid)
    {
        GlobalBuffer::SetLastError(R"(Material file is missing required fields: "{}")", _path.native());
        return false;
    }

    // material properties
    m_ambientColor                   = Vec3From(*pJmatMat->ambient_color());
    m_diffuseColor                   = Vec3From(*pJmatMat->diffuse_color());
    m_specularColor                  = Vec3From(*pJmatMat->specular_color());
    m_shininess                      = pJmatMat->shininess();
    m_albedoColor                    = Vec3From(*pJmatMat->albedo_color());
    m_roughness                      = pJmatMat->roughness();
    m_metallic                       = pJmatMat->metallic();
    m_roughnessTextureChannel        = TextureChannelFrom(pJmatMat->roughness_texture_channel());
    m_metallicTextureChannel         = TextureChannelFrom(pJmatMat->metallic_texture_channel());
    m_emissiveColor                  = Vec3From(*pJmatMat->emissive_color());
    m_emissiveIntensity              = pJmatMat->emissive_intensity();
    m_bAlphaCutout                   = pJmatMat->b_alpha_cutout();
    m_bDoubleSided                   = pJmatMat->b_double_sided();
    m_bNormalTextureFliped           = pJmatMat->b_normal_texture_fliped();
    m_lightTextureIntensity          = pJmatMat->light_texture_intensity();
    m_emissiveTextureIntensity       = pJmatMat->emissive_texture_intensity();
    m_vertexDisplacementTextureScale = pJmatMat->vertex_displacement_scale();

    // load image textures
    m_pAlbedoTexture             = TryImageTextureFrom(pJmatMat->albedo_texture_path());
    m_pNormalTexture             = TryImageTextureFrom(pJmatMat->normal_texture_path());
    m_pMetallicTexture           = TryImageTextureFrom(pJmatMat->metallic_texture_path());
    m_pRoughnessTexture          = TryImageTextureFrom(pJmatMat->roughness_texture_path());
    m_pAoTexture                 = TryImageTextureFrom(pJmatMat->ao_texture_path());
    m_pEmissiveTexture           = TryImageTextureFrom(pJmatMat->emissive_texture_path());
    m_pLightTexture              = TryImageTextureFrom(pJmatMat->light_texture_path());
    m_pVertexDisplacementTexture = TryImageTextureFrom(pJmatMat->vertex_displacement_texture_path());

    // create sort key
    const std::string path = _path.native();
    const UInt64      hash = HashOf(path);

    // done
    m_sortKey = static_cast<UInt16>(hash & 0xFFFF);
    m_path    = _path;
    m_bValid  = true;
    return true;
}

void Material::Unload()
{
    CleanUp_();
}

bool Material::Save(const fs::path& _path) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    JAM_ASSERT(IsValid(), "Cannot save invalid material");

    flatbuffers::FlatBufferBuilder builder { 1024 };

    // colors
    const jmat::Vec3 fbsAmbientColor  = JmatVec3From(m_ambientColor);
    const jmat::Vec3 fbsDiffuseColor  = JmatVec3From(m_diffuseColor);
    const jmat::Vec3 fbsSpecularColor = JmatVec3From(m_specularColor);
    const jmat::Vec3 fbsAlbedoColor   = JmatVec3From(m_albedoColor);
    const jmat::Vec3 fbsEmissiveColor = JmatVec3From(m_emissiveColor);

    // channels
    const jmat::eTextureChannel fbsMetallicTextureChannel  = JmatTextureChannelFrom(m_metallicTextureChannel);
    const jmat::eTextureChannel fbsRoughnessTextureChannel = JmatTextureChannelFrom(m_roughnessTextureChannel);

    // textures
    const flatbuffers::Offset<flatbuffers::String> fbsAlbedoTexturePath             = JmatStringOffsetFrom(builder, m_pAlbedoTexture);
    const flatbuffers::Offset<flatbuffers::String> fbsNormalTexturePath             = JmatStringOffsetFrom(builder, m_pNormalTexture);
    const flatbuffers::Offset<flatbuffers::String> fbsMetallicTexturePath           = JmatStringOffsetFrom(builder, m_pMetallicTexture);
    const flatbuffers::Offset<flatbuffers::String> fbsRoughnessTexturePath          = JmatStringOffsetFrom(builder, m_pRoughnessTexture);
    const flatbuffers::Offset<flatbuffers::String> fbsAOTexturePath                 = JmatStringOffsetFrom(builder, m_pAoTexture);
    const flatbuffers::Offset<flatbuffers::String> fbsEmissiveTexturePath           = JmatStringOffsetFrom(builder, m_pEmissiveTexture);
    const flatbuffers::Offset<flatbuffers::String> fbsLightTexturePath              = JmatStringOffsetFrom(builder, m_pLightTexture);
    const flatbuffers::Offset<flatbuffers::String> fbsVertexDisplacementTexturePath = JmatStringOffsetFrom(builder, m_pVertexDisplacementTexture);

    const flatbuffers::Offset<jmat::Material> fbsMaterial =
        jmat::CreateMaterial(
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
    return SaveBinaryFile(_path, { builder.GetBufferPointer(), NarrowCast<Int32>(builder.GetSize()) });
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

StringView Material::GetTypeName() const
{
    return k_name;
}

UInt16 Material::GetSortKey() const
{
    return m_sortKey;
}

void Material::CleanUp_()
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