#include "pch.h"

#include "Event.h"

namespace Jam
{

bool Event::IsHandled() const
{
    return m_bHandled;
}

void Event::SetHandled(const bool _bHandled)
{
    m_bHandled = _bHandled;
}

}   // namespace Jam