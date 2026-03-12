#pragma once
#include "../JamEngine/Dialog.h"
#include "../JamEngine/FileDialog.h"

class MainScene;

class NewProjectDialog : public Dialog
{
public:
    explicit NewProjectDialog();
    void OnShow() override;

private:
    bool OnClickCreateButton_() const;

    String     m_dir      = {};
    String     m_name = "new project";
    FileDialog m_dlg;
};
