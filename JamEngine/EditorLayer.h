#pragma once
#include "AssetInspectorPanel.h"
#include "ConsolePanel.h"
#include "ContentsBrowserPanel.h"
#include "DebugPanel.h"
#include "Entity.h"
#include "EntityInspectorPanel.h"
#include "EnumUtilities.h"
#include "ILayer.h"
#include "MainMenuBarPanel.h"
#include "SceneHierarchyPanel.h"
#include "ViewportPanel.h"

namespace jam
{

class BackBufferCleanupEvent;
class WindowResizeEvent;
class IMessageBox;

enum class ePanelShow : Int32
{
    Debug,
    Viewport,
    SceneHierarchy,
    AssetInspector,
    EntityInspector,
    ContentsBrowser,
    Console,
};

class EditorLayer : public ILayer
{
public:
    EditorLayer();
    ~EditorLayer() override;

    EditorLayer(const EditorLayer&)                = delete;
    EditorLayer& operator=(const EditorLayer&)     = delete;
    EditorLayer(EditorLayer&&) noexcept            = default;
    EditorLayer& operator=(EditorLayer&&) noexcept = default;

    void OnUpdate(float _deltaTime) override;
    void OnRender() override;
    void OnEvent(Event& _eventRef) override;

    void           SetPanelShow(ePanelShow _flags, bool _bEnable);
    void           TogglePanelShow(ePanelShow _flags);
    NODISCARD bool IsShowPanel(ePanelShow _flags) const;

    // use this to show a message box -> may be very useful!
    void OpenMessageBox(Scope<IMessageBox>&& _pMessageBox);

    void             SetEditEntity(const Entity& _entity);
    NODISCARD Entity GetEditEntity() const;

    NODISCARD UInt32 GetHash() const override { return HashOf<EditorLayer>(); }
    NODISCARD std::string_view GetName() const override { return NameOf<EditorLayer>(); }

private:
    // message box
    void ShowMessageBox_();

private:
    bool m_panelShowBit[EnumCount<ePanelShow>()];

    // panels
    ContentsBrowserPanel m_contentsBrowserPanel;
    EntityInspectorPanel m_entityInspectorPanel;
    AssetInspectorPanel  m_assetInspectorPanel;
    SceneHierarchyPanel  m_sceneHierarchyPanel;
    MainMenuBarPanel     m_mainMenuBarPanel;
    ViewportPanel        m_viewportPanel;
    ConsolePanel         m_consolePanel;
    DebugPanel           m_debugPanel;

    // shared data
    Entity m_editEntity = Entity::s_null;   // 현재 편집중인 엔티티

    // message box
    Scope<IMessageBox> m_pMessageBox     = nullptr;
    bool               m_bShowMessageBox = false;
};

}   // namespace jam
