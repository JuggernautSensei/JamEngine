#pragma once
#include "CullingSystem.h"
#include "RenderView.h"

// clang-format off
namespace jam { class Event; class Scene; }
// clang-format on

namespace jam
{

class SceneManager
{
public:
    static void Init();
    static void Shutdown();
    static void OnEvent(Event& _event);

    static void Resize(Int32 _width, Int32 _height);
    static void Update(float _deltaSec);
    static void RenderRuntime();
    static void RenderOffScreen(const RenderView& _view);
    static void ImGuiRender();

    JAM_NODISCARD static eErrorCode LoadScene(const fs::path& _scenePath);
    static void                     LoadScene(const std::shared_ptr<Scene>& _pScene);

    JAM_NODISCARD static Scene*     GetActiveScene();
    JAM_NODISCARD static Resolution GetSceneResoultion();
};

}   // namespace jam