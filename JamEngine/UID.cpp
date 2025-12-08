#include "pch.h"
#include "UID.h"

namespace jam
{

UID UIDGenerator::Alloc()
{
    while (m_allocatedUIDs.contains(nextUID))
    {
        ++nextUID;
        if (nextUID == k_invalidUID)
            nextUID = 0;
    }
    m_allocatedUIDs.insert(nextUID);
    return nextUID++;
}

UID UIDGenerator::Alloc(UID _uid)
{
    JAM_ASSERT(_uid != k_invalidUID, "Cannot allocate invalid UID");
    JAM_ASSERT(m_allocatedUIDs.contains(_uid) == false, "UID {} is already allocated", _uid);
    m_allocatedUIDs.insert(_uid);
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
    size_t numOfErase = m_allocatedUIDs.erase(_uid);
    JAM_ASSERT(numOfErase == 1, "UID {} is not allocated", _uid);
}

void UIDGenerator::Clear()
{
    m_allocatedUIDs.clear();
    nextUID = 0;
}

bool UIDGenerator::IsAllocated(const UID _uid) const
{
    return m_allocatedUIDs.contains(_uid);
}
}