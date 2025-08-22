#include "pch.h"

#include "SceneLayer.h"

#include "Application.h"
#include "Components.h"
#include "Scene.h"
#include "Script.h"

namespace jam
{

SceneLayer::SceneLayer()
{
}

SceneLayer::~SceneLayer()
{
}

void SceneLayer::OnUpdate(const float _deltaSec)
{
    if (m_pActiveScene) // 활성화된 씬이 있다먄
    {
        m_pActiveScene->OnUpdate(_deltaSec); // 씬을 우선 업데이트

        // 스크립트를 업데이트
        m_pActiveScene->CreateView<ScriptComponent>().each(
            [_deltaSec](ScriptComponent& _scriptComponent)
            {
                Scope<Script>& scriptRef = _scriptComponent.script;

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

Scene* SceneLayer::AddScene(Scope<Scene>&& _scene)
{
    auto [iter, bResult] = m_container.emplace(_scene->GetName(), std::move(_scene));
    JAM_ASSERT(bResult, "Scene with name '{}' already exists", iter->first);
    Scene* pScene = iter->second.get();
    return pScene;
}

Scene* SceneLayer::GetScene(std::string_view _name) const
{
    const auto it = m_container.find(std::string(_name));
    JAM_ASSERT(it != m_container.end(), "Scene with name '{}' not found", _name);
    return it->second.get();
}

void SceneLayer::ChangeScene_(Scene* _pScene)
{
    if (m_pActiveScene != _pScene)
    {
        if (m_pActiveScene)   // 현재 활성화된 씬이 있다면
        {
            m_pActiveScene->OnExit();   // 현재 씬을 종료
        }

        m_pActiveScene = _pScene; // 새 씬을 활성화
        m_pActiveScene->Load();   // 씬을 로드 (기본 경로)
        m_pActiveScene->OnEnter();
    }
}

void SceneLayer::RemoveScene(std::string_view _name)
{
    const auto it = m_container.find(std::string(_name));
    JAM_ASSERT(it != m_container.end(), "Scene with name '{}' not found", _name);
    m_container.erase(it);
}

void SceneLayer::ChangeScene(std::string_view _name)
{
    Scene* pScene = GetScene(_name);
    JAM_ASSERT(pScene, "Scene with name '{}' not found", _name);

    // 지연 처리
    GetApplication().SubmitCommand(
        [pScene, this]
        {
            ChangeScene_(pScene);   // 씬 변경
        });
}

Scene* SceneLayer::GetActiveScene() const
{
    JAM_ASSERT(m_pActiveScene, "No active scene set");
    return m_pActiveScene;
}

}   // namespace jam