#pragma once

namespace jam
{

using UID                  = UInt32;
constexpr UID k_invalidUID = std::numeric_limits<UID>::max();

class UIDGenerator
{
public:
    [[nodiscard]] UID Alloc();                    // auto alloc
    [[nodiscard]] UID Alloc(UID _uid);            // named alloc
    [[nodiscard]] UID AllocWithHint(UID _hint);   // try named alloc. if not available, auto alloc

    [[nodiscard]] bool IsAllocated(UID _uid) const;
    void               Free(UID _uid);
    void               Clear();

private:
    UID          nextUid = 0;
    HashSet<UID> m_allocatedUids;
};

}   // namespace jam
