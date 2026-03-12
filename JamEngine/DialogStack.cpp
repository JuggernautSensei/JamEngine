#include "pch.h"
#include "DialogStack.h"

#include "Dialog.h"
#include "STLAlgorithm.h"

namespace Jam
{

void DialogStack::Push(
    Scoped<Dialog>&& _pDialog)
{
    const StringView title = _pDialog->GetTitle();
    if (VectorContainsIf(m_dialogs, [title](const DialogItem& _pItem) { return _pItem.pDialog->GetTitle() == title; }))
    {
        return;   // not allow duplicate dialog title in stack.
    }

    m_dialogs.emplace_back(std::move(_pDialog), false);
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
    const SizeT _idx)   // rendering modal stack
{
    Scoped<Dialog>& pDialog = m_dialogs[_idx].pDialog;
    if (pDialog->BeginShow())
    {
        // modal popup stack
        if (_idx + 1 < m_dialogs.size())
        {
            ShowDialogs_Recursive_(_idx + 1);
        }

        pDialog->EndShow();
    }
    else
    {
        m_dialogs[_idx].bClosed = true;
        m_bAnyDialogClosed      = true;
    }
}

}   // namespace Jam