#pragma once
#include "GraphicsBase.h"
#include "ImageBase.h"
#include "MemView.h"

namespace jam
{

struct ImageImporterOptions
{
    // format conversion
    bool           bConvertFormat   = false;
    eTextureFormat convertFormat    = eTextureFormat::Unknown;
    float          convertThreshold = 0.5f;

    // sRGB
    bool bForceSRGB = false;

    // mips
    bool bGenerateMips = false;

    // resize
    bool  bResize   = false;
    Int32 newWidth  = 0;
    Int32 newHeight = 0;

    // flip
    bool bFlipVertical   = false;
    bool bFlipHorizontal = false;

    // alpha
    bool bPreMultiplyAlpha = false;
};

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
    bool Import(const fs::path& _path, eImageFormat _format);
    bool ImportMemory(CMemView _mem, eImageFormat _format);

    // postprocess settings
    void ConvertFormat(bool _bEnable, eTextureFormat _format, float _threshold = 0.5f);
    void ForceSRGB(bool _bEnable);
    void GenerateMips(bool _bEnable);
    void Resize(bool _bEnable, Int32 _newWidth, Int32 _newHeight);
    void FlipVertical(bool _bEnable);
    void FlipHorizontal(bool _bEnable);
    void PreMultiplyAlpha(bool _bEnable);
    void SetOptions(const ImageImporterOptions& _options);

    // result
    [[nodiscard]] const DirectX::ScratchImage& GetScratchImage() const;

private:
    [[nodiscard]] Optional<DirectX::ScratchImage> PostProcess_(DirectX::ScratchImage&& _srcScratchImg);

    // result
    DirectX::ScratchImage m_scratchImg = {};
    ImageImporterOptions  m_options    = {};
};

}   // namespace jam