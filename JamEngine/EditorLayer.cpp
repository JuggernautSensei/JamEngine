#include "pch.h"

#include "EditorLayer.h"

#include "Application.h"
#include "Components.h"
#include "Input.h"
#include "Renderer.h"
#include "Scene.h"
#include "SceneLayer.h"

namespace
{

NODISCARD ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2 { lhs.x + rhs.x, lhs.y + rhs.y }; }
NODISCARD ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2 { lhs.x - rhs.x, lhs.y - rhs.y }; }
NODISCARD ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2 { lhs.x * rhs.x, lhs.y * rhs.y }; }
NODISCARD ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2 { lhs.x / rhs.x, lhs.y / rhs.y }; }
NODISCARD ImVec2 operator*(const ImVec2& lhs, const float rhs) { return ImVec2 { lhs.x * rhs, lhs.y * rhs }; }
NODISCARD ImVec2 operator/(const ImVec2& lhs, const float rhs) { return ImVec2 { lhs.x / rhs, lhs.y / rhs }; }
NODISCARD ImVec2 operator*(const float lhs, const ImVec2& rhs) { return ImVec2 { lhs * rhs.x, lhs * rhs.y }; }
NODISCARD ImVec2 operator/(const float lhs, const ImVec2& rhs) { return ImVec2 { lhs / rhs.x, lhs / rhs.y }; }
NODISCARD ImVec2 operator-(const ImVec2& v) { return ImVec2 { -v.x, -v.y }; }
NODISCARD ImVec2 operator+(const ImVec2& v) { return ImVec2 { +v.x, +v.y }; }
NODISCARD bool   operator==(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
NODISCARD bool   operator!=(const ImVec2& lhs, const ImVec2& rhs) { return !(lhs == rhs); }
using namespace jam;

void DrawCenterAlignText(const char* _text, const bool _bDisable = false)
{
    ImVec2 size = ImGui::CalcTextSize(_text);
    ImGui::SetCursorPos((ImGui::GetContentRegionAvail() - size) * 0.5f);

    if (_bDisable)
    {
        ImGui::TextDisabled(_text);
    }
    else
    {
        ImGui::TextUnformatted(_text);
    }
}

NODISCARD ImVec2 CalcPrettyButtonSize(const Int32 _count)
{
    JAM_ASSERT(_count > 0, "Count must be greater than zero.");
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2      avail = ImGui::GetContentRegionAvail();
    avail.x           = (avail.x - static_cast<float>(_count - 1) * style.ItemSpacing.x) / static_cast<float>(_count);
    avail.x           = std::floorf(avail.x);
    return ImVec2 { avail.x, 0.f };   // height is auto-calculated
}

}   // namespace

namespace jam
{

EditorLayer::EditorLayer()
{
    m_dispatcher.AddListener<WindowResizeEvent>(JAM_ADD_LISTENER_MEMBER_FUNCTION(EditorLayer::OnWindowResizeEvent_));
}

void EditorLayer::OnUpdate(float _deltaTime)
{
}

void EditorLayer::OnRender()
{
    // dock space
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);

    // main menu bar
    ShowMainMenuBar_();

    if (m_flags & eEditorPanelShowFlags_Viewport)
    {
        ShowViewport_();
    }

    if (m_flags & eEditorPanelShowFlags_DebugPanel)
    {
        ShowDebugPanel_();
    }

    if (m_flags & eEditorPanelShowFlags_SceneHierarchy)
    {
        ShowSceneHierarchy_();
    }

    if (m_flags & eEditorPanelShowFlags_AssetInspector)
    {
        ShowAssetInspector_();
    }

    if (m_flags & eEditorPanelShowFlags_EntityInspector)
    {
        ShowEntityInspector_();
    }

    if (m_flags & eEditorPanelShowFlags_ContentsBrowser)
    {
        ShowContentsBrowser_();
    }

    if (m_flags & eEditorPanelShowFlags_Console)
    {
        ShowConsole_();
    }
}

