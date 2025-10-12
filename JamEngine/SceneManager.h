#pragma once
#include "GfxBase.h"

// clang-format off
namespace jam { struct Transform; struct Camera; class Event; class Scene; }
// clang-format on

namespace jam::scene_manager
{

void Init(Int32 _width, Int32 _height);
void Shutdown();

void OnEvent(Event& _event);
void Update(float _deltaSec);
void RenderRuntime();
void Render(FrameBufferHandle _handle, const Camera& _camera, const Transform& _transform);
void ImGuiRender();

eErrorCode           LoadScene(const fs::path& _scenePath);
void                 LoadScene(std::unique_ptr<Scene>&& _pScene);
JAM_NODISCARD Scene* GetActiveScene();

}   // namespace jam::scene_manager