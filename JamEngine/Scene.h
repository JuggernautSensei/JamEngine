#pragma once
#include "Entity.h"
#include "GraphicsBase.h"
#include "RenderView.h"
#include "UID.h"

// clang-format off
namespace jam { class Event; class Transform; }
// clang-format on

namespace jam
{

class Scene
{
public:
    virtual ~Scene() = default;

    Scene(const Scene&)                = delete;
    Scene& operator=(const Scene&)     = delete;
    Scene(Scene&&) noexcept            = delete;
    Scene& operator=(Scene&&) noexcept = delete;

    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual void OnUpdate(float _deltaSec) {}
    virtual void OnRender(const RenderView& _view) {}
    virtual void OnImGuiRender() {}
    virtual void OnEvent(Event& _event) {}
    virtual void OnResize(Int32 _width, Int32 _height) {}

    JAM_NODISCARD Entity CreateEntity(std::string_view _name = {});
    JAM_NODISCARD Entity CreateEntity(UID _uid, std::string_view _name = {});   // 특정 UID로 엔티티 생성, 이미 존재하는 UID면 assert 발생

    JAM_NODISCARD Entity GetEntity(UID _uid) const;
    JAM_NODISCARD Entity FindEntity(UID _uid) const;   // 해당 UID를 가진 엔티티가 없으면 null 엔티티 반환

    JAM_NODISCARD Entity CloneEntity(Entity _entity);
    void                 DestroyEntity(Entity _entity);
    JAM_NODISCARD bool   IsEntityValid(Entity _entity) const;

    template<typename... Args>
    JAM_NODISCARD decltype(auto) View()
    {
        return m_registry.view<Args...>();
    }

    template<typename... Args>
    JAM_NODISCARD decltype(auto) View() const
    {
        return m_registry.view<Args...>();
    }

    template<typename... Args>
    JAM_NODISCARD decltype(auto) Each()
    {
        return m_registry.view<Args...>().each();
    }

    template<typename... Args>
    JAM_NODISCARD decltype(auto) Each() const
    {
        return m_registry.view<Args...>().each();
    }

    JAM_NODISCARD const entt::registry& GetRegistry() const;
    JAM_NODISCARD entt::registry& GetRegistry();

    JAM_NODISCARD Entity GetRootEntity() const;
    JAM_NODISCARD const HashMap<UID, Entity>& GetEntities() const;
    JAM_NODISCARD virtual std::string_view    GetName() const = 0;

protected:
    Scene();

private:
    void                 DestroyEntity_Recursive_(Entity _entity);
    JAM_NODISCARD Entity CreateEntity_(bool _bCreateRoot, UID _uid, std::string_view _name = {});

    entt::registry       m_registry     = {};
    Entity               m_rootEntity   = Entity::s_null;
    UIDGenerator         m_uidGenerator = {};
    HashMap<UID, Entity> m_entityMap    = {};
};

}   // namespace jam

#define JAM_SCENE(_class)                                                      \
public:                                                                        \
    constexpr static std::string_view k_name = NameOf<_class>();               \
    JAM_NODISCARD std::string_view GetName() const override { return k_name; } \
                                                                               \
private:                                                                       \
    inline static bool _scene_reflect_ = []                                    \
    {                                                                          \
        jam::SceneDB::Reflect<_class>();                                       \
        return true;                                                           \
    }()
