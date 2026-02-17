#pragma once
#include "Dialog.h"

namespace Jam
{

struct MessageDialogInfo
{
    DialogInfo baseInfo = {};
    StringView msg      = {};
};

class MessageDialog : public Dialog
{
public:
    explicit MessageDialog(const MessageDialogInfo& _desc);
    void OnShow() override;

private:
    String m_msg = {};
};

}   // namespace Jam
