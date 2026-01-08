#pragma once
#include "RenderView.h"
#include "Scene.h"

namespace jam
{

class Event;

class SceneManager
{
public:
    static void Init();
    static void Shutdown();
    static void OnEvent(Event& _event);

    static void Resize(Int32 _width, Int32 _height);
    static void Update(float _deltaSec);

    static void RenderRuntime();                                       // 백버퍼에 메인 카메라로 렌더링
    static void RenderRuntime(FrameBufferHandle _fbh, Viewport _vp);   // 특정 버퍼에 메인 카메라로 렌더링
    static void Render(const RenderView& _view);

    static void ImGuiRender();

    [[nodiscard]] static bool LoadScene(const fs::path& _path);
    static void               LoadScene(const Ref<Scene>& _pScene);

    [[nodiscard]] static bool        HasActiveScene();
    [[nodiscard]] static Ref<Scene>& GetActiveScene();

private:
    static void ChangeSceneDefered_(const Ref<Scene>& _pScene);
    static void ChangeScene_(const Ref<Scene>& _pScene);
};

}   // namespace jam