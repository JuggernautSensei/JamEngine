#pragma once
#include "AssetDialog.h"

namespace jam
{

class MaterialCreationDialog : public AssetDialog
{
public:
    explicit MaterialCreationDialog(const fs::path& _dir);
    void OnShow() override;

private:
    bool OnClickCreate_();

    fs::path    m_dir;
    std::string m_name;
};

}   // namespace jam
