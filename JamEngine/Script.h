#pragma once
#include "Entity.h"

namespace jam
{

class Script
{
public:
    virtual ~Script() = default;

    Script(const Script&)            = delete;
    Script& operator=(const Script&) = delete;
    Script(Script&&)                 = default;
    Script& operator=(Script&&)      = default;

    virtual void OnStart() {}
    virtual void OnUpdate(float _deltaSec) {}

    void StartScript();
    void StopScript();

    NODISCARD Entity GetEntity() const { return m_entity; }
    NODISCARD bool   IsRunning() const { return m_bRun; }

    virtual std::string_view GetName() const = 0;
    virtual UInt32           GetHash() const = 0;

    // utility
    template<typename Ty, typename... Args>
    decltype(auto) CreateComponent(Args&&... args)
    {
        JAM_ASSERT(m_entity.IsValid(), "Script must be associated with a valid entity");
        return m_entity.CreateCompoenent<Ty>(std::forward<Args>(args)...);
    }

    template<typename... Ty>
    decltype(auto) GetComponent() const
    {
        JAM_ASSERT(m_entity.IsValid(), "Script must be associated with a valid entity");
        return m_entity.GetComponent<Ty...>();
    }

    template<typename... Ty>
    NODISCARD bool HasComponent() const
    {
        JAM_ASSERT(m_entity.IsValid(), "Script must be associated with a valid entity");
        return m_entity.HasComponent<Ty...>();
    }

    template<typename... Ty>
    decltype(auto) RemoveComponent() const
    {
        JAM_ASSERT(m_entity.IsValid(), "Script must be associated with a valid entity");
        return m_entity.RemoveComponent<Ty...>();
    }

protected:
    explicit Script(Entity _entity);

private:
    Entity m_entity   = Entity::s_null;
    bool   m_bRun     = true;
    bool   m_bStarted = false;

    friend class SceneLayer;
};

// 자동 메타 데이터 등록 헬퍼
#define JAM_SCRIPT(_scriptType)                                                           \
private:                                                                                  \
    inline static const bool _jam_script_meta_registered_ = []() {                        \
        RegisterScript<_scriptType>();                                                    \
        return true;                                                                      \
    }();                                                                                  \
                                                                                          \
public:                                                                                   \
    NODISCARD std::string_view GetName() const override { return NameOf<_scriptType>(); } \
    NODISCARD UInt32           GetHash() const override { return HashOf<_scriptType>(); }

}   // namespace jam