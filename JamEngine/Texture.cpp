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
    _other.m_texh            = k_nullHandle;
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

        _other.m_texh            = k_nullHandle;
        _other.m_res             = { 0, 0 };
        _other.m_type            = eTextureType::Texture2D;
        _other.m_bLoadedFromFile = false;
    }
    return *this;
}

Texture::Texture(
    const TextureHandle _texh)
    : m_texh(_texh)
{
    JAM_ASSERT(m_texh, "Texture handle must be valid.");

    const TextureInfo info = GetGraphics().GetTextureInfo(m_texh);
    m_res                  = { info.width, info.height };
    m_type                 = info.type;
}

Ref<Texture> Texture::Load(
    const TextureHandle _texh)
{
    return new Texture(_texh);
}

Expect<Ref<Texture>> Texture::LoadFromFile(
    const StringView _path)
{
    Ref<Texture> pTexture = new Texture;
    if (const Error err = pTexture->Load_(_path))
    {
        return err;
    }
    return pTexture;
}

Error Texture::Load_(
    StringView _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path);

    // import dds (native image format is dds.)
    ImageImporterInfo options = {};
    options.bGenerateMips     = true;

    Expect<ImageImporter> importer = ImageImporter::ImportFromFile(_path, eImageFormat::DDS, options);
    if (!importer)
    {
        return importer.GetError();
    }

    // done.
    CleanUp_();
    m_texh = GetGraphics().CreateTexture(importer->GetImageContainer());

    const TextureInfo info = GetGraphics().GetTextureInfo(m_texh);
    m_res                  = { info.width, info.height };
    m_type                 = info.type;
    m_path                 = _path;
    m_bLoadedFromFile      = true;
    return Error::None;
}

Error Texture::Save(
    const StringView                 _path,
    const bool                       _bReplaceExtension,
    const eFileWriterCollisionPolicy _policy) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path);
    JAM_ASSERT(IsValid(), "Cannot save an invalid texture.");

    StringView dir = {}, base = {}, ext = {};
    _path.Decompose(&dir, nullptr, &base, &ext);

    String path = dir;
    path.Set(dir);
    path.Append(base);
    path.Append(k_extension);

    return ImageExporter::ExportToFile(m_texh, path, eImageFormat::DDS, false, _policy);
}

void Texture::SetName(
    const StringView _name)
{
    m_name = _name;
}

void Texture::SetVirtualPath(
    const StringView _path)
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

StringView Texture::GetPath() const
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
    GetGraphics().SetTexture(m_texh, _shader, _slot);
}

void Texture::Unbind(
    const eShader _shader,
    const Int32   _slot) const
{
    GetGraphics().SetTexture(k_nullHandle, _shader, _slot);
}

void Texture::CleanUp_()
{
    JAM_DESTROY_TEXTURE(m_texh);
    m_path.Clear();
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

    Expect<Ref<Texture>> pTexture = GetAssetManager().LoadTexture(path);
    if (!pTexture)
    {
        return pTexture.GetError();
    }

    _pTexture = pTexture.Take();
    return Error::None;
}

}   // namespace Jam