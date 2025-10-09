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

    void Start();
    void Stop();
    void Update(float _deltaSec);

    JAM_NODISCARD virtual std::string_view GetName() const = 0;
    JAM_NODISCARD Entity                   GetEntity() const { return m_owner; }
    JAM_NODISCARD bool                     IsRunning() const { return m_bRun; }

protected:
    explicit Script(const Entity& _entity);

    JAM_NODISCARD const Entity& GetOwner() const { return m_owner; }
    JAM_NODISCARD Entity&       GetOwner() { return m_owner; }

private:
    virtual void OnStart() {}
    virtual void OnStop() {}
    virtual void OnUpdate(float _deltaSec) {}

    Entity m_owner    = Entity::s_null;
    bool   m_bRun     = true;
    bool   m_bStarted = false;
};

}   // namespace jam

#define JAM_SCRIPT(_class)                                                      \
public:                                                                         \
    constexpr static bool _script_marker_ = true;                               \
    JAM_NODISCARD std::string_view GetName() const override { return #_class; } \
                                                                                \
private:                                                                        \
    inline static bool _script_reflect_ = []                                    \
    {                                                                           \
        jam::script_db::Reflect<_class>();                                      \
        return true;                                                            \
    }()
