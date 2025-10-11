#pragma once

// clang-format off
namespace jam { class Entity; class Event; }
// clang-format on

namespace jam
{

enum class ePanel : Int8
{
    Debug = 0,
    Viewport,
    SceneHierarchy,
    AssetInspector,
    EntityInspector,
    ContentsBrowser,
    Toolbar,
    Console,
};

namespace editor
{
    void Init();
    void Shutdown();

    void Render();
    void OnEvent(Event& _event);

    void               SetPanelVisible(ePanel _panel, bool _bVisible);
    JAM_NODISCARD bool IsPanelVisible(ePanel _panel);

    void                 SetFocusEntity(Entity _entity);
    JAM_NODISCARD Entity GetFocusEntity();

}   // namespace editor
}   // namespace jam