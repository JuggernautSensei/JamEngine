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
    eastl::erase_if(m_dialogs,
                    [](const Ref<Dialog>& _pDlg)
                    {
                        return _pDlg == nullptr;
                    });
}

}   // namespace Jam