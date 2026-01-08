#pragma once
#include "AssetDialog.h"
#include "AssimpImporter.h"
#include "WindowsUtilities.h"

namespace jam
{
class VertexLayoutEntry;

class ModelImportDialog : public AssetDialog
{
public:
    explicit ModelImportDialog(const fs::path& _importPathOrEmpty);
    void OnShow() override;

private:
    bool OnClickImportButton_();

    fs::path   m_importPath = {};
    FileDialog m_importFd;

    fs::path    m_dir  = {};
    std::string m_name = {};
    FileDialog  m_selectDirFd;

    AssimpImporter         m_importer = {};
    AssimpImportOptions    m_options  = {};
    Ref<VertexLayoutEntry> m_pVle     = nullptr;
};

}   // namespace jam