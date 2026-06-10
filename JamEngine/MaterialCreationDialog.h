#pragma once
#include "AssetCreationDialog.h"
#include "FileDialog.h"

namespace Jam
{

struct MaterialCreationDialogInfo
{
    StringView title      = {};
    ImVec2     size       = {};
    StringView dirOrEmpty = {};
};

class MaterialCreationDialog : public AssetCreationDialog
{
public:
    explicit MaterialCreationDialog(const MaterialCreationDialogInfo& _desc);
    void OnShow() override;

private:
    bool OnClickCreateButton_();
    bool CreateAndSaveAsset_(StringView _path) override;

    FileDialog m_saveFd;
    String     m_saveDir       = {};
    String     m_assetBaseName = {};
};

}   // namespace Jam
