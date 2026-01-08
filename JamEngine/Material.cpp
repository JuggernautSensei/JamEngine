#include "pch.h"

#include "Material.h"

#include "AssetManager.h"
#include "FileIOUtilities.h"
#include "Magics.h"
#include "MemView.h"
#include "Profile.h"

#include "resources/flatc/jmat_generated.h"
namespace fbs = flatbuffers;

namespace jam
{

[[nodiscard]] static Vec3 Vec3From(
    const jmat::Vec3& _vec)
{
    return { _vec.x(), _vec.y(), _vec.z() };
}

[[nodiscard]] static fs::path PathFrom(
    const fbs::String& _str)
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

[[nodiscard]] static fbs::Offset<fbs::String> FbsStringOffsetFrom(
    fbs::FlatBufferBuilder&  _builder,
    const Ref<ImageTexture>& _pTexOrNull)
{
    if (!_pTexOrNull)
    {
        return 0;
    }

    const fs::path& path = _pTexOrNull->GetPath();
    return _builder.CreateString(path.native());
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

[[nodiscard]] static Ref<ImageTexture> ImageTextureFromOrNull(
    const fbs::String* _pStrOrNull)
{
    // no texture
    if (!_pStrOrNull)
    {
        return nullptr;
    }

    // get path
    const fs::path path = PathFrom(*_pStrOrNull);

    // load texture
    Ref<ImageTexture> pTex;
    if (!MoveIfHasValue(AssetManager::LoadImageTexture(path), pTex))
    {
        return nullptr;
    }

    return pTex;
}

}   // namespace jam

//////////////////////////////////////////////////////////////////////

namespace jam
{

bool Material::Load(
    const fs::path& _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    CleanUp_();

    // load file
    Vector<Byte> data = {};
    if (!MoveIfHasValue(LoadBinaryFile(_path), data))
    {
        GlobalBuffer::SetLastError("Failed to load material file: {}. {}", _path.native(), GlobalBuffer::GetLastError());
        return false;
    }

    // verify
    fbs::Verifier verifier { data.data(), static_cast<size_t>(data.size()) };
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

    // name
    {
        const fbs::String* pJmatName = pJmatMat->name();
        if (!pJmatName)
        {
            GlobalBuffer::SetLastError(R"(Material file is missing name: "{}")", _path.native());
            return false;
        }

        m_name = pJmatName->c_str();
    }

    // params
    {
        const jmat::MaterialParams* pJmatParams = pJmatMat->params();
        if (!pJmatParams)
        {
            GlobalBuffer::SetLastError(R"(Material file is missing parameters: "{}")", _path.native());
            return false;
        }

        // fill native data
        m_params.ambient           = Vec3From(pJmatParams->ambient());
        m_params.diffuse           = Vec3From(pJmatParams->diffuse());
        m_params.specular          = Vec3From(pJmatParams->specular());
        m_params.shininess         = pJmatParams->shininess();
        m_params.albedo            = Vec3From(pJmatParams->albedo());
        m_params.roughness         = pJmatParams->roughness();
        m_params.metallic          = pJmatParams->metallic();
        m_params.emissive          = Vec3From(pJmatParams->emissive());
        m_params.emissiveIntensity = pJmatParams->emission_intensity();
        m_params.bAlphaCutout      = pJmatParams->b_alpha_cutout();
        m_params.bDoubleSided      = pJmatParams->b_double_sided();
    }

    // textures
    {
        const jmat::MaterialTextures* pJmatTextures = pJmatMat->textures();
        if (!pJmatTextures)
        {
            GlobalBuffer::SetLastError(R"(Material file is missing textures: "{}")", _path.native());
            return false;
        }

        // fill native textures
        SetTexture(eMaterialTextureType::Albedo, ImageTextureFromOrNull(pJmatTextures->albedo_map_path()));
        SetTexture(eMaterialTextureType::Ao, ImageTextureFromOrNull(pJmatTextures->ao_map_path()));
        SetTexture(eMaterialTextureType::Light, ImageTextureFromOrNull(pJmatTextures->light_map_path()));
        SetTexture(eMaterialTextureType::VertexDisplacement, ImageTextureFromOrNull(pJmatTextures->vertex_displacement_map_path()));
        SetTexture(eMaterialTextureType::Emissive, ImageTextureFromOrNull(pJmatTextures->emissive_map_path()));
        SetTexture(eMaterialTextureType::Normal, ImageTextureFromOrNull(pJmatTextures->normal_map_path()));
        SetTexture(eMaterialTextureType::Metallic, ImageTextureFromOrNull(pJmatTextures->metallic_map_path()));
        SetTexture(eMaterialTextureType::Roughness, ImageTextureFromOrNull(pJmatTextures->roughness_map_path()));
    }

    // settings
    {
        const jmat::MaterialTextureSettings* pJmatSettings = pJmatMat->texture_settings();
        if (!pJmatSettings)
        {
            GlobalBuffer::SetLastError(R"(Material file is missing texture settings: "{}")", _path.native());
            return false;
        }

        // fill texture settings
        m_settings.lightMapIntensity          = pJmatSettings->light_map_intensity();
        m_settings.vertexDisplacementMapScale = pJmatSettings->vertex_displacement_scale();
        m_settings.emissiveMapIntensity       = pJmatSettings->emissive_map_intensity();
        m_settings.bNormalMapFlipedY          = pJmatSettings->normal_map_fliped_y();
        m_settings.metallicMapChannel         = TextureChannelFrom(pJmatSettings->metallic_map_channel());
        m_settings.roughnessMapChannel        = TextureChannelFrom(pJmatSettings->roughness_map_channel());
    }

    // sort key
    {
        const UInt32 hash = HashOf(_path.native());
        m_sortKey         = static_cast<UInt16>(hash);
    }

    // finalize
    m_path            = _path;
    m_bValid          = true;
    m_bLoadedFromFile = true;
    return true;
}

void Material::LoadMemory(
    const MaterialParams&          _params,
    const MaterialTextures&        _textures,
    const MaterialTextureSettings& _settings)
{
    CleanUp_();

    m_params          = _params;
    m_textures        = _textures;
    m_settings        = _settings;
    m_bValid          = true;
    m_bLoadedFromFile = false;

    // sort key
    m_sortKey = MakeRandom<UInt16>(0);
}

void Material::Unload()
{
    CleanUp_();
}

bool Material::Save(
    const fs::path&      _path,
    const bool           _bReplaceExtension,
    const eSaveFileFlags _flags) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    JAM_ASSERT(IsValid(), "Cannot save invalid material");

