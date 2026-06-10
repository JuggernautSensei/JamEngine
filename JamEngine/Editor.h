#pragma once
#include "DialogStack.h"
#include "EditorIcons.h"
#include "EditorStyle.h"
#include "Entity.h"

namespace Jam
{

namespace FileSystem
{
    enum class eMkdirCollisionPolicy;
    enum class eFileCollisionPolicy;
}   // namespace FileSystem

class Mesh;
class Model;
class Event;
class Dialog;
class DirEntry;
class Material;
class FileEntry;
class VertexLayoutEntry;
class SceneChangedEvent;
class EntityDestroyedEvent;
class Editor_ProfilerWindow;
class Editor_SceneViewWindow;
class VertexLayoutEntryManager;
class Editor_FileBrowserWindow;
class Editor_SceneHierarchyWindow;
class Editor_EntityInspectorWindow;
class Editor_VertexLayoutEditorWindow;

enum class eEditorWindow
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

struct EditorSharedData
{
    // gui
    EditorStyle                          style = {};
    EnumMap<eEditorIcon, Ref<Texture>> icons = {};

    // editor core state
    eEditorMode mode = eEditorMode::Edit;

    // game
    Entity focusingEntity = Entity::kNull;

    // vertex layout
    VertexLayoutEntryManager vertexLayoutEntryManager = {};

    // file
    Ref<FileEntry> pFocusingFileEntry = nullptr;
    Ref<DirEntry>  pFocusingDirEntry  = nullptr;
};

class Editor
{
public:
    Editor(const Editor&)            = delete;
    Editor& operator=(const Editor&) = delete;
    Editor(Editor&&)                 = delete;
    Editor& operator=(Editor&&)      = delete;

    [[nodiscard]] static Editor& GetInstance();

    // shared data
    void                      SetMode(eEditorMode _mode);
    [[nodiscard]] eEditorMode GetMode() const;

    void                 SetFocusingEntity(Entity _entity);
    [[nodiscard]] Entity GetFocusingEntityOrNull() const;
    [[nodiscard]] Entity GetFocusingEntity() const;

    [[nodiscard]] const EditorStyle& GetStyle() const;
    [[nodiscard]] float              GetStyleVar(eEditorStyleVar _var) const;
    [[nodiscard]] ImVec4             GetStyleColor(eEditorStyleColor _col) const;
    [[nodiscard]] ImU32              GetStyleColorU32(eEditorStyleColor _col) const;
    [[nodiscard]] ImGuiKeyChord      GetStyleShortCut(eEditorShortCut _shortCut) const;

    void                       SetIcon(eEditorIcon _icon, const Ref<Texture>& _pTexture);
    [[nodiscard]] Ref<Texture> GetIcon(eEditorIcon _icon) const;

    void                                SetFocusingFileEntry(const Ref<FileEntry>& _pEntry);
    [[nodiscard]] const Ref<FileEntry>& GetFocusingFileEntryOrNull() const;
    [[nodiscard]] const Ref<FileEntry>& GetFocusingFileEntry() const;

    void                               SetFocusingDirEntry(const Ref<DirEntry>& _pEntry);
    [[nodiscard]] const Ref<DirEntry>& GetFocusingDirEntryOrNull() const;
    [[nodiscard]] const Ref<DirEntry>& GetFocusingDirEntry() const;

    // vertex layout
    [[nodiscard]] VertexLayoutEntryManager& GetVertexLayoutManager();

    // dialog + short cut
    void PushDialog(Scoped<Dialog>&& _pDialog);
    void PushMessageDialog(StringView _title, StringView _msg);
    void PushConfirmationDialog(StringView _title, StringView _msg, const Delegate<void()>& _onConfirm);

    // scene
    void LoadScenByFileDialog();
    void SaveSceneByFileDialog(Scene* _pScene);

    // entity operator
    void               CreateEntitiesFrom(Scene* _pScene, const Ref<Model>& _pModel);
    void               CreateEntity(Scene* _pScene, Optional<Entity> _parent = kNullopt);
    void               CloneEntity(Scene* _pScene, Entity _srcEntity);
    void               DestroyEntity(Scene* _pScene, Entity _entity);
    [[nodiscard]] bool CanEntityClone(Entity _entity) const;
    [[nodiscard]] bool CanEntityDestroy(Entity _entity) const;

    // filesystem operator
    bool RemoveFile(StringView _path, bool _bForced = false);   // if _bForced is true, it will remove non-empty directory
    bool RenameFile(StringView _srcPath, StringView _dstPath, FileSystem::eFileCollisionPolicy _policy);
    bool CreateDir(StringView _dir, FileSystem::eMkdirCollisionPolicy _policy);

    // asset operator
    [[nodiscard]] Expect<Ref<Mesh>>     LoadMesh(StringView _path, bool _bForceReload = false);
    [[nodiscard]] Expect<Ref<Texture>>  LoadTexture(StringView _path, bool _bForceReload = false);
    [[nodiscard]] Expect<Ref<Material>> LoadMaterial(StringView _path, bool _bForceReload = false);
    [[nodiscard]] Expect<Ref<Model>>    LoadModel(StringView _path, bool _bForceReload = false);

private:
    Editor();
    ~Editor();

    static void CreateInstance();
    static void DestroyInstance();

    void SetupDefaultTheme_() const;
    void SetupIcons_();

    void Update(float _deltaSec);
    void Render() const;
    void ImGuiRender();
    void OnEvent(Event& _event);

    void OnSceneChanged_(SceneChangedEvent& _event);
    void OnEntityDestroyed_(const EntityDestroyedEvent& _event);

    void Panel_Dockspace_() const;
    void Panel_MainMenuBar_();

    void Menu_File_() const;
    void Menu_Window_();
    void Menu_Options_() const;

    // config
    void UpdateAutoSaveConfig_(float _deltaSec);
    void SaveConfig_() const;
    void TryLoadConfig_();

    [[nodiscard]] Ref<Texture> LoadIconFromSvgSource(StringView _svgSource) const;
    [[nodiscard]] bool&        GetWindowShow(eEditorWindow _type);

    // windows
    Scoped<Editor_SceneViewWindow>          m_pSceneViewWindow          = nullptr;
    Scoped<Editor_ProfilerWindow>           m_pProfilerWindow           = nullptr;
    Scoped<Editor_SceneHierarchyWindow>     m_pSceneHierarchyWindow     = nullptr;
    Scoped<Editor_EntityInspectorWindow>    m_pEntityInspectorWindow    = nullptr;
    Scoped<Editor_FileBrowserWindow>        m_pFileBrowserWindow        = nullptr;
    Scoped<Editor_VertexLayoutEditorWindow> m_pVertexLayoutEditorWindow = nullptr;

    EditorSharedData               m_sharedData  = {};
    EnumMap<eEditorWindow, bool> m_windowShows = {};
    DialogStack                    m_dialogStack = {};

    float m_configSaveCounterSec = 0.f;

    constexpr static float       kConfigFileAutoSaveIntervalSec = 10.f;   // save config every 10 seconds
    constexpr static const char* kConfigFileName                = "EditorConfig.json";
    constexpr static Vec2i       kIconSize                      = { 64, 64 };

    inline static Editor* s_pInstance = nullptr;
    friend class Application;
};

[[nodiscard]] inline Editor& GetEditor()
{
    return Editor::GetInstance();
}

}   // namespace Jam
