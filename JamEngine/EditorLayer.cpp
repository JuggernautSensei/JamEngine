#include "pch.h"

#include "EditorLayer.h"

#include "Application.h"
#include "AssetUtilities.h"
#include "BuiltInEditorResources.h"
#include "Components.h"
#include "ImageUtilities.h"
#include "Input.h"
#include "Renderer.h"
#include "Scene.h"
#include "SceneLayer.h"
#include "WindowsUtilities.h"
#include <gdiplus.h>
#include <shlwapi.h>
#include <shobjidl.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace jam;

// global variables
namespace
{

ULONG_PTR g_gdiplusToken = NULL;

}

// functions
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

NODISCARD const char* GetAssetPayload(const eAssetType _type)
{
    constexpr const char* const k_payloads[EnumCount<eAssetType>()] = {
        "asset_model",
        "asset_texture",
    };
    return k_payloads[EnumToInt(_type)];
}

void UnloadAsset(AssetManager& _assetManagerRef, const eAssetType _type, const fs::path& _path)
{
    switch (_type)
    {
        case eAssetType::Model:
            _assetManagerRef.UnloadModel(_path);
            break;
        case eAssetType::Texture:
            _assetManagerRef.UnloadTexture(_path);
            break;
        default:
            JAM_ASSERT(false, "Unknown asset typeOrNull: {}", EnumToInt(_type));
            break;
    }
}

std::optional<Texture2D> CreateThumbnailFromFilepath(const fs::path& _filePath, const Int32 _width, const Int32 _height)
{
    ComPtr<IShellItem> pItem = nullptr;
    HRESULT            hr    = SHCreateItemFromParsingName(_filePath.c_str(), nullptr, IID_PPV_ARGS(&pItem));
    if (FAILED(hr))
    {
        JAM_ERROR("Failed to create IShellItem from path: {}. Error: {}", _filePath.string(), GetSystemErrorMessage(hr));
        return std::nullopt;
    }

    ComPtr<IShellItemImageFactory> pImageFactory = nullptr;
    hr                                           = pItem.As(&pImageFactory);
    if (FAILED(hr))
    {
        JAM_ERROR("Failed to get IShellItemImageFactory. Error: {}", GetSystemErrorMessage(hr));
        return std::nullopt;
    }

    SIZE    size    = { _width, _height };
    HBITMAP hBitmap = nullptr;
    hr              = pImageFactory->GetImage(size, SIIGBF_RESIZETOFIT, &hBitmap);
    if (FAILED(hr) || hBitmap == nullptr)
    {
        JAM_ERROR("Failed to get thumbnail image. Error: {}", GetSystemErrorMessage(hr));
        return std::nullopt;
    }

    Gdiplus::Bitmap gdiBitmap(hBitmap, nullptr);
    DeleteObject(hBitmap);   // GDI 객체는 Bitmap 생성 후 바로 해제

    if (gdiBitmap.GetLastStatus() != Gdiplus::Ok)
    {
        JAM_ERROR("Failed to create Gdiplus::Bitmap from HBITMAP.");
        return std::nullopt;
    }

    Gdiplus::Rect       rect(0, 0, static_cast<INT>(gdiBitmap.GetWidth()), static_cast<INT>(gdiBitmap.GetHeight()));
    Gdiplus::BitmapData bitmapData;
    if (gdiBitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) != Gdiplus::Ok)
    {
        JAM_ERROR("Failed to lock Gdiplus::Bitmap bits.");
        return std::nullopt;
    }

    Texture2DInitializeData initData;
    initData.pData = bitmapData.Scan0;
    initData.pitch = bitmapData.Stride;

    Texture2D thumbnail;
    thumbnail.Initialize(bitmapData.Width, bitmapData.Height, DXGI_FORMAT_B8G8R8A8_UNORM, eResourceAccess::Immutable, eViewFlags_ShaderResource, 1, 1, false, false, initData);
    if (gdiBitmap.UnlockBits(&bitmapData) != Gdiplus::Ok)
    {
        JAM_ERROR("Failed to unlock Gdiplus::Bitmap bits.");
        return std::nullopt;
    }
    return thumbnail;
}

void DrawAssetItem(AssetManager& _assetManagerRef, const eAssetType _type, const char* _name, const fs::path& _path)
{
    ImGui::Selectable(_name, false, ImGuiSelectableFlags_SpanAllColumns);

    if (ImGui::BeginPopupContextItem())   // 우클릭 시 팝업 메뉴
    {
        if (ImGui::Selectable("open in explorer")) OpenInExplorer(_path);
        if (ImGui::Selectable("unload asset")) UnloadAsset(_assetManagerRef, _type, _path);
        ImGui::EndPopup();
    }

    if (ImGui::BeginDragDropSource())   // 드래그 드롭 지원
    {
        ImGui::SetDragDropPayload(GetAssetPayload(_type), &_path, sizeof(_path));
        ImGui::TextUnformatted(_name);
        ImGui::EndDragDropSource();
    }
}

}   // namespace

namespace jam
{

namespace detail
{

