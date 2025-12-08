#pragma once
#include "EditorStyle.h"

namespace jam
{

class Scene;
class VertexLayoutStorage;
class Entity;
class Event;

enum class eEditorWindowType : Int8
{
    Viewport,
    SceneHierarchy,
    EntityInspector,
    FileSystemBrowser,
    Profiler,
    VertexLayoutManager,
};

enum class eEditorMode
{
    Edit = 0,
    Runtime,
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

    [[nodiscard]] static bool IsEditMode();
    [[nodiscard]] static bool IsRuntimeMode();
    static void               SetEditorMode(eEditorMode _mode);

    static void                 SetFocusEntity(Entity _entity);
    [[nodiscard]] static Entity GetFocusEntity();

    [[nodiscard]] static const EditorStyle&   GetStyle();
    [[nodiscard]] static ImVec4               GetStyleColor(eEditorStyleColor _color);
    [[nodiscard]] static ImU32                GetStyleColorU32(eEditorStyleColor _color);
    [[nodiscard]] static VertexLayoutStorage& GetVertexLayoutStorage();

    static void OpenCorePopupDialog(std::string_view _name, std::string_view _message);
    static void OpenCorePopupDialog(std::string_view _name, std::function<void()> _onImGuiRender);

    static void CreateEntityToActiveScene(Entity _parentOrNull = Entity::s_null);
    static void CloneEntityToActiveScene(Entity _entity, Entity _parentOrNull = Entity::s_null);
    static void DestroyEntityToActiveScene(Entity _entity, bool _bForce = false);

    static void SaveActiveSceneByDialog();
    static void LoadActiveSceneByDialog();
};

}   // namespace jam