#include "pch.h"

#include "Script.h"

namespace jam
{

Script::Script(const Entity _entity)
    : m_owner(_entity)
{
    JAM_ASSERT(m_owner.IsValid(), "Script must be associated with a valid entity");
}

void Script::Start()
{
    if (!IsRunning())
    {
        m_bRun      = true;
        m_bCallOnce = false;
    }
}

void Script::Stop()
{
    if (IsRunning())
    {
        m_bRun      = false;
        m_bCallOnce = false;
    }
}

void Script::Update(const float _deltaSec)
{
    if (m_bRun)
    {
        if (!m_bCallOnce)
        {
            OnStart();
            m_bCallOnce = true;
        }
        OnUpdate(_deltaSec);
    }
    else
    {
        if (!m_bCallOnce)
        {
            OnStop();
            m_bCallOnce = true;
        }
    }
}

Entity Script::GetEntity() const
{
    return m_owner;
}

bool Script::IsRunning() const
{
    return m_bRun;
}

}   // namespace jam