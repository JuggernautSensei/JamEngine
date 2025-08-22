#pragma once
#include "ILayer.h"

namespace jam
{

class Scene;

class SceneLayer : public ILayer
{
public:
    using Container = std::unordered_map<std::string, Scope<Scene>>;   // scene displayName, scene pointer

    SceneLayer();
    ~SceneLayer() override;

    SceneLayer(const SceneLayer&)                = default;
    SceneLayer& operator=(const SceneLayer&)     = default;
    SceneLayer(SceneLayer&&) noexcept            = default;
    SceneLayer& operator=(SceneLayer&&) noexcept = default;

    void OnUpdate(float _deltaSec) override;
    void OnFinalUpdate(float _deltaSec) override;

    void OnBeginRender() override;
    void OnRender() override;
    void OnEndRender() override;

    void             OnEvent(Event& _event) override;
    NODISCARD UInt32 GetHash() const override;
    NODISCARD std::string_view GetName() const override;

    Scene* AddScene(Scope<Scene>&& _scene);
    void   RemoveScene(std::string_view _name);
    void   ChangeScene(std::string_view _name);

    NODISCARD Scene* GetActiveScene() const;
    NODISCARD Scene* GetScene(std::string_view _name) const;

    NODISCARD const Container& GetContainer() const { return m_container; }
    NODISCARD Container&       GetContainerRef() { return m_container; }

private:
    void ChangeScene_(Scene* _pScene);

    Container m_container;
    Scene*    m_pActiveScene = nullptr;   // currently active scene
};

}   // namespace jam