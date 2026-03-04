#pragma once

namespace Jam
{

class Editor_EntityInspectorWindow 
{
public:
    void OnImGuiRender(bool& _pShow) const;

private:
    void Menu_RemoveComponent_() const;
    void Menu_AddComponent_() const;
};

}   // namespace Jam
