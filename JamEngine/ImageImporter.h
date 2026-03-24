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
    [[nodiscard]] static Expect<ImageImporter> ImportFromFile(StringView _path, eImageFormat _format, const ImageImporterInfo& _info = {});
    [[nodiscard]] static Expect<ImageImporter> ImportFromFile(StringView _path, const ImageImporterInfo& _info = {});
    [[nodiscard]] static Expect<ImageImporter> ImportFromMemory(MemoryView _mem, eImageFormat _format, const ImageImporterInfo& _info = {});

    [[nodiscard]] const ImageContainer& GetImageContainer();

private:
    ImageImporter() = default;

    Error ImportFromFile_(StringView _path, eImageFormat _format, const ImageImporterInfo& _info = {});
    Error ImportFromFile_(StringView _path, const ImageImporterInfo& _info = {});
    Error ImportFromMemory_(MemoryView _mem, eImageFormat _format, const ImageImporterInfo& _info = {});
    Error PostProcess_(const ImageImporterInfo& _info);

    ImageContainer m_container = {};
};

}   // namespace Jam