#pragma once
#include "IExporter.h"
#include "ImageBase.h"

namespace jam
{

class ImageExporter : public IExporter
{
public:
    eErrorCode Export() override;

    void SetImage(DirectX::ScratchImage&& _image);
    void SetPath(const fs::path& _filePath) override;
    void Reset() override;

    JAM_NODISCARD bool IsExportablePath(const fs::path& _filePath) const override;

private:
    DirectX::ScratchImage m_scratchImg;
    fs::path              m_filePath = {};
};

}