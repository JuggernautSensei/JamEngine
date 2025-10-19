#include "pch.h"

#include "Material.h"

#include "AssetManager.h"
#include "FileUtilities.h"
#include "Hash.h"
#include "Profile.h"
#include "resources/flatc/jmat_generated.h"

namespace jam
{

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

JAM_NODISCARD static Vec3 ConvertToNativeVec3(const jmat::Vec3& _vec)
{
    return { _vec.x(), _vec.y(), _vec.z() };
}

JAM_NODISCARD static jmat::Vec3 ConvertToFbsVec3(const Vec3& _vec)
{
    return { _vec.x, _vec.y, _vec.z };
}

JAM_NODISCARD static fs::path ConvertToPath(const flatbuffers::String& _str)
{
    return fs::path(_str.c_str());
}

JAM_NODISCARD static flatbuffers::Offset<flatbuffers::String> ConvertToFbsString(flatbuffers::FlatBufferBuilder& _fbb, const fs::path& _path)
{
    return _fbb.CreateString(_path.native());
}

////////////////////////////////////////////////////////////

void Material::Load()
{
    m_params    = MaterialParams {};
    m_bIsLoaded = true;
    m_path.clear();

    // sort key는 랜덤 값으로
    m_sortKey = CreateRandom<UInt16>(0, UInt16Limits::max());
}

eErrorCode Material::Load(const fs::path& _path)
{
    JAM_PROFILE_SCOPE_MS(R"(jam::Material::Save "{}")", _path.native());

    Unload();

    auto [ec, buffer] = ReadBinaryFromFile(_path);
    if (IsError(ec))
    {
        return ec;
    }

    // verify file
    flatbuffers::Verifier verifier { buffer.data(), buffer.size() };
    if (!jmat::VerifyMaterialBuffer(verifier))
    {
        SetErrorMessage(R"(Material file is corrupted or invalid: "{}")", _path.native());
        return eErrorCode::FileVertifyFailed;
    }

    const jmat::Material* pMat = jmat::GetMaterial(buffer.data());
    if (!pMat)
    {
        SetErrorMessage(R"(Failed to parse material file: "{}")", _path.native());
        return eErrorCode::FileParseFailed;
    }

    // 버전 검사
    Version version { pMat->version() };
    if (version.major != k_version.major)
    {
        SetErrorMessage(R"(Material file major version is different: {} (file) vs {} (engine))", version.ToString(), k_version.ToString());
        return eErrorCode::VersionMismatch;
    }

    if (version.minor != k_version.minor)
    {
        logger::Warn("Material file minor version is different: {} (file) vs {} (engine)", version.ToString(), k_version.ToString());
    }

    // 파라미터 로드
    bool bValid = true;
    bValid &= (pMat->ambient_color() != nullptr);
    bValid &= (pMat->diffuse_color() != nullptr);
    bValid &= (pMat->specular_color() != nullptr);
    bValid &= (pMat->albedo_color() != nullptr);
    bValid &= (pMat->emissive_color() != nullptr);
    if (!bValid)
    {
        return eErrorCode::FileParseFailed;
    }

    m_params.ambientColor      = ConvertToNativeVec3(*pMat->ambient_color());
    m_params.diffuseColor      = ConvertToNativeVec3(*pMat->diffuse_color());
    m_params.specularColor     = ConvertToNativeVec3(*pMat->specular_color());
    m_params.shininess         = pMat->shininess();
    m_params.albedoColor       = ConvertToNativeVec3(*pMat->albedo_color());
    m_params.roughness         = pMat->roughness();
    m_params.metallic          = pMat->metallic();
    m_params.emissiveColor     = ConvertToNativeVec3(*pMat->emissive_color());
    m_params.emissiveIntensity = pMat->emissive_intensity();
    m_params.opacity           = pMat->opacity();
    m_params.bDoubleSide       = pMat->b_double_side();
    m_params.bFlipYNormalMap   = pMat->b_fliped_y_normal_map();
    m_params.lightMapIntensity = pMat->light_map_intensity();
    m_params.aoMapIntensity    = pMat->ao_map_intensity();

    std::shared_ptr<ImageTexture>* const imageTextures[] = {
        &m_params.albedoMap,
        &m_params.normalMap,
        &m_params.metallicMap,
        &m_params.roughnessMap,
        &m_params.aoMap,
        &m_params.emissiveMap,
        &m_params.lightMap,
        &m_params.vertexDisplacementMap
    };

    const flatbuffers::String* const texturepaths[] = {
        pMat->albedo_map_path(),
        pMat->normal_map_path(),
        pMat->metallic_map_path(),
        pMat->roughness_map_path(),
        pMat->ao_map_path(),
        pMat->emissive_map_path(),
        pMat->light_map_path(),
        pMat->vertex_displacement_map_path()
    };

    static_assert(std::size(imageTextures) == std::size(texturepaths), "Texture handles and paths size mismatch");
    for (size_t i = 0; i < std::size(imageTextures); ++i)
    {
        if (texturepaths[i])
        {
            fs::path path    = ConvertToPath(*texturepaths[i]);
            auto [ec, asset] = asset_manager::GetOrLoadImageTexture(path);
            if (IsError(ec))
            {
                logger::Warn("Failed to load texture: {} (Error: {})", path.native(), EnumToString(ec));
            }
            *imageTextures[i] = asset;
        }
    }

    m_path      = _path;
    m_bIsLoaded = true;

    // 해쉬 갱신 (경로 기반)
    std::string_view pathStr = _path.native();
    MurmurHash64     hasher;
    hasher.Begin();
    hasher.Add(pathStr.data(), pathStr.size());
    UInt64 hash = hasher.End();
    m_sortKey   = static_cast<UInt16>(hash);

    return eErrorCode::Ok;
}

void Material::Unload()
{
    m_path.clear();
    m_bIsLoaded = false;
    m_params    = MaterialParams {};
}

eErrorCode Material::Save(const fs::path& _path) const
{
    JAM_ASSERT(IsLoaded(), "Material is not loaded");
    JAM_PROFILE_SCOPE_MS(R"(jam::Material::Save "{}")", _path.native());

    flatbuffers::FlatBufferBuilder builder { 1024 };

    // colors
    jmat::Vec3 fbsAmbientColor  = ConvertToFbsVec3(m_params.ambientColor);
    jmat::Vec3 fbsDiffuseColor  = ConvertToFbsVec3(m_params.diffuseColor);
    jmat::Vec3 fbsSpecularColor = ConvertToFbsVec3(m_params.specularColor);
    jmat::Vec3 fbsAlbedoColor   = ConvertToFbsVec3(m_params.albedoColor);
    jmat::Vec3 fbsEmissiveColor = ConvertToFbsVec3(m_params.emissiveColor);

    // textures
    flatbuffers::Offset<flatbuffers::String> fbsAlbedoMapPath             = m_params.albedoMap ? ConvertToFbsString(builder, m_params.albedoMap->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsNormalMapPath             = m_params.normalMap ? ConvertToFbsString(builder, m_params.normalMap->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsMetallicMapPath           = m_params.metallicMap ? ConvertToFbsString(builder, m_params.metallicMap->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsRoughnessMapPath          = m_params.roughnessMap ? ConvertToFbsString(builder, m_params.roughnessMap->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsAOMapPath                 = m_params.aoMap ? ConvertToFbsString(builder, m_params.aoMap->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsEmissiveMapPath           = m_params.emissiveMap ? ConvertToFbsString(builder, m_params.emissiveMap->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsLightMapPath              = m_params.lightMap ? ConvertToFbsString(builder, m_params.lightMap->GetPath()) : 0;
    flatbuffers::Offset<flatbuffers::String> fbsVertexDisplacementMapPath = m_params.vertexDisplacementMap ? ConvertToFbsString(builder, m_params.vertexDisplacementMap->GetPath()) : 0;

    Int32 version     = k_version.Pack();
    auto  fbsMaterial = jmat::CreateMaterial(
        builder,
        version,
        &fbsAmbientColor,
        &fbsDiffuseColor,
        &fbsSpecularColor,
        m_params.shininess,
        &fbsAlbedoColor,
        m_params.roughness,
        m_params.metallic,
        &fbsEmissiveColor,
        m_params.emissiveIntensity,
        m_params.opacity,
        m_params.bDoubleSide,
        m_params.bFlipYNormalMap,
        m_params.lightMapIntensity,
        m_params.aoMapIntensity,
        m_params.vertexDisplacementMapScale,
        fbsAlbedoMapPath,
        fbsNormalMapPath,
        fbsMetallicMapPath,
        fbsRoughnessMapPath,
        fbsAOMapPath,
        fbsEmissiveMapPath,
        fbsLightMapPath,
        fbsVertexDisplacementMapPath);
    builder.Finish(fbsMaterial);

    // 파일 쓰기
    std::ofstream ofs { _path, std::ios::binary };
    if (!ofs.is_open())
    {
        SetErrorMessage(R"(Failed to open material file for writing: "{}")", _path.native());
        return eErrorCode::FileOpenFailed;
    }

    Byte* buf = builder.GetBufferPointer();
    ofs.write(reinterpret_cast<const char*>(buf), builder.GetSize());
    if (ofs.bad() || ofs.fail())
    {
        SetErrorMessage(R"(Failed to write material file: "{}")", _path.native());
        return eErrorCode::FileWriteFailed;
    }

    return eErrorCode::Ok;
}

bool Material::IsLoaded() const
{
    return m_bIsLoaded;
}

const fs::path& Material::GetPath() const
{
    return m_path;
}

eAssetType Material::GetType() const
{
    return k_type;
}

UInt32 Material::GetHash() const
{
    return k_hash;
}

std::string_view Material::GetName() const
{
    return k_name;
}

UInt16 Material::GetSortKey() const
{
    return m_sortKey;
}

void Material::SetParams(const MaterialParams& _params)
{
    m_params = _params;
}

const MaterialParams& Material::GetParams() const
{
    return m_params;
}

}   // namespace jam