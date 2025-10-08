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
    m_bRun     = true;
    m_bStarted = false;
}

void Script::Stop()
{
    m_bRun     = false;
    m_bStarted = false;
}

void Script::Update(const float _deltaSec)
{
    if (m_bRun)
    {
        if (!m_bStarted)
        {
            OnStart();
            m_bStarted = true;
        }
        OnUpdate(_deltaSec);
    }
}

}   // namespace jam