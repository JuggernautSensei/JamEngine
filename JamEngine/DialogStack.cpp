#include "pch.h"
#include "DialogStack.h"

#include "Dialog.h"

namespace Jam
{

void DialogStack::Push(
    const Ref<Dialog>& _pDialog)
{
    m_dialogs.emplace_back(_pDialog);
}

void DialogStack::Show()
{
    bool bClosedAny = false;
    for (Ref<Dialog>& pDlg: m_dialogs)
    {
        if (pDlg)
        {
            if (pDlg->IsOpen())
            {
                pDlg->Show();
            }
            else
            {
                bClosedAny = true;
                pDlg.Reset();
            }
        }
    }

    // compact closed dialogs
    if (bClosedAny)
    {
        ShrinkToFit_();
    }
}

void DialogStack::ShrinkToFit_()
{
    const SizeT numDlgs = m_dialogs.size();

    SizeT i = 0;
    SizeT j = 0;
    while (true)
    {
        // find nullptr
        while (i < numDlgs && m_dialogs[i])
        {
            ++i;
        }

        // reached end
        if (i == numDlgs)
        {
            break;
        }

        j = i + 1;

        // find next non-nullptr
        while (j < numDlgs && !m_dialogs[j])
        {
            ++j;
        }

        // reached end
        if (j == numDlgs)
        {
            break;
        }

        // swap
        JAM_ASSERT(m_dialogs[i] == nullptr, "Expected nullptr at position i");
        JAM_ASSERT(m_dialogs[j], "Expected non-nullptr at position j");
        std::swap(m_dialogs[i], m_dialogs[j]);

        // next
        ++i;
    }

    m_dialogs.erase(m_dialogs.begin() + i, m_dialogs.end());
}

}   // namespace Jam