    fbs::FlatBufferBuilder builder { 1024 };

    // name
    const fbs::Offset<fbs::String> fbsName = builder.CreateString(m_name);

    // params
    jmat::MaterialParams fbsParams = {
        JmatVec3From(m_params.ambient),
        JmatVec3From(m_params.diffuse),
        JmatVec3From(m_params.specular),
        m_params.shininess,
        JmatVec3From(m_params.albedo),
        m_params.roughness,
        m_params.metallic,
        JmatVec3From(m_params.emissive),
        m_params.emissiveIntensity,
        m_params.bAlphaCutout,
        m_params.bDoubleSided
    };

    // texture
    const fbs::Offset<fbs::String> fbsAlbedoMapPath             = FbsStringOffsetFrom(builder, GetTexture(eMaterialTextureType::Albedo));
    const fbs::Offset<fbs::String> fbsAoMapPath                 = FbsStringOffsetFrom(builder, GetTexture(eMaterialTextureType::Ao));
    const fbs::Offset<fbs::String> fbsLightMapPath              = FbsStringOffsetFrom(builder, GetTexture(eMaterialTextureType::Light));
    const fbs::Offset<fbs::String> fbsVertexDisplacementMapPath = FbsStringOffsetFrom(builder, GetTexture(eMaterialTextureType::VertexDisplacement));
    const fbs::Offset<fbs::String> fbsEmissiveMapPath           = FbsStringOffsetFrom(builder, GetTexture(eMaterialTextureType::Emissive));
    const fbs::Offset<fbs::String> fbsNormalMapPath             = FbsStringOffsetFrom(builder, GetTexture(eMaterialTextureType::Normal));
    const fbs::Offset<fbs::String> fbsMetallicMapPath           = FbsStringOffsetFrom(builder, GetTexture(eMaterialTextureType::Metallic));
    const fbs::Offset<fbs::String> fbsRoughnessMapPath          = FbsStringOffsetFrom(builder, GetTexture(eMaterialTextureType::Roughness));

