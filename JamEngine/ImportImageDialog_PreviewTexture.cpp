#include "pch.h"

#include "Graphics.h"

#include "ImportImageDialog.h"

namespace jam
{

ImportImageDialog::PreviewTexture::PreviewTexture() = default;

ImportImageDialog::PreviewTexture::~PreviewTexture()
{
    CleanUp_();
}

ImportImageDialog::PreviewTexture::PreviewTexture(PreviewTexture&& _other) noexcept
{
    if (this != &_other)
    {
        m_texh = _other.m_texh;
        m_info = _other.m_info;

        _other.m_texh = k_invalidHandle;
        _other.m_info = {};
    }
}

ImportImageDialog::PreviewTexture& ImportImageDialog::PreviewTexture::operator=(PreviewTexture&& _other) noexcept
{
    if (this != &_other)
    {
        CleanUp_();

        m_texh = _other.m_texh;
        m_info = _other.m_info;

        _other.m_texh = k_invalidHandle;
        _other.m_info = {};
    }
    return *this;
}

void ImportImageDialog::PreviewTexture::Init(const TextureHandle _texh)
{
    CleanUp_();
    m_texh = _texh;
    m_info = Graphics::GetTextureInfo(_texh);
}

TextureHandle ImportImageDialog::PreviewTexture::GetHandle() const
{
    return m_texh;
}

const TextureInfo& ImportImageDialog::PreviewTexture::GetInfo() const
{
    return m_info;
}

void ImportImageDialog::PreviewTexture::CleanUp_()
{
    JAM_DESTROY_TEXTURE(m_texh);
    m_info = {};
}

}   // namespace jam