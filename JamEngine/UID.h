#pragma once
#include "Archive.h"

namespace Jam
{

class UID
{
    static constexpr UInt32 k_nullId = Limits<UInt32>::k_max;

public:
    constexpr UID()
        : value(k_nullId)
    {
    }

    explicit constexpr UID(const UInt32 _value) noexcept
        : value(_value)
    {
    }

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value != k_nullId;
    }

    /*implicit*/ constexpr operator bool() const noexcept
    {
        return IsValid();
    }

    [[nodiscard]] constexpr UInt32 GetValue() const noexcept
    {
        return value;
    }

    [[nodiscard]] constexpr bool operator==(const UID _uid) const
    {
        return value == _uid.value;
    }

    [[nodiscard]] constexpr bool operator!=(const UID _uid) const
    {
        return value != _uid.value;
    }

    static UID k_null;

private:
    UInt32 value = k_nullId;
};

void  WriteToArchive(ArchiveWriter& _archive, const UID& _uid);
Error ReadFromArchive(ArchiveReader& _archive, UID& _uid);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(UID);

}   // namespace Jam

template<>
struct std::hash<Jam::UID>
{
    size_t operator()(const Jam::UID _uid) const noexcept
    {
        return std::hash<Jam::UInt32>()(_uid.GetValue());
    }
};

template<>
struct ankerl::unordered_dense::hash<Jam::UID>
{
    size_t operator()(const Jam::UID _uid) const noexcept
    {
        return ankerl::unordered_dense::hash<Jam::UInt32>()(_uid.GetValue());
    }
};

template<>
struct fmt::formatter<Jam::UID> : fmt::formatter<Jam::UInt32>
{
    template<typename FormatContext>
    auto format(const Jam::UID& _uid, FormatContext& _ctx) const
    {
        return fmt::formatter<Jam::UInt32>::format(_uid.GetValue(), _ctx);
    }
};