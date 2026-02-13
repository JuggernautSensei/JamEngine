#include "pch.h"

#include "MaterialCreationDialog.h"

#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"

#include "Material.h"

namespace Jam
{

MaterialCreationDialog::MaterialCreationDialog(
    const MaterialCreationDialogDesc& _desc)
    : AssetCreationDialog(_desc.baseDesc)
    , m_saveDir(_desc.dirOrEmpty)
{
}

void MaterialCreationDialog::OnShow()
{
    if (BeginLabelTable("MaterialCreationDialog"))
    {
        // dir
        ImGui::TableNextColumn();
        Widget_Text("output directory", true);
        ImGui::TableNextColumn();
        Widget_FileDialog("##DirectorySelector", m_saveDir, m_saveFd);

        // name
        ImGui::TableNextColumn();
        Widget_Text("asset name", true);
        ImGui::TableNextColumn();
        Widget_InputText("##name", m_assetBaseName);

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

    if (m_saveDir.empty())
    {
        PushEmptySavePathErrorMessageDialog_();
        return false;
    }

    Path path = JoinedPath(m_saveDir, m_assetBaseName);
    ReplaceExtension(path, Material::k_extension);

    if (fs::exists(path))
    {
        PushOverwriteConfirmationDialog_(path);
        return false;
    }

    return CreateAndSaveAsset_(path);
}

bool MaterialCreationDialog::CreateAndSaveAsset_(
    const Path& _path)
{
    Ref<Material> pMat = Material::Create({}, {}, {});
    return SaveAsset_(_path, pMat);
}

}   // namespace Jam