#include "pch.h"

#include "SceneHierarchyPanel.h"

#include "Application.h"
#include "Components.h"
#include "Config.h"
#include "EditorLayer.h"
#include "EditorUtilities.h"
#include "Entity.h"
#include "Scene.h"
#include "SceneLayer.h"
#include "WindowsUtilities.h"

namespace
{

using namespace jam;
constexpr FileDialogFilter kSceneFileFilters[] = {
    { "jam scene file", kJamSceneExtension }
};

}   // namespace

namespace jam
{

SceneHierarchyPanel::SceneHierarchyPanel(EditorLayer* _pEditorLayer)
    : m_pEditorLayer(_pEditorLayer)
{
    JAM_ASSERT(_pEditorLayer, "EditorLayer pointer must not be null.");
}

void SceneHierarchyPanel::Show(bool* _pOpenOrNull) const
{
    if (ImGui::Begin("scene hierarchy", _pOpenOrNull))
    {
        SceneLayer* pSceneLayer     = Application::GetInstance().GetSceneLayer();
        Scene*      pActiveScene    = pSceneLayer->GetActiveScene();
        bool        bHasActiveScene = pSceneLayer->HasActiveScene();

#pragma region Scene Selector UI
        // 콤보 박스를 이용하여 씬을 선택할 수 있음
        {
            const char* previewName = "no active scene";   // 콤보박스의 프리뷰 문자열
            if (bHasActiveScene)                           // 만약 활성화된 씬이 있다면
            {
                std::string_view sceneName = pActiveScene->GetName();
                previewName                = sceneName.data();
            }

            // 라벨 렌더링
            ImGui::TextUnformatted("scene selector");

            // 콤보 박스 렌더링
            editor::SetNextItemWidthToAvailableWidth();
            if (ImGui::BeginCombo("##scene selector", previewName))
            {
                const SceneLayer::Container& container = pSceneLayer->GetContainer();   // 씬 컨테이너를 가져와서
                for (auto&& [name, scene]: container)                                   // 씬 이름과 씬 객체를 순회하며 버튼 렌더링
                {
                    bool bIsActiveScene = scene.get() == pActiveScene;   // 현재 활성화된 씬인지 여부
                    if (ImGui::Selectable(name.data(), bIsActiveScene))
                    {
                        pSceneLayer->ChangeScene(name);   // 씬 변경
                    }
                }
                ImGui::EndCombo();
            }
        }
#pragma endregion

#pragma region Scene Management UI
        // 씬을 저장할 수 있는 버튼 등을 생성
        {
            ImGui::BeginDisabled(!bHasActiveScene);   // 활성화된 씬이 없으면 버튼 비활성화
            ImVec2 buttonSize = editor::CalcFittedButtonSize(2);
            // 저장 버튼
            {
                // 씬 저장 버튼
                ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_S);   // Ctrl + S 단축키 설정
                if (ImGui::Button("save", buttonSize))
                {
                    m_pEditorLayer->SaveScene(pActiveScene);   // 현재 활성화된 씬을 저장
                }
                // 우클릭시 다른 이름으로 저장 가능
                if (ImGui::BeginPopupContextItem())
                {
                    // 파일로 씬 저장
                    if (ImGui::MenuItem("save", "ctrl + s"))
                    {
                        m_pEditorLayer->SaveScene(pActiveScene);   // 씬 저장
                    }
                    // 다른 이름으로 저장
                    if (ImGui::MenuItem("save as..."))
                    {
                        auto [path, bResult] = SaveFileDialog(kSceneFileFilters);   // 다이얼 로그로 저장할 경로 선택
                        if (bResult)
                        {
                            m_pEditorLayer->SaveScene(pActiveScene, path);   // 씬 저장
                        }
                    }
                    ImGui::EndPopup();
                }
            }
            // 로드 버튼
            {
                // 씬 로드 버튼
                ImGui::SameLine();                                        // 버튼을 옆에 배치
                ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_O);   // Ctrl + O 단축키 설정
                if (ImGui::Button("load", buttonSize))                    // 씬 로드 버튼
                {
                    m_pEditorLayer->LoadScene(pActiveScene);   // 현재 활성화된 씬을 로드
                }
                // 우클릭시 다른 씬 파일 로드 가능
                if (ImGui::BeginPopupContextItem())
                {
                    // 현재 씬 로드
                    if (ImGui::MenuItem("load", "ctrl + o"))   // Ctrl + O 단축키로 씬 로드
                    {
                        m_pEditorLayer->LoadScene(pActiveScene);   // 씬 로드
                    }
                    // 파일에서 씬 로드
                    if (ImGui::MenuItem("load from file..."))   // 다른 씬 파일 로드
                    {
                        auto [path, bResult] = OpenFileDialog(kSceneFileFilters);   // 다이얼 로그로 씬 파일 선택
                        if (bResult)
                        {
                            m_pEditorLayer->LoadScene(pActiveScene, path);   // 씬 로드
                        }
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::EndDisabled();   // 활성화된 씬이 없으면 버튼 비활성화
        }
#pragma endregion

#pragma region Scene & Entity Info UI
        {
            if (bHasActiveScene)   // 씬이 있다면 씬에 대한 정보를 출력
            {
                auto   view              = pActiveScene->CreateView<entt::entity>();   // 엔티티 리스트
                Int32  entityCount       = static_cast<Int32>(view.size());            // 엔티티 수
                Entity editEntity        = m_pEditorLayer->GetEditEntity();            // 현재 편집중인 엔티티
                UInt32 renderedItemCount = 0;                                          // 렌더링된 아이템 수 [디버깅]

                // 자식 윈도웅 크기 결정
                float  buttonHeight   = ImGui::GetFrameHeightWithSpacing();
                float  textItemHeight = ImGui::GetTextLineHeightWithSpacing();
                ImVec2 childWinSize   = ImGui::GetContentRegionAvail();                                                         // 자식 윈도우의 크기
                childWinSize.y        = childWinSize.y - (buttonHeight + textItemHeight + ImGui::GetStyle().WindowPadding.y);   // 텍스트 + 버튼 영역을 제외

                // 렌더링
                if (ImGui::BeginChild("entity list", childWinSize, ImGuiChildFlags_Border))
                {
                    static ImGuiListClipper s_clipper;   // 클리핑

                    s_clipper.Begin(entityCount, textItemHeight);   // 클리핑 영역 계산
                    auto viewBegin = view.begin();                  // 엔티티 뷰의 시작점
                    while (s_clipper.Step())                        // 클리핑 + 엔티티 렌더링
                    {
                        auto beginIt = viewBegin + s_clipper.DisplayStart;                                  // 클리핑된 시작점
                        auto endIt   = viewBegin + s_clipper.DisplayEnd;                                    // 클리핑된 끝점
                        JAM_ASSERT(endIt <= view.end(), "Clipper end iterator exceeds entity view size");   // 클리핑된 끝점이 엔티티 뷰의 크기를 초과하지 않도록 확인

                        // 엔티티 렌더링
                        for (auto entityIt = beginIt; entityIt != endIt; ++entityIt)
                        {
                            ++renderedItemCount;                                 // 렌더링된 엔티티 수 증가 [디버깅용]
                            Entity entity = pActiveScene->FindEntity(*entityIt); // 핸들로부터 엔티티 객체 가져오기
                            JAM_ASSERT(entity.IsValid(), "Invalid entity in scene hierarchy");
                            ImGui::PushID(static_cast<int>(entity.GetID()));

                            // 엔티티를 선택할 수 있는 ui
                            bool bIsEditEntity = editEntity == entity;   // 렌더링 중인 엔티티가 현재 편집중인 엔티티인지?
                            if (entity.HasComponent<TagComponent>())
                            {
                                TagComponent&      tag  = entity.GetComponent<TagComponent>();
                                const std::string& name = tag.name;
                                ImGui::Selectable(name.c_str(), bIsEditEntity);
                            }
                            else
                            {
                                std::string name = std::format("entity {}", entity.GetID());   // 아마도 short string optimization
                                ImGui::Selectable(name.c_str(), bIsEditEntity);
                            }

                            // entity 아이템이 호버링된 상태에서 클릭 이벤트 처리
                            if (ImGui::IsItemHovered())
                            {
                                bool bClickedLeft       = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                                bool bClickedRight      = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
                                bool bDoubleClickedLeft = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

                                if (bClickedLeft || bClickedRight)   // 왼쪽을 누르던 오른쪽을 누르던 엔티티 선택
                                {
                                    // 엔티티가 선택되면 편집 엔티티로 설정
                                    m_pEditorLayer->SetEditEntity(entity);
                                }

                                if (bDoubleClickedLeft)   // 더블 클릭하면 편집창을 연다
                                {
                                    m_pEditorLayer->SetPanelVisible(ePanel::EntityInspector, true);
                                }
                            }

                            // 셀렉트 버튼을 우클릭 시 팝업 메뉴
                            if (ImGui::BeginPopupContextItem())
                            {
                                if (ImGui::Selectable("delete entity"))   // 엔티티 삭제
                                {
                                    m_pEditorLayer->DestroyEntity(entity);
                                }
                                if (ImGui::Selectable("clone entity"))   // 엔티티 복제
                                {
                                    m_pEditorLayer->CloneEntity(entity);
                                }
                                ImGui::EndPopup();
                            }
                            ImGui::PopID();
                        }
                    }
                    // 빈 공간을 우클릭하면 엔티티 생성 팝업 메뉴를 띄움
                    if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))   // 우클릭 시 팝업 메뉴
                    {
                        if (ImGui::Selectable("create entity"))
                        {
                            m_pEditorLayer->SetEditEntity(pActiveScene->CreateEntity());
                        }
                        ImGui::EndPopup();
                    }
                }
                ImGui::EndChild();

                // 엔티티 리스트 관련 정보 [디버깅용]
                ImGui::TextDisabled("entity count: %d, rendered: %d", entityCount, renderedItemCount);

                // 엔티티 관련 버튼
                {
                    ImVec2 buttonSize = editor::CalcFittedButtonSize(3);

                    ImGui::Spacing();
                    if (ImGui::Button("create", buttonSize))   // 엔티티 생성
                    {
                        m_pEditorLayer->SetEditEntity(pActiveScene->CreateEntity());
                    }

                    // 해당 버튼은 선택된 엔티티가 있을 때만 활성화
                    ImGui::BeginDisabled(!editEntity.IsValid());
                    {
                        ImGui::SameLine();
                        if (ImGui::Button("delete", buttonSize))   // 엔티티 삭제
                        {
                            m_pEditorLayer->DestroyEntity(editEntity);
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("clone", buttonSize))   // 엔티티 복제
                        {
                            m_pEditorLayer->CloneEntity(editEntity);
                        }
                    }
                    ImGui::EndDisabled();
                }
            }
            else
            {
                editor::CenterAlignedText("no active scene", true);
            }
        }
#pragma endregion
    }
    ImGui::End();
}

}   // namespace jam