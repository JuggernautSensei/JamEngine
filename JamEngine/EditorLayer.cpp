#include "pch.h"

#include "EditorLayer.h"

#include "Application.h"
#include "Components.h"
#include "IMessageBox.h"
#include <efsw/efsw.hpp>

using namespace jam;

// struct
namespace
{

class EditorFileWatcherListener : public efsw::FileWatchListener
{
public:
    void handleFileAction(efsw::WatchID _watchId, const std::string& _dir, const std::string& _filename, const efsw::Action _action, const std::string _oldFilename) override
    {
        Application& app  = GetApplication();
        fs::path     path = fs::path(_dir) / _filename;

        switch (_action)
        {
            case efsw::Action::Add:
            {
                Log::Debug("FileWatcher: Add: {} - {}", _dir, _filename);
                app.SubmitCommand(
                    [&app, path]
                    {
                        FileAddEvent e { path };
                        app.DispatchEvent(e);
                    });
            }
            break;

            case efsw::Action::Delete:
            {
                Log::Debug("FileWatcher: Delete: {} - {}", _dir, _filename);
                app.SubmitCommand(
                    [&app, path]
                    {
                        FileRemoveEvent e { path };
                        app.DispatchEvent(e);
                    });
            }
            break;

            case efsw::Action::Modified:
            {
                Log::Debug("FileWatcher: Modified: {} - {}", _dir, _filename);
                app.SubmitCommand(
                    [&app, path]
                    {
                        FileModifiedEvent e { path };
                        app.DispatchEvent(e);
                    });
            }
            break;

            case efsw::Action::Moved:
            {
                fs::path oldPath = fs::path(_dir) / _oldFilename;
                Log::Debug("FileWatcher: Renamed: {} - {} -> {}", _dir, _oldFilename, _filename);
                app.SubmitCommand(
                    [&app, oldPath, path]
                    {
                        FileRenamedEvent e { oldPath, path };
                        app.DispatchEvent(e);
                    });
            }
            break;
        }
    }
};

}   // namespace

// functions
namespace
{

}   // namespace

// global variables
namespace
{

Scope<efsw::FileWatcher>         g_fileWatcher         = nullptr;   // file watcher for content browser
Scope<EditorFileWatcherListener> g_fileWatcherListener = nullptr;   // file watcher listener for content browser
efsw::WatchID                    g_fileWatcgerID;                   // watch ID for content browser

}   // namespace

namespace jam
{

EditorLayer::EditorLayer()
    : m_panelShowBit({})
    , m_contentsBrowserPanel(this)
    , m_entityInspectorPanel(this)
    , m_sceneHierarchyPanel(this)
    , m_mainMenuBarPanel(this)
{

    // file watcher
    g_fileWatcher         = MakeScope<efsw::FileWatcher>();
    g_fileWatcherListener = MakeScope<EditorFileWatcherListener>();
    g_fileWatcgerID       = g_fileWatcher->addWatch(GetApplication().GetContentsDirectory().string(), g_fileWatcherListener.get(), true);
    g_fileWatcher->watch();
}

EditorLayer::~EditorLayer()
{
    // remove file watcher
    g_fileWatcher->removeWatch(g_fileWatcgerID);
}

void EditorLayer::OnUpdate(float _deltaTime)
{
}

void EditorLayer::OnRender()
{
    // dock space
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);

    // main menu bar
    m_mainMenuBarPanel.Show();

    bool& bShowViewport = m_panelShowBit[EnumToInt(ePanelShow::Viewport)];
    if (bShowViewport)
    {
        m_viewportPanel.Show(&bShowViewport);
    }

    bool& bShowSceneHierarchy = m_panelShowBit[EnumToInt(ePanelShow::SceneHierarchy)];
    if (bShowSceneHierarchy)
    {
        m_sceneHierarchyPanel.Show(&bShowSceneHierarchy);
    }

    bool& bShowDebugPanel = m_panelShowBit[EnumToInt(ePanelShow::Debug)];
    if (bShowDebugPanel)
    {
        m_debugPanel.Show(&bShowDebugPanel);
    }

    bool& bShowAssetInspector = m_panelShowBit[EnumToInt(ePanelShow::AssetInspector)];
    if (bShowAssetInspector)
    {
        m_assetInspectorPanel.Show(&bShowAssetInspector);
    }

    bool& bShowEntityInspector = m_panelShowBit[EnumToInt(ePanelShow::EntityInspector)];
    if (bShowEntityInspector)
    {
        m_entityInspectorPanel.Show(&bShowEntityInspector);
    }

    bool& bShowContentsBrowser = m_panelShowBit[EnumToInt(ePanelShow::ContentsBrowser)];
    if (bShowContentsBrowser)
    {
        m_contentsBrowserPanel.Show(&bShowContentsBrowser);
    }

    bool& bShowConsole = m_panelShowBit[EnumToInt(ePanelShow::Console)];
    if (bShowConsole)
    {
        m_consolePanel.Show(&bShowConsole);
    }

    ShowMessageBox_();
}

void EditorLayer::OnEvent(Event& _eventRef)
{
    m_contentsBrowserPanel.OnEvent(_eventRef);
}

void EditorLayer::SetPanelShow(const ePanelShow _flags, const bool _bEnable)
{
    m_panelShowBit[EnumToInt(_flags)] = _bEnable;
}

void EditorLayer::TogglePanelShow(const ePanelShow _flags)
{
    m_panelShowBit[EnumToInt(_flags)] = !m_panelShowBit[EnumToInt(_flags)];
}

bool EditorLayer::IsShowPanel(const ePanelShow _flags) const
{
    return m_panelShowBit[EnumToInt(_flags)];
}

void EditorLayer::OpenMessageBox(Scope<IMessageBox>&& _pMessageBox)
{
    JAM_ASSERT(_pMessageBox, "Message box pointer is null.");
    m_bShowMessageBox = true;
    m_pMessageBox     = std::move(_pMessageBox);
}

void EditorLayer::ShowMessageBox_()
{
    if (m_pMessageBox)
    {
        const char* title = m_pMessageBox->GetTitle().data();
        if (m_bShowMessageBox)
        {
            ImGui::OpenPopup(title);
            m_bShowMessageBox = false;
        }

        bool bIsOpen = ImGui::IsPopupOpen(title);
        if (ImGui::BeginPopupModal(title, &bIsOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (bIsOpen)
            {
                m_pMessageBox->Show();
            }
            else   // x 버튼을 눌렀을 경우
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}

void EditorLayer::SetEditEntity(const Entity& _entity)
{
    JAM_ASSERT(_entity.IsValid(), "Invalid entity passed to SetEditEntity.");
    m_editEntity = _entity;
}

Entity EditorLayer::GetEditEntity() const
{
    return m_editEntity;
}

}   // namespace jam