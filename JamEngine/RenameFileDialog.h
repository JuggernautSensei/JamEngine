#pragma once
#include "Dialog.h"

namespace Jam
{

struct RenameFileDialogDesc
{
    DialogDesc baseDesc = {};
    Path       srcPath  = {};
};

class RenameFileDialog : public Dialog
{
public:
    explicit RenameFileDialog(const RenameFileDialogDesc& _desc);
    void OnShow() override;

private:
    Path   m_oldPath  = {};
    String m_filename = {};
};

}   // namespace Jam
