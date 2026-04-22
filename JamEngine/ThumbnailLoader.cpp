#include "pch.h"

#include "ThumbnailLoader.h"

#include "Textures.h"
#include "WindowsUtilities.h"

#include <shobjidl.h>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "shlwapi.lib")

namespace jam
{

ThumbnailLoader::ThumbnailLoader()
{
    HRESULT hr = CoInit(NULL);

    if (hr == S_FALSE)   // 이미 초기화 되어 있음
    {
        m_bNeedDestoryCOM = false;   // COM이 이미 초기화되어 있으므로 해제할 필요 없음 (Com 해제 책임은 외부에 있음)
    }
    else if (hr == S_OK)   // COM이 성공적으로 초기화됨
    {
        m_bNeedDestoryCOM = true;   // COM 해제 책임이 나에게 있음
    }
    else
    {
        JAM_ASSERT(0, "Failed to initialize COM library. Error: {}", GetSystemErrorMessage(hr));
        m_bNeedDestoryCOM = false;   // COM 초기화 실패 시 해제 책임 없음
    }

    InitWICFactory_();   // WIC 팩토리 초기화
}

ThumbnailLoader::~ThumbnailLoader()
{
    if (m_bNeedDestoryCOM)
    {
        CoUninitialize();   // 나에게 책임이 있다면 COM 해제
    }
}

Expected<TextureRef> ThumbnailLoader::CreateThumbnailTexture(const fs::path& _filePath, const Int32 _width, const Int32 _height) const
{
    HRESULT            hr;
    ComPtr<IShellItem> pItem = nullptr;
    hr                       = SHCreateItemFromParsingName(_filePath.c_str(), nullptr, IID_PPV_ARGS(&pItem));
    if (FAILED(hr))
    {
        JAM_ASSERT(0, "Failed to create IShellItem from path: {}. Error: {}", _filePath.string(), GetSystemErrorMessage(hr));
        return kError;
    }

    ComPtr<IShellItemImageFactory> pImageFactory = nullptr;
    hr                                           = pItem.As(&pImageFactory);
    if (FAILED(hr))
    {
        JAM_ASSERT(0, "Failed to get IShellItemImageFactory. Error: {}", GetSystemErrorMessage(hr));
        return kError;
    }

    SIZE    size    = { _width, _height };
    HBITMAP hBitmap = nullptr;
    hr              = pImageFactory->GetImage(size, SIIGBF_RESIZETOFIT | SIIGBF_BIGGERSIZEOK, &hBitmap);
    if (FAILED(hr) || !hBitmap)
    {
        JAM_ASSERT(0, "Failed to get thumbnail. Error: {}", GetSystemErrorMessage(hr));
        return kError;
    }

    ComPtr<IWICBitmap> pWICBitmap = nullptr;
    hr                            = m_pWICFactory->CreateBitmapFromHBITMAP(hBitmap, nullptr, WICBitmapUseAlpha, &pWICBitmap);
    DeleteObject(hBitmap);
    if (FAILED(hr))
    {
        JAM_ASSERT(0, "Failed to create IWICBitmap. Error: {}", GetSystemErrorMessage(hr));
        return kError;
    }

    ComPtr<IWICFormatConverter> pConverter = nullptr;
    hr                                     = m_pWICFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr))
    {
        JAM_ASSERT(0, "Failed to create IWICFormatConverter. Error: {}", GetSystemErrorMessage(hr));
        return kError;
    }

    hr = pConverter->Init(pWICBitmap.GetResource(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
    if (FAILED(hr))
    {
        JAM_ASSERT(0, "Failed to initialize format converter. Error: {}", GetSystemErrorMessage(hr));
        return kError;
    }

    UINT w = 0, h = 0;
    hr = pConverter->GetSize(&w, &h);
    if (FAILED(hr))
    {
        JAM_ASSERT(0, "Failed to get size from IWICFormatConverter. Error: {}", GetSystemErrorMessage(hr));
        return kError;
    }

    constexpr Int32   kPixelByte = 4;
    std::vector<BYTE> pixels(w * h * kPixelByte);
    hr = pConverter->CopyPixels(nullptr, w * kPixelByte, w * h * kPixelByte, pixels.data());
    if (FAILED(hr))
    {
        JAM_ASSERT(0, "Failed to copy pixels from IWICFormatConverter. Error: {}", GetSystemErrorMessage(hr));
        return kError;
    }

    Texture2DInitData initData;
    initData.pData = pixels.data();
    initData.pitch = w * kPixelByte;

    TextureRef thumbnail;
    thumbnail.Init(w, h, DXGI_FORMAT_B8G8R8A8_UNORM, eUsage::Immutable, eViewFlags::ShaderResourceView, 1, 1, false, false, initData);
    return thumbnail;
}

bool ThumbnailLoader::InitWICFactory_()
{
    if (!m_pWICFactory)
    {
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_pWICFactory.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
        {
            JAM_ASSERT(0, "Failed to create IWICImagingFactory. Error: {}", GetSystemErrorMessage(hr));
            return false;
        }
    }
    return true;
}
}   // namespace jam