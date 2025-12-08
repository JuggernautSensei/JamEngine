#include "pch.h"

#include "RemoveNotEmptyDirectoryDialog.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"

namespace jam
{

RemoveNotEmptyDirectoryDialog::RemoveNotEmptyDirectoryDialog(const fs::path& _path)
    : m_path(_path)
{
}

void RemoveNotEmptyDirectoryDialog::operator()()
{
    ImGui::TextUnformatted("the directory is not empty.\nare you sure you want to delete it and all its contents?");

    const ImVec2 btnRes = CalcFitButtonSize(2);
    if (ImGui::Button("yes", btnRes))
    {
        if (OnClickYes_())
            ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("no", btnRes))
        ImGui::CloseCurrentPopup();

    m_popup.Show();
}

bool RemoveNotEmptyDirectoryDialog::OnClickYes_()
{
    try
    {
        fs::remove_all(m_path);
        return true;
    }
    catch (const fs::filesystem_error& ec)
    {
        std::string msg = std::format("failed to remove directory: {}\nerror: {}", m_path.native(), ec.what());
        m_popup.Open("directory remove error", msg);
        return false;
    }
}

}   // namespace jam