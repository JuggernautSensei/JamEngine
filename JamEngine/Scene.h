#pragma once
#include "Entity.h"
#include "Environment.h"
#include "RenderView.h"
#include "UID.h"

namespace jam
{

class Event;
class Transform;

class Scene : public RefCounted
{
public:
    ~Scene() override = default;

    Scene(const Scene&)                = delete;
    Scene& operator=(const Scene&)     = delete;
    Scene(Scene&&) noexcept            = delete;
    Scene& operator=(Scene&&) noexcept = delete;

    // scene runtime interface
    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual void OnUpdate(float _deltaSec) {}
    virtual void OnRender(const RenderView& _desc) {}
    virtual void OnImGuiRender() {}
    virtual void OnEvent(Event& _event) {}
    virtual void OnResize(Int32 _width, Int32 _height) {}

    // scene buffer
    [[nodiscard]] virtual TextureHandle GetDepthTexture() const;

    // item management
    [[nodiscard]] Entity CreateEntity(std::string_view _name = {});
    [[nodiscard]] Entity CreateEntity(UID _uid, std::string_view _name = {});   // 특정 UID로 엔티티 생성, 이미 존재하는 UID면 assert 발생

    [[nodiscard]] Entity GetEntity(UID _uid) const;
    [[nodiscard]] Entity FindEntity(UID _uid) const;   // 해당 UID를 가진 엔티티가 없으면 null 엔티티 반환

    [[nodiscard]] Entity CloneEntity(Entity _targetEntity);
    void                 DestroyEntity(Entity _entity);
    [[nodiscard]] bool   IsEntityValid(Entity _entity) const;

    // item iteration
    template<typename... Args>
    [[nodiscard]] decltype(auto) View()
    {
        return m_registry.view<Args...>();
    }

    template<typename... Args>
    [[nodiscard]] decltype(auto) View() const
    {
        return m_registry.view<Args...>();
    }

    template<typename... Args>
    [[nodiscard]] decltype(auto) Each()
    {
        return m_registry.view<Args...>().each();
    }

    template<typename... Args>
    [[nodiscard]] decltype(auto) Each() const
    {
        return m_registry.view<Args...>().each();
    }

    [[nodiscard]] const entt::registry& GetRegistry() const;
    [[nodiscard]] entt::registry&       GetRegistry();

    // item
    [[nodiscard]] Entity                      GetRootEntity() const;
    [[nodiscard]] Int32                       GetEntityCount() const;
    [[nodiscard]] const HashMap<UID, Entity>& GetEntities() const;

    // culling
    

    // metadata
    [[nodiscard]] virtual std::string_view GetTypeName() const = 0;

protected:
    Scene();

private:
    void                 DestroyEntity_Recursive_(Entity _entity);
    [[nodiscard]] Entity CloneEntity_Recursive_(Entity _srcEntity);
    [[nodiscard]] Entity CreateEntity_(UID _uid, std::string_view _name = {});
    [[nodiscard]] Entity CreateRootEntity_();

    // entt
    entt::registry       m_registry     = {};
    UIDGenerator         m_uidGenerator = {};
    HashMap<UID, Entity> m_entities     = {};

    // scene item hierarchy
    Entity m_root = Entity::s_null;

    constexpr static UID k_rootEntityReservedUID = 0;
};

}   // namespace jam

#define JAM_SCENE(_class)                                                             \
public:                                                                               \
    constexpr static std::string_view k_name = NameOf<_class>();                      \
    [[nodiscard]] std::string_view    GetTypeName() const override { return k_name; } \
                                                                                      \
private:                                                                              \
    inline static bool _scene_reflect_ = []                                           \
    {                                                                                 \
        jam::SceneDB::GetInstance().Reflect<_class>();                                \
        return true;                                                                  \
    }()
