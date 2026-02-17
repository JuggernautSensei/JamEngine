#pragma once
#include "Dialog.h"

namespace Jam
{

struct ConfirmationDialogInfo
{
    DialogInfo       baseInfo  = {};
    StringView       msg       = {};
    Delegate<void()> OnConfirm = nullptr;
    Delegate<void()> OnCancel  = nullptr;
};

class ConfirmationDialog : public Dialog
{
public:
    explicit ConfirmationDialog(const ConfirmationDialogInfo& _desc);
    void OnShow() override;

private:
    String           m_msg           = {};
    Delegate<void()> m_OnClickOk     = nullptr;
    Delegate<void()> m_OnClickCancel = nullptr;
};

}   // namespace Jam
