#pragma once
#include "PopupDialog.h"

namespace jam
{


class RemoveNotEmptyDirectoryDialog
{
public:
    explicit RemoveNotEmptyDirectoryDialog(const fs::path& _path);
    void operator()();

private:
    bool OnClickYes_();

    PopupDialog m_popup;
    fs::path    m_path;
};


}