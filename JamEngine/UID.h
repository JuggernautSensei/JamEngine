#pragma once

namespace jam
{

using UID                  = UInt32;
constexpr UID k_invalidUID = std::numeric_limits<UID>::max();

class UIDGenerator
{
public:
    [[nodiscard]] UID Alloc(); // 
    [[nodiscard]] UID Alloc(UID _uid);
    [[nodiscard]] UID AllocWithHint(UID _hint);

    [[nodiscard]] bool IsAllocated(UID _uid) const;
    void               Free(UID _uid);
    void               Clear();

private:
    UID          nextUID = 0;
    HashSet<UID> m_allocatedUIDs;
};

}   // namespace jam
