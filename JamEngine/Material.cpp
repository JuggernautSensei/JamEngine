#include "pch.h"

#include "Material.h"

#include "AssetManager.h"
#include "AssetSaveLoader.h"
#include "Profile.h"

namespace Jam
{

namespace   // data transfer objects
{

    struct MaterialParamsDto
    {
        Vec3Dto ambient   = {};
        Vec3Dto diffuse   = {};
        Vec3Dto specular  = {};
        float   shininess = 32.0f;

        Vec3Dto albedo    = {};
        float   roughness = 1.f;
        float   metallic  = 1.f;

        Vec3Dto emissive          = {};
        float   emissiveIntensity = 1.f;

        bool bAlphaCutout = false;
        bool bDoubleSided = false;
    };
    static_assert(sizeof(MaterialParamsDto) == sizeof(MaterialParams));

    using MaterialTexturesDto        = ArrayDto<StringViewDto, CountOf<eMaterialTextureType>()>;
    using MaterialTextureSettingsDto = MaterialTextureSettings;   // same layout

    struct MaterialDto
    {
        StringViewDto              name     = {};
        MaterialParamsDto          params   = {};
        MaterialTexturesDto        textures = {};
        MaterialTextureSettingsDto settings = {};
    };

    using MaterialLoader = AssetLoader<MaterialDto>;
    using MaterialSaver  = AssetSaver<MaterialDto>;

}   // namespace

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Expect<Ref<Material>> Material::Create(
    const Path& _path)
{
    Ref<Material> pMaterial = new Material;
    if (const Error err = pMaterial->Load(_path))
    {
        return err;
    }
    return pMaterial;
}

Ref<Material> Material::Create(
    const MaterialParams&          _params,
    const MaterialTextures&        _textures,
    const MaterialTextureSettings& _settings)
{
    Ref<Material> pMaterial = new Material;
    pMaterial->Load(_params, _textures, _settings);
    return pMaterial;
}

Error Material::Load(
    const Path& _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());

    // io
    Expect<MaterialLoader> loader = MaterialLoader::Open(_path);
    if (!loader)
    {
        return loader.GetError();
    }

    // read
    Expect<MaterialDto> pDto = loader->Read();
    if (!pDto)
    {
        return pDto.GetError();
    }

    // read data
    CleanUp_();
    m_name                                = StringFrom(pDto->name);
    m_params.ambient                      = pDto->params.ambient.ToVec3();
    m_params.diffuse                      = pDto->params.diffuse.ToVec3();
    m_params.specular                     = pDto->params.specular.ToVec3();
    m_params.shininess                    = pDto->params.shininess;
    m_params.albedo                       = pDto->params.albedo.ToVec3();
    m_params.roughness                    = pDto->params.roughness;
    m_params.metallic                     = pDto->params.metallic;
    m_params.emissive                     = pDto->params.emissive.ToVec3();
    m_params.emissiveIntensity            = pDto->params.emissiveIntensity;
    m_params.bAlphaCutout                 = pDto->params.bAlphaCutout;
    m_params.bDoubleSided                 = pDto->params.bDoubleSided;
    m_settings.lightMapIntensity          = pDto->settings.lightMapIntensity;
    m_settings.vertexDisplacementMapScale = pDto->settings.vertexDisplacementMapScale;
    m_settings.bNormalMapFlipedY          = pDto->settings.bNormalMapFlipedY;
    m_settings.emissiveMapIntensity       = pDto->settings.emissiveMapIntensity;

    // load texture
    for (eMaterialTextureType type: EnumRanges<eMaterialTextureType>())
    {
        const StringViewDto& path = pDto->textures[UnderlyingFrom(type)];
        if (!path.empty())
        {
            Expect<Ref<Texture>> pTexture = JAM_ASSET_MGR.LoadTexture(PathFrom(path));
            if (!pTexture)
            {
                return pTexture.GetError();
            }

            m_textures[type] = pTexture.Take();
        }
    }

    // finalize
    const StringView path = _path.native();
    const UInt32     hash = path.Hash32();
    m_sortKey             = static_cast<UInt16>(hash);
    m_path                = _path;
    m_bValid              = true;
    m_bLoadedFromFile     = true;
    return Error::None;
}

void Material::Load(
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
    m_sortKey         = MakeRandom<UInt16>(0);
}

