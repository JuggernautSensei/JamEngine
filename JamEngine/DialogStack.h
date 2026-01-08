#pragma once

namespace jam
{
class Dialog;

class DialogStack
{
public:
    void Push(Scoped<Dialog>&& _pDialog);
    void Show();

private:
    void ShrinkToFit_();

    Vector<Scoped<Dialog>> m_dialogs;
};

}   // namespace jam
