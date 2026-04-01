#include "pch.h"
#include "Blob.h"

namespace Jam
{

Blob AllocBlob(const SizeT _byteWidth)
{
    return Blob { new Byte[_byteWidth], _byteWidth };
}

}   // namespace Jam