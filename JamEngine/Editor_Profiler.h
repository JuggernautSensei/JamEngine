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
};

}   // namespace jam