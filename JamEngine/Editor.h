#pragma once
#include "EditorStyle.h"

namespace jam
{
class Dialog;
class IEditorCommand;

class Event;

enum class eEditorWindowType : Int8
{
    SceneView,
    SceneHierarchy,
    EntityInspector,
    FileBrowser,
    Profiler,
    VertexLayoutEditor,
};

enum class eEditorMode
{
    Edit = 0,
    Play,
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

    static void                      SetMode(eEditorMode _mode);
    [[nodiscard]] static eEditorMode GetMode();

    static void                 SetFocusedEntity(Entity _entity);
    [[nodiscard]] static Entity GetFocusedEntity();

    // style
    [[nodiscard]] static EditorStyle& GetStyle();
    [[nodiscard]] static ImVec4       GetStyleColor(eEditorStyleColor _col);
    [[nodiscard]] static ImU32        GetStyleColorU32(eEditorStyleColor _col);

    [[nodiscard]] static VertexLayoutStorage& GetVertexLayoutStorage();

    // command
    static void               ExecuteCommand(Scoped<IEditorCommand>&& _command);
    static void               UndoCommand();
    static void               RedoCommand();
    [[nodiscard]] static bool CanUndoCommand();
    [[nodiscard]] static bool CanRedoCommand();

    // dialog
    static void PushDialog(Scoped<Dialog>&& _pDialog);

private:
    static void UpdateAutoSaveConfig_(float _deltaSec);
};

}   // namespace jam