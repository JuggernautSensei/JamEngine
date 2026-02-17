#pragma once
#include "AssetCreationDialog.h"
#include "ImageBase.h"
#include "ImageImporter.h"
#include "WindowsUtilities.h"

namespace Jam
{

struct ImageImportDialogInfo
{
    DialogInfo baseInfo          = {};
    Path       importPathOrEmpty = {};
};

class ImageImportDialog : public AssetCreationDialog
{

public:
    explicit ImageImportDialog(const ImageImportDialogInfo& _info);
    void OnShow() override;

private:
    Error LoadPreviewTexture_();
    bool  OnClickImportButton_();
    bool  CreateAndSaveAsset_(const Path& _path) override;
    void  Widget_PreviewTexture_() const;
    bool  Widget_TextureFormatCombo_(const char* _label, eTextureFormat& _format) const;

    Path       m_importPath = {};
    FileDialog m_importFd { eFileDialogMode::OpenFile };
    FileDialog m_saveFd { eFileDialogMode::SaveFile };

    Ref<Texture>      m_pPreviewTex = nullptr;
    ImageImporterInfo m_options     = {};

    Path   m_assetSaveDir  = {};
    String m_assetBaseName = {};

    float                  m_previewLoadCounterSec  = 0.f;
    bool                   m_bPreviewLoadRequested  = false;
    constexpr static float k_previewLoadDebounceSec = 1.f;
};

}   // namespace Jam