void EditorLayer::ShowMainMenuBar_()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("view"))
        {
            ImGui::SetNextItemShortcut(ImGuiMod_Shift | ImGuiKey_F1);
            if (ImGui::MenuItem("debug panel", "Shift + F1", m_flags & eEditorPanelShowFlags_DebugPanel))
            {
                ToggleShowFlags(eEditorPanelShowFlags_DebugPanel);
            }

            ImGui::SetNextItemShortcut(ImGuiMod_Shift | ImGuiKey_F2);
            if (ImGui::MenuItem("viewport", "Shift + F2", m_flags & eEditorPanelShowFlags_Viewport))
            {
                ToggleShowFlags(eEditorPanelShowFlags_Viewport);
            }

            ImGui::SetNextItemShortcut(ImGuiMod_Shift | ImGuiKey_F3);
            if (ImGui::MenuItem("scene hierarchy", "Shift + F3", m_flags & eEditorPanelShowFlags_SceneHierarchy))
            {
                ToggleShowFlags(eEditorPanelShowFlags_SceneHierarchy);
            }

            ImGui::SetNextItemShortcut(ImGuiMod_Shift | ImGuiKey_F4);
            if (ImGui::MenuItem("asset inspector", "Shift + F4", m_flags & eEditorPanelShowFlags_AssetInspector))
            {
                ToggleShowFlags(eEditorPanelShowFlags_AssetInspector);
            }

            ImGui::SetNextItemShortcut(ImGuiMod_Shift | ImGuiKey_F5);
            if (ImGui::MenuItem("inspector", "Shift + F5", m_flags & eEditorPanelShowFlags_EntityInspector))
            {
                ToggleShowFlags(eEditorPanelShowFlags_EntityInspector);
            }

            ImGui::SetNextItemShortcut(ImGuiMod_Shift | ImGuiKey_F6);
            if (ImGui::MenuItem("asset browser", "Shift + F6", m_flags & eEditorPanelShowFlags_ContentsBrowser))
            {
                ToggleShowFlags(eEditorPanelShowFlags_ContentsBrowser);
            }

            ImGui::SetNextItemShortcut(ImGuiMod_Shift | ImGuiKey_F7);
            if (ImGui::MenuItem("console", "Shift + F7", m_flags & eEditorPanelShowFlags_Console))
            {
                ToggleShowFlags(eEditorPanelShowFlags_Console);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void EditorLayer::OnEvent(Event& _event)
{
    m_dispatcher.Dispatch(_event);
}

void EditorLayer::SetShowFlags(const eEditorPanelShowFlags _flags, const bool _bEnable)
{
    if (_bEnable)
    {
        m_flags |= _flags;
    }
    else
    {
        m_flags &= ~_flags;
    }
}

void EditorLayer::ToggleShowFlags(const eEditorPanelShowFlags _flags)
{
    m_flags ^= _flags;
}

void EditorLayer::ShowDebugPanel_()
{
    bool isOpen = m_flags & eEditorPanelShowFlags_DebugPanel;
    if (ImGui::Begin("debug panel", &isOpen))
    {
        if (ImGui::BeginTabBar("debug tab bar"))
        {
            if (ImGui::BeginTabItem("frame performence"))
            {
                const TickTimer& timer = GetApplication().GetTimer();
                ImGui::Text("frame rate: %.4f hz", 1.f / timer.GetDeltaSec());
                ImGui::Text("delta milli sec: %.4f ms", timer.GetDeltaSec() * 1e-3);
                ImGui::EndTabBar();
            }

            if (ImGui::BeginTabBar("input test"))
            {
                auto [mx, my] = Input::GetMousePosition();
                ImGui::Text("mouse position: (%u, %u)", mx, my);

                auto [dx, dy] = Input::GetMouseDeltaPosition();
                ImGui::Text("mouse delta position: (%u, %u)", dx, dy);

                Int32 mouseWheelDelta = Input::GetMouseWheelDelta();
                ImGui::Text("mouse wheel delta: %d", mouseWheelDelta);

                if (ImGui::TreeNode("mouse actions"))
                {
                    if (ImGui::BeginChild("mouse actions view", ImVec2 { 0.f, 300.f }, ImGuiChildFlags_Border))
                    {
                        // mouse button state
                        for (eMouse mouse: EnumValues<eMouse>())
                        {
                            if (mouse == eMouse::None)
                                continue;

                            bool isDown     = Input::IsMouseDown(mouse);
                            bool isPressed  = Input::IsMousePressed(mouse);
                            bool isHold     = Input::IsMouseHold(mouse);
                            bool isUp       = Input::IsMouseUp(mouse);
                            bool isReleased = Input::IsMouseReleased(mouse);
                            bool isAway     = Input::IsMouseAway(mouse);

                            ImGui::Text("Mouse %s: Down=%s, Pressed=%s, Hold=%s, Up=%s, Released=%s, Away=%s",
                                        magic_enum::enum_name(mouse).data(),
                                        isDown ? "true" : "false",
                                        isPressed ? "true" : "false",
                                        isHold ? "true" : "false",
                                        isUp ? "true" : "false",
                                        isReleased ? "true" : "false",
                                        isAway ? "true" : "false");
                        }
                    }

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("keyboard actions"))
                {
                    if (ImGui::BeginChild("keyboard actions view", ImVec2 { 0.f, 300.f }, ImGuiChildFlags_Border))
                    {
                        // keyboard key state
                        for (eKey key: EnumValues<eKey>())
                        {
                            if (key == eKey::None)
                                continue;

                            bool isDown     = Input::IsKeyDown(key);
                            bool isPressed  = Input::IsKeyPressed(key);
                            bool isHold     = Input::IsKeyHold(key);
                            bool isUp       = Input::IsKeyUp(key);
                            bool isReleased = Input::IsKeyReleased(key);
                            bool isAway     = Input::IsKeyAway(key);
                            ImGui::Text("Key %s: Down=%s, Pressed=%s, Hold=%s, Up=%s, Released=%s, Away=%s",
                                        magic_enum::enum_name(key).data(),
                                        isDown ? "true" : "false",
                                        isPressed ? "true" : "false",
                                        isHold ? "true" : "false",
                                        isUp ? "true" : "false",
                                        isReleased ? "true" : "false",
                                        isAway ? "true" : "false");
                        }
                    }
                    ImGui::TreePop();
                }

                ImGui::End();
            }

            ImGui::EndTabBar();
        }
    }

    SetShowFlags(eEditorPanelShowFlags_DebugPanel, isOpen);
    ImGui::End();
}

void EditorLayer::ShowViewport_()
{
    bool isOpen = m_flags & eEditorPanelShowFlags_Viewport;

    // 현재 백버퍼를 캡처
    if (isOpen)
    {
        // 성능 체크 필요
        m_viewportTexture.CopyFrom(Renderer::GetBackBufferTexture());
    }

    if (ImGui::Begin("viewport", &isOpen))
    {
        auto [width, height]            = m_viewportTexture.GetSize();
        float  aspectRatio              = static_cast<float>(width) / static_cast<float>(height);
        ImVec2 viewportAvailRegion      = ImGui::GetContentRegionAvail();
        float  viewportAvailAspectRatio = viewportAvailRegion.x / viewportAvailRegion.y;

        ImVec2 imageSize;
        if (viewportAvailAspectRatio > aspectRatio)
        {
            // 기준은 높이
            imageSize.y = viewportAvailRegion.y;
            imageSize.x = imageSize.y * aspectRatio;
        }
        else if (viewportAvailAspectRatio < aspectRatio)
        {
            // 기준은 너비
            imageSize.x = viewportAvailRegion.x;
            imageSize.y = imageSize.x / aspectRatio;
        }

        // 중앙에 맞춰 이쁘게 배치
        ImVec2      imagePos = ImGui::GetCursorScreenPos() + ImVec2 { (viewportAvailRegion.x - imageSize.x) * 0.5f, (viewportAvailRegion.y - imageSize.y) * 0.5f };
        ImTextureID texID    = reinterpret_cast<ImTextureID>(m_viewportTexture.GetSRV());
        ImGui::SetCursorScreenPos(imagePos);
        ImGui::Image(texID, imageSize);
    }

    SetShowFlags(eEditorPanelShowFlags_Viewport, isOpen);
    ImGui::End();
}

void EditorLayer::ShowSceneHierarchy_()
{
    bool isOpen = m_flags & eEditorPanelShowFlags_SceneHierarchy;
    if (ImGui::Begin("scene hierarchy", &isOpen))
    {
        Application& app          = GetApplication();
        SceneLayer*  pSceneLayer  = app.GetSceneLayer();
        Scene*       pActiveScene = pSceneLayer->GetActiveScene();

        // scene selector: 등록된 씬을 선택 할 수 있는 콤보 박스
        ImGui::TextUnformatted("scene selector");
        if (ImGui::BeginCombo("##scene selector", pActiveScene ? pActiveScene->GetName().data() : "none", ImGuiComboFlags_None))
        {
            for (const auto& [name, scene]: pSceneLayer->GetContainer())
            {
                if (ImGui::Selectable(name.data(), scene.get() == pActiveScene))
                {
                    pSceneLayer->ChangeScene(name);
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        // scene
        if (pActiveScene)
        {
            // simple scene info
            auto view = pActiveScene->CreateView<entt::entity>();
            ImGui::Text("scene name: %s", pActiveScene->GetName().data());
            ImGui::Text("entity count: %zu", view.size());

            ImGui::Separator();

            // entity list
            if (ImGui::BeginChild("entity list", ImVec2 { 0.f, 300.f }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY | ImGuiChildFlags_AutoResizeX))
            {
                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(view.size()));
                auto iterator = view.begin();
                while (clipper.Step())
                {
                    for (Int32 i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                    {
                        Entity e = pActiveScene->GetEntity(*(iterator + i));
                        JAM_ASSERT(e.IsValid(), "Invalid entity in scene hierarchy");

                        // entity selection label
                        static std::string s_buf;
                        if (e.HasComponent<TagComponent>())
                        {
                            s_buf = e.GetComponent<TagComponent>().name;
                        }
                        else
                        {
                            s_buf = "entity " + std::to_string(e.GetID());
                        }

                        // entity selection
                        ImGui::PushID(static_cast<int>(e.GetID()));
                        if (ImGui::Selectable(s_buf.c_str(), m_selectedEntity == e, ImGuiSelectableFlags_SpanAllColumns))
                        {
                            m_selectedEntity = e;
                        }

                        // 더블 클릭하면 inspector 오픈
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            SetShowFlags(eEditorPanelShowFlags_EntityInspector, true);
                        }

                        // 편의성 메뉴
                        if (ImGui::IsItemHovered())
                        {
                            // 우클릭 시 팝업 메뉴
                            if (ImGui::BeginPopupContextItem())
                            {
                                if (ImGui::Selectable("select entity"))
                                {
                                    m_selectedEntity = e;
                                }
                                if (ImGui::Selectable("delete entity"))
                                {
                                    pActiveScene->DestroyEntity(e);

                                    // 선택된 엔티티가 삭제된 경우, 선택 해제
                                    if (m_selectedEntity == e)
                                    {
                                        m_selectedEntity = Entity::s_null;
                                    }
                                }
                                if (ImGui::Selectable("clone entity"))
                                {
                                    m_selectedEntity = pActiveScene->CloneEntity(e);
                                }
                                ImGui::EndPopup();
                            }
                        }
                        else
                        {
                            // 우클릭 시 팝업 메뉴
                            if (ImGui::BeginPopupContextWindow())
                            {
                                if (ImGui::Selectable("create entity"))
                                {
                                    m_selectedEntity = pActiveScene->CreateEntity();
                                }
                            }
                        }

                        ImGui::SameLine();
                        ImGui::PopID();
                    }
                }

                ImGui::EndChild();
                ImGui::TextDisabled("[debug] rendered item: %d/%d", clipper.DisplayEnd - clipper.DisplayStart, view.size());

                // entity creation button
                ImGui::Spacing();
                ImVec2 buttonSize = CalcPrettyButtonSize(3);

                if (ImGui::Button("create entity", buttonSize))
                {
                    m_selectedEntity = pActiveScene->CreateEntity();
                }

                // 해당 버튼은 선택된 엔티티가 있을 때만 활성화
                ImGui::BeginDisabled(!m_selectedEntity.IsValid());
                {
                    ImGui::SameLine();
                    if (ImGui::Button("delete entity", buttonSize))
                    {
                        pActiveScene->DestroyEntity(m_selectedEntity);
                        m_selectedEntity = Entity::s_null;
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("clone entity", buttonSize))
                    {
                        m_selectedEntity = pActiveScene->CloneEntity(m_selectedEntity);
                    }
                }
                ImGui::EndDisabled();
            }
        }
        else
        {
            DrawCenterAlignText("no active scene", true);
        }
    }

    SetShowFlags(eEditorPanelShowFlags_SceneHierarchy, isOpen);
    ImGui::End();
}

void EditorLayer::ShowEntityInspector_()
{
    Application& app          = GetApplication();
    SceneLayer*  pSceneLayer  = app.GetSceneLayer();
    Scene*       pActiveScene = pSceneLayer->GetActiveScene();
    bool         isOpen       = m_flags & eEditorPanelShowFlags_EntityInspector;

    if (ImGui::Begin("entity inspector", &isOpen))
    {
        if (pActiveScene)
        {
            auto container = ComponentMetaManager::GetMetaContainer();

            if (m_selectedEntity.IsValid())
            {
                // 엔티티 아이디
                ImGui::Text("entity id: %u", m_selectedEntity.GetID());

                ImGui::Spacing();
                ImVec2 buttonSize = CalcPrettyButtonSize(2);

                // 컴포넌트 생성 버튼
                ImGui::Button("create component", buttonSize);
                if (ImGui::BeginPopupContextItem())
                {
                    // 내가 가지지 않은 컴포넌트 목록을 보여줌
                    for (const ComponentMeta& meta: container | std::views::values)
                    {
                        // 이미 가지고 있다면 스킵
                        if (meta.hasComponentCallback(m_selectedEntity))
                        {
                            continue;
                        }

                        // 컴포넌트를 선택하여 생성
                        if (ImGui::Selectable(meta.componentName.data()))
                        {
                            meta.createComponentCallback(m_selectedEntity);
                        }
                    }

                    ImGui::EndPopup();
                }

                // 컴포넌트 삭제 팝업
                ImGui::SameLine();
                ImGui::Button("delete component", buttonSize);
                if (ImGui::BeginPopupContextItem())
                {
                    for (const ComponentMeta& meta: container | std::views::values)
                    {
                        // 가지고 있는 컴포넌트 목록을 보여줌
                        if (meta.hasComponentCallback(m_selectedEntity))
                        {
                            // 클릭 시 삭제
                            if (ImGui::Selectable(meta.componentName.data()))
                            {
                                meta.removeComponentCallback(m_selectedEntity);
                            }
                        }
                    }
                    ImGui::EndPopup();
                }

                // 컴포넌트 편집
                for (const ComponentMeta& meta: container | std::views::values)
                {
                    // 선택된 엔티티가 해당 컴포넌트를 가지고 있지 않으면 스킵
                    if (!meta.hasComponentCallback(m_selectedEntity))
                    {
                        continue;   // 스킵
                    }

                    // 컴포넌트 편집 패널
                    if (ImGui::CollapsingHeader(meta.componentName.data(), ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        if (meta.drawComponentEditorCallback)
                        {
                            DrawComponentEditorParameter param = {
                                .pEditorLayer = this,
                                .pOwnerEntity = &m_selectedEntity,
                            };
                            void* componentValue = meta.getComponentOrNullCallback(m_selectedEntity);
                            meta.drawComponentEditorCallback(param, componentValue);
                        }
                    }

                    // 오른쪽 클릭 시 팝업이 뜲
                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::Selectable("remove component"))
                        {
                            meta.removeComponentCallback(m_selectedEntity);
                        }
                        ImGui::EndPopup();
                    }
                }
            }
            else
            {
                DrawCenterAlignText("no selected entity");
            }
        }
        else
        {
            DrawCenterAlignText("no active scene", true);
        }
    }

    SetShowFlags(eEditorPanelShowFlags_EntityInspector, isOpen);
    ImGui::End();
}

void EditorLayer::ShowAssetInspector_()
{
}

void EditorLayer::ShowContentsBrowser_()
{
}

void EditorLayer::ShowConsole_()
{
}

void EditorLayer::CreateScreenDependentResources_(MAYBE_UNUSED const Int32 _width, MAYBE_UNUSED const Int32 _height)
{
    Texture2D            backBufferTex = Renderer::GetBackBufferTexture();
    D3D11_TEXTURE2D_DESC desc;
    backBufferTex.Get()->GetDesc(&desc);

    // viewport texture initialize
    m_viewportTexture.Initialize(desc.Width, desc.Height, desc.Format, eResourceAccess::GPUWriteable, eViewFlags_ShaderResource, desc.ArraySize, desc.SampleDesc.Count);
    m_viewportTexture.AttachSRV();
}

void EditorLayer::OnWindowResizeEvent_(const WindowResizeEvent& e)
{
    Int32 width  = e.GetWidth();
    Int32 height = e.GetHeight();
    CreateScreenDependentResources_(width, height);
}

void EditorLayer::ShowMessageBox_()
{
    if (m_bShowMessageBox)
    {
        ImGui::OpenPopup(m_messageBoxTitle.c_str());
        m_bShowMessageBox = false;
    }

    bool bIsOpen = ImGui::IsPopupOpen(m_messageBoxTitle.c_str());
    if (ImGui::BeginPopupModal(m_messageBoxTitle.c_str(), &bIsOpen, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (bIsOpen)
        {
            JAM_ASSERT(m_messageBoxDrawCallback, "Message box draw callback is not set.");
            m_messageBoxDrawCallback();
        }
        else
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void EditorLayer::OpenMessageBox(const std::string_view _title, const std::function<void()>& _drawCallback)
{
    m_bShowMessageBox        = true;
    m_messageBoxDrawCallback = _drawCallback;
    m_messageBoxTitle        = _title.data();
}

}   // namespace jam