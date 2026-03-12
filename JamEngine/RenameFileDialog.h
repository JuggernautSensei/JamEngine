#pragma once
#include "Dialog.h"

namespace Jam
{

struct RenameFileDialogInfo
{
    StringView title   = {};
    ImVec2     size    = { 400.f, 0.f };
    String     srcPath = {};
};

class RenameFileDialog : public Dialog
{
public:
    explicit RenameFileDialog(const RenameFileDialogInfo& _info);
    void OnShow() override;

private:
    String m_oldPath  = {};
    String m_filename = {};
};

}   // namespace Jam
