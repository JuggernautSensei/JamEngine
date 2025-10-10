#include "pch.h"

#include "ImageTexture.h"

#include "Gfx.h"
#include "ImageExporter.h"
#include "ImageImporter.h"
#include "Profile.h"

namespace jam
{

eErrorCode ImageTexture::Load(const fs::path& _path)
{
    JAM_PROFILE_SCOPE_MS(R"(ImageTexture::Load "{}")", _path.native());

    CleanUp_();

    auto [ec, handle] = gfx::LoadTexture(_path);
    if (IsOk(ec))
    {
        m_path   = _path;
        m_handle = handle;
    }
    return ec;
}

eErrorCode ImageTexture::Save(const fs::path& _path) const
{
    JAM_PROFILE_SCOPE_MS(R"(ImageTexture::Save "{}")", _path.native());

    DirectX::ScratchImage sImg = gfx::Capture(m_handle);

    ImageExporter exporter;
    exporter.SetPath(_path);
    exporter.SetImage(std::move(sImg));
    return exporter.Export();
}

ImageTexture::~ImageTexture()
{
    CleanUp_();
}

ImageTexture::ImageTexture(ImageTexture&& _other) noexcept
{
    m_handle = _other.m_handle;
    m_path   = std::move(_other.m_path);

    _other.m_handle = k_invalidHandle;
    _other.m_path.clear();
}

ImageTexture& ImageTexture::operator=(ImageTexture&& _other) noexcept
{
    if (this != &_other)
    {
        ImageTexture temp(std::move(_other));
        std::swap(*this, temp);
    }
    return *this;
}

void ImageTexture::Unload()
{
    CleanUp_();
}

bool ImageTexture::IsLoaded() const
{
    return m_handle != k_invalidHandle;
}

const fs::path& ImageTexture::GetPath() const
{
    return m_path;
}

eAssetType ImageTexture::GetType() const
{
    return k_type;
}

UInt32 ImageTexture::GetHash() const
{
    return k_hash;
}

std::string_view ImageTexture::GetName() const
{
    return k_name;
}

void ImageTexture::Bind(const eShader _shader, const Int32 _slot) const
{
    gfx::SetTexture(m_handle, _shader, _slot);
}

void ImageTexture::Unbind(const eShader _shader, const Int32 _slot) const
{
    gfx::SetTexture(k_invalidHandle, _shader, _slot);
}

void ImageTexture::CleanUp_()
{
    JAM_DESTROY_TEXTURE(m_handle);
    m_path.clear();
}


}   // namespace jam