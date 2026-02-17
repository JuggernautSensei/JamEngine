#pragma once
#include "AssetCreationDialog.h"
#include "WindowsUtilities.h"

namespace Jam
{

struct MaterialCreationDialogDesc
{
    DialogInfo baseInfo   = {};
    Path       dirOrEmpty = {};
};

class MaterialCreationDialog : public AssetCreationDialog
{
public:
    explicit MaterialCreationDialog(const MaterialCreationDialogDesc& _desc);
    void OnShow() override;

private:
    bool OnClickCreateButton_();
    bool CreateAndSaveAsset_(const Path& _path) override;

    FileDialog m_saveFd { eFileDialogMode::SaveFile };
    Path       m_saveDir       = {};
    String     m_assetBaseName = {};
};

}   // namespace Jam
