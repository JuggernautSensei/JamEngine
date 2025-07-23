#include "pch.h"

#include "SceneLayer.h"

#include "Application.h"
#include "Components.h"
#include "Scene.h"
#include "Script.h"

namespace jam
{

void SceneLayer::OnUpdate(const float _deltaSec)
{
    if (m_pActiveScene)
    {
        m_pActiveScene->OnUpdate(_deltaSec);

        m_pActiveScene->CreateView<ScriptComponent>().each(
            [_deltaSec](ScriptComponent& _scriptComponent)
            {
                std::unique_ptr<Script>& scriptRef = _scriptComponent.script;

                if (scriptRef && scriptRef->IsRunning())
                {
                    if (scriptRef->m_bStarted == false)
                    {
                        scriptRef->OnStart();
                        scriptRef->m_bStarted = true;
                    }
                    scriptRef->OnUpdate(_deltaSec);
                }
            });
    }
}

void SceneLayer::OnFinalUpdate(const float _deltaSec)
{
    if (m_pActiveScene)
    {
        m_pActiveScene->OnFinalUpdate(_deltaSec);
    }
}

void SceneLayer::OnBeginRender()
{
    if (m_pActiveScene)
    {
        m_pActiveScene->OnBeginRender();
    }
}

void SceneLayer::OnRender()
{
    if (m_pActiveScene)
    {
        m_pActiveScene->OnRender();
        m_pActiveScene->OnRenderUI();
    }
}

void SceneLayer::OnEndRender()
{
    if (m_pActiveScene)
    {
        m_pActiveScene->OnEndRender();
    }
}

void SceneLayer::OnEvent(Event& _event)
{
    if (m_pActiveScene)
    {
        m_pActiveScene->OnEvent(_event);
    }
}

UInt32 SceneLayer::GetHash() const
{
    return HashOf<SceneLayer>();
}

std::string_view SceneLayer::GetName() const
{
    return NameOf<SceneLayer>();
}

Scene* SceneLayer::AddScene(std::unique_ptr<Scene>&& _scene)
{
    auto [iter, bResult] = m_scenes.emplace(_scene->GetName(), std::move(_scene));
    JAM_ASSERT(bResult, "Scene with name '{}' already exists", iter->first);
    Scene* pScene = iter->second.get();
    pScene->OnAttach();
    return pScene;
}

Scene* SceneLayer::GetScene(std::string_view _name) const
{
    const auto it = m_scenes.find(std::string(_name));
    JAM_ASSERT(it != m_scenes.end(), "Scene with name '{}' not found", _name);
    return it->second.get();
}

void SceneLayer::RemoveScene(std::string_view _name)
{
    const auto it = m_scenes.find(std::string(_name));
    JAM_ASSERT(it != m_scenes.end(), "Scene with name '{}' not found", _name);
    it->second->OnDetach();
    m_scenes.erase(it);
}

void SceneLayer::ChangeScene(std::string_view _name)
{
    Scene* pScene = GetScene(_name);
    JAM_ASSERT(pScene, "Scene with name '{}' not found", _name);

    // 지연 처리
    Application& app = Application::GetInstance();
    app.SubmitCommand(
        [pScene, this]
        {
            if (m_pActiveScene != pScene)
            {
                if (m_pActiveScene)
                {
                    m_pActiveScene->OnExit();
                }

                m_pActiveScene = pScene;
                m_pActiveScene->OnEnter();
            }
        });
}

Scene* SceneLayer::GetActiveScene() const
{
    JAM_ASSERT(m_pActiveScene, "No active scene set");
    return m_pActiveScene;
}

}   // namespace jam