#include "pch.h"
#include "DialogStack.h"

#include "Dialog.h"

namespace jam
{

void DialogStack::Push(
    Scoped<Dialog>&& _pDialog)
{
    JAM_ASSERT(m_top + 1 < k_maxDialogStackSize, "DialogStack overflow.");
    m_dialogs[++m_top] = std::move(_pDialog);
}

void DialogStack::Show()
{
    if (!IsEmpty())
    {
        // render only top dialog (not overlapped)
        Scoped<Dialog>& pDlg = m_dialogs[m_top];
        if (pDlg->IsOpen())
        {
            pDlg->Show();
        }
        else
        {
            m_dialogs[m_top].Reset();   // pop from stack
            --m_top;
        }
    }
}

bool DialogStack::IsEmpty() const
{
    return m_top < 0;
}

}   // namespace jam