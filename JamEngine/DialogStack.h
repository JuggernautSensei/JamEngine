#pragma once

namespace jam
{
class Dialog;

class DialogStack
{
    constexpr static Int32 k_maxDialogStackSize = 16;

public:
    void Push(Scoped<Dialog>&& _pDialog);
    void Show();

private:
    [[nodiscard]] bool IsEmpty() const;

    Array<Scoped<Dialog>, k_maxDialogStackSize> m_dialogs = {};
    Int32                                       m_top     = -1;
};

}   // namespace jam
