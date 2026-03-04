#include "pch.h"
#include "UIDAllocator.h"
#include "UID.h"

namespace Jam
{

UID UIDAllocator::Alloc()
{
    UID uid = UID::k_null;
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

UID UIDAllocator::Alloc(UID _uid)
{
    JAM_ASSERT(_uid != UID::k_null, "Cannot allocate invalid UID");
    JAM_ASSERT(m_allocatedUids.contains(_uid) == false, "UID {} is already allocated", _uid);
    m_allocatedUids.insert(_uid);
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
    return m_allocatedUids.contains(_uid);
}

void UIDAllocator::Free(UID _uid)
{
    JAM_ASSERT(IsAllocated(_uid), "UID {} is not allocated", _uid);
    m_allocatedUids.erase(_uid);
    m_freeUids.push_back(_uid);
}

void UIDAllocator::Clear()
{
    m_allocatedUids.clear();
    m_freeUids.clear();
    m_nextUidValue = 0;
}

}   // namespace Jam