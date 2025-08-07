#pragma once
#include "ILayer.h"

namespace jam
{

class Scene;

class SceneLayer : public ILayer
{
public:
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

    NODISCARD decltype(auto) GetContainer() const { return std::views::all(m_scenes); }

private:
    std::unordered_map<std::string, Scope<Scene>> m_scenes;
    Scene*                                                  m_pActiveScene = nullptr;   // currently active scene
};

}   // namespace jam