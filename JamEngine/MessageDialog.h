#pragma once
#include "Dialog.h"

namespace Jam
{

struct MessageDialogInfo
{
    StringView title = {};
    StringView msg   = {};
    ImVec2     size  = { 0.f, 0.f };
};

class MessageDialog : public Dialog
{
public:
    explicit MessageDialog(const MessageDialogInfo& _info);
    void OnShow() override;

private:
    String m_msg = {};
};

}   // namespace Jam
