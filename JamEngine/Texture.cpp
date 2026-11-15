#include "pch.h"

#include "Texture.h"

#include "AssetManager.h"
#include "Graphics.h"
#include "Profile.h"
#include "Image.h"

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
    , m_info(_other.m_info)
{
    _other.m_texh = kInvalidHandle;
    _other.m_info = {};
}

Texture& Texture::operator=(Texture&& _other) noexcept
{
    if (this != &_other)
    {
        CleanUp_();

        m_path = std::move(_other.m_path);
        m_name = std::move(_other.m_name);
        m_texh = _other.m_texh;
        m_info = _other.m_info;

        _other.m_texh = kInvalidHandle;
        _other.m_info = {};
    }
    return *this;
}

Texture::Texture(
    const TextureHandle _texh)
    : m_texh(_texh)
{
    JAM_ASSERT(m_texh, "Texture handle must be valid.");

    Graphics& gfx = GetGraphics();
    m_info        = gfx.GetTextureInfo(m_texh);
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
    if (const Error err = pTexture->LoadFromFile_(_path))
    {
        return err;
    }
    return pTexture;
}

Error Texture::LoadFromFile_(
    const StringView _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path);

    // import dds (native image format is dds.)
    Expect<Image> importer = Image::ImportFromFile(_path);
    if (!importer)
    {
        return importer.GetError();
    }

    // Ordinary image formats contain only the base level. Build the full chain
    // before upload; authored DDS/IBL mip chains stay untouched.
    const DirectX::ScratchImage& source   = importer->GetImageContainer();
    const DirectX::TexMetadata&  metadata = source.GetMetadata();
    if (metadata.mipLevels == 1 && metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE2D
        && !metadata.IsCubemap() && (metadata.width > 1 || metadata.height > 1)
        && _path.GetExtension().IgnoreCaseHash() != IgnoreCaseHashOf(".dds"))
    {
        DirectX::ScratchImage mips;
        const HRESULT         hr = DirectX::GenerateMipMaps(source.GetImages(), source.GetImageCount(), metadata, DirectX::TEX_FILTER_DEFAULT, 0, mips);
        if (FAILED(hr))
        {
            JAM_LOG_ERROR("Failed to generate mipmaps for '{}' (HRESULT {})", _path, hr);
            return Error::ImportFailed;
        }
        *importer = Image { std::move(mips) };
    }

    // done.
    CleanUp_();

    Graphics& gfx = GetGraphics();
    m_texh        = gfx.CreateTexture(*importer);
    m_info        = gfx.GetTextureInfo(m_texh);
    m_path        = _path;
    return Error::None;
}

Error Texture::Save(
    const StringView _path,
    const bool       _bReplaceExtension,
    const eFileWrite _mode) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path);
    JAM_ASSERT(IsValid(), "Cannot save an invalid texture.");

    String path = _path;
    if (_bReplaceExtension)
    {
        path.ReplaceExtension(kExtension);
    }

    Graphics& gfx = GetGraphics();
    return gfx.Capture(m_texh).ExportToFile(path, eImageFormat::DDS, _mode);
}

void Texture::SetName(
    const StringView _name)
{
    m_name = _name;
}

void Texture::Invalidate()
{
    CleanUp_();
}

bool Texture::IsValid() const
{
    return !!m_texh;
}

eAsset Texture::GetAssetType() const
{
    return kAsestType;
}

StringView Texture::GetTypeName() const
{
    return kTypeName;
}

UInt64 Texture::GetTypeHash() const
{
    return kTypeHash;
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
    return kExtension;
}

StringView Texture::GetFilter() const
{
    return kFilter;
}

TextureHandle Texture::GetHandle() const
{
    return m_texh;
}

const TextureInfo& Texture::GetInfo() const
{
    return m_info;
}

void Texture::Bind(
    const eShader _shader,
    const Int32   _slot) const
{
    Graphics& gfx = GetGraphics();
    gfx.SetTexture(m_texh, _shader, _slot);
}

void Texture::Unbind(
    const eShader _shader,
    const Int32   _slot) const
{
    Graphics& gfx = GetGraphics();
    gfx.SetTexture(kInvalidHandle, _shader, _slot);
}

void Texture::CleanUp_()
{
    JAM_DESTROY_TEXTURE(m_texh);
    m_path.Clear();
    m_name.Clear();
    m_info = {};
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
