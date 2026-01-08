#include "pch.h"

#include "MaterialCreationDialog.h"

#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "Material.h"

namespace jam
{

MaterialCreationDialog::MaterialCreationDialog(
    const fs::path& _dir)
    : AssetDialog("create material")
    , m_dir(_dir)
{
    JAM_ASSERT(fs::is_directory(_dir), "parent path must be a directory.");
}

void MaterialCreationDialog::OnShow()
{
    // test & input
    Widget_WrappedText("write the name of the new material.", k_textWrapWidth);
    SetNextItemAvailWidth();
    ImGui::InputText("##CreateMaterialDialogInput", &m_name);

    // buttons
    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnSize))
    {
        if (OnClickCreate_())
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

bool MaterialCreationDialog::OnClickCreate_()
{
    fs::path filename = m_name;
    filename.replace_extension(k_jamMaterialExt.GetPtr());

    Ref<Material> material = MakeRef<Material>();
    material->LoadMemory({}, {}, {});
    return SaveAsset(material, m_dir, filename);
}

}   // namespace jam