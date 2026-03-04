#pragma once
#include "../JamEngine/Dialog.h"
#include "../JamEngine/FileDialog.h"

class MainScene;

struct NewProjectDialogInfo
{
    StringView title = "new project";
    ImVec2     size  = { 400.f, 0.f };
    StringView dir   = {};
};

class NewProjectDialog : public Dialog
{
public:
    explicit NewProjectDialog(const NewProjectDialogInfo& _info);
    void OnShow() override;

private:
    bool OnClickCreateButton_() const;

    String     m_dir      = {};
    String     m_baseName = "new project";
    FileDialog m_dlg;
};