    // textures
    const fbs::Offset<jmat::MaterialTextures> fbsTextures = jmat::CreateMaterialTextures(
        builder,
        fbsAlbedoMapPath,
        fbsAoMapPath,
        fbsLightMapPath,
        fbsVertexDisplacementMapPath,
        fbsEmissiveMapPath,
        fbsNormalMapPath,
        fbsMetallicMapPath,
        fbsRoughnessMapPath);

    // texture settings
    const float                 fbsLightMapIntensity       = m_settings.lightMapIntensity;
    const float                 fbsVertexDisplacementScale = m_settings.vertexDisplacementMapScale;
    const float                 fbsEmissiveMapIntensity    = m_settings.emissiveMapIntensity;
    const bool                  bFbsNormalMapFlipedY       = m_settings.bNormalMapFlipedY;
    const jmat::eTextureChannel fbsMetallicMapChannel      = JmatTextureChannelFrom(m_settings.metallicMapChannel);
    const jmat::eTextureChannel fbsRoughnessMapChannel     = JmatTextureChannelFrom(m_settings.roughnessMapChannel);

    const jmat::MaterialTextureSettings fbsTextureSettings = {
        fbsLightMapIntensity,
        fbsVertexDisplacementScale,
        fbsEmissiveMapIntensity,
        bFbsNormalMapFlipedY,
        fbsMetallicMapChannel,
        fbsRoughnessMapChannel
    };

    // material
    fbs::Offset<jmat::Material> fbsMaterial = jmat::CreateMaterial(builder, fbsName, &fbsParams, fbsTextures, &fbsTextureSettings);
    builder.Finish(fbsMaterial);

    // path
    if (_bReplaceExtension)
    {
        fs::path path = _path;
        path.replace_extension(k_jamMaterialExt.GetPtr());
        return SaveBinaryFile(_path, { builder.GetBufferPointer(), NarrowCast<Int32>(builder.GetSize()) }, _flags);
    }

    return SaveBinaryFile(_path, { builder.GetBufferPointer(), NarrowCast<Int32>(builder.GetSize()) }, _flags);
}

void Material::SetName(
    const StringView _name)
{
    m_name = _name.ToString();
}

void Material::SetVirtualPath(
    const fs::path& _path)
{
    JAM_ASSERT(m_bLoadedFromFile == false, "Cannot change path of a material loaded from file");
    m_path = _path;
}

void Material::SetParams(
    const MaterialParams& _params)
{
    m_params = _params;
}

const MaterialParams& Material::GetParams() const
{
    return m_params;
}

void Material::SetTextures(
    const MaterialTextures& _textures)
{
    m_textures = _textures;
}

const MaterialTextures& Material::GetTextures() const
{
    return m_textures;
}

void Material::SetTexture(
    const eMaterialTextureType _type,
    const Ref<ImageTexture>&   _texture)
{
    m_textures.Set(_type, _texture);
}

Ref<ImageTexture> Material::GetTexture(
    const eMaterialTextureType _type) const
{
    return m_textures.Get(_type);
}

void Material::SetTextureSettings(
    const MaterialTextureSettings& _settings)
{
    m_settings = _settings;
}

const MaterialTextureSettings& Material::GetTextureSettings() const
{
    return m_settings;
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

const StringView Material::GetName() const
{
    return m_name;
}

void Material::CleanUp_()
{
    m_path.clear();
    m_name            = {};
    m_sortKey         = 0;
    m_bValid          = false;
    m_bLoadedFromFile = false;
    m_params          = {};
    m_settings        = {};
    m_textures        = {};
}

}   // namespace jam