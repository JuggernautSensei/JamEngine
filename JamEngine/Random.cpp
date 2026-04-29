#include "pch.h"
#include "Random.h"

namespace Jam
{
namespace Detail
{

    UInt64 MakeU64FromRd() noexcept
    {
        std::random_device rd;
        return (static_cast<UInt64>(rd()) << 32) | rd();
    }

}   // namespace Detail
}   // namespace Jam