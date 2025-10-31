#pragma once

// clang-format off
namespace jam { class Entity; class Event; }
// clang-format on

namespace jam
{

enum class ePanel : Int8
{
    Profiler = 0,
    Viewport,
    SceneHierarchy,
    EntityInspector,
    AssetBrowser,
    FileSystemBrowser,
    Console,
};

class Editor
{
public:
    static void Init();
    static void Shutdown();

    static void Update(float _deltaSec);
    static void Render();
    static void ImGuiRender();
    static void OnEvent(Event& _event);

    static void               SetPanelVisible(ePanel _panel, bool _bVisible);
    static JAM_NODISCARD bool IsPanelVisible(ePanel _panel);

    static void                 SetFocusEntity(Entity _entity);
    static JAM_NODISCARD Entity GetFocusEntity();

};   // namespace editor

}   // namespace jam