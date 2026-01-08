#include "pch.h"

#include "ThumbnailCache.h"

#include <Shlobj.h>
#include <atlstr.h>

#include "Graphics.h"
#include "ImageTexture.h"

namespace jam
{

ThumbnailCache::ThumbnailCache()
    : m_cache(256)
{
    // create placeholder thumbnail (solid white)
    Array<Byte, 4 * 128 * 128> pixels = {};
    pixels.fill(0xFF);
    m_pPlaceholderThumbnail = CreateImageTexture_(128, 128, pixels);
}

void ThumbnailCache::GatherThumbnailsFrom_(
    const fs::path& _dir)
{
    JAM_ASSERT(fs::is_directory(_dir), "Path is not a directory: {}", _dir.native());

    // count files first
    // because.. if too many files, expand lru cache capacity
    m_tmpEntries.clear();
    for (const fs::directory_entry& entry: fs::directory_iterator(_dir))
    {
        m_tmpEntries.push_back(entry);
    }

    // expand cache if needed
    const Int32 numEntries = m_tmpEntries.size();
    if (numEntries > m_cache.GetCapacity())
    {
        m_cache.SetCapacity(numEntries + 16);   // add some margin
    }

    // gather thumbnails
    for (const fs::directory_entry& entry: m_tmpEntries)
    {
        const fs::path& path = entry.path();
        InsertThumbnail_(path);
    }
}

Ref<ImageTexture> ThumbnailCache::GetOrPlaceholder(
    const fs::path& _path)
{
    // first search
    {
        const auto it = m_cache.Find(_path);
        if (it != m_cache.End())
        {
            return it->value;   // or placeholder
        }
    }

    // if not found.. gather from parent dir
    GatherThumbnailsFrom_(_path.parent_path());

    // second search
    {
        const auto it = m_cache.Find(_path);
        if (it != m_cache.End())
        {
            return it->value;   // or placeholder
        }

        JAM_ASSERT(0, "never reach here");
        return m_pPlaceholderThumbnail;
    }
}

void ThumbnailCache::OnEvent(Event& _event)
{
    HandleEvent<FileModifiedEvent>(_event, JAM_BIND_LISTENER_METHOD(this,ThumbnailCache::OnFileModifiedEvent_));

    // on add -> 다루지 않음. 필요하면 그때 로드
    // on delete -> 다루지 않음. lru 캐시에서 자연스럽게 사라지도록 둠
    // on rename -> 다루지 않음. 필요하면 그때 로드 + modified로 자연스럽게 처리되는 부분도 있음
}

void ThumbnailCache::InsertThumbnail_(
    const fs::path& _path)
{
    // already cached
    const auto it = m_cache.Find(_path);
    if (it != m_cache.End())
    {
        return;
    }

    // create
    Ref<ImageTexture> pThumbnail;
    if (!MoveIfHasValue(CreateThumbnail_(_path), pThumbnail))
    {
        // insert placeholder
        pThumbnail = m_pPlaceholderThumbnail;
    }

    m_cache.Insert(_path, pThumbnail);
}

Optional<Ref<ImageTexture>> ThumbnailCache::CreateThumbnail_(
    const fs::path& _path) const
{
    constexpr SIZE     k_thumbnailSize = { 128, 128 };
    const std::wstring path            = _path.wstring();

    CComPtr<IShellItem> psi = nullptr;
    if (FAILED(SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&psi))))
    {
        GlobalBuffer::SetLastError("Failed to create IShellItem from path: {}", _path.native());
        return k_nullopt;
    }

    CComPtr<IShellItemImageFactory> pImageFactory = nullptr;
    if (FAILED(psi->QueryInterface(IID_PPV_ARGS(&pImageFactory))))
    {
        GlobalBuffer::SetLastError("Failed to get IShellItemImageFactory from IShellItem: {}", _path.native());
        return k_nullopt;
    }

    HBITMAP hBmp = NULL;
    if (FAILED(pImageFactory->GetImage(k_thumbnailSize, SIIGBF_THUMBNAILONLY, &hBmp)))
    {
        if (FAILED(pImageFactory->GetImage(k_thumbnailSize, SIIGBF_RESIZETOFIT, &hBmp)))
        {
            GlobalBuffer::SetLastError("Failed to get thumbnail image for path: {}", _path.native());
            return k_nullopt;
        }
    }

    BITMAP bmp = {};
    if (GetObject(hBmp, sizeof(BITMAP), &bmp) == 0)
    {
        GlobalBuffer::SetLastError("GetObject failed for thumbnail bitmap: {}", _path.native());
        goto lb_error;
    }

    // read pixel
    const Int32   width  = bmp.bmWidth;
    const Int32   height = bmp.bmHeight;
    Vector<UInt8> pixels(4 * width * height);
    if (GetBitmapBits(hBmp, pixels.size(), pixels.data()) == 0)
    {
        GlobalBuffer::SetLastError("GetBitmapBits failed for thumbnail bitmap: {}", _path.native());
        goto lb_error;
    }

    DeleteObject(hBmp);
    return CreateImageTexture_(width, height, pixels);

lb_error:
    DeleteObject(hBmp);
    return k_nullopt;
}

Ref<ImageTexture> ThumbnailCache::CreateImageTexture_(
    const Int32    _w,
    const Int32    _h,
    const CMemView _initData) const
{
    const CMemView      mems[]   = { _initData };
    const TextureHandle texh     = Graphics::CreateTexture2D(_w, _h, eTextureFormat::BGRA8_UNorm, 1, 0, mems);
    Ref<ImageTexture>   pTexture = MakeRef<ImageTexture>();
    pTexture->LoadHandle(texh);
    return pTexture;
}

void ThumbnailCache::OnFileModifiedEvent_(
    const FileModifiedEvent& _event)
{
    const fs::path& path = _event.GetPath();
    const auto      it   = m_cache.Find(path);
    if (it != m_cache.End())
    {
        m_cache.Remove(path);
        InsertThumbnail_(path);
    }
}

}   // namespace jam