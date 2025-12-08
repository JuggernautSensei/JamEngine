#pragma once

namespace jam
{

using UID                  = UInt32;
constexpr UID k_invalidUID = std::numeric_limits<UID>::max();

class UIDGenerator
{
public:
    // 임의의 UID 생성
    [[nodiscard]] UID Alloc();

    // 특정 UID 할당
    // 할당 실패시 assert 발생
    [[nodiscard]] UID Alloc(UID _uid);

    // 특정 UID 할당, 이미 할당된 경우 새로운 UID 생성
    [[nodiscard]] UID AllocWithHint(const UID _hint);

    // UID 반환
    void Free(UID _uid);

    // 모든 UID 반환
    void Clear();

    [[nodiscard]] bool IsAllocated(const UID _uid) const;

private:
    UID          nextUID = 0;
    HashSet<UID> m_allocatedUIDs;
};

}   // namespace jam
