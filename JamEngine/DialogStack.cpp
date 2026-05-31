#include "pch.h"
#include "DialogStack.h"

#include "Dialog.h"

namespace Jam
{

void DialogStack::Push(
    Scoped<Dialog>&& _pDialog)
{
    const StringView title = _pDialog->GetTitle();
    auto             it    = std::ranges::find_if(m_dialogs, [title](const DialogItem& _pItem) { return _pItem.pDialog->GetTitle() == title; });
    if (it != m_dialogs.end())
    {
        *it = DialogItem { std::move(_pDialog), false };   // overwrite
    }
    else
    {
        m_dialogs.emplace_back(std::move(_pDialog), false);
    }
}

void DialogStack::Show()
{
    if (m_dialogs.empty())
    {
        return;
    }

    ShowDialogs_Recursive_(0);

    // clean up closed dialog
    if (m_bAnyDialogClosed)
    {
        eastl::erase_if(m_dialogs, [](const DialogItem& item) { return item.bClosed; });
    }

    m_bAnyDialogClosed = false;
}

void DialogStack::Clear()
{
    m_dialogs.clear();
}

void DialogStack::ShowDialogs_Recursive_(
    const SizeT _index)   // rendering modal stack
{
    Scoped<Dialog>& pDialog = m_dialogs[_index].pDialog;
    if (pDialog->BeginShow())
    {
        // modal popup stack
        if (_index + 1 < m_dialogs.size())
        {
            ShowDialogs_Recursive_(_index + 1);
        }

        pDialog->EndShow();
    }
    else
    {
        m_dialogs[_index].bClosed = true;
        m_bAnyDialogClosed      = true;
    }
}

}   // namespace Jam