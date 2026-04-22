#include "pch.h"

#include "MaterialCreationDialog.h"

#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"

#include "Material.h"

namespace Jam
{

namespace
{
    [[nodiscard]] FileDialog CreateSaveDirectoryDialog_()
    {
        SaveFileDialogInfo info = {};
        info.title              = "select save directory";
        return FileDialog { info };
    }

}   // namespace

MaterialCreationDialog::MaterialCreationDialog(
    const MaterialCreationDialogInfo& _desc)
    : AssetCreationDialog(_desc.title, _desc.size)
    , m_saveFd(CreateSaveDirectoryDialog_())
    , m_saveDir(_desc.dirOrEmpty)
{
}

void MaterialCreationDialog::OnShow()
{
    if (BeginLabelValueTable("MaterialCreationDialog"))
    {
        // dir
        ImGui::TableNextColumn();
        Widget_Text("output directory", true);
        ImGui::TableNextColumn();
        Widget_FileDialog("##DirectorySelector", m_saveDir, m_saveFd);

        // title
        ImGui::TableNextColumn();
        Widget_Text("asset title", true);
        ImGui::TableNextColumn();
        Widget_InputText("##title", m_assetBaseName);

        ImGui::EndTable();
    }

    // buttons
    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnSize))
    {
        if (OnClickCreateButton_())
        {
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
    {
        ImGui::CloseCurrentPopup();
    }
}

bool MaterialCreationDialog::OnClickCreateButton_()
{
    if (m_assetBaseName.IsEmpty())
    {
        PushEmptyNameErrorMessageDialog_();
        return false;
    }

    if (m_saveDir.IsEmpty())
    {
        PushEmptySavePathErrorMessageDialog_();
        return false;
    }

    String path = String::Joined(m_saveDir, m_assetBaseName, eJoin::File);
    path += Material::kExtension;

    if (FileSystem::Exists(path))
    {
        PushOverwriteConfirmationDialog_(path);
        return false;
    }

    return CreateAndSaveAsset_(path);
}

bool MaterialCreationDialog::CreateAndSaveAsset_(
    const StringView _path)
{
    Ref<Material> pMaterial = Material::Load();
    return SaveAsset_(_path, pMaterial);
}

}   // namespace Jam