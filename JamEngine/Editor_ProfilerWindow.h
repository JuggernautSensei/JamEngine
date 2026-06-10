#pragma once

namespace Jam
{

class Editor_ProfilerWindow
{
public:
    void OnImGuiRender(bool& _pShow) const;

private:
    void DrawFrameTabItem_() const;
    void DrawInputTabItem_() const;
};

}   // namespace Jam