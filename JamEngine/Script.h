#pragma once
#include "Entity.h"

namespace jam
{

class Event;

class Script : public RefCounted
{
public:
    ~Script() override = default;

    Script(const Script&)            = delete;
    Script& operator=(const Script&) = delete;
    Script(Script&&)                 = default;
    Script& operator=(Script&&)      = default;

    void Start();
    void Stop();
    void Update(float _deltaSec);

    [[nodiscard]] Entity             GetOwner() const;
    [[nodiscard]] bool               IsRunning() const;
    [[nodiscard]] virtual StringView GetTypeName() const = 0;

protected:
    explicit Script(Entity _entity);

private:
    virtual void OnStart() {}                   // 스크립트가 시작될 때 호출
    virtual void OnStop() {}                    // 스크립트가 멈출 때 호출
    virtual void OnUpdate(float _deltaSec) {}   // 매 프레임 호출

    Entity m_owner     = Entity::s_null;
    bool   m_bRun      = true;
    bool   m_bCallOnce = false;
};

}   // namespace jam

#define JAM_SCRIPT(_class)                                                          \
public:                                                                             \
    constexpr static StringView k_typename = NameOf<_class>();                      \
    [[nodiscard]] StringView    GetTypeName() const override { return k_typename; } \
                                                                                    \
private:                                                                            \
    inline static bool _script_reflect_ = []                                        \
    {                                                                               \
        jam::ScriptDB::GetInstance().Reflect<_class>();                             \
        return true;                                                                \
    }()
