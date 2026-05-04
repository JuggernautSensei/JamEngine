#pragma once

namespace Jam
{

class Dialog;

class DialogStack
{
    struct DialogItem
    {
        Scoped<Dialog> pDialog = nullptr;
        bool           bClosed = false;
    };

public:
    void Push(Scoped<Dialog>&& _pDialog);
    void Show();
    void Clear();

private:
    void ShowDialogs_Recursive_(SizeT _index);

    Vector<DialogItem> m_dialogs          = {};
    bool               m_bAnyDialogClosed = false;
};

}   // namespace Jam
