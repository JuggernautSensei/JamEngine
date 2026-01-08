#pragma once
#include "Dialog.h"

namespace jam
{

class RenameFileDialog : public Dialog
{
public:
    explicit RenameFileDialog(
        const fs::path& _path,
        ImVec2          _size  = ImVec2 { 0.f, 0.f },
        eDialogFlags    _flags = eDialogFlags_None);

    void OnShow() override;

private:
    bool OnClickOkButton() const;

    fs::path    m_parentPath  = {};
    fs::path    m_srcFilename = {};
    std::string m_name        = {};
};

}   // namespace jam
