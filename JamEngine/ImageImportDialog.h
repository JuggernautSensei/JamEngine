#pragma once
#include "AssetCreationDialog.h"
#include "FileDialog.h"
#include "ImageBase.h"
#include "ImageImporter.h"

namespace Jam
{

struct ImageImportDialogInfo
{
    StringView title             = {};
    ImVec2     size              = {};
    String     importPathOrEmpty = {};
};

class ImageImportDialog : public AssetCreationDialog
{

public:
    explicit ImageImportDialog(const ImageImportDialogInfo& _info);
    void OnShow() override;

private:
    Error LoadPreviewTexture_();
    bool  OnClickImportButton_();
    bool  CreateAndSaveAsset_(StringView _path) override;
    void  Widget_PreviewTexture_() const;
    bool  Widget_TextureFormatCombo_(const char* _label, eTextureFormat& _format) const;

    String     m_importPath = {};
    FileDialog m_importFd;
    FileDialog m_saveFd;

    Ref<Texture>      m_pPreviewTex = nullptr;
    ImageImporterInfo m_options     = {};

    String m_assetSaveDir  = {};
    String m_assetBaseName = {};

    float                  m_previewLoadCounterSec  = 0.f;
    bool                   m_bPreviewLoadRequested  = false;
    constexpr static float kPreviewLoadDebounceSec = 1.f;
};

}   // namespace Jam