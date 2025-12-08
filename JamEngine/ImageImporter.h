#pragma once
#include "GraphicsBase.h"
#include "IImporter.h"
#include "ImageBase.h"
#include "MemView.h"

namespace jam
{

class ImageImporter : public IImporter
{

    enum eImportType
    {
        Unknown = 0,
        FromPath,
        FromMemory
    };

public:
    bool                                       Import() override;
    [[nodiscard]] const DirectX::ScratchImage& GetScratchImage() const;

    // set source
    void SetMemory(ConstMemView _mem, eImageFormat _imageFormat);
    void SetPath(const fs::path& _filePath) override;

    // post-process
    void ConvertFormat(bool _bEnable, eTextureFormat _format, float _threshold = 0.5f);
    void ForceSRGB(bool _bEnable);
    void DropMips(bool _bEnable);   // default generate mips
    void Resize(bool _bEnable, size_t _newWidth, size_t _newHeight);
    void FlipVertical(bool _bEnable);
    void FlipHorizontal(bool _bEnable);
    void PreMultiplyAlpha(bool _bEnable);

private:
    bool ImportFromPath_();
    bool ImportFromMemory_();

    // result
    DirectX::ScratchImage m_scratchImg;

    // import from path
    fs::path m_path = {};

    // import from mem
    ConstMemView m_imageSource       = {};
    eImageFormat    m_imageSourceFormat = eImageFormat::Unknown;

    // import type
    eImportType m_importType = eImportType::Unknown;

    // format conversion
    bool           m_bConvertFormat = false;
    eTextureFormat m_convertFormat  = eTextureFormat::Unknown;
    float          m_convertThreshold  = 0.5f;

    // sRGB
    bool m_bForceSRGB = false;

    // mips
    bool m_bDropMips = false;

    // resize
    bool   m_bResize   = false;
    size_t m_newWidth  = 0;
    size_t m_newHeight = 0;

    // flip
    bool m_bFlipVertical   = false;
    bool m_bFlipHorizontal = false;

    // alpha
    bool m_bPreMultiplyAlpha = false;
};

}   // namespace jam