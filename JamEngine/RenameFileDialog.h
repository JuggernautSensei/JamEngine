#pragma once
#include "PopupDialog.h"

namespace jam
{

class RenameFileDialog
{
public:
    explicit RenameFileDialog(const fs::path& _path);

    void operator()();

private:
    bool OnClickOk_();

    PopupDialog m_popup;
    std::string m_buf;
    fs::path    m_srcPath;
};
}