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
    //};
    //// static_assert(IsMemCpyable<MaterialParamsDto, MaterialParams>, "MaterialParamsDto must be trivially copyable for direct memory serialization.");
    // static_assert(std::is_trivially_copyable_v<MaterialParamsDto>);
    // static_assert(std::is_trivially_copyable_v<MaterialParams>);
    // static_assert(sizeof(MaterialParamsDto) == sizeof(MaterialParams));

    using MaterialTexturesDto        = ArrayDto<StringViewDto, CountOf<eMaterialTexture>()>;
    using MaterialTextureSettingsDto = MaterialTextureSettings;

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

Ref<Material> Material::Load(
    const MaterialDesc& _desc)
{
    return new Material(_desc);
}

Expect<Ref<Material>> Material::LoadFromFile(
    const StringView _path)
{
    Ref<Material> pMaterial = new Material;
    if (const Error err = pMaterial->Load_(_path))
    {
        return err;
    }
    return pMaterial;
}

Error Material::Save(
    const StringView                 _path,
    const bool                       _bReplaceExtension,
    const eFileWriterCollisionPolicy _policy) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path);
    JAM_ASSERT(IsValid(), "Cannot save an invalid material");

    StringView dir = {}, base = {}, ext = {};
    _path.Decompose(&dir, nullptr, &base, &ext);

    String path = dir;
    path.Append(base);
    path.Append(_bReplaceExtension ? k_extension : ext);

    Expect<MaterialSaver> saver = MaterialSaver::Open(path, _policy);
    if (!saver)
    {
        return saver.GetError();
    }

    // write data
    MaterialDto dto = {};
    dto.name        = StringViewDtoFrom(m_name);
    dto.params      = std::bit_cast<MaterialParamsDto>(m_desc.params);
    dto.settings    = m_desc.settings;

    // write texture paths
    for (eMaterialTexture type: RangesOf<eMaterialTexture>())
    {
        const Ref<Texture>& pTexture = m_desc.textures[type];
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
    m_name = _name;
}

void Material::SetVirtualPath(
    const StringView _path)
{
    JAM_ASSERT(m_bLoadedFromFile == false, "Cannot change path of a material loaded from file");
    m_path = _path;
}

void Material::Invalidate()
{
    CleanUp_();
}

void Material::SetDesc(const MaterialDesc& _desc)
{
    m_desc = _desc;
}

const MaterialDesc& Material::GetDesc() const
{
    return m_desc;
}

bool Material::IsValid() const
{
    return m_bValid;
}

StringView Material::GetPath() const
{
    return m_path;
}

eAsset Material::GetAssetType() const
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

Material::Material(const MaterialDesc& _desc)
    : m_desc(_desc)
{
    m_bValid          = true;
    m_bLoadedFromFile = false;
    m_sortKey         = MakeRandom<UInt16>(0);
}

Error Material::Load_(
    StringView _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path);

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
    m_name          = StringFrom(pDto->name);
    m_desc.params   = std::bit_cast<MaterialParams>(pDto->params);
    m_desc.settings = pDto->settings;

    // load texture
    for (eMaterialTexture type: RangesOf<eMaterialTexture>())
    {
        const StringViewDto& path = pDto->textures[UnderlyingFrom(type)];
        if (!path.empty())
        {
            Expect<Ref<Texture>> pTexture = GetAssetManager().LoadTexture(StringViewFrom(path));
            if (!pTexture)
            {
                return pTexture.GetError();
            }

            m_desc.textures[type] = pTexture.Take();
        }
    }

    // finalize
    const StringView path = _path;
    const UInt32     hash = path.Hash32();
    m_sortKey             = static_cast<UInt16>(hash);
    m_path                = _path;
    m_bValid              = true;
    m_bLoadedFromFile     = true;
    return Error::None;
}

void Material::CleanUp_()
{
    m_path.Clear();
    m_name.Clear();
    m_sortKey         = 0;
    m_bValid          = false;
    m_bLoadedFromFile = false;
    m_desc            = {};
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

    Expect<Ref<Material>> pMaterial = GetAssetManager().LoadMaterial(path);
    if (!pMaterial)
    {
        return pMaterial.GetError();
    }

    _pMaterial = pMaterial.Take();
    return Error::None;
}

}   // namespace Jam