    NODISCARD static std::string_view ConvertPathToStringView(const fs::path& _path)
    {
        // 힙 할당을 피하기 위해 SSO 버퍼를 사용
        // 임시 문자열이기 때문에 imgui 쪽 코드에서만 사용
        static char       ssoBuffer[MAX_PATH];
        std::wstring_view wStr = _path.native();
        const int         size = WideCharToMultiByte(CP_UTF8, 0, wStr.data(), -1, nullptr, 0, nullptr, nullptr);
        JAM_ASSERT(size < MAX_PATH, "Path is too long to fit in the SSO buffer.");
        WideCharToMultiByte(CP_UTF8, 0, wStr.data(), -1, ssoBuffer, size, nullptr, nullptr);
        return ssoBuffer;
    }

}   // namespace detail

EditorLayer::EditorLayer()
{
    m_dispatcher.AddListener<WindowResizeEvent>(JAM_ADD_LISTENER_MEMBER_FUNCTION(EditorLayer::OnWindowResizeEvent_));

    // intialize GDI+
    if (g_gdiplusToken == NULL)
    {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        if (Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok)
        {
            JAM_CRASH("Failed to initialize GDI+.");
        }
    }

    // load default file icon
    m_defaultFileIcon.LoadFromMemory(k_defaultFileIconDDS, k_defaultFileIconDDSSize, eImageFormat::DDS);
}

EditorLayer::~EditorLayer()
{
    // clean up GDI+
    Gdiplus::GdiplusShutdown(g_gdiplusToken);
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

    ShowMessageBox_();
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
                    ImGui::EndChild();
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
                    ImGui::EndChild();
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

            // entity list
            ImGui::Separator();
            if (ImGui::BeginChild("entity list", ImVec2 { 0.f, 300.f }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY | ImGuiChildFlags_AutoResizeX))
            {
                static ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(view.size()));
                auto it = view.begin();
                while (clipper.Step())
                {
                    for (Int32 i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                    {
                        Entity e = pActiveScene->GetEntity(*(it + i));
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
                            if (ImGui::BeginPopupContextWindow())   // 우클릭 시 팝업 메뉴
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
            }
            ImGui::EndChild();

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
                DrawEditorParameter drawEditorParam = { this, &m_selectedEntity };   // 컴포넌트 편집에 필요한 데이터
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
                            void* componentValue = meta.getComponentOrNullCallback(m_selectedEntity);
                            meta.drawComponentEditorCallback(drawEditorParam, componentValue);
                        }
                    }

                    // 헤더에서 오른쪽 클릭 시 팝업이 뜲
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
    bool isOpen = m_flags & eEditorPanelShowFlags_AssetInspector;
    if (ImGui::Begin("asset inspector", &isOpen))
    {
        Application& app          = GetApplication();
        SceneLayer*  pSceneLayer  = app.GetSceneLayer();
        Scene*       pActiveScene = pSceneLayer->GetActiveScene();
        if (pActiveScene)
        {
            AssetManager& assetManager = pActiveScene->GetAssetManager();

            // 에셋 타입별로 정리
            if (ImGui::BeginTabBar("asset inspector"))
            {
                for (eAssetType type: EnumValues<eAssetType>())
                {
                    if (ImGui::BeginTabItem(EnumToString(type).data()))
                    {
                        const AssetManager::Container& container = assetManager.GetContainer(type);

                        // 디버깅 용 렌더링 아이템 체크
                        UInt32 renderedItemCount = 0;

                        // 검색 기능 지원
                        static ImGuiTextFilter filter;
                        filter.Draw("search");

                        // 경계 박스 생성
                        if (ImGui::BeginChild("asset list", ImVec2 { 0.f, 300.f }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY | ImGuiChildFlags_AutoResizeX))
                        {
                            if (filter.IsActive())   // 필터링이 작동할 경우
                            {
                                for (const fs::path& path: container | std::views::keys)
                                {
                                    std::string_view name = detail::ConvertPathToStringView(path);
                                    if (filter.PassFilter(name.data()))   // 필터링 조건에 맞는 경우에만 렌더링
                                    {
                                        DrawAssetItem(assetManager, type, name.data(), path);
                                        ++renderedItemCount;
                                    }
                                }
                            }
                            else   // 필터링이 작동하지 않을 경우. 리스트 클리핑 후 렌더링
                            {
                                static ImGuiListClipper clipper;
                                clipper.Begin(static_cast<int>(container.size()));

                                auto it = container.begin();
                                while (clipper.Step())
                                {
                                    std::advance(it, clipper.DisplayStart);
                                    for (Int32 i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i, std::advance(it, 1))
                                    {
                                        const fs::path&  path = it->first;
                                        std::string_view name = detail::ConvertPathToStringView(path);
                                        DrawAssetItem(assetManager, type, name.data(), path);
                                        ++renderedItemCount;
                                    }
                                }
                            }
                        }
                        ImGui::EndChild();

                        // 정보
                        ImGui::Text("asset type: %s   asset count: %zu", EnumToString(type).data(), container.size());
                        ImGui::TextDisabled("[debug] rendered item count: %u", renderedItemCount);
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }
        }
        else
        {
            DrawCenterAlignText("no active scene", true);
        }
    }
    SetShowFlags(eEditorPanelShowFlags_AssetInspector, isOpen);
    ImGui::End();
}

void EditorLayer::ShowContentsBrowser_()
{
    bool isOpen = m_flags & eEditorPanelShowFlags_ContentsBrowser;
    if (ImGui::Begin("contents browser", &isOpen))
    {
        // 컨텐츠 브라우저 - 디렉토리 트리
        ImGui::BeginGroup();
        {
            // 재귀 함수 정의
            auto DrawDirectoryTreeRecursive = [this](auto _func, const Directory& _dir)
            {
                for (const Directory& dir: _dir.subDirectories)
                {
                    if (ImGui::TreeNodeEx(dir.name.c_str(), ImGuiTreeNodeFlags_OpenOnDoubleClick))
                    {
                        _func(_func, dir);
                        ImGui::TreePop();
                    }

                    if (ImGui::IsItemClicked())
                    {
                        m_focusDirectoryPath = dir.path;
                    }
                }
            };

            // 루트 디렉토리 렌더링
            ImGui::TextUnformatted("directory tree");
            if (ImGui::BeginChild("directory tree", ImVec2 { 200.f, 0.f }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX))
            {
                DrawDirectoryTreeRecursive(DrawDirectoryTreeRecursive, m_rootDirectory);
            }
        }
        ImGui::EndGroup();

        // 컨텐츠 브라우저 - 컨텐츠 뷰
        ImGui::SameLine();
        ImGui::BeginGroup();
        {
            // 1행
            {
                // 액션 버튼
                ImGui::Button("menu");

                // path
                ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
                std::string_view path = detail::ConvertPathToStringView(m_focusDirectoryPath);
                ImGui::Text("path: %s", path.data());
            }

            // 2행
            static ImGuiTextFilter filter;
            {
                // 필터
                ImGui::TextUnformatted("filter:");
                ImGui::SameLine();
                filter.Draw("##filter", ImGui::GetContentRegionAvail().x);
            }

            if (ImGui::BeginChild("contents view", ImGui::GetContentRegionAvail()))
            {
            }
            ImGui::EndChild();
        }
        ImGui::EndGroup();
    }
    SetShowFlags(eEditorPanelShowFlags_ContentsBrowser, isOpen);
    ImGui::End();
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

void EditorLayer::UpdateFocusDirectoryContents()
{
    m_focusDirectoryContents.clear();
    for (const fs::directory_entry& entry: fs::directory_iterator(m_focusDirectoryPath))
    {
        if (entry.is_directory() || entry.is_regular_file())
        {
            const fs::path& path = entry.path();

            // 컨텐츠 데이터 초기화
            Content content;
            content.path            = path;
            content.name            = path.filename().string();
            content.assetTypeOrNull = GetAssetTypeFromPath(path);

            // 썸네일
            std::optional<Texture2D> thumbnailOrNull = CreateThumbnailFromFilepath(path, 64, 64);
            if (thumbnailOrNull)
            {
                content.thumbnail = std::move(*thumbnailOrNull);
            }
            else   // 기본 아이콘 사용
            {
                content.thumbnail = m_defaultFileIcon;
            }

            m_focusDirectoryContents.emplace_back(std::move(content));
        }
    }
}

void EditorLayer::UpdateRootDirectory()
{
    auto UpdateDirectoryRecursive = [](auto _func, const fs::path& _path) -> Directory
    {
        Directory dir;
        dir.path = _path;
        for (const fs::directory_entry& entry: fs::directory_iterator(_path))
        {
            if (entry.is_directory())
            {
                const fs::path& path = entry.path();
                dir.subDirectories.push_back(_func(_func, path));
                dir.name = path.filename().string();
            }
        }
        return dir;
    };

    Application& app = GetApplication();
    m_rootDirectory  = UpdateDirectoryRecursive(UpdateDirectoryRecursive, app.GetContentsDirectory());
}

void EditorLayer::SetFocusDirectory(const fs::path& _path)
{
    m_focusDirectoryPath = _path;
    UpdateFocusDirectoryContents();
}

void EditorLayer::SetFocusDirectoryToParentDirectory()
{
    Application& app = GetApplication();
    if (m_focusDirectoryPath != app.GetContentsDirectory())
    {
        m_focusDirectoryPath = m_focusDirectoryPath.parent_path();
        UpdateFocusDirectoryContents();
    }
}

void EditorLayer::DrawContentButton(const Content& _content, const ImVec2& _buttonSize)
{
    ImGui::PushID(_content.name.c_str());
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));   // 투명 버튼

    // todo 직접 그리기..

    ImGui::PopStyleColor();
    ImGui::PopID();
}

void EditorLayer::OpenMessageBox(const std::string_view _title, const std::function<void()>& _drawCallback)
{
    m_bShowMessageBox        = true;
    m_messageBoxDrawCallback = _drawCallback;
    m_messageBoxTitle        = _title.data();
}

}   // namespace jam