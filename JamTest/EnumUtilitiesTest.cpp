#include "pch.h"

using namespace Jam;

// Test enums for EnumUtilities tests
enum class eColor : UInt8
{
    Red    = 0,
    Green  = 1,
    Blue   = 2,
    Yellow = 3
};

enum class eStatus
{
    Idle    = 0,
    Running = 1,
    Paused  = 2,
    Stopped = 3,
    Error   = 4
};

enum class eSparseEnum
{
    First  = 0,
    Second = 1,
    Third  = 10,
    Fourth = 20
};

namespace NestedNamespace
{
    enum class eNestedEnum
    {
        Alpha = 0,
        Beta  = 1
    };
}   // namespace NestedNamespace

// --- 1. UnderlyingFrom: Convert enum to underlying type ---
TEST(EnumUtilitiesTest, UnderlyingFrom)
{
    EXPECT_EQ(UnderlyingFrom(eColor::Red), 0);
    EXPECT_EQ(UnderlyingFrom(eColor::Green), 1);
    EXPECT_EQ(UnderlyingFrom(eColor::Blue), 2);

    EXPECT_EQ(UnderlyingFrom(eStatus::Idle), 0);
    EXPECT_EQ(UnderlyingFrom(eStatus::Running), 1);
    EXPECT_EQ(UnderlyingFrom(eStatus::Error), 4);

    // Constexpr test
    static_assert(UnderlyingFrom(eColor::Red) == 0);
    static_assert(UnderlyingFrom(eStatus::Running) == 1);
}

// --- 2. IsValid: Check if enum value is valid ---
TEST(EnumUtilitiesTest, IsValidEnumValue)
{
    EXPECT_TRUE(IsValid(eColor::Red));
    EXPECT_TRUE(IsValid(eColor::Green));
    EXPECT_TRUE(IsValid(eColor::Blue));
    EXPECT_TRUE(IsValid(eColor::Yellow));

    EXPECT_TRUE(IsValid(eStatus::Idle));
    EXPECT_TRUE(IsValid(eStatus::Running));
    EXPECT_TRUE(IsValid(eStatus::Error));

    // Invalid values
    EXPECT_FALSE(IsValid(static_cast<eColor>(99)));
    EXPECT_FALSE(IsValid(static_cast<eStatus>(99)));
}

// --- 3. CountOf: Count number of enum values ---
TEST(EnumUtilitiesTest, CountOf)
{
    EXPECT_EQ(CountOf<eColor>(), 4);
    EXPECT_EQ(CountOf<eStatus>(), 5);
    EXPECT_EQ(CountOf<eSparseEnum>(), 4);
}

