#include "pch.h"

#include "ImageTexture.h"

#include "Graphics.h"
#include "ImageExporter.h"
#include "Profile.h"

namespace jam
{

bool ImageTexture::Load(
    const fs::path& _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    CleanUp_();

    if (const Optional<TextureHandle> opt = Graphics::LoadTextureFile(_path))
    {
        const TextureInfo info = Graphics::GetTextureInfo(m_texh);

        m_texh = opt.value();
        m_path = _path;
        m_res  = { info.width, info.height };
        m_type = info.type;
        return true;
    }
    else
    {
        return false;
    }
}

bool ImageTexture::LoadSource(
    const CMemView _source,
    const eImageFormat _format,
    const fs::path&    _virtualPath)
{
    JAM_FUNCTION_MSG_PROFILE("virtualPath: {}", _virtualPath.native());
    CleanUp_();

    if (const Optional<TextureHandle> opt = Graphics::LoadTextureSource(_source, _format))
    {
        const TextureInfo info = Graphics::GetTextureInfo(m_texh);

        m_texh = opt.value();
        m_path = _virtualPath;
        m_res  = { info.width, info.height };
        m_type = info.type;
        return true;
    }
    else
    {
        return false;
    }
}

bool ImageTexture::Save(
    const fs::path& _path) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());

    ImageExporter exporter;
    exporter.SetPath(_path);
    exporter.SetTexture(m_texh);
    return exporter.Export();
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
{
    m_texh        = _other.m_texh;
    m_path        = std::move(_other.m_path);
    _other.m_texh = k_invalidHandle;
    _other.m_path.clear();
}

ImageTexture& ImageTexture::operator=(
    ImageTexture&& _other) noexcept
{
    if (this != &_other)
    {
        CleanUp_();

        m_texh        = _other.m_texh;
        m_path        = std::move(_other.m_path);
        _other.m_texh = k_invalidHandle;
        _other.m_path.clear();
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
}

}   // namespace jam