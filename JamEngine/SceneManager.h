#pragma once
#include "GfxBase.h"

// clang-format off
namespace jam { struct Transform; struct Camera; class Event; class Scene; }
// clang-format on

namespace jam::scene_manager
{

void Init();
void Shutdown();

void OnEvent(Event& _event);
void Update(float _deltaSec);

/// 씬을 렌더링 후 백버퍼로 복사
void RenderRuntime();

/// 씬을 특정 프레임버퍼에 렌더링
void Render(FrameBufferHandle _handle, const Camera& _camera, const Transform& _transform);

eErrorCode           LoadScene(const fs::path& _scenePath);
void                 LoadScene(std::unique_ptr<Scene>&& _pScene);
JAM_NODISCARD Scene* GetActiveScene();

}   // namespace jam::scene_manager