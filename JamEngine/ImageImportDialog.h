#pragma once
#include "AssetDialog.h"
#include "ImageImporter.h"
#include "WindowsUtilities.h"

namespace jam
{

class ImageImportDialog : public AssetDialog
{

public:
    explicit ImageImportDialog(const fs::path& _path);
    void OnShow() override;

private:
    void LoadPreviewTexture_();
    bool OnClickImportButton_();
    void Widget_PreviewTexture_() const;

    fs::path    m_dir;
    std::string m_name;

    ImageImporter        m_importer    = {};
    Ref<ImageTexture>    m_pPreviewTex = nullptr;
    ImageImporterOptions m_options     = {};
    FileDialog           m_fd;   // init in ctor

    float           m_previewLoadCounterSec  = 0.f;
    bool            m_bPreviewLoadRequested  = false;
    constexpr float k_previewLoadDebounceSec = 1.f;
};

}   // namespace jam