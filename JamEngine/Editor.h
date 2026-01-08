#pragma once
#include "BasicRenderer.h"
#include "EditorIcons.h"
#include "EditorStyle.h"
#include "Entity.h"
#include "Model.h"

namespace jam
{

class Event;
class Dialog;
class IEditorCommand;
class VertexLayoutEntry;

enum class eEditorWindowType
{
    SceneView,
    SceneHierarchy,
    EntityInspector,
    FileBrowser,
    Profiler,
    VertexLayoutEditor,

    Min = SceneView,
    Max = VertexLayoutEditor,
};
JAM_ENUM_STATIC_ASSERT(eEditorWindowType);

enum class eEditorMode
{
    Edit = 0,
    Play,
};

using VertexLayoutMap = HashMap<std::string, Ref<VertexLayoutEntry>>;

class Editor
{
public:
    static void Init();
    static void Shutdown();

    static void Update(float _deltaSec);
    static void Render();
    static void ImGuiRender();
    static void OnEvent(Event& _event);

    static void                      SetMode(eEditorMode _mode);
    [[nodiscard]] static eEditorMode GetMode();

    static void                 SetFocusedEntity(Entity _entityOrNull);
    [[nodiscard]] static Entity GetFocusedEntity();

    // style
    [[nodiscard]] static EditorStyle& GetStyle();
    [[nodiscard]] static float        GetStyleVar(eEditorStyleVar _var);
    [[nodiscard]] static ImVec4       GetStyleColor(eEditorStyleColor _col);
    [[nodiscard]] static ImU32        GetStyleColorU32(eEditorStyleColor _col);

    // icons
    [[nodiscard]] static Ref<ImageTexture> GetIcon(eEditorIcon _icon);

    // vertex layout
    [[nodiscard]] static VertexLayoutMap& GetVertexLayoutMap();

    // dialog
    static void PushDialog(Scoped<Dialog>&& _pDialog);
    static void PushMessageDialog(StringView _title, StringView _msg);   // helpers

    // entity operations
    static void               CreateEntitiesFrom(const Ref<Scene>& _pScene, const Ref<Model>& _pModel);
    static void               CreateEntity(const Ref<Scene>& _pScene, Entity _parentOrNull = Entity::s_null);
    static void               CloneEntity(const Ref<Scene>& _pScene, Entity _srcEntity);
    static void               DestroyEntity(const Ref<Scene>& _pScene, Entity _entity);
    [[nodiscard]] static bool IsClonableEntity(const Ref<Scene>& _pScene, Entity _entity);
    [[nodiscard]] static bool IsDestroyableEntity(const Ref<Scene>& _pScene, Entity _entity);

    static void SaveSceneBySaveDialog(const Ref<Scene>& _pScene);
    static void LoadSceneByOpenDialog();
};

}   // namespace jam