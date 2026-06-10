#pragma once

namespace Jam
{

using Int8   = std::int8_t;
using Int16  = std::int16_t;
using Int32  = std::int32_t;
using Int64  = std::int64_t;
using UInt8  = std::uint8_t;
using UInt16 = std::uint16_t;
using UInt32 = std::uint32_t;
using UInt64 = std::uint64_t;
using Byte   = UInt8;
using SizeT  = std::ptrdiff_t;
using USizeT = std::make_unsigned_t<SizeT>;

using namespace std::chrono_literals;

}   // namespace Jam