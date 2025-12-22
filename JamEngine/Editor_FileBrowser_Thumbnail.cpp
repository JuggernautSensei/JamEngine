#include "pch.h"

#include "Editor_FileBrowser.h"

#include <Shlobj.h>
#include <atlstr.h>

namespace jam
{

Editor_FileBrowser::Thumbnail::Thumbnail()
{
}

Editor_FileBrowser::Thumbnail::~Thumbnail()
{
    CleanUp_();
}

Editor_FileBrowser::Thumbnail::Thumbnail(Thumbnail&& other) noexcept
    : m_path(std::move(other.m_path))
    , m_res(other.m_res)
    , m_handle(other.m_handle)
{
    other.m_handle = k_invalidHandle;
    other.m_res    = {};
}

Editor_FileBrowser::Thumbnail& Editor_FileBrowser::Thumbnail::operator=(Thumbnail&& other) noexcept
{
    if (this != &other)
    {
        CleanUp_();
        m_handle       = other.m_handle;
        m_res          = other.m_res;
        m_path         = std::move(other.m_path);
        other.m_handle = k_invalidHandle;
        other.m_res    = {};
    }
    return *this;
}

bool Editor_FileBrowser::Thumbnail::Load(const fs::path& _path)
{
    CleanUp_();

    const std::wstring path = _path.wstring();

    CComPtr<IShellItem>             psi;
    CComPtr<IShellItemImageFactory> pImageFactory;

    if (FAILED(SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&psi))))
    {
        GlobalBuffer::SetLastError("Failed to create IShellItem from path: {}", _path.native());
        return false;
    }

    if (FAILED(psi->QueryInterface(IID_PPV_ARGS(&pImageFactory))))
    {
        GlobalBuffer::SetLastError("Failed to get IShellItemImageFactory from IShellItem: {}", _path.native());
        return false;
    }

    HBITMAP hBmp;
    if (FAILED(pImageFactory->GetImage(k_thumbnailSize, SIIGBF_THUMBNAILONLY, &hBmp)))
    {
        if (FAILED(pImageFactory->GetImage(k_thumbnailSize, SIIGBF_RESIZETOFIT, &hBmp)))
        {
            GlobalBuffer::SetLastError("Failed to get thumbnail image for path: {}", _path.native());
            return false;
        }
    }

    BITMAP bmp;
    if (GetObject(hBmp, sizeof(BITMAP), &bmp) == 0)
    {
        DeleteObject(hBmp);
        GlobalBuffer::SetLastError("GetObject failed for thumbnail bitmap: {}", _path.native());
        return false;
    }

    const Int32        width    = bmp.bmWidth;
    const Int32        height   = bmp.bmHeight;
    const Int32        rowPitch = ((width * 32 + 31) / 32) * 4;
    Vector<UInt8> pixels(rowPitch * height);
    if (GetBitmapBits(hBmp, NarrowCast<LONG>(pixels.size()), pixels.data()) == 0)
    {
        DeleteObject(hBmp);
        GlobalBuffer::SetLastError("GetBitmapBits failed for thumbnail bitmap: {}", _path.native());
        return false;
    }

    const CMemView mems[] = { pixels };
    m_handle        = Graphics::CreateTexture2D(width, height, eTextureFormat::BGRA8_UNorm, 1, eTextureCreateFlags_None, mems);
    m_res           = { width, height };
    m_path          = _path;

    DeleteObject(hBmp);
    return true;
}

void Editor_FileBrowser::Thumbnail::LoadDummy()
{
    CleanUp_();

    constexpr Int32    w        = k_thumbnailSize.cx;
    constexpr Int32    h        = k_thumbnailSize.cy;
    constexpr Int32    rowPitch = ((w * 32 + 31) / 32) * 4;
    Vector<UInt8> pixels(rowPitch * h, 255);

    const CMemView bytes[] = { pixels };
    m_handle         = Graphics::CreateTexture2D(w, h, eTextureFormat::BGRA8_UNorm, 1, eTextureCreateFlags_None, bytes);
    m_res            = { w, h };
}

bool Editor_FileBrowser::Thumbnail::IsDummy() const
{
    return m_path.empty();
}

ImTextureRef Editor_FileBrowser::Thumbnail::GetImTexturRef() const
{
    return m_handle.value;
}

Vec2i Editor_FileBrowser::Thumbnail::GetResolution() const
{
    return m_res;
}

const fs::path& Editor_FileBrowser::Thumbnail::GetPath() const
{
    return m_path;
}

void Editor_FileBrowser::Thumbnail::CleanUp_()
{
    JAM_DESTROY_TEXTURE(m_handle);
    m_res = {};
    m_path.clear();
}

}   // namespace jam
