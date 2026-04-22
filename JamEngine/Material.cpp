#include "pch.h"

#include "Material.h"

#include "AssetManager.h"
#include "AssetSaveLoader.h"
#include "Profile.h"
#include "Random.h"

namespace Jam
{

namespace Dto
{

    struct MaterialParams
    {
        Vec3  ambient   = {};
        Vec3  diffuse   = {};
        Vec3  specular  = {};
        float shininess = 32.0f;

        Vec3  albedo    = {};
        float roughness = 1.f;
        float metallic  = 1.f;

        Vec3  emissive          = {};
        float emissiveIntensity = 1.f;

        bool bAlphaCutout = false;
        bool bDoubleSided = false;
    };

    struct Material
    {
        StringView                                     name     = {};
        MaterialParams                                 params   = {};
        Array<StringView, CountOf<eMaterialTexture>()> textures = {};
        MaterialTextureSettings                        settings = {};
    };
}   // namespace Dto

namespace
{

    using MaterialLoader = AssetLoader<Dto::Material>;
    using MaterialSaver  = AssetSaver<Dto::Material>;

}   // namespace

Ref<Material> Material::Load()
{
    return new Material;
}

Expect<Ref<Material>> Material::LoadFromFile(
    const StringView _path)
{
    Ref<Material> pMaterial = new Material;
    if (const Error err = pMaterial->LoadFromFile_(_path))
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

    String path = _path;
    if (_bReplaceExtension)
    {
        path.ReplaceExtension(kExtension);
    }

    Expect<MaterialSaver> saver = MaterialSaver::Open(path, _policy);
    if (!saver)
    {
        return saver.GetError();
    }

    // write data
    Dto::Material material = {};
    material.name          = Dto::Wrap(m_name);
    material.params        = std::bit_cast<Dto::MaterialParams>(m_params);
    material.settings      = m_settings;

    for (eMaterialTexture type: RangesOf<eMaterialTexture>())
    {
        const Ref<Texture>& pTexture = m_textures[type];
        if (pTexture && pTexture->IsValid())
        {
            material.textures[UnderlyingFrom(type)] = Dto::Wrap(pTexture->GetPath());
        }
    }

    return saver->Write(material);
}

void Material::SetName(
    const StringView _name)
{
    m_name = _name;
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

void Material::SetTextures(
    const EnumArray<eMaterialTexture, Ref<Texture>>& _textures)
{
    m_textures = _textures;
}

void Material::SetTexture(
    const eMaterialTexture _type,
    const Ref<Texture>&    _texture)
{
    m_textures[_type] = _texture;
}

void Material::SetSettings(
    const MaterialTextureSettings& _settings)
{
    m_settings = _settings;
}

const MaterialParams& Material::GetParams() const
{
    return m_params;
}

const EnumArray<eMaterialTexture, Ref<Texture>>& Material::GetTextures() const
{
    return m_textures;
}

const Ref<Texture>& Material::GetTexture(
    const eMaterialTexture _type) const
{
    return m_textures[_type];
}

const MaterialTextureSettings& Material::GetSettings() const
{
    return m_settings;
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
    return kAssetType;
}

StringView Material::GetTypeName() const
{
    return kTypeName;
}

UInt32 Material::GetTypeHash() const
{
    return kTypeHash;
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
    return kExtension;
}

StringView Material::GetFilter() const
{
    return kFilter;
}

Material::Material()
{
    RngMWC rng {};
    m_sortKey = static_cast<UInt16>(rng.Generate());
}

Error Material::LoadFromFile_(
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
    Expect<Dto::Material> materialOrNot = loader->Read();
    if (!materialOrNot)
    {
        return materialOrNot.GetError();
    }

    // clean up before loading new data
    CleanUp_();

    const Dto::Material& material = materialOrNot.GetValue();
    m_name                        = Dto::Unwrap(material.name);
    m_params                      = std::bit_cast<MaterialParams>(materialOrNot->params);
    m_settings                    = materialOrNot->settings;

    // load texture
    for (eMaterialTexture type: RangesOf<eMaterialTexture>())
    {
        const StringView path = Dto::Unwrap(materialOrNot->textures[UnderlyingFrom(type)]);
        if (!path.IsEmpty())
        {
            AssetManager&        mgr      = GetAssetManager();
            Expect<Ref<Texture>> pTexture = mgr.LoadTexture(path);
            if (!pTexture)
            {
                return pTexture.GetError();
            }

            m_textures[type] = pTexture.Take();
        }
    }

    // finalize
    const StringView path = _path;
    const UInt32     hash = path.Hash32();
    m_sortKey             = static_cast<UInt16>(hash);
    m_path                = _path;
    m_bValid              = true;
    return Error::None;
}

void Material::CleanUp_()
{
    m_path.Clear();
    m_name.Clear();
    m_sortKey  = 0;
    m_bValid   = false;
    m_params   = {};
    m_textures = {};
    m_settings = {};
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

    AssetManager&         mgr           = GetAssetManager();
    Expect<Ref<Material>> materialOrNot = mgr.LoadMaterial(path);
    if (!materialOrNot)
    {
        return materialOrNot.GetError();
    }

    _pMaterial = materialOrNot.Take();
    return Error::None;
}

}   // namespace Jam