// --- 4. NameOf: Get enum value name as string ---
TEST(EnumUtilitiesTest, NameOfEnum)
{
    EXPECT_EQ(NameOf(eColor::Red), "Red");
    EXPECT_EQ(NameOf(eColor::Green), "Green");
    EXPECT_EQ(NameOf(eColor::Blue), "Blue");
    EXPECT_EQ(NameOf(eColor::Yellow), "Yellow");

    EXPECT_EQ(NameOf(eStatus::Idle), "Idle");
    EXPECT_EQ(NameOf(eStatus::Running), "Running");
    EXPECT_EQ(NameOf(eStatus::Paused), "Paused");
    EXPECT_EQ(NameOf(eStatus::Stopped), "Stopped");
    EXPECT_EQ(NameOf(eStatus::Error), "Error");

    EXPECT_EQ(NameOf(eSparseEnum::First), "First");
    EXPECT_EQ(NameOf(eSparseEnum::Second), "Second");
    EXPECT_EQ(NameOf(eSparseEnum::Third), "Third");
    EXPECT_EQ(NameOf(eSparseEnum::Fourth), "Fourth");

    EXPECT_EQ(FullNameOf(eColor::Red), "eColor::Red");
    EXPECT_EQ(FullNameOf(eColor::Green), "eColor::Green");
    EXPECT_EQ(FullNameOf(eColor::Blue), "eColor::Blue");
    EXPECT_EQ(FullNameOf(eColor::Yellow), "eColor::Yellow");
    EXPECT_EQ(FullNameOf(eStatus::Idle), "eStatus::Idle");
    EXPECT_EQ(FullNameOf(eStatus::Running), "eStatus::Running");
    EXPECT_EQ(FullNameOf(eStatus::Paused), "eStatus::Paused");
    EXPECT_EQ(FullNameOf(eStatus::Stopped), "eStatus::Stopped");
    EXPECT_EQ(FullNameOf(eStatus::Error), "eStatus::Error");
    EXPECT_EQ(FullNameOf(eSparseEnum::First), "eSparseEnum::First");
    EXPECT_EQ(FullNameOf(eSparseEnum::Second), "eSparseEnum::Second");
    EXPECT_EQ(FullNameOf(eSparseEnum::Third), "eSparseEnum::Third");
    EXPECT_EQ(FullNameOf(eSparseEnum::Fourth), "eSparseEnum::Fourth");

    EXPECT_EQ(NameOf(NestedNamespace::eNestedEnum::Alpha), "Alpha");
    EXPECT_EQ(NameOf(NestedNamespace::eNestedEnum::Beta), "Beta");
    EXPECT_EQ(FullNameOf(NestedNamespace::eNestedEnum::Alpha), "NestedNamespace::eNestedEnum::Alpha");
    EXPECT_EQ(FullNameOf(NestedNamespace::eNestedEnum::Beta), "NestedNamespace::eNestedEnum::Beta");
}

// --- 5. RangesOf: Get all enum values ---
TEST(EnumUtilitiesTest, RangesOf)
{
    const auto& colorRanges = RangesOf<eColor>();
    EXPECT_EQ(colorRanges.size(), 4);
    EXPECT_EQ(colorRanges[0], eColor::Red);
    EXPECT_EQ(colorRanges[1], eColor::Green);
    EXPECT_EQ(colorRanges[2], eColor::Blue);
    EXPECT_EQ(colorRanges[3], eColor::Yellow);

    const auto& statusRanges = RangesOf<eStatus>();
    EXPECT_EQ(statusRanges.size(), 5);
    EXPECT_EQ(statusRanges[0], eStatus::Idle);
    EXPECT_EQ(statusRanges[1], eStatus::Running);
    EXPECT_EQ(statusRanges[2], eStatus::Paused);
    EXPECT_EQ(statusRanges[3], eStatus::Stopped);
    EXPECT_EQ(statusRanges[4], eStatus::Error);
}

// --- 6. ParseAs: Parse string to enum ---
TEST(EnumUtilitiesTest, ParseAs)
{
    // Valid names
    auto red = ParseAs<eColor>("Red");
    EXPECT_TRUE(red.HasValue());
    EXPECT_EQ(red.GetValue(), eColor::Red);

    auto green = ParseAs<eColor>("Green");
    EXPECT_TRUE(green.HasValue());
    EXPECT_EQ(green.GetValue(), eColor::Green);

    auto idle = ParseAs<eStatus>("Idle");
    EXPECT_TRUE(idle.HasValue());
    EXPECT_EQ(idle.GetValue(), eStatus::Idle);

    auto error = ParseAs<eStatus>("Error");
    EXPECT_TRUE(error.HasValue());
    EXPECT_EQ(error.GetValue(), eStatus::Error);

    // Invalid names
    auto invalid1 = ParseAs<eColor>("InvalidColor");
    EXPECT_FALSE(invalid1.HasValue());

    auto invalid2 = ParseAs<eStatus>("InvalidStatus");
    EXPECT_FALSE(invalid2.HasValue());

    // Case sensitive
    auto lowercase = ParseAs<eColor>("red");
    EXPECT_FALSE(lowercase.HasValue());
}

