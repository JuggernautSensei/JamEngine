#pragma once
#include <wincodec.h>

namespace jam
{

struct TextureRef;

class ThumbnailLoader
{
public:
    ThumbnailLoader();
    ~ThumbnailLoader();

    ThumbnailLoader(const ThumbnailLoader&)                = default;
    ThumbnailLoader& operator=(const ThumbnailLoader&)     = default;
    ThumbnailLoader(ThumbnailLoader&&) noexcept            = default;
    ThumbnailLoader& operator=(ThumbnailLoader&&) noexcept = default;

    JAM_NODISCARD Expected<TextureRef> CreateThumbnailTexture(const fs::path& _filePath, Int32 _width, Int32 _height) const;

private:
    bool InitWICFactory_();

    ComPtr<IWICImagingFactory> m_pWICFactory     = nullptr;
    bool                       m_bNeedDestoryCOM = false;
};

}   // namespace jam