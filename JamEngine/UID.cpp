#include "pch.h"
#include "UID.h"

namespace Jam
{

UID UID::k_invalid = UID::k_invalid;

void WriteToArchive(ArchiveWriter& _archive, const UID& _uid)
{
    _archive.Write(_uid.GetValue());
}

Error ReadFromArchive(ArchiveReader& _archive, UID& _uid)
{
    UInt32 uid = 0;
    if (const Error err = _archive.Read(uid))
    {
        return err;
    }

    _uid = UID { uid };
    return Error::None;
}

}   // namespace Jam