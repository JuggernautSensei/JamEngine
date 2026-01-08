#pragma once

namespace jam
{

class Editor_EntityInspector
{
public:
    void OnImGuiRender(bool& _pShow) const;

private:
    void Menu_RemoveComponent_() const;
    void Menu_AddComponent_() const;
};

}   // namespace jam
