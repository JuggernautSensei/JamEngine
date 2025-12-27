#pragma once
#include "GraphicsBase.h"
#include "ImageBase.h"
#include "MemView.h"

namespace jam
{

class ImageImporter
{

    enum eImportType
    {
        Unknown = 0,
        FromPath,
        FromSource
    };

public:
    bool Import(const fs::path& _path);
    bool ImportMemory(CMemView _mem, eImageFormat _imageFormat);

    // postprocess settings
    void ConvertFormat(bool _bEnable, eTextureFormat _format, float _threshold = 0.5f);
    void ForceSRGB(bool _bEnable);
    void GenerateMips(bool _bEnable);
    void Resize(bool _bEnable, Int32 _newWidth, Int32 _newHeight);
    void FlipVertical(bool _bEnable);
    void FlipHorizontal(bool _bEnable);
    void PreMultiplyAlpha(bool _bEnable);

    // result
    [[nodiscard]] TextureHandle GetTexture() const;

private:
    [[nodiscard]] Optional<DirectX::ScratchImage> PostProcess_(DirectX::ScratchImage&& _srcScratchImg);

    // result
    TextureHandle m_texh = k_invalidHandle;

    // import type
    eImportType m_importType = eImportType::Unknown;

    // format conversion
    bool           m_bConvertFormat   = false;
    eTextureFormat m_convertFormat    = eTextureFormat::Unknown;
    float          m_convertThreshold = 0.5f;

    // sRGB
    bool m_bForceSRGB = false;

    // mips
    bool m_bGenerateMips = false;

    // resize
    bool  m_bResize   = false;
    Int32 m_newWidth  = 0;
    Int32 m_newHeight = 0;

    // flip
    bool m_bFlipVertical   = false;
    bool m_bFlipHorizontal = false;

    // alpha
    bool m_bPreMultiplyAlpha = false;
};

}   // namespace jam