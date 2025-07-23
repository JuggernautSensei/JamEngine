#include "pch.h"

#include "Script.h"

namespace jam
{

Script::Script(const Entity _entity)
    : m_entity(_entity)
{
    JAM_ASSERT(m_entity.IsValid(), "Script must be associated with a valid entity");
}

void Script::StartScript()
{
    m_bRun = true;
}

void Script::StopScript()
{
    m_bRun     = false;
    m_bStarted = false;
}

}   // namespace jam