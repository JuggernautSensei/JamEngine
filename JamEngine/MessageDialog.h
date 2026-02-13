#pragma once
#include "Dialog.h"

namespace Jam
{

struct MessageDialogDesc
{
    DialogDesc baseDesc = {};
    StringView msg      = {};
};

class MessageDialog : public Dialog
{
public:
    explicit MessageDialog(const MessageDialogDesc& _desc);
    void OnShow() override;

private:
    String m_msg = {};
};

}   // namespace Jam
