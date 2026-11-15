#include "pch.h"

#include "SandboxApp.h"

#include "DeferredScene.h"
#include "../JamEngine/FileSystem.h"

#include "SimpleScene.h"
#include "InstancingScene.h"
#include "MultiVertexStreamScene.h"
#include "PBRScene.h"
#include "ImageFilterManager.h"
#include "ShadowScene.h"
#include "StencilScene.h"
#include "CullingScene.h"
#include "TerrainScene.h"
#include "SSAOScene.h"
#include "SSRScene.h"
#include "ParticleScene.h"
#include "MousePickingScene.h"
#include "TextureScene.h"
#include "ComputeScene.h"
#include "SkinningScene.h"

namespace
{

struct SceneEntry
{
    StringView name;
    Scoped<Scene> (*create)();
};

template<typename TScene>
Scoped<Scene> CreateScene()
{
    return MakeScoped<TScene>();
}

#define SCENE_ENTRY(_m) { #_m, &CreateScene<_m> }

const SceneEntry kSceneEntries[] = {
    SCENE_ENTRY(SimpleScene),
    SCENE_ENTRY(InstancingScene),
    SCENE_ENTRY(MultiVertexStreamScene),
    SCENE_ENTRY(TextureScene),
    SCENE_ENTRY(PBRScene),
    SCENE_ENTRY(DeferredScene),
    SCENE_ENTRY(ShadowScene),
    SCENE_ENTRY(StencilScene),
    SCENE_ENTRY(CullingScene),
    SCENE_ENTRY(TerrainScene),
    SCENE_ENTRY(SSAOScene),
    SCENE_ENTRY(SSRScene),
    SCENE_ENTRY(ParticleScene),
    SCENE_ENTRY(MousePickingScene),
    SCENE_ENTRY(ComputeScene),
    SCENE_ENTRY(SkinningScene)
};

#undef SCENE_ENTRY

}   // namespace

SandboxApp::SandboxApp()
    : Application("SandboxApp", kDefaultAppInitFlags)
{
}

void SandboxApp::Init(
    const Int32 _argc,
    char**      _argv)
{
    ImageFilterManager::CreateInstance();
    GetGraphics().SetVSync(true);
    GetWindowsPlatform().AdjustClientRect(100, 100, 1600, 900);
    GetSceneManager().LoadScene(MakeScoped<SimpleScene>(), false);
}

void SandboxApp::ImGuiRender()
{
    Application::ImGuiRender();

    SceneManager&    sm      = GetSceneManager();
    const Scene*     pActive = sm.GetActiveSceneOrNull();
    const StringView current = pActive ? pActive->GetTypeName() : StringView {};

    ImGui::SetNextWindowPos(ImVec2 { 10.f, 10.f }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scenes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        for (const SceneEntry& entry: kSceneEntries)
        {
            const bool bActive = (entry.name == current);
            if (bActive)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }
            if (ImGui::Button(entry.name.GetPtr(), ImVec2 { 190.f, 0.f }) && !bActive)
            {
                sm.LoadScene(entry.create());
            }
            if (bActive)
            {
                ImGui::PopStyleColor();
            }
        }
    }
    ImGui::End();
}

void SandboxApp::Shutdown()
{
    ImageFilterManager::DestroyInstance();
}

int main(
    const int _argc,
    char**    _argv)
{
    SandboxApp app;
    return app.Run(_argc, _argv);
}
