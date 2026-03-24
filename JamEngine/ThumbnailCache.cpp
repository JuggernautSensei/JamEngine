#include "pch.h"

#include "ThumbnailCache.h"

#include <Shlobj.h>
#include <atlstr.h>

#include "DirIter.h"
#include "FileSystem.h"
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
    const StringView _path)
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
    GatherThumbnailsFrom_(_path.GetDirectory());

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
    StringView _dir)
{
    JAM_ASSERT(_dir.IsDirectory(), "String is not a directory: {}", _dir);

    // count files first
    // because.. if too many files, expand lru cache capacity
    m_pathes.clear();
    const Expect<DirIter> dirIter = DirIter::Create(_dir);
    JAM_ASSERT(dirIter, "Failed to create directory iterator for path: {}", _dir);
    for (const StringView path: dirIter.GetValue())
    {
        m_pathes.emplace_back(path);
    }

    // expand cache if needed
    const SizeT numPathes = m_pathes.size();
    if (numPathes > m_cache.GetCapacity())
    {
        m_cache.SetCapacity(numPathes + 16);   // add some margin
    }

    // gather thumbnails
    for (const StringView path: m_pathes)
    {
        InsertThumbnail_(path);
    }
}

void ThumbnailCache::InsertThumbnail_(
    const StringView _path)
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
    const StringView _path) const
{
    constexpr SIZE k_thumbnailSize = { 128, 128 };

    const WString       path = WString::Encoded(_path);
    CComPtr<IShellItem> psi  = nullptr;
    if (FAILED(SHCreateItemFromParsingName(path.GetPtr(), nullptr, IID_PPV_ARGS(&psi))))
    {
        JAM_LOG_ERROR("Failed to create IShellItem from path: {}", _path);
        return k_nullopt;
    }

    CComPtr<IShellItemImageFactory> pImageFactory = nullptr;
    if (FAILED(psi->QueryInterface(IID_PPV_ARGS(&pImageFactory))))
    {
        JAM_LOG_ERROR("Failed to get IShellItemImageFactory from IShellItem: {}", _path);
        return k_nullopt;
    }

    HBITMAP hBmp = {};
    if (FAILED(pImageFactory->GetImage(k_thumbnailSize, SIIGBF_THUMBNAILONLY, &hBmp)))
    {
        if (FAILED(pImageFactory->GetImage(k_thumbnailSize, SIIGBF_RESIZETOFIT, &hBmp)))
        {
            JAM_LOG_ERROR("Failed to get thumbnail image for path: {}", _path);
            return k_nullopt;
        }
    }

    BITMAP bmp = {};
    if (GetObject(hBmp, sizeof(BITMAP), &bmp) == 0)
    {
        JAM_LOG_ERROR("GetObject failed for thumbnail bitmap: {}", _path);
        DeleteObject(hBmp);
        return k_nullopt;
    }

    // read pixel
    const Int32   width  = bmp.bmWidth;
    const Int32   height = bmp.bmHeight;
    Vector<UInt8> pixels(4 * width * height);
    if (GetBitmapBits(hBmp, NumericCast<LONG>(pixels.size()), pixels.data()) == 0)
    {
        JAM_LOG_ERROR("GetBitmapBits failed for thumbnail bitmap: {}", _path);
        DeleteObject(hBmp);
        return k_nullopt;
    }

    DeleteObject(hBmp);
    return CreateTexture_(width, height, pixels);
}

Ref<Texture> ThumbnailCache::CreateTexture_(
    const Int32        _w,
    const Int32        _h,
    const MemoryView _initData) const
{
    const MemoryView  mems[] = { _initData };
    const TextureHandle texh   = GetGraphics().CreateTexture2D(_w, _h, eTextureFormat::BGRA8_UNorm, 1, eTextureCreateFlags::None, mems);
    return Texture::Load(texh);
}

void ThumbnailCache::OnFileModifiedEvent_(
    const FileModifiedEvent& _event)
{
    StringView path = _event.GetPath();
    const auto it   = m_cache.Find(path);
    if (it != m_cache.End())
    {
        m_cache.Remove(path);
        InsertThumbnail_(path);
    }
}

}   // namespace Jam