#pragma once
#include "Dialog.h"

namespace jam
{

class MessageDialog : public Dialog
{
public:
    explicit MessageDialog(StringView _title, StringView _msg, ImVec2 _size = { 0.f, 0.f }, eDialogFlags _flags = 0);
    void OnShow() override;

    void SetMessage(StringView _msg);

private:
    std::string m_msg = {};
};

}   // namespace jam
