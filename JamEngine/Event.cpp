#include "pch.h"

#include "Event.h"

namespace jam
{

bool jam::Event::IsHandled() const
{
    return m_bHandled;
}

void jam::Event::SetHandled(const bool _bHandled)
{
    m_bHandled = _bHandled;
}

}   // namespace jam