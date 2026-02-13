#include "pch.h"

#include "ThumbnailCache.h"

#include <Shlobj.h>
#include <atlstr.h>

#include "Graphics.h"
#include "Texture.h"

namespace Jam
{

ThumbnailCache::ThumbnailCache()
    : m_cache(256)
{
    // create placeholder thumbnail (solid white)
    Array<Byte, 4 * 128 * 128> pixels = {};
    pixels.fill(0xFF);
    m_pPlaceholderThumbnail = CreateTexture_(128, 128, pixels);
}

Ref<Texture> ThumbnailCache::GetOrPlaceholder(
    const Path& _path)
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

        JAM_UNREACHABLE;
    }
}

void ThumbnailCache::OnEvent(Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<FileModifiedEvent>(this, &ThumbnailCache::OnFileModifiedEvent_);

    // on add -> 다루지 않음. 필요하면 그때 로드
    // on delete -> 다루지 않음. lru 캐시에서 자연스럽게 사라지도록 둠
    // on rename -> 다루지 않음. 필요하면 그때 로드 + modified로 자연스럽게 처리되는 부분도 있음
}

void ThumbnailCache::GatherThumbnailsFrom_(
    const Path& _dir)
{
    JAM_ASSERT(fs::is_directory(_dir), "Path is not a directory: {}", _dir.native());

    // count files first
    // because.. if too many files, expand lru cache capacity
    m_tmpEntries.clear();
    for (const fs::directory_entry& entry: fs::directory_iterator(_dir))
    {
        m_tmpEntries.emplace_back(entry);
    }

    // expand cache if needed
    const SizeT numEntries = m_tmpEntries.size();
    if (numEntries > m_cache.GetCapacity())
    {
        m_cache.SetCapacity(numEntries + 16);   // add some margin
    }

    // gather thumbnails
    for (const fs::directory_entry& entry: m_tmpEntries)
    {
        const Path& path = entry.path();
        InsertThumbnail_(path);
    }
}

void ThumbnailCache::InsertThumbnail_(
    const Path& _path)
{
    // already cached
    const auto it = m_cache.Find(_path);
    if (it != m_cache.End())
    {
        return;
    }

    // create
    Optional<Ref<Texture>> pThumbnail = CreateThumbnail_(_path);
    if (!pThumbnail)
    {
        // insert placeholder
        pThumbnail = m_pPlaceholderThumbnail;
    }

    m_cache.Insert(_path, pThumbnail.Take());
}

Optional<Ref<Texture>> ThumbnailCache::CreateThumbnail_(
    const Path& _path) const
{
    constexpr SIZE k_thumbnailSize = { 128, 128 };

    ScratchWBuffer buf;
    buf.SetUtf8(_path.native());

    CComPtr<IShellItem> psi = nullptr;
    if (FAILED(SHCreateItemFromParsingName(buf.GetPtr(), nullptr, IID_PPV_ARGS(&psi))))
    {
        JAM_LOG.Error("Failed to create IShellItem from path: {}", _path.native());
        return k_nullopt;
    }

    CComPtr<IShellItemImageFactory> pImageFactory = nullptr;
    if (FAILED(psi->QueryInterface(IID_PPV_ARGS(&pImageFactory))))
    {
        JAM_LOG.Error("Failed to get IShellItemImageFactory from IShellItem: {}", _path.native());
        return k_nullopt;
    }

    HBITMAP hBmp = {};
    if (FAILED(pImageFactory->GetImage(k_thumbnailSize, SIIGBF_THUMBNAILONLY, &hBmp)))
    {
        if (FAILED(pImageFactory->GetImage(k_thumbnailSize, SIIGBF_RESIZETOFIT, &hBmp)))
        {
            JAM_LOG.Error("Failed to get thumbnail image for path: {}", _path.native());
            return k_nullopt;
        }
    }

    BITMAP bmp = {};
    if (GetObject(hBmp, sizeof(BITMAP), &bmp) == 0)
    {
        JAM_LOG.Error("GetObject failed for thumbnail bitmap: {}", _path.native());
        DeleteObject(hBmp);
        return k_nullopt;
    }

    // read pixel
    const Int32   width  = bmp.bmWidth;
    const Int32   height = bmp.bmHeight;
    Vector<UInt8> pixels(4 * width * height);
    if (GetBitmapBits(hBmp, NumericCast<LONG>(pixels.size()), pixels.data()) == 0)
    {
        JAM_LOG.Error("GetBitmapBits failed for thumbnail bitmap: {}", _path.native());
        DeleteObject(hBmp);
        return k_nullopt;
    }

    DeleteObject(hBmp);
    return CreateTexture_(width, height, pixels);
}

Ref<Texture> ThumbnailCache::CreateTexture_(
    const Int32        _w,
    const Int32        _h,
    const CMemoryBlock _initData) const
{
    const CMemoryBlock  mems[] = { _initData };
    const TextureHandle texh   = JAM_GFX.CreateTexture2D(_w, _h, eTextureFormat::BGRA8_UNorm, 1, eTextureCreateFlags::None, mems);
    return Texture::Create(texh);
}

void ThumbnailCache::OnFileModifiedEvent_(
    const FileModifiedEvent& _event)
{
    const Path& path = _event.GetPath();
    const auto  it   = m_cache.Find(path);
    if (it != m_cache.End())
    {
        m_cache.Remove(path);
        InsertThumbnail_(path);
    }
}

}   // namespace Jam