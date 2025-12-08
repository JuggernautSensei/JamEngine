#pragma once
#include "PopupDialog.h"

namespace jam
{

class CreateMaterialDialog
{
public:
    explicit CreateMaterialDialog(const fs::path& _parentPath);
    void operator()();

private:
    bool OnClickCreate_();

    fs::path    m_parentPath;
    std::string m_buf;
    PopupDialog m_popup;
};

}   // namespace jam