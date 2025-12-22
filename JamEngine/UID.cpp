#include "pch.h"
#include "UID.h"

namespace jam
{

UID UIDGenerator::Alloc()
{
    while (m_allocatedUids.contains(nextUid))
    {
        ++nextUid;
        if (nextUid == k_invalidUID)
        {
            nextUid = 0;
        }
    }
    m_allocatedUids.insert(nextUid);
    return nextUid++;
}

UID UIDGenerator::Alloc(UID _uid)
{
    JAM_ASSERT(_uid != k_invalidUID, "Cannot allocate invalid UID");
    JAM_ASSERT(m_allocatedUids.contains(_uid) == false, "UID {} is already allocated", _uid);
    m_allocatedUids.insert(_uid);
    return _uid;
}

UID UIDGenerator::AllocWithHint(const UID _hint)
{
    if (IsAllocated(_hint))
    {
        return Alloc();
    }
    return Alloc(_hint);
}

void UIDGenerator::Free(UID _uid)
{
    JAM_ASSERT(_uid != k_invalidUID, "Cannot free invalid UID");
    const size_t numOfErase = m_allocatedUids.erase(_uid);
    JAM_ASSERT(numOfErase == 1, "UID {} is not allocated", _uid);
}

void UIDGenerator::Clear()
{
    m_allocatedUids.clear();
    nextUid = 0;
}

bool UIDGenerator::IsAllocated(const UID _uid) const
{
    return m_allocatedUids.contains(_uid);
}
}   // namespace jam