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

void RenderRuntime();
void Render(FrameBufferHandle _handle, const Camera& _camera, const Transform& _transform);

Scene* AddScene(std::string_view _name, std::unique_ptr<Scene>&& _pScene);
void   RemoveScene(std::string_view _name);
void   ChangeScene(std::string_view _name);

JAM_NODISCARD bool   HasActiveScene();
JAM_NODISCARD Scene* GetActiveScene();

JAM_NODISCARD bool   HasScene(std::string_view _name);
JAM_NODISCARD Scene* GetScene(std::string_view _name);

JAM_NODISCARD const HashMap<std::string, std::unique_ptr<Scene>>& GetAllScenes();

}