// --- 7. ParseAs: Full name parsing ---
TEST(EnumUtilitiesTest, ParseAsFullName)
{
    // Full qualified names should also work
    auto red = ParseAs<eColor>("eColor::Red");
    EXPECT_TRUE(red.HasValue());
    EXPECT_EQ(red.GetValue(), eColor::Red);

    auto idle = ParseAs<eStatus>("eStatus::Idle");
    EXPECT_TRUE(idle.HasValue());
    EXPECT_EQ(idle.GetValue(), eStatus::Idle);
}

// --- 8. CastAs: Cast underlying value to enum ---
TEST(EnumUtilitiesTest, CastAs)
{
    // Valid casts
    auto red = CastAs<eColor>(0);
    EXPECT_TRUE(red.HasValue());
    EXPECT_EQ(red.GetValue(), eColor::Red);

    auto green = CastAs<eColor>(1);
    EXPECT_TRUE(green.HasValue());
    EXPECT_EQ(green.GetValue(), eColor::Green);

    auto idle = CastAs<eStatus>(0);
    EXPECT_TRUE(idle.HasValue());
    EXPECT_EQ(idle.GetValue(), eStatus::Idle);

    auto error = CastAs<eStatus>(4);
    EXPECT_TRUE(error.HasValue());
    EXPECT_EQ(error.GetValue(), eStatus::Error);

    // Invalid casts
    auto invalid1 = CastAs<eColor>(99);
    EXPECT_FALSE(invalid1.HasValue());

    auto invalid2 = CastAs<eStatus>(99);
    EXPECT_FALSE(invalid2.HasValue());
}

// --- 9. Sparse enum handling ---
TEST(EnumUtilitiesTest, SparseEnum)
{
    EXPECT_EQ(CountOf<eSparseEnum>(), 4);

    EXPECT_TRUE(IsValid(eSparseEnum::First));
    EXPECT_TRUE(IsValid(eSparseEnum::Second));
    EXPECT_TRUE(IsValid(eSparseEnum::Third));
    EXPECT_TRUE(IsValid(eSparseEnum::Fourth));

    // Check that gaps are invalid
    EXPECT_FALSE(IsValid(static_cast<eSparseEnum>(5)));
    EXPECT_FALSE(IsValid(static_cast<eSparseEnum>(15)));

    EXPECT_EQ(NameOf(eSparseEnum::Third), "Third");
    EXPECT_EQ(NameOf(eSparseEnum::Fourth), "Fourth");

    auto parsed = ParseAs<eSparseEnum>("Third");
    EXPECT_TRUE(parsed.HasValue());
    EXPECT_EQ(parsed.GetValue(), eSparseEnum::Third);
}

// --- 10. Constexpr tests ---
TEST(EnumUtilitiesTest, ConstexprOperations)
{
    // NameOf is constexpr
    constexpr auto redName = NameOf(eColor::Red);
    static_assert(redName == "Red");

    constexpr auto idleName = NameOf(eStatus::Idle);
    static_assert(idleName == "Idle");

    // IsValidEnumValue is constexpr
    static_assert(IsValid(eColor::Red));
    static_assert(!IsValid(static_cast<eColor>(99)));
}

// --- 11. Integration test: Enum iteration ---
TEST(EnumUtilitiesTest, EnumIteration)
{
    const auto& colors = RangesOf<eColor>();

    String result;
    for (const auto& color: colors)
    {
        if (!result.IsEmpty())
            result += ", ";
        result += NameOf(color);
    }

    EXPECT_EQ(result, "Red, Green, Blue, Yellow");

    const auto& statuses = RangesOf<eStatus>();
    String      statusResult;
    for (const auto& status: statuses)
    {
        if (!statusResult.IsEmpty())
            statusResult += ", ";
        statusResult += NameOf(status);
    }

    EXPECT_EQ(statusResult, "Idle, Running, Paused, Stopped, Error");
}
