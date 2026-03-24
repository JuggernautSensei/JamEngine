#pragma once

namespace Jam
{
class SceneChangedEvent;

class Scene;

class Editor_EntityInspectorWindow
{
public:
    void OnImGuiRender(bool& _pShow) const;
    void OnEvent_(const SceneChangedEvent& _event);

private:
    void Menu_RemoveComponent_() const;
    void Menu_AddComponent_() const;

    Scene* m_pScene = nullptr;
};

}   // namespace Jam
