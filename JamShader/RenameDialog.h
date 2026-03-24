#pragma once
#include "../JamEngine/Dialog.h"

class RenameDialog : public Dialog
{
public:
    explicit RenameDialog(StringView _path);
    void OnShow() override;

private:
    bool OnClickRenameButton_() const;

    String m_oldPath  = {};
    String m_path     = {};
    String m_basename = {};
};
