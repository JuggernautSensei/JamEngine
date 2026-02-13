#pragma once
#include "Input.h"

namespace Jam
{

class Editor_Profiler
{
public:
    void OnImGuiRender(bool& _pShow) const;

private:
    void DrawFrameTabItem_() const;
    void DrawInputTabItem_() const;
};

}   // namespace Jam