#include "pch.h"

#include "Material.h"

#include "AssetManager.h"
#include "FileIOUtilities.h"
#include "Magics.h"
#include "MemView.h"
#include "Profile.h"
#include "Cista.h"

namespace jam
{

struct MaterialTexturesSchema
{
    [[nodiscard]] const cista::offset::string& Get(const eMaterialTextureType _type) const
    {
        return textures[IntFrom(_type)];
    }

    void Set(const eMaterialTextureType _type, const cista::offset::string& _texturePath)
    {
        textures[IntFrom(_type)] = _texturePath;
    }

    CistaArray<CistaString, NumEnums<eMaterialTextureType>()> textures = {};
};

struct MaterialSchema
{
    CistaString             name     = {};
    MaterialParams          params   = {};
    MaterialTexturesSchema  textures = {};
    MaterialTextureSettings settings = {};
};

//////////////////////////////////////////////////////////////

bool Material::Load(
    const fs::path& _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    CleanUp_();

    // load file
    Vector<Byte> data = {};
    if (!MoveIfHasValue(LoadBinaryFile(_path), data))
    {
        return false;
    }

    // deserialize
    MaterialSchema* const pMat = nullptr;
    if (!MoveIfHasValue(DeserializeCista<MaterialSchema>(data), pMat))
    {
        return false;
    }

    // fill native data
    m_name     = pMat->name;
    m_params   = pMat->params;
    m_settings = pMat->settings;

    // textures
    MaterialTexturesSchema& texSchema = pMat->textures;
    for (eMaterialTextureType type: eMaterialTextureType {})
    {
        const CistaString& path = texSchema.Get(type);
        if (!path.empty())
        {
            Ref<ImageTexture> pTexture = nullptr;
            if (!MoveIfHasValue(AssetManager::LoadImageTexture(path.str()), pTexture))
            {
                return false;
            }

            SetTexture(type, pTexture);
        }
    }

    // sort key
    const UInt32 hash = HashOf(_path.native());
    m_sortKey         = static_cast<UInt16>(hash);

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
    if (!IsValid())
    {
        GlobalBuffer::SetLastError("Cannot save invalid material to file: {}", _path.native());
        return false;
    }

    // prepare schema
    MaterialSchema schema = {};
    schema.name           = m_name;
    schema.params         = m_params;
    schema.settings       = m_settings;

    // textures
    MaterialTexturesSchema& texSchema = schema.textures;
    for (eMaterialTextureType type: eMaterialTextureType {})
    {
        const Ref<ImageTexture> pTexture = GetTexture(type);
        if (pTexture)
        {
            texSchema.Set(type, CistaString { pTexture->GetPath().native() });
        }
    }

    // serialize
    Vector<Byte> data = {};
    if (!MoveIfHasValue(SerializeCista(schema), data))
    {
        return false;
    }

    // save file
    return SaveBinaryFile(_path, data, _flags);
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