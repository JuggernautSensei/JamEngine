#include "pch.h"
#include "UIDAllocator.h"
#include "UID.h"

namespace Jam
{

UID UIDAllocator::Alloc()
{
    UID uid = UID::kNull;
    if (m_freeUIDs.empty())
    {
        do
        {
            uid = UID { m_nextUIDValue++ };
        } while (IsAllocated(uid));
    }
    else
    {
        uid = m_freeUIDs.back();
        m_freeUIDs.pop_back();
        JAM_ASSERT(IsAllocated(uid) == false, "UID {} is already allocated", uid);
    }

    m_allocatedUIDs.insert(uid);
    return uid;
}

UID UIDAllocator::Alloc(UID _uid)
{
    JAM_ASSERT(_uid != UID::kNull, "Cannot allocate invalid UID");
    JAM_ASSERT(m_allocatedUIDs.contains(_uid) == false, "UID {} is already allocated", _uid);
    m_allocatedUIDs.insert(_uid);
    return _uid;
}

UID UIDAllocator::Alloc(const UInt32 _uid)
{
    return Alloc(UID { _uid });
}

UID UIDAllocator::AllocWithHint(const UID _hint)
{
    if (IsAllocated(_hint))
    {
        return Alloc();
    }
    else
    {
        return Alloc(_hint);
    }
}

UID UIDAllocator::AllocWithHint(const UInt32 _hint)
{
    return AllocWithHint(UID { _hint });
}

bool UIDAllocator::IsAllocated(const UID _uid) const
{
    return m_allocatedUIDs.contains(_uid);
}

void UIDAllocator::Free(UID _uid)
{
    JAM_ASSERT(IsAllocated(_uid), "UID {} is not allocated", _uid);
    m_allocatedUIDs.erase(_uid);
    m_freeUIDs.push_back(_uid);
}

void UIDAllocator::Clear()
{
    m_allocatedUIDs.clear();
    m_freeUIDs.clear();
    m_nextUIDValue = 0;
}

}   // namespace Jam