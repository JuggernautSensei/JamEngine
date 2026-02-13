#pragma once
#include "Dialog.h"

namespace Jam
{

struct ConfirmationDialogDesc
{
    DialogDesc       baseDesc  = {};
    StringView       msg       = {};
    Delegate<void()> OnConfirm = nullptr;
    Delegate<void()> OnCancel  = nullptr;
};

class ConfirmationDialog : public Dialog
{
public:
    explicit ConfirmationDialog(const ConfirmationDialogDesc& _desc);
    void OnShow() override;

private:
    String           m_msg           = {};
    Delegate<void()> m_OnClickOk     = nullptr;
    Delegate<void()> m_OnClickCancel = nullptr;
};

}   // namespace Jam
