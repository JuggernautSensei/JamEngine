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

    static void RenderRuntime();                                                // 백버퍼에 메인 카메라로 렌더링
    static void RenderRuntime(FrameBufferHandle _handle, Viewport _viewport);   // 특정 버퍼에 메인 카메라로 렌더링
    static void Render(const RenderView& _view);

    static void ImGuiRender();

    [[nodiscard]] static bool LoadScene(const fs::path& _path);
    static void               LoadScene(const Ref<Scene>& _pScene);

    [[nodiscard]] static bool        HasActiveScene();
    [[nodiscard]] static Ref<Scene>& GetActiveScene();
};

}   // namespace jam