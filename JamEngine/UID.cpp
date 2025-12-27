#include "pch.h"
#include "UID.h"

namespace jam
{

UID UIDGenerator::Alloc()
{
    JAM_ASSERT(m_allocatedUids.size() < k_invalidUid, "All UIDs are allocated");

    UID uid = k_invalidUid;
    if (m_freeUids.empty())
    {
        while (IsAllocated(nextUid))
        {
            ++nextUid;
            if (nextUid == k_invalidUid)
            {
                nextUid = 0;   // wrap
            }
        }

        uid = nextUid++;
    }
    else
    {
        uid = m_freeUids.back();
        m_freeUids.pop_back();
        JAM_ASSERT(IsAllocated(uid) == false, "UID {} is already allocated", uid);
    }

    m_allocatedUids.insert(uid);
    return uid;
}

UID UIDGenerator::Alloc(UID _uid)
{
    JAM_ASSERT(_uid != k_invalidUid, "Cannot allocate invalid UID");
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
    else
    {
        return Alloc(_hint);
    }
}

void UIDGenerator::Free(UID _uid)
{
    JAM_ASSERT(IsAllocated(_uid), "UID {} is not allocated", _uid);
    m_allocatedUids.erase(_uid);
    m_freeUids.push_back(_uid);
}

void UIDGenerator::Clear()
{
    m_allocatedUids.clear();
    m_freeUids.clear();
    nextUid = 0;
}

bool UIDGenerator::IsAllocated(const UID _uid) const
{
    return m_allocatedUids.contains(_uid);
}
}   // namespace jam