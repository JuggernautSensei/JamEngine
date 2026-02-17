#pragma once
#include "ImageBase.h"

namespace Jam
{

struct ImageImporterInfo
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
public:
    [[nodiscard]] static Expect<ImageImporter> ImportFromFile(const Path& _path, eImageFormat _format, const ImageImporterInfo& _info = {});
    [[nodiscard]] static Expect<ImageImporter> ImportFromFile(const Path& _path, const ImageImporterInfo& _info = {});
    [[nodiscard]] static Expect<ImageImporter> ImportFromMemory(CMemoryBlock _mem, eImageFormat _format, const ImageImporterInfo& _info = {});

    [[nodiscard]] const ImageContainer& GetImageContainer();

private:
    ImageImporter() = default;

    Error ImportFromFile_(const Path& _path, eImageFormat _format, const ImageImporterInfo& _options = {});
    Error ImportFromFile_(const Path& _path, const ImageImporterInfo& _options = {});
    Error ImportFromMemory_(CMemoryBlock _mem, eImageFormat _format, const ImageImporterInfo& _options = {});
    Error PostProcess_(const ImageImporterInfo& _options);

    ImageContainer m_container = {};
};

}   // namespace Jam