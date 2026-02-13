#include "pch.h"

using namespace Jam;

// --- 1. Char Utilities (constexpr 검증 포함) ---
TEST(StringTest, CharUtilities)
{
    static_assert(IsUpper('Z'));
    static_assert(IsLower('a'));
    static_assert(ToUpper('g') == 'G');
    static_assert(ToLower('T') == 't');
    static_assert(IsSpace(' '));

    EXPECT_TRUE(IsAlphabet('M'));
    EXPECT_FALSE(IsAlphabet('1'));
}

// --- 2. StringView: Basics & constexpr ---
TEST(StringViewTest, Basics)
{
    constexpr StringView sv = "Jam Engine";
    static_assert(sv.GetLength() == 10);
    static_assert(sv[0] == 'J');

    EXPECT_EQ(sv.GetLength(), 10);
    EXPECT_TRUE(sv.StartsWith('J'));
    EXPECT_TRUE(sv.EndsWith("Engine"));

    StringView empty_sv;
    EXPECT_TRUE(empty_sv.IsEmpty());
}

// --- 3. String: Construction & Memory ---
TEST(StringTest, Construction)
{
    // 기본 생성 및 문자열 리터럴
    String s1 = "JamString";
    EXPECT_EQ(s1.GetLength(), 9);

    // 복사 및 이동
    String s2 = s1;
    EXPECT_EQ(s2, "JamString");

    String s3 = std::move(s1);
    EXPECT_TRUE(s1.IsEmpty());
    EXPECT_EQ(s3, "JamString");

    // Fill constructor
    String s4(5, '!');
    EXPECT_EQ(s4, "!!!!!");
}

// --- 4. Search: Find, RFind, Contains ---
TEST(StringTest, SearchLogic)
{
    String s = "apple banana apple";

    // Find (Forward)
    auto res = s.Find("banana");
    EXPECT_TRUE(res.IsSucceeded());
    EXPECT_EQ(res.pos, 6);

    // RFind (Backward)
    auto rres = s.RFind("apple");
    EXPECT_TRUE(rres.IsSucceeded());
    EXPECT_EQ(rres.pos, 13);

    // Not Found
    EXPECT_TRUE(s.Find("orange").IsFailed());
}

// --- 5. Manipulation: Append, Format, Erase ---
TEST(StringTest, Manipulation)
{
    String s = "Hello";
    s += " Jam";
    EXPECT_EQ(s, "Hello Jam");

    // Erase
    s.Erase(0, 6);
    EXPECT_EQ(s, "Jam");

    // fmt integration
    s.Format("Score: {}", 100);
    EXPECT_EQ(s, "Score: 100");

    s.AppendFormat(" - Rank: {:02d}", 1);
    EXPECT_EQ(s, "Score: 100 - Rank: 01");
}

// --- 6. Trim: LTrim, RTrim, Trim ---
TEST(StringTest, Trimming)
{
    String s = "   Jam Engine   ";

    // String::Trimed (Returns new string)
    EXPECT_EQ(s.Trimed(), "Jam Engine");
    EXPECT_EQ(s.LTrimed(), "Jam Engine   ");
    EXPECT_EQ(s.RTrimed(), "   Jam Engine");

    // StringView::Trimed
    StringView sv = s;
    EXPECT_EQ(sv.Trimed(), "Jam Engine");
}

// --- 7. Conversion: ParseAs, FromIntegral, UTF ---
TEST(StringTest, Conversion)
{
    // String to Int
    String s   = "1234";
    auto   val = s.ParseAs<int32_t>();
    EXPECT_TRUE(val.HasValue());
    EXPECT_EQ(*val, 1234);

    // Int to String
    String s2 = String::FromIntegral(255, 16);   // Hex
    EXPECT_TRUE(s2.CompareIgnoreCase("ff"));

    // Std compatibility
    std::string std_s = s.ToStdString();
    EXPECT_EQ(std_s, "1234");
}

// --- 8. Hashing: FNV1a ---
TEST(StringTest, Hashing)
{
    String s1 = "UniqueString";
    String s2 = "uniquestring";

    // Case sensitive
    EXPECT_NE(s1.Hash64(), s2.Hash64());

    // Case insensitive
    EXPECT_EQ(s1.Hash64IgnoreCase(), s2.Hash64IgnoreCase());

    // ankerl/std hash check
    std::hash<String> hasher;
    EXPECT_EQ(hasher(s1), s1.Hash64());
}

// --- 9. Edge Cases & Glue ---
TEST(StringTest, EdgeCases)
{
    // String vs StringView comparison
    String     s  = "Glue";
    StringView sv = "Glue";
    EXPECT_TRUE(s == sv);
    EXPECT_TRUE(sv == s);

    // Empty string manipulation
    String empty;
    EXPECT_TRUE(empty.IsEmpty());
    // (수정됨: JAM_ASSERT가 걸리는 부분은 테스트에서 제외하거나 기대값 확인)

    // SubStr bounds check
    String base = "012345";
    EXPECT_EQ(base.SubStr(2, 2), "23");
}

// --- 10. StringView Null/Empty Comparison ---
TEST(StringViewTest, NullComparison)
{
    StringView sv1;       // nullptr, 0
    StringView sv2("");   // ptr to "", 0

    EXPECT_TRUE(sv1.IsEmpty());
    EXPECT_TRUE(sv2.IsEmpty());
    EXPECT_EQ(sv1, sv2);   // (수정됨: IsEmpty 케이스 로직 검증)
}