Error Material::Save(
    const Path&                 _path,
    const bool                  _bReplaceExtension,
    const Flags<eSaveFileFlags> _flags) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    JAM_ASSERT(IsValid(), "Cannot save an invalid material");

    Expect<MaterialSaver> saver = {};
    if (_bReplaceExtension)
    {
        const Path path = ReplacedExtension(_path, k_extension);
        saver           = MaterialSaver::Open(path, _flags);
    }
    else
    {
        saver = MaterialSaver::Open(_path, _flags);
    }

    if (!saver)
    {
        return saver.GetError();
    }

    // write data
    MaterialDto dto                         = {};
    dto.name                                = StringViewDtoFrom(m_name);
    dto.params.ambient                      = Vec3Dto::FromVec3(m_params.ambient);
    dto.params.diffuse                      = Vec3Dto::FromVec3(m_params.diffuse);
    dto.params.specular                     = Vec3Dto::FromVec3(m_params.specular);
    dto.params.shininess                    = m_params.shininess;
    dto.params.albedo                       = Vec3Dto::FromVec3(m_params.albedo);
    dto.params.roughness                    = m_params.roughness;
    dto.params.metallic                     = m_params.metallic;
    dto.params.emissive                     = Vec3Dto::FromVec3(m_params.emissive);
    dto.params.emissiveIntensity            = m_params.emissiveIntensity;
    dto.params.bAlphaCutout                 = m_params.bAlphaCutout;
    dto.params.bDoubleSided                 = m_params.bDoubleSided;
    dto.settings.lightMapIntensity          = m_settings.lightMapIntensity;
    dto.settings.vertexDisplacementMapScale = m_settings.vertexDisplacementMapScale;
    dto.settings.bNormalMapFlipedY          = m_settings.bNormalMapFlipedY;
    dto.settings.emissiveMapIntensity       = m_settings.emissiveMapIntensity;

    // write texture paths
    for (eMaterialTextureType type: EnumRanges<eMaterialTextureType>())
    {
        const Ref<Texture>& pTexture = m_textures[type];
        if (pTexture && pTexture->IsValid())
        {
            dto.textures[UnderlyingFrom(type)] = StringViewDtoFrom(pTexture->GetPath());
        }
    }

    return saver->Write(dto);
}

void Material::SetName(
    const StringView _name)
{
    m_name = _name.ToString();
}

void Material::SetVirtualPath(
    const Path& _path)
{
    JAM_ASSERT(m_bLoadedFromFile == false, "Cannot change path of a material loaded from file");
    m_path = _path;
}

void Material::Invalidate()
{
    CleanUp_();
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
    const MaterialTextures& _texes)
{
    m_textures = _texes;
}

const MaterialTextures& Material::GetTextures() const
{
    return m_textures;
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

const Path& Material::GetPath() const
{
    return m_path;
}

eAssetType Material::GetAssetType() const
{
    return k_assetType;
}

StringView Material::GetTypeName() const
{
    return k_typeName;
}

UInt32 Material::GetTypeHash() const
{
    return k_typeHash;
}

UInt16 Material::GetSortKey() const
{
    return m_sortKey;
}

StringView Material::GetName() const
{
    return m_name;
}

StringView Material::GetExtension() const
{
    return k_extension;
}

StringView Material::GetFilter() const
{
    return k_filter;
}

void Material::CleanUp_()
{
    m_path.clear();
    m_name.Clear();
    m_sortKey         = 0;
    m_bValid          = false;
    m_bLoadedFromFile = false;
    m_params          = {};
    m_settings        = {};
    m_textures        = {};
}

void WriteToArchive(
    ArchiveWriter&       _archive,
    const Ref<Material>& _pMaterial)
{
    if (_pMaterial && _pMaterial->IsValid())
    {
        _archive.Write(_pMaterial->GetPath());
    }
}

Error ReadFromArchive(
    ArchiveReader& _archive,
    Ref<Material>& _pMaterial)
{
    StringView path = {};
    if (const Error err = _archive.Read(path))
    {
        return err;
    }

    Expect<Ref<Material>> pMaterial = JAM_ASSET_MGR.LoadMaterial(path.ToStdString());
    if (!pMaterial)
    {
        return pMaterial.GetError();
    }

    _pMaterial = pMaterial.Take();
    return Error::None;
}

}   // namespace Jam