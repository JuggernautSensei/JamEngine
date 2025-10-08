#pragma once

namespace jam
{

using UID                  = UInt32;
constexpr UID k_invalidUID = k_uint32Max;

class UIDGenerator
{
public:
    // 임의의 UID 생성
    JAM_NODISCARD UID Alloc()
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

    // 특정 UID 할당
    // 할당 실패시 assert 발생
    JAM_NODISCARD UID Alloc(UID _uid)
    {
        JAM_ASSERT(_uid != k_invalidUID, "Cannot allocate invalid UID");
        JAM_ASSERT(m_allocatedUIDs.contains(_uid) == false, "UID {} is already allocated", _uid);
        m_allocatedUIDs.insert(_uid);
        return _uid;
    }

    // 특정 UID 할당, 이미 할당된 경우 새로운 UID 생성
    JAM_NODISCARD UID AllocWithHint(const UID _hint)
    {
        if (IsAllocated(_hint))
        {
            return Alloc();
        }
        return Alloc(_hint);
    }

    // UID 반환
    void Free(UID _uid)
    {
        JAM_ASSERT(_uid != k_invalidUID, "Cannot free invalid UID");
        size_t numOfErase = m_allocatedUIDs.erase(_uid);
        JAM_ASSERT(numOfErase == 1, "UID {} is not allocated", _uid);
    }

    // 모든 UID 반환
    void Clear()
    {
        m_allocatedUIDs.clear();
        nextUID = 0;
    }

    JAM_NODISCARD bool IsAllocated(const UID _uid) const
    {
        return m_allocatedUIDs.contains(_uid);
    }

private:
    UID          nextUID = 0;
    HashSet<UID> m_allocatedUIDs;
};

}   // namespace jam
