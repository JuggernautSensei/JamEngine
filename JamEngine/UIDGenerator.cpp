#include "pch.h"
#include "UIDGenerator.h"
#include "UID.h"

namespace Jam
{

UID UIDGenerator::Alloc()
{
    UID uid = UID::k_invalid;
    if (m_freeUids.empty())
    {
        do
        {
            uid = UID { m_nextUidValue++ };
        } while (IsAllocated(uid));
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
    JAM_ASSERT(_uid != UID::k_invalid, "Cannot allocate invalid UID");
    JAM_ASSERT(m_allocatedUids.contains(_uid) == false, "UID {} is already allocated", _uid);
    m_allocatedUids.insert(_uid);
    return _uid;
}

UID UIDGenerator::Alloc(const UInt32 _uid)
{
    return Alloc(UID { _uid });
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

UID UIDGenerator::AllocWithHint(const UInt32 _hint)
{
    return AllocWithHint(UID { _hint });
}

bool UIDGenerator::IsAllocated(const UID _uid) const
{
    return m_allocatedUids.contains(_uid);
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
    m_nextUidValue = 0;
}

}   // namespace Jam