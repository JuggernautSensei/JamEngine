#pragma once
#include "Input.h"

namespace jam
{

class Editor_Profiler
{
    enum eMagic
    {
        eKeyHistoryStackCap  = 8,
        eKeyHistoryStackMask = eKeyHistoryStackCap - 1,
    };

public:
    void OnImGuiRender(bool& _pShow) const;

private:
    void DrawFrameTabItem_() const;
    void DrawInputTabItem_() const;

    eKey  m_keyHistoryStack[eKeyHistoryStackCap] = { eKey::None, eKey::None, eKey::None, eKey::None, eKey::None };
    Int32 m_keyHistoryTop                        = -1;
};

}   // namespace jam