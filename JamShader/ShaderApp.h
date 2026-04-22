#pragma once
#include "Icons.h"
#include "FileBrowserWindow.h"
#include "ProjectInspectorWindow.h"
#include "ShaderItemEditorWindow.h"
#include "../JamEngine/DialogStack.h"

class ProjectDirtyChangedEvent;

namespace Jam
{

class Dialog;

}

class ProjectChangedEvent;
class ProjectRenamedEvent;
struct FileItem;

enum class eShortCut
{
    NewProject,          // default: Ctrl + Shift + N
    OpenProject,         // default: Ctrl + Shift + O
    SaveProject,         // default: Ctrl + S
    SaveAsProject,       // default: Ctrl + Shift + S
    ExitApp,             // default: Alt + F4
    CompileShaderItem,   // default: F5
    ExportProject,       // default: Ctrl + Shift + E
};

enum class eWindows
{
    FileBrowser,
    ProjectInspector,
    ShaderItemEditor,
};

class ShaderApp : public Application
{
public:
    explicit ShaderApp(StringView _appName, Flags<eAppCoreFlags> _subSystems);

    // app resource
    void                              SetIcon(eIcon _icon, const Ref<Texture>& _pTexture);
    [[nodiscard]] const Ref<Texture>& GetIcon(eIcon _icon) const;
    void                              SetShortCut(eShortCut _shortCut, ImGuiKeyChord _key);
    [[nodiscard]] ImGuiKeyChord       GetShortCut(eShortCut _shortCut) const;

    // project
    Error                  LoadProject(StringView _path);
    void                   SetProject(const Project& _pProject);
    void                   ReleaseProject();
    [[nodiscard]] Project* GetProjectOrNull();

    // focusing shader item
    void                            SetFocusingShaderItem(StringView _name);
    void                            ReleaseFocusingShaderItem();
    [[nodiscard]] const ShaderItem* GetFocusingShaderItemOrNull() const;

    // short cut for dialog
    void PushDialog(Scoped<Dialog>&& _pDialog);
    void PushMessageDialog(StringView _title, StringView _message);
    void PushConfirmationDialog(StringView _title, StringView _message, const Delegate<void()>& _onConfirm);

    // core systems
    [[nodiscard]] DialogStack& GetDialogStack();

    // serialization
    void  WriteToArchive(ArchiveWriter& _archive) const;
    Error ReadFromArchive(ArchiveReader& _archive);

private:
    // interface
    void Init(Int32 _argc, char** _argv) override;
    void Shutdown() override;

    void Render() override;
    void Update(float _dt) override;
    void ImGuiRender() override;
    void OnEvent(Event& _event) override;

    void         SetupTheme_() const;
    void         SetupIcons_();
    void         SetupFont_() const;
    void         SetupShortCuts_();
    Ref<Texture> LoadIconFromSVGSource_(StringView _source) const;

    void OnProjectChagnedEvent_(const ProjectChangedEvent& _event);
    void OnProjectRenamedEvent_(const ProjectRenamedEvent& _event) const;
    void OnProjectDirtyChangedEvent_(const ProjectDirtyChangedEvent& _event) const;
    void OnWindowClosedEvent_(const WindowClosedEvent& _event);

    void UpdateAppTitle_() const;
    void NewProject_();
    void OpenProject_();
    void SaveProject_();
    void SaveAsProject_();

    void Panel_Dockspace_();
    void Panel_MainMenuBar_();
    void Panel_OpenProject_();

    void SaveConfig_() const;
    void LoadConfig_();
    void LoadConfigIfExists_();
    void UpdateAutoSaveConfig_(float _dt);

    void GlobalShortCut_();
    void TryQuit_();
    void SaveAndQuit_();
    void ExportProject_();

    EnumArray<eIcon, Ref<Texture>>      m_icons                   = {};
    EnumArray<eShortCut, ImGuiKeyChord> m_shortcuts               = {};
    EnumArray<eWindows, bool>           m_windowsOpen             = {};
    DialogStack                         m_dialogStack             = {};
    float                               m_configFileAutoSaveTimer = 0.0f;

    Optional<Project> m_project                = {};
    String            m_focusingShaderItemName = {};

    FileBrowserWindow      m_fileBrowserWindow      = {};
    ProjectInspectorWindow m_projectInspectorWindow = {};
    ShaderItemEditorWindow m_shaderItemEditorWindow = {};
    ImGuiID                m_dockspaceID            = 0;

    constexpr static StringView kAppName                    = "JamShader";
    constexpr static StringView kConfigFileName             = "Config.json";
    constexpr static float      kConfigFileAutoSaveInterval = 10.f;   // seconds
};

[[nodiscard]] inline ShaderApp& GetShaderApp()
{
    return static_cast<ShaderApp&>(Application::GetInstance());
}

JAM_MAIN(ShaderApp, "JamShader", eAppCoreFlags::ImGuiEngine);