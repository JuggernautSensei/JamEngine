#include "pch.h"

#include "CreateMaterialDialog.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "Material.h"

namespace jam
{

CreateMaterialDialog::CreateMaterialDialog(const fs::path& _parentPath)
    : m_parentPath(_parentPath)
{
    JAM_ASSERT(fs::is_directory(_parentPath), "parent path must be a directory.");
}

void CreateMaterialDialog::operator()()
{
    ImGui::TextUnformatted("write material name");
    SetNextItemWidthToAvailableWidth();
    ImGui::InputText("##MaterialInputText", &m_buf);

    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnSize))
    {
        if (OnClickCreate_())
            ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
        ImGui::CloseCurrentPopup();

    m_popup.Show();
}

bool CreateMaterialDialog::OnClickCreate_()
{
    if (m_buf.empty())
    {
        m_popup.Open("error", "material name cannot be empty.");
        return false;
    }

    fs::path path = m_parentPath / m_buf;
    path.replace_extension(k_jamMaterialExt);
    if (fs::exists(path))
    {
        std::string msg = std::format("cannot create material.\ntarget file already exists: {}", path.native());
        m_popup.Open("error", msg);
        return false;
    }

    Material mat;
    mat.LoadEmpty();
    if (!mat.Save(path))
    {
        std::string msg = std::format("failed to create material: {}\nerror: {}", path.native(), GlobalBuffer::GetLastError());
        m_popup.Open("error", msg);
        return false;
    }

    return true;
}

}   // namespace jam