#include "pch.h"

#include "ViewportPanel.h"

#include "Application.h"
#include "Components.h"

#include "EditorUtilities.h"
#include "SceneLayer.h"

namespace jam
{

ViewportPanel::ViewportPanel(EditorLayer* _pEditorLayer)
    : m_pEditorLayer(_pEditorLayer)
{
    JAM_ASSERT(m_pEditorLayer != nullptr, "EditorLayer pointer is null");
}

void ViewportPanel::Show(bool* _pOpen)
{
    if (ImGui::Begin("viewport", _pOpen))
    {
        // 씬 카메라 조작
        if (ImGui::IsWindowFocused())   // 윈도우가 포커스 상태에서만 작동
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                const TickTimer& timer        = Application::GetInstance().GetTimer();
                float            deltaTimeSec = timer.GetDeltaSec();

                // 마우스 오른쪽 버튼을 클릭한 채로 드래그하면 회전
                auto [dx, dy] = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
                if (dx != 0 || dy != 0)
                {
                    Transform& transformRef = m_editorCamera.transform;
                    float      yaw          = -dx * m_editorCamera.rotateSpeed;
                    float      pitch        = -dy * m_editorCamera.rotateSpeed;
                    Quat       q            = Quat::CreateFromYawPitchRoll(yaw, pitch, 0.f);
                    transformRef.rotate *= q;
                }

                // 마우스 오른쪽 버튼을 클릭한 채로 W,A,S,D,Q,E 키를 누르면 이동
                {
                    Transform& transformRef = m_editorCamera.transform;
                    float      moveSpeed    = m_editorCamera.moveSpeed;
                    if (ImGui::IsKeyDown(ImGuiKey_W))
                    {
                        transformRef.translate += moveSpeed * deltaTimeSec * transformRef.Forward();
                    }
                    if (ImGui::IsKeyDown(ImGuiKey_S))
                    {
                        transformRef.translate += moveSpeed * deltaTimeSec * transformRef.Backward();
                    }
                    if (ImGui::IsKeyDown(ImGuiKey_A))
                    {
                        transformRef.translate += moveSpeed * deltaTimeSec * transformRef.Left();
                    }
                    if (ImGui::IsKeyDown(ImGuiKey_D))
                    {
                        transformRef.translate += moveSpeed * deltaTimeSec * transformRef.Right();
                    }
                    if (ImGui::IsKeyDown(ImGuiKey_Q))
                    {
                        transformRef.translate -= moveSpeed * deltaTimeSec * Vec3::Up;
                    }
                    if (ImGui::IsKeyDown(ImGuiKey_E))
                    {
                        transformRef.translate += moveSpeed * deltaTimeSec * Vec3::Up;
                    }
                }
            }
        }

        ImVec2 startCursorPos = ImGui::GetCursorScreenPos();

        // 씬 그리기
        {
            eRunMode          runMode         = m_pEditorLayer->GetRunMode();
            const SceneLayer* pSceneLayer     = Application::GetInstance().GetSceneLayer();
            bool              bHasActiveScene = pSceneLayer->HasActiveScene();

            if (bHasActiveScene)
            {
                // 씬을 렌더링
                const TextureRef& sceneTexture = pSceneLayer->GetSceneTexture();
                if (runMode == eRunMode::Play)
                {
                    // 런타임 카메라를 기준으로 렌더링
                    pSceneLayer->RenderSceneRuntime();
                }
                else
                {
                    // 에디터 카메라를 기준으로 렌더링
                    JAM_ASSERT(runMode == eRunMode::Edit || runMode == eRunMode::Simulate, "Invalid run mode");
                    pSceneLayer->RenderScene(sceneTexture, m_editorCamera.camera, m_editorCamera.transform);
                }

                {
                    // 렌더링될 위치와 크기 계산
                    auto [width, height]          = sceneTexture.GetSize();
                    ImVec2 outerBox               = ImGui::GetContentRegionAvail();   // 현재 컨텐츠 영역의 크기
                    ImVec2 sceneTexSize           = { static_cast<float>(width), static_cast<float>(height) };
                    auto [imageSize, imageOffset] = editor::CalcFittedCenteredRect(outerBox, sceneTexSize);

                    // 이미지 렌더링
                    ImGui::SetCursorPos(startCursorPos + imageOffset);
                    editor::Image(sceneTexture.GetSRV(), imageSize);
                }

            }
            else
            {
                editor::CenterAlignedText("no active scene", true);
            }
        }

        // 오버레이 메뉴
        {
            editor::PushOverlayStyle();
            ImGui::SetCursorScreenPos(startCursorPos);

            // 카메라 관련 설정
            ImGui::Button("camera");
            if (ImGui::BeginPopupContextItem(NULL, ImGuiPopupFlags_MouseButtonLeft))
            {
                Camera&    cameraRef    = m_editorCamera.camera;
                Transform& transformRef = m_editorCamera.transform;

                if (Widget_MenuItem("reset transform"))
                {
                    transformRef = Transform {};
                }
                if (Widget_MenuItem("reset propertise"))
                {
                    cameraRef = Camera {};
                }
                ImGui::Separator();
                if (ImGui::BeginMenu("projection"))
                {
                    Camera::eProjection projection = cameraRef.projection;
                    if (Widget_MenuItem("perspective", NULL, projection == Camera::eProjection::Perspective))
                    {
                        cameraRef.projection = Camera::eProjection::Perspective;
                    }
                    if (Widget_MenuItem("orthographic", NULL, projection == Camera::eProjection::Orthographic))
                    {
                        cameraRef.projection = Camera::eProjection::Orthographic;
                    }
                    ImGui::EndMenu();
                }
                ImGui::DragFloat("##cameraNearZ", &cameraRef.nearZ, 0.01f, Camera::k_nearestZ, cameraRef.farZ - Camera::k_nearestZ, "near Z: %.2f m", ImGuiSliderFlags_AlwaysClamp);
                ImGui::DragFloat("##cameraFarZ", &cameraRef.farZ, 1.f, cameraRef.nearZ + Camera::k_nearestZ, Camera::k_farthestZ, "far Z: %.2f m", ImGuiSliderFlags_AlwaysClamp);
                ImGui::SliderAngle("##cameraFovY", &cameraRef.fovYRad, ToRad(1.f), ToRad(180.f), "FOV Y: %.1f deg", ImGuiSliderFlags_AlwaysClamp);
                ImGui::DragFloat("##cameraMoveSpeed", &m_editorCamera.moveSpeed, 0.1f, 0.1f, 100.f, "move speed: %.1f m/s", ImGuiSliderFlags_AlwaysClamp);

                ImGui::EndPopup();
            }

            editor::PopOverlayStyle();
        }
    }
    ImGui::End();
}

}   // namespace jam