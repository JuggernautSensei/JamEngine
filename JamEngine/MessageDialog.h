#pragma once
#include "Dialog.h"

// this is short-cut
namespace Jam
{

struct MessageDialogButtonInfo
{
    StringView       label   = {};
    Delegate<void()> onClick = {};
};

struct MessageDialogInfo
{
    StringView                          title   = {};
    ImVec2                              size    = { 500.f, 0.f };
    StringView                          message = {};
    Span<const MessageDialogButtonInfo> buttons = {};
};

class MessageDialog : public Dialog
{
public:
    explicit MessageDialog(const MessageDialogInfo& _info);
    void OnShow() override;

private:
    String                          m_message = {};
    Vector<MessageDialogButtonInfo> m_buttons = {};
};

}   // namespace Jam
