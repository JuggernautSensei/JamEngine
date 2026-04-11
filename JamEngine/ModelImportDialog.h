#pragma once
#include "AssetCreationDialog.h"
#include "AssimpImporter.h"
#include "FileDialog.h"

namespace Jam
{

class VertexLayoutEntry;

struct ModelImportDialogInfo
{
    StringView title             = {};
    ImVec2     size              = {};
    StringView importPathOrEmpty = {};
};

class ModelImportDialog : public AssetCreationDialog
{
public:
    explicit ModelImportDialog(const ModelImportDialogInfo& _info);
    void OnShow() override;

protected:
    bool OnClickImportButton_();
    bool CreateAndSaveAsset_(StringView _path) override;

    String     m_importPath = {};
    FileDialog m_importFd;
    FileDialog m_saveFd;

    String m_assetSaveDir  = {};
    String m_assetBaseName = {};

    Flags<eAssimpImporterFlags> m_flags = eAssimpImporterFlags::None;
    Ref<VertexLayoutEntry>      m_pVle  = nullptr;
};

}   // namespace Jam