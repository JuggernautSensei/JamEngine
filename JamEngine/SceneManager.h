#pragma once
#include "CullingSystem.h"
#include "GraphicsBase.h"

// clang-format off
namespace jam { struct Transform; struct Camera; class Event; class Scene; }
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
    static void RenderOffScreen(FrameBufferHandle _handle, const Camera& _camera, const Transform& _transform);
    static void ImGuiRender();

    JAM_NODISCARD static eErrorCode LoadScene(const fs::path& _scenePath);
    static void                     LoadScene(std::unique_ptr<Scene>&& _pScene);

    JAM_NODISCARD static Scene* GetActiveScene();
};

}   // namespace jam