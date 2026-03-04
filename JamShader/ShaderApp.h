#pragma once
#include "Icons.h"
#include "../JamEngine/DialogStack.h"

class FileBrowserWindow;
class WorkspaceWindow;

namespace Jam
{

class Dialog;

}

class ProjectChangedEvent;
class ProjectRenamedEvent;
struct FileEntry;

enum class eWindows
{
    FileBrowser,
    Workspace,
};

class ShaderApp : public Application
{
public:
    explicit ShaderApp(
        StringView                _appName,
        Flags<eAppSubSystemFlags> _subSystems);

    // app resource
    void                              SetIcon(eIcon _icon, const Ref<Texture>& _pTexture);
    [[nodiscard]] const Ref<Texture>& GetIcon(eIcon _icon) const;

    // project
    Error                             LoadProject(StringView _path);
    void                              SetProject(const Ref<Project>& _pProject);
    [[nodiscard]] const Ref<Project>& GetProject() const;
    [[nodiscard]] const Ref<Project>& GetProjectOrNull() const;

    // dialog & short cut for dialog
    void PushDialog(const Ref<Dialog>& _pDialog);
    void PushMessageDialog(StringView _title, StringView _message);
    void PushConfirmationDialog(StringView _title, StringView _message, const Delegate<void()>& _onConfirm);

    // serialization
    void  WriteToArchive(ArchiveWriter& _archive) const;
    Error ReadFromArchive(ArchiveReader& _archive);

private:
    // interface
    void Init(Int32 _argc, char** _argv) override;
    void Shutdown() override;

    void Update(float _dt) override;
    void ImGuiRender() override;
    void OnEvent(Event& _event) override;

    void         SetupTheme_() const;
    void         SetupIcons_();
    void         SetupFont_() const;
    Ref<Texture> LoadIconFromSVGSource_(StringView _source) const;

    void OnProjectChagnedEvent_(const ProjectChangedEvent& _event) const;
    void OnProjectRenamedEvent_(const ProjectRenamedEvent& _event) const;
    void OnWindowClosedEvent_(const WindowCloseEvent& _event);

    void UpdateAppTitle_() const;
    void NewProject_();
    void OpenProject_();

    void Panel_Dockspace_();
    void Panel_MainMenuBar_();
    void Panel_OpenProject_();

    void SaveConfig_() const;
    void LoadConfig_();
    void LoadConfigIfExists_();
    void UpdateAutoSaveConfig_(float _dt);

    EnumArray<eIcon, Ref<Texture>> icons                     = {};
    EnumArray<eWindows, bool>      m_windowsOpen             = {};
    DialogStack                    m_dialogStack             = {};
    Ref<Project>                   m_pProject                = nullptr;
    float                          m_configFileAutoSaveTimer = 0.0f;

    Scoped<FileBrowserWindow> m_pFileBrowserWindow = {};
    Scoped<WorkspaceWindow>   m_pWorkspaceWindow   = {};
    ImGuiID                   m_dockspaceID        = 0;

    constexpr static StringView k_appName                    = "JamShader";
    constexpr static StringView k_configFileName             = "Config.json";
    constexpr static float      k_configFileAutoSaveInterval = 10.f;   // seconds
};

[[nodiscard]] inline ShaderApp& GetShaderApp()
{
    return static_cast<ShaderApp&>(Application::GetInstance());
}

JAM_MAIN(ShaderApp, "JamShader", eAppSubSystemFlags::ImGuiEngine);