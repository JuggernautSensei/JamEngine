#pragma once
#include "Dialog.h"

namespace jam
{

class ConfirmationDialog : public Dialog
{
public:
    explicit ConfirmationDialog(StringView _title, StringView _msg, const std::function<void()>& _onOk, const std::function<void()>& _onCancel = nullptr, ImVec2 _size = { 0.f, 0.f }, eDialogFlags _flags = 0);
    void OnShow() override;

private:
    std::function<void()> m_onOk     = nullptr;
    std::function<void()> m_onCancel = nullptr;
    std::string           m_msg      = {};
};

}   // namespace jam
