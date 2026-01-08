#include "pch.h"

#include "ImageTexture.h"

#include "Graphics.h"
#include "ImageExporter.h"
#include "Profile.h"
#include "ImageImporter.h"
#include "Magics.h"

namespace jam
{

bool ImageTexture::Load(
    const fs::path& _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    CleanUp_();

    // import dds (native image format is dds.)
    ImageImporter importer = {};
    importer.GenerateMips(true);
    if (!importer.Import(_path, eImageFormat::DDS))
    {
        return false;
    }

    // import
    const DirectX::ScratchImage& scratchImg = importer.GetScratchImage();
    m_texh                                  = Graphics::CreateTexture(scratchImg);

    // cache
    const TextureInfo info = Graphics::GetTextureInfo(m_texh);
    m_res                  = { info.width, info.height };
    m_type                 = info.type;

    // set path
    m_path            = _path;
    m_bLoadedFromFile = true;
    return true;
}

void ImageTexture::LoadHandle(
    const TextureHandle _texh)
{
    JAM_ASSERT(JAM_IS_VALID_HANDLE(_texh), "_texh is invalid.");
    CleanUp_();

    const TextureInfo info = Graphics::GetTextureInfo(m_texh);
    m_texh                 = _texh;
    m_res                  = { info.width, info.height };
    m_type                 = info.type;
    m_bLoadedFromFile      = false;
}

bool ImageTexture::Save(
    const fs::path&      _path,
    const bool           _bReplaceExtension,
    const eSaveFileFlags _flags) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    JAM_ASSERT(JAM_IS_VALID_HANDLE(m_texh), "..");

    ImageExporter exporter = {};
    if (_bReplaceExtension)
    {
        fs::path path = _path;
        path.replace_extension(k_jamImageTextureExt.GetPtr());
        return exporter.Export(m_texh, path, eImageFormat::DDS, false, _flags);
    }
    return exporter.Export(m_texh, _path, eImageFormat::DDS, false, _flags);
}

void ImageTexture::SetName(
    const StringView _name)
{
    m_name = _name.ToString();
}

void ImageTexture::SetVirtualPath(
    const fs::path& _path)
{
    JAM_ASSERT(m_bLoadedFromFile == false, "Cannot change path of an asset loaded from file.");
    m_path = _path;
}

bool ImageTexture::IsValid() const
{
    return JAM_IS_VALID_HANDLE(m_texh);
}

ImageTexture::~ImageTexture()
{
    CleanUp_();
}

ImageTexture::ImageTexture(
    ImageTexture&& _other) noexcept
    : m_path(std::move(_other.m_path))
    , m_name(std::move(_other.m_name))
    , m_texh(_other.m_texh)
    , m_bLoadedFromFile(_other.m_bLoadedFromFile)
    , m_res(_other.m_res)
    , m_type(_other.m_type)
{
    _other.CleanUp_();
}

ImageTexture& ImageTexture::operator=(
    ImageTexture&& _other) noexcept
{
    if (this != &_other)
    {
        JAM_DESTROY_TEXTURE(m_texh);

        m_path            = std::move(_other.m_path);
        m_name            = std::move(_other.m_name);
        m_texh            = _other.m_texh;
        m_bLoadedFromFile = _other.m_bLoadedFromFile;
        m_res             = _other.m_res;
        m_type            = _other.m_type;

        _other.CleanUp_();
    }
    return *this;
}

void ImageTexture::Unload()
{
    CleanUp_();
}

const fs::path& ImageTexture::GetPath() const
{
    return m_path;
}

const StringView ImageTexture::GetName() const
{
    return m_name;
}

eAssetType ImageTexture::GetAssetType() const
{
    return k_type;
}

UInt32 ImageTexture::GetTypeHash() const
{
    return k_hash;
}

StringView ImageTexture::GetTypeName() const
{
    return k_name;
}

TextureHandle ImageTexture::GetHandle() const
{
    return m_texh;
}

Vec2i ImageTexture::GetResolution() const
{
    return m_res;
}

eTextureType ImageTexture::GetTextureType() const
{
    return m_type;
}

void ImageTexture::Bind(
    const eShader _shader,
    const Int32   _slot) const
{
    Graphics::SetTexture(m_texh, _shader, _slot);
}

void ImageTexture::Unbind(
    const eShader _shader,
    const Int32   _slot) const
{
    Graphics::SetTexture(k_invalidHandle, _shader, _slot);
}

void ImageTexture::CleanUp_()
{
    JAM_DESTROY_TEXTURE(m_texh);
    m_path.clear();
    m_name            = {};
    m_bLoadedFromFile = false;

    // cache
    m_res  = { 0, 0 };
    m_type = eTextureType::Texture2D;
}

}   // namespace jam