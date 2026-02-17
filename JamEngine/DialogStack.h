#pragma once

namespace Jam
{

class Dialog;

class DialogStack
{
public:
    void Push(const Ref<Dialog>& _pDialog);
    void Show();

private:
    void ShrinkToFit_();

    Vector<Ref<Dialog>> m_dialogs = {};
};

}   // namespace Jam
