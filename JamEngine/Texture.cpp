#include "pch.h"

#include "Texture.h"

#include "AssetManager.h"
#include "Graphics.h"
#include "Profile.h"
#include "ImageBase.h"
#include "ImageExporter.h"
#include "ImageImporter.h"

namespace Jam
{

Texture::~Texture()
{
    CleanUp_();
}

Texture::Texture(Texture&& _other) noexcept
    : m_path(std::move(_other.m_path))
    , m_name(std::move(_other.m_name))
    , m_texh(_other.m_texh)
    , m_res(_other.m_res)
    , m_type(_other.m_type)
    , m_bLoadedFromFile(_other.m_bLoadedFromFile)
{
    _other.m_texh            = k_invalidHandle;
    _other.m_res             = { 0, 0 };
    _other.m_type            = eTextureType::Texture2D;
    _other.m_bLoadedFromFile = false;
}

Texture& Texture::operator=(Texture&& _other) noexcept
{
    if (this != &_other)
    {
        CleanUp_();

        m_path            = std::move(_other.m_path);
        m_name            = std::move(_other.m_name);
        m_texh            = _other.m_texh;
        m_res             = _other.m_res;
        m_type            = _other.m_type;
        m_bLoadedFromFile = _other.m_bLoadedFromFile;

        _other.m_texh            = k_invalidHandle;
        _other.m_res             = { 0, 0 };
        _other.m_type            = eTextureType::Texture2D;
        _other.m_bLoadedFromFile = false;
    }
    return *this;
}

Expect<Ref<Texture>> Texture::Create(
    const Path& _path)
{
    Ref<Texture> pTex = new Texture;
    if (const Error err = pTex->Load(_path))
    {
        return err;
    }
    return pTex;
}

Ref<Texture> Texture::Create(
    const TextureHandle _texh)
{
    Ref<Texture> pTex = new Texture;
    pTex->Load(_texh);
    return pTex;
}

Error Texture::Load(
    const Path& _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path);

    // import dds (native image format is dds.)
    ImageImportOptions options = {};
    options.bGenerateMips      = true;

    Expect<ImageImporter> importer = ImageImporter::ImportFromFile(_path, eImageFormat::DDS, options);
    if (!importer)
    {
        return importer.GetError();
    }

    // done.
    CleanUp_();
    m_texh = JAM_GFX.CreateTexture(importer->GetImageContainer());

    const TextureInfo info = JAM_GFX.GetTextureInfo(m_texh);
    m_res                  = { info.width, info.height };
    m_type                 = info.type;
    m_path                 = _path;
    m_bLoadedFromFile      = true;
    return Error::None;
}

void Texture::Load(
    const TextureHandle _texh)
{
    JAM_ASSERT(_texh, "_texh is invalid.");
    CleanUp_();
    m_texh = _texh;

    const TextureInfo info = JAM_GFX.GetTextureInfo(m_texh);
    m_res                  = { info.width, info.height };
    m_type                 = info.type;
}

Error Texture::Save(
    const Path&                 _path,
    const bool                  _bReplaceExtension,
    const Flags<eSaveFileFlags> _flags) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    JAM_ASSERT(IsValid(), "Cannot save an invalid texture.");

    if (_bReplaceExtension)
    {
        const Path path = ReplacedExtension(_path, k_extension);
        return ImageExporter::ExportToFile(m_texh, path, eImageFormat::DDS, _flags);
    }
    else
    {
        return ImageExporter::ExportToFile(m_texh, _path, eImageFormat::DDS, _flags);
    }
}

void Texture::SetName(
    const StringView _name)
{
    m_name = _name.ToString();
}

void Texture::SetVirtualPath(
    const Path& _path)
{
    JAM_ASSERT(m_bLoadedFromFile == false, "Cannot change path of an asset loaded from file.");
    m_path = _path;
}

void Texture::Invalidate()
{
    CleanUp_();
}

bool Texture::IsValid() const
{
    return m_texh;
}

eAssetType Texture::GetAssetType() const
{
    return k_asestType;
}

StringView Texture::GetTypeName() const
{
    return k_typeName;
}

UInt32 Texture::GetTypeHash() const
{
    return k_typeHash;
}

const Path& Texture::GetPath() const
{
    return m_path;
}

StringView Texture::GetName() const
{
    return m_name;
}

StringView Texture::GetExtension() const
{
    return k_extension;
}

StringView Texture::GetFilter() const
{
    return k_filter;
}

TextureHandle Texture::GetHandle() const
{
    return m_texh;
}

Vec2i Texture::GetResolution() const
{
    return m_res;
}

eTextureType Texture::GetTextureType() const
{
    return m_type;
}

void Texture::Bind(
    const eShader _shader,
    const Int32   _slot) const
{
    JAM_GFX.SetTexture(m_texh, _shader, _slot);
}

void Texture::Unbind(
    const eShader _shader,
    const Int32   _slot) const
{
    JAM_GFX.SetTexture(k_invalidHandle, _shader, _slot);
}

void Texture::CleanUp_()
{
    JAM_DESTROY_TEXTURE(m_texh);
    m_path.clear();
    m_name.Clear();
    m_bLoadedFromFile = false;
    m_res             = { 0, 0 };
    m_type            = eTextureType::Texture2D;
}

void WriteToArchive(
    ArchiveWriter&      _archive,
    const Ref<Texture>& _pTexture)
{
    if (_pTexture && _pTexture->IsValid())
    {
        _archive.Write(_pTexture->GetPath());
    }
}

Error ReadFromArchive(
    ArchiveReader& _archive,
    Ref<Texture>&  _pTexture)
{
    StringView path = {};
    if (const Error err = _archive.Read(path))
    {
        return err;
    }

    Expect<Ref<Texture>> pTex = JAM_ASSET_MGR.LoadTexture(PathFrom(path));
    if (!pTex)
    {
        return pTex.GetError();
    }

    _pTexture = pTex.Take();
    return Error::None;
}

}   // namespace Jam