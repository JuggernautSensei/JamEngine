#include "pch.h"

#include "RenameFileDialog.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"

namespace jam
{

RenameFileDialog::RenameFileDialog(const fs::path& _path)
    : m_srcPath(_path)
{
    m_buf = _path.filename();
}

void RenameFileDialog::operator()()
{
    ImGui::TextUnformatted("write new file name");
    SetNextItemWidthToAvailableWidth();
    ImGui::InputText("##RenameFileInputText", &m_buf);

    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("ok", btnSize))
    {
        if (OnClickOk_())
        {
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
    {
        ImGui::CloseCurrentPopup();
    }

    m_popup.Show();
}

bool RenameFileDialog::OnClickOk_()
{
    if (m_buf.empty())
    {
        m_popup.Open("error", "file name cannot be empty.");
        return false;
    }

    fs::path dstPath = m_srcPath.parent_path() / m_buf;
    if (dstPath != m_srcPath)
    {
        if (fs::exists(dstPath))
        {
            std::string msg = std::format("cannot rename file.\ntarget file already exists: {}", dstPath.native());
            m_popup.Open("error", msg);
            return false;
        }

        try
        {
            fs::rename(m_srcPath, dstPath);
        }
        catch (const fs::filesystem_error& e)
        {
            std::string msg = std::format("failed to rename file: {}\nerror: {}", m_srcPath.native(), e.what());
            m_popup.Open("error", msg);
            return false;
        }
    }

    return true;
}

}   // namespace jam