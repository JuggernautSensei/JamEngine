#pragma once
#include "BasicRenderer.h"
#include "DialogStack.h"
#include "EditorIcons.h"
#include "EditorStyle.h"
#include "Editor_EntityInspector.h"
#include "Editor_FileBrowser.h"
#include "Editor_Profiler.h"
#include "Editor_SceneHierarchy.h"
#include "Editor_SceneView.h"
#include "Editor_VertexLayoutEditor.h"
#include "VertexLayoutEntry.h"

namespace Jam
{

class Event;
class Dialog;
class IEditorCommand;
class VertexLayoutMap;
class VertexLayoutEntry;

enum class eEditorWindowType
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
    Editor(const Editor&)            = delete;
    Editor& operator=(const Editor&) = delete;
    Editor(Editor&&)                 = delete;
    Editor& operator=(Editor&&)      = delete;

    [[nodiscard]] static Editor& GetInstance();

    // mode
    void                      SetMode(eEditorMode _mode);
    [[nodiscard]] eEditorMode GetMode() const;

    // focused entity
    void                 SetFocusingEntity(Entity _entity);
    void                 ReleaseFocusingEntity();
    [[nodiscard]] Entity GetFocusingEntity() const;

    // style
    [[nodiscard]] EditorStyle&  GetStyle();
    [[nodiscard]] float         GetStyleVar(eEditorStyleVar _var) const;
    [[nodiscard]] ImVec4        GetStyleColor(eEditorStyleColor _col) const;
    [[nodiscard]] ImU32         GetStyleColorU32(eEditorStyleColor _col) const;
    [[nodiscard]] ImGuiKeyChord GetStyleShortCut(eEditorShortCut _shortCut) const;

    // icons
    [[nodiscard]] Ref<Texture> GetIcon(eEditorIcon _icon) const;

    // vertex layout
    [[nodiscard]] VertexLayoutMap& GetVertexLayoutMap();

    // dialog
    void PushDialog(const Ref<Dialog>& _pDialog);

    // scene
    void LoadScenByFileDialog();
    void SaveSceneByFileDialog(const Ref<Scene>& _pScene, Flags<eSaveFileFlags> _flags = eSaveFileFlags::None);

    // entity operator
    void               CreateEntitiesFrom(const Ref<Scene>& _pScene, const Ref<Model>& _pModel);
    void               CreateEntity(const Ref<Scene>& _pScene, Optional<Entity> _parent = k_nullopt);
    void               CloneEntity(const Ref<Scene>& _pScene, Entity _srcEntity);
    void               DestroyEntity(const Ref<Scene>& _pScene, Entity _entity);
    [[nodiscard]] bool CanEntityClone(Entity _entity) const;
    [[nodiscard]] bool CanEntityDestroy(Entity _entity) const;

    // filesystem operator
    bool RemoveFile(const Path& _path);
    bool ReplaceFileDir(const Path& _srcPath, const Path& _newDir);
    bool ReplaceFileName(const Path& _srcPath, StringView _newFilename);
    bool RenameFile(const Path& _srcPath, const Path& _dstPath);
    bool CreateDir(const Path& _dirPath);
    bool CreateNewDir(const Path& _parentDir);

    // asset operator
    [[nodiscard]] Optional<Ref<Mesh>>     LoadMesh(const Path& _path, bool _bForceReload = false);
    [[nodiscard]] Optional<Ref<Texture>>  LoadTexture(const Path& _path, bool _bForceReload = false);
    [[nodiscard]] Optional<Ref<Material>> LoadMaterial(const Path& _path, bool _bForceReload = false);
    [[nodiscard]] Optional<Ref<Model>>    LoadModel(const Path& _path, bool _bForceReload = false);

private:
    Editor();
    ~Editor();

    static void CreateInstance();
    static void DestroyInstance();

    void Update(float _deltaSec);
    void Render();
    void ImGuiRender();
    void OnEvent(Event& _event);

    void OnSceneChanged_(SceneChangedEvent& _event);
    void OnEntityDestroyed_(const EntityDestroyedEvent& _event);

    void Menu_File_() const;
    void Menu_Window_();
    void Menu_Options_() const;

    void ShowWorkspace_();
    void SetupDefaultTheme_() const;
    void LoadIcons_();

    // config
    void UpdateAutoSaveState_(float _deltaSec);
    void SaveConfig_() const;
    void LoadConfigIfExist_();

    [[nodiscard]] Ref<Texture> LoadIconFromSvgSource(StringView _svgSource) const;
    [[nodiscard]] bool&        GetWindowShow(eEditorWindowType _type);

    // windows
    Editor_SceneView          renderViewWindow         = {};
    Editor_Profiler           profilerWindow           = {};
    Editor_SceneHierarchy     sceneHierarchyWindow     = {};
    Editor_EntityInspector    entityInspectorWindow    = {};
    Editor_FileBrowser        fileBrowserWindow        = {};
    Editor_VertexLayoutEditor vertexLayoutEditorWindow = {};

    Array<bool, CountOf<eEditorWindowType>()> windowShows = {};

    // data
    Entity          focusedEntity = Entity::k_invalid;
    eEditorMode     mode          = eEditorMode::Edit;
    EditorStyle     style         = {};
    VertexLayoutMap vlMap         = {};
    DialogStack     dialogStack   = {};
    EditorIcons     icons         = {};

    Path  configPath           = {};
    float configSaveCounterSec = 0.f;

    constexpr static float       k_configSaveIntervalSec = 10.f;   // save config every 10 seconds
    constexpr static const char* k_configFileName        = "EditorConfig.json";
    constexpr static Vec2i       k_iconSize              = { 64, 64 };

    inline static Editor* s_pInstance = nullptr;
    friend class Application;
};

}   // namespace Jam

#define JAM_EDITOR Jam::Editor::GetInstance()