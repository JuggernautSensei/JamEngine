#include "pch.h"

#include "RenameFileDialog.h"

#include "Editor.h"
#include "EditorWidgets.h"
#include "ImGuiUtilities.h"

namespace jam
{

RenameFileDialog::RenameFileDialog(
    const fs::path&    _path,
    const ImVec2       _size,
    const eDialogFlags _flags)
    : Dialog("rename file", _size, _flags)
    , m_parentPath(_path.parent_path())
    , m_srcFilename(_path.filename())
    , m_name(m_srcFilename.native())
{
}

void RenameFileDialog::OnShow()
{
    const ImVec2 btnSize = CalcFitButtonSize(2);

    // message
    Widget_WrappedText("enter new name for file:", k_textWrapWidth);
    ImGui::Spacing();

    // input
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##Input", &m_name);
    ImGui::Spacing();

    // buttons
    if (ImGui::Button("ok", btnSize))
    {
        if (OnClickOkButton())
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

bool RenameFileDialog::OnClickOkButton() const
{
    const fs::path oldPath = m_parentPath / m_srcFilename;
    const fs::path newPath = m_parentPath / m_name;

    std::error_code ec = {};
    fs::rename(oldPath, newPath, ec);
    if (ec)
    {
        const StringView msg = GlobalBuffer::FormatTmp("failed to rename file from '{}' to '{}'.\n{}", oldPath.native(), newPath.native(), ec.message());
        Editor::PushMessageDialog("error", msg);
        return false;
    }

    // succeeded
    return true;
}

}   // namespace jam