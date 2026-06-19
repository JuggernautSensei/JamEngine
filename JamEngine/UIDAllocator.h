#pragma once
#include "UID.h"

namespace Jam
{

class UIDAllocator
{
public:
    [[nodiscard]] UID  Alloc();                       // auto alloc
    [[nodiscard]] UID  Alloc(UID _uid);               // named alloc
    [[nodiscard]] UID  Alloc(UInt32 _uid);            // named alloc
    [[nodiscard]] UID  AllocWithHint(UID _hint);      // try named alloc. if not available, auto alloc
    [[nodiscard]] UID  AllocWithHint(UInt32 _hint);   // try named alloc. if not available, auto alloc
    [[nodiscard]] bool IsAllocated(UID _uid) const;
    void               Free(UID _uid);
    void               Clear();

private:
    UInt32       m_nextUIDValue  = 0;
    Vector<UID>  m_freeUIDs      = {};
    HashSet<UID> m_allocatedUIDs = {};
};

}   // namespace Jam
