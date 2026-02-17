#pragma once
#include "AssetCreationDialog.h"
#include "AssimpImporter.h"
#include "WindowsUtilities.h"

namespace Jam
{
class VertexLayoutEntry;

struct ModelImportDialogInfo
{
    DialogInfo baseInfo          = {};
    Path       importPathOrEmpty = {};
};

class ModelImportDialog : public AssetCreationDialog
{
public:
    explicit ModelImportDialog(const ModelImportDialogInfo& _info);
    void OnShow() override;

protected:
    bool OnClickImportButton_();
    bool CreateAndSaveAsset_(const Path& _path) override;

    Path       m_importPath = {};
    FileDialog m_importFd { eFileDialogMode::OpenFile };
    FileDialog m_saveFd { eFileDialogMode::SaveFile };

    Path   m_assetSaveDir  = {};
    String m_assetBaseName = {};

    AssimpImporterInfo  m_options = {};
    Ref<VertexLayoutEntry> m_pVle    = nullptr;
};

}   // namespace Jam