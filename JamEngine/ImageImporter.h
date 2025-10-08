#pragma once
#include "GfxBase.h"
#include "IImporter.h"
#include "ImageBase.h"

namespace jam
{

class ImageImporter : public IImporter
{
public:
    eErrorCode          Import() override;
    JAM_NODISCARD const DirectX::ScratchImage& GetScratchImage() const;

    void Reset() override;
    void SetPath(const fs::path& _filePath) override;
    void ConvertFormat(eFormat _format, float _threshold = 0.5f);
    void ForceSRGB() { m_bForceSRGB = true; }
    void GenerateMips();
    void Resize(size_t _newWidth, size_t _newHeight);
    void FlipVertical();
    void FlipHorizontal();
    void PreMultiplyAlpha();

    JAM_NODISCARD bool IsImportablePath(const fs::path& _filePath) const override;

private:
    fs::path              m_filePath = {};
    eImageFormat          m_format   = eImageFormat::Unknown;
    DirectX::ScratchImage m_scratchImg;

    bool    m_bConvertFormat   = false;
    eFormat m_convertFormat    = eFormat::Unknown;
    float   m_convertThreshold = 0.5f;

    bool m_bForceSRGB = false;

    bool m_bGenerateMips = false;

    bool   m_bResize   = false;
    size_t m_newWidth  = 0;
    size_t m_newHeight = 0;

    bool m_bFlipVertical   = false;
    bool m_bFlipHorizontal = false;

    bool m_bPreMultiplyAlpha = false;
};

}   // namespace jam