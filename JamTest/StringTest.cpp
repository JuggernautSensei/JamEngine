#include "pch.h"

using namespace Jam;

// ─────────────────────────────────────────────
// 헬퍼 매크로
// ─────────────────────────────────────────────
#define EXPECT_FOUND(cr, expectedPos)                                \
    EXPECT_TRUE((cr).IsSucceeded())                                  \
        << "Expected Found at " << (expectedPos) << " but IsFailed"; \
    EXPECT_EQ((cr).pos, static_cast<SizeT>(expectedPos))

#define EXPECT_NOT_FOUND(cr)     \
    EXPECT_TRUE((cr).IsFailed()) \
        << "Expected IsFailed but IsSucceeded at pos=" << (cr).pos

// SSO capacity: k_ssoBufSize - 1 = (32 - 1)/sizeof(char) - 1 = 30
static constexpr SizeT k_SSOCap = 30;

// =============================================================================
// BasicStringView — 생성 및 기본 접근
// =============================================================================

TEST(StringViewConstruct, Default_IsEmpty)
{
    StringView sv;
    EXPECT_TRUE(sv.IsEmpty());
    EXPECT_EQ(sv.GetLength(), 0u);
    EXPECT_NE(sv.GetPtr(), nullptr);
}

TEST(StringViewConstruct, FromCStr)
{
    StringView sv("hello");
    EXPECT_EQ(sv.GetLength(), 5u);
    EXPECT_FALSE(sv.IsEmpty());
    EXPECT_EQ(sv[0], 'h');
    EXPECT_EQ(sv[4], 'o');
}

TEST(StringViewConstruct, FromCStrAndLen)
{
    StringView sv("hello world", 5);
    EXPECT_EQ(sv.GetLength(), 5u);
    EXPECT_EQ(sv[4], 'o');
}

TEST(StringViewConstruct, FrontBack)
{
    StringView sv("abcde");
    EXPECT_EQ(sv.Front(), 'a');
    EXPECT_EQ(sv.Back(), 'e');
}

TEST(StringViewConstruct, GetTerminate)
{
    StringView sv("abc");
    EXPECT_EQ(sv.GetTerminate() - sv.GetPtr(), 3);
}

// ─────────────────────────────────────────────
// BasicStringView — 비교 연산자 / CompareEqual
// ─────────────────────────────────────────────

TEST(StringViewCompare, EqualOperator_SameContent)
{
    StringView a("hello"), b("hello");
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(StringViewCompare, EqualOperator_DifferentContent)
{
    StringView a("hello"), b("world");
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST(StringViewCompare, EqualOperator_DifferentLength)
{
    StringView a("hell"), b("hello");
    EXPECT_FALSE(a == b);
}

TEST(StringViewCompare, CompareEqual_Full)
{
    StringView sv("hello");
    EXPECT_TRUE(sv.CompareEqual(StringView("hello")));
    EXPECT_FALSE(sv.CompareEqual(StringView("hell")));
    EXPECT_FALSE(sv.CompareEqual(StringView("helloo")));
}

TEST(StringViewCompare, CompareEqual_IgnoreCase)
{
    StringView sv("Hello");
    EXPECT_TRUE(sv.CompareEqualIgnoreCase(StringView("hello"), 0));
    EXPECT_TRUE(sv.CompareEqualIgnoreCase(StringView("HELLO"), 0));
    EXPECT_FALSE(sv.CompareEqualIgnoreCase(StringView("world"), 0));
}

TEST(StringViewCompare, CompareEqual_WithOffset)
{
    // sv[2..] == "llo"
    StringView sv("hello");
    EXPECT_TRUE(sv.CompareEqual(StringView("llo"), 2));
    EXPECT_FALSE(sv.CompareEqual(StringView("llo"), 1));
}

TEST(StringViewCompare, CompareEqual_SingleChar)
{
    StringView sv("A");
    EXPECT_TRUE(sv.CompareEqual('A'));
    EXPECT_FALSE(sv.CompareEqual('a'));
    EXPECT_FALSE(sv.CompareEqual('B'));
}

// ─────────────────────────────────────────────
// BasicStringView — StartsWith / EndsWith
// ─────────────────────────────────────────────

TEST(StringViewStartsEndsWith, StartsWith_Char_True)
{
    EXPECT_TRUE(StringView("hello").StartsWith('h'));
}

TEST(StringViewStartsEndsWith, StartsWith_Char_False)
{
    EXPECT_FALSE(StringView("hello").StartsWith('e'));
}

TEST(StringViewStartsEndsWith, StartsWith_Substring_True)
{
    EXPECT_TRUE(StringView("hello world").StartsWith(StringView("hello")));
}

TEST(StringViewStartsEndsWith, StartsWith_Substring_False)
{
    EXPECT_FALSE(StringView("hello world").StartsWith(StringView("world")));
}


TEST(StringViewStartsEndsWith, EndsWith_Char_True)
{
    EXPECT_TRUE(StringView("hello").EndsWith('o'));
}

TEST(StringViewStartsEndsWith, EndsWith_Char_False)
{
    EXPECT_FALSE(StringView("hello").EndsWith('h'));
}

TEST(StringViewStartsEndsWith, EndsWith_Substring_True)
{
    EXPECT_TRUE(StringView("hello world").EndsWith(StringView("world")));
}

TEST(StringViewStartsEndsWith, EndsWith_Substring_False)
{
    EXPECT_FALSE(StringView("hello world").EndsWith(StringView("hello")));
}

// ─────────────────────────────────────────────
// BasicStringView — Find
// ─────────────────────────────────────────────

TEST(StringViewFind, Find_Char_First)
{
    EXPECT_FOUND(StringView("hello").Find('l'), 2u);
}

TEST(StringViewFind, Find_Char_NotFound)
{
    EXPECT_NOT_FOUND(StringView("hello").Find('z'));
}

TEST(StringViewFind, Find_Char_WithOffset)
{
    // 'l'은 2,3에 있고 offset=3부터 찾으면 3
    EXPECT_FOUND(StringView("hello").Find('l', 3), 3u);
}

TEST(StringViewFind, Find_Char_WithOffsetAndCount)
{
    // "hello world" 에서 offset=4, count=4 범위 "o wo" 에서 'o' → pos 4
    EXPECT_FOUND(StringView("hello world").Find('o', 4, 4), 4u);
}

TEST(StringViewFind, Find_Substring_Found)
{
    EXPECT_FOUND(StringView("hello world").Find(StringView("world")), 6u);
}

TEST(StringViewFind, Find_Substring_NotFound)
{
    EXPECT_NOT_FOUND(StringView("hello world").Find(StringView("xyz")));
}

TEST(StringViewFind, Find_Substring_WithOffset)
{
    // "abcabc" offset=1 이후 "abc" → pos 3
    EXPECT_FOUND(StringView("abcabc").Find(StringView("abc"), 1), 3u);
}

// ─────────────────────────────────────────────
// BasicStringView — RFind
// ─────────────────────────────────────────────

TEST(StringViewRFind, RFind_Char_Last)
{
    // "hello"에서 'l' 마지막 = 3
    EXPECT_FOUND(StringView("hello").RFind('l'), 3u);
}

TEST(StringViewRFind, RFind_Char_NotFound)
{
    EXPECT_NOT_FOUND(StringView("hello").RFind('z'));
}

TEST(StringViewRFind, RFind_Substring_Last)
{
    EXPECT_FOUND(StringView("abcabc").RFind(StringView("abc")), 3u);
}

TEST(StringViewRFind, RFind_Substring_NotFound)
{
    EXPECT_NOT_FOUND(StringView("hello").RFind(StringView("xyz")));
}

TEST(StringViewRFind, RFind_WithOffsetAndCount)
{
    // "abcabc" offset=0, count=4 → "abca" 범위에서 "abc" 마지막 → pos 0
    EXPECT_FOUND(StringView("abcabc").RFind(StringView("abc"), 0, 4), 0u);
}

// ─────────────────────────────────────────────
// BasicStringView — FindNotOf / RFindNotOf (신규)
// ─────────────────────────────────────────────

TEST(StringViewFindNotOf, FindNotOf_Char_First)
{
    // "aaabcd" 에서 'a'가 아닌 첫 위치 = 3
    EXPECT_FOUND(StringView("aaabcd").FindNotOf('a'), 3u);
}

TEST(StringViewFindNotOf, FindNotOf_Char_AllSame)
{
    EXPECT_NOT_FOUND(StringView("aaaa").FindNotOf('a'));
}

TEST(StringViewFindNotOf, FindNotOf_Char_WithOffset)
{
    // "aabaa" offset=2부터 → 'b' pos=2
    EXPECT_FOUND(StringView("aabaa").FindNotOf('a', 2), 2u);
}

TEST(StringViewFindNotOf, FindNotOf_Substring_Found)
{
    // "aabc" 에서 "a"가 아닌 첫 위치 = 2
    EXPECT_FOUND(StringView("aabc").FindNotOf(StringView("a")), 2u);
}

TEST(StringViewFindNotOf, FindNotOf_WithOffsetAndCount)
{
    // "aabba" offset=1, count=3 → "abb" 에서 'a'가 아닌 첫 위치 → pos 2
    EXPECT_FOUND(StringView("aabba").FindNotOf('a', 1, 3), 2u);
}

TEST(StringViewFindNotOf, RFindNotOf_Char_Last)
{
    // "bcdaaa" 에서 'a'가 아닌 마지막 위치 = 2 ('d')
    EXPECT_FOUND(StringView("bcdaaa").RFindNotOf('a'), 2u);
}

TEST(StringViewFindNotOf, RFindNotOf_Char_AllSame)
{
    EXPECT_NOT_FOUND(StringView("aaaa").RFindNotOf('a'));
}

TEST(StringViewFindNotOf, RFindNotOf_Substring_Found)
{
    // "bcaa" 에서 "a"가 아닌 마지막 → pos 1 ('c')
    EXPECT_FOUND(StringView("bcaa").RFindNotOf(StringView("a")), 1u);
}

TEST(StringViewFindNotOf, RFindNotOf_WithOffsetAndCount)
{
    // "aabba" offset=0, count=4 → "aabb" 에서 'a'가 아닌 마지막 → pos 3 ('b')
    EXPECT_FOUND(StringView("aabba").RFindNotOf('a', 0, 4), 3u);
}

// ─────────────────────────────────────────────
// BasicStringView — SubView
// ─────────────────────────────────────────────

TEST(StringViewSubView, FromPos)
{
    EXPECT_EQ(StringView("hello world").SubView(6), StringView("world"));
}

TEST(StringViewSubView, PosAndLen)
{
    EXPECT_EQ(StringView("hello world").SubView(0, 5), StringView("hello"));
}

TEST(StringViewSubView, ZeroLen)
{
    EXPECT_TRUE(StringView("hello").SubView(2, 0).IsEmpty());
}

TEST(StringViewSubView, FullRange)
{
    StringView sv("hello");
    EXPECT_EQ(sv.SubView(0, 5), sv);
}

TEST(StringViewSubView, AtEnd_IsEmpty)
{
    EXPECT_TRUE(StringView("hello").SubView(5).IsEmpty());
}

// ─────────────────────────────────────────────
// BasicStringView — Trim
// ─────────────────────────────────────────────

TEST(StringViewTrim, LTrimed_LeadingSpaces)
{
    EXPECT_EQ(StringView("   hello").LTrimmed(), StringView("hello"));
}

TEST(StringViewTrim, LTrimed_NoSpaces)
{
    EXPECT_EQ(StringView("hello").LTrimmed(), StringView("hello"));
}

TEST(StringViewTrim, LTrimed_AllSpaces)
{
    EXPECT_TRUE(StringView("   ").LTrimmed().IsEmpty());
}

TEST(StringViewTrim, RTrimed_TrailingSpaces)
{
    EXPECT_EQ(StringView("hello   ").RTrimmed(), StringView("hello"));
}

TEST(StringViewTrim, RTrimed_NoSpaces)
{
    EXPECT_EQ(StringView("hello").RTrimmed(), StringView("hello"));
}

TEST(StringViewTrim, RTrimed_AllSpaces)
{
    EXPECT_TRUE(StringView("   ").RTrimmed().IsEmpty());
}

TEST(StringViewTrim, Trimed_Both)
{
    EXPECT_EQ(StringView("  hello  ").Trimmed(), StringView("hello"));
}

TEST(StringViewTrim, Trimed_Tabs)
{
    EXPECT_EQ(StringView("\t\nhello\r\n").Trimmed(), StringView("hello"));
}

TEST(StringViewTrim, Trimed_Empty)
{
    EXPECT_TRUE(StringView("").Trimmed().IsEmpty());
}

// ─────────────────────────────────────────────
// BasicStringView — Hash
// ─────────────────────────────────────────────

TEST(StringViewHash, Hash32_Deterministic)
{
    EXPECT_EQ(StringView("hello").Hash32(), StringView("hello").Hash32());
}

TEST(StringViewHash, Hash64_Deterministic)
{
    EXPECT_EQ(StringView("hello").Hash64(), StringView("hello").Hash64());
}

TEST(StringViewHash, Hash32_DifferentStrings)
{
    EXPECT_NE(StringView("hello").Hash32(), StringView("world").Hash32());
}

TEST(StringViewHash, Hash64_DifferentStrings)
{
    EXPECT_NE(StringView("hello").Hash64(), StringView("world").Hash64());
}

TEST(StringViewHash, Hash32_IgnoreCase)
{
    EXPECT_EQ(StringView("HELLO").IgnoreCaseHash32(), StringView("hello").IgnoreCaseHash32());
    EXPECT_EQ(StringView("Hello").IgnoreCaseHash32(), StringView("hElLo").IgnoreCaseHash32());
}

TEST(StringViewHash, Hash64_IgnoreCase)
{
    EXPECT_EQ(StringView("WORLD").IgnoreCaseHash64(), StringView("world").IgnoreCaseHash64());
}

TEST(StringViewHash, EmptyString_NoCrash)
{
    EXPECT_NO_FATAL_FAILURE(StringView("").Hash32());
    EXPECT_NO_FATAL_FAILURE(StringView("").Hash64());
}

// ─────────────────────────────────────────────
// BasicStringView — 파일시스템
// ─────────────────────────────────────────────

TEST(StringViewFS, Decompose_FullPath)
{
    StringView sv("C:/foo/bar/baz.txt");
    StringView dir, fileName, baseName, ext;
    sv.Decompose(&dir, &fileName, &baseName, &ext);

    EXPECT_EQ(dir, StringView("C:/foo/bar/"));
    EXPECT_EQ(fileName, StringView("baz.txt"));
    EXPECT_EQ(baseName, StringView("baz"));
    EXPECT_EQ(ext, StringView(".txt"));
}

TEST(StringViewFS, Decompose_NoDir)
{
    StringView sv("file.txt");
    StringView dir, fileName, baseName, ext;
    sv.Decompose(&dir, &fileName, &baseName, &ext);

    EXPECT_TRUE(dir.IsEmpty());
    EXPECT_EQ(fileName, StringView("file.txt"));
    EXPECT_EQ(baseName, StringView("file"));
    EXPECT_EQ(ext, StringView(".txt"));
}

TEST(StringViewFS, Decompose_NoExt)
{
    StringView sv("dir/file");
    StringView dir, baseName, ext;
    sv.Decompose(&dir, nullptr, &baseName, &ext);

    EXPECT_EQ(dir, StringView("dir/"));
    EXPECT_EQ(baseName, StringView("file"));
    EXPECT_TRUE(ext.IsEmpty());
}

TEST(StringViewFS, Decompose_DirectoryPath)
{
    StringView sv("C:/foo/bar/");
    StringView dir, fileName;   
    sv.Decompose(&dir, &fileName, nullptr, nullptr);

    EXPECT_EQ(dir, StringView("C:/foo/"));
    EXPECT_EQ(fileName, StringView("bar"));
}

TEST(StringViewFS, Decompose_Empty)
{
    StringView sv("");
    StringView dir, fileName, baseName, ext;
    sv.Decompose(&dir, &fileName, &baseName, &ext);
    EXPECT_TRUE(dir.IsEmpty());
    EXPECT_TRUE(fileName.IsEmpty());
    EXPECT_TRUE(baseName.IsEmpty());
    EXPECT_TRUE(ext.IsEmpty());
}

TEST(StringViewFS, GetDirectory)
{
    EXPECT_EQ(StringView("C:/foo/bar.txt").GetDirectory(), StringView("C:/foo/"));
}

TEST(StringViewFS, GetFileName)
{
    EXPECT_EQ(StringView("C:/foo/bar.txt").GetFileName(), StringView("bar.txt"));
}

TEST(StringViewFS, GetBaseName)
{
    EXPECT_EQ(StringView("C:/foo/bar.txt").GetBaseName(), StringView("bar"));
}

TEST(StringViewFS, GetExtension)
{
    EXPECT_EQ(StringView("C:/foo/bar.txt").GetExtension(), StringView(".txt"));
}

TEST(StringViewFS, IsAbsolute_Drive)
{
    EXPECT_TRUE(StringView("C:/foo").IsAbsolute());
}

TEST(StringViewFS, IsAbsolute_UnixRoot)
{
    EXPECT_TRUE(StringView("/foo").IsAbsolute());
}

TEST(StringViewFS, IsRelative)
{
    EXPECT_TRUE(StringView("foo/bar").IsRelative());
    EXPECT_TRUE(StringView("../foo").IsRelative());
}

TEST(StringViewFS, IsDirectory_True)
{
    EXPECT_TRUE(StringView("C:/foo/bar/").IsDirectory());
}

TEST(StringViewFS, IsDirectory_False)
{
    EXPECT_FALSE(StringView("C:/foo/bar").IsDirectory());
}

TEST(StringViewFS, IsFile_True)
{
    EXPECT_TRUE(StringView("foo.txt").IsFile());
}

TEST(StringViewFS, IsFile_False)
{
    EXPECT_FALSE(StringView("foo/").IsFile());
}

TEST(StringViewFS, MatchExtension_WithDot)
{
    EXPECT_TRUE(StringView("foo.txt").MatchExtension(StringView(".txt")));
    EXPECT_FALSE(StringView("foo.txt").MatchExtension(StringView(".png")));
}

TEST(StringViewFS, MatchExtension_WithoutDot)
{
    EXPECT_TRUE(StringView("foo.txt").MatchExtension(StringView("txt")));
}

TEST(StringViewFS, MatchExtension_IgnoreCase)
{
    EXPECT_TRUE(StringView("foo.TXT").MatchExtension(StringView(".txt")));
}

TEST(StringViewFS, IsBasePathOf_True)
{
    EXPECT_TRUE(StringView("C:/foo/").IsBasePathOf(StringView("C:/foo/bar/baz.txt")));
}

TEST(StringViewFS, IsBasePathOf_False)
{
    EXPECT_FALSE(StringView("C:/other/").IsBasePathOf(StringView("C:/foo/bar.txt")));
}

// ─────────────────────────────────────────────
// BasicStringView — ParseAs
// ─────────────────────────────────────────────

TEST(StringViewParse, Int32_Positive)
{
    auto r = StringView("12345").ParseAs<Int32>();
    ASSERT_TRUE(r.HasValue());
    EXPECT_EQ(*r, 12345);
}

TEST(StringViewParse, Int32_Negative)
{
    auto r = StringView("-9999").ParseAs<Int32>();
    ASSERT_TRUE(r.HasValue());
    EXPECT_EQ(*r, -9999);
}

TEST(StringViewParse, UInt64_Max)
{
    auto r = StringView("18446744073709551615").ParseAs<UInt64>();
    ASSERT_TRUE(r.HasValue());
    EXPECT_EQ(*r, UINT64_MAX);
}

TEST(StringViewParse, Hex)
{
    auto r = StringView("FF").ParseAs<UInt32>(16);
    ASSERT_TRUE(r.HasValue());
    EXPECT_EQ(*r, 255u);
}

TEST(StringViewParse, Float)
{
    auto r = StringView("3.14").ParseAs<float>();
    ASSERT_TRUE(r.HasValue());
    EXPECT_NEAR(*r, 3.14f, 0.001f);
}

TEST(StringViewParse, Double)
{
    auto r = StringView("2.718281828").ParseAs<double>();
    ASSERT_TRUE(r.HasValue());
    EXPECT_NEAR(*r, 2.718281828, 1e-9);
}

// ─────────────────────────────────────────────
// BasicStringView — IteratorBase / std glue
// ─────────────────────────────────────────────

TEST(StringViewStdGlue, RangeFor)
{
    std::string collected;
    for (char c: StringView("abc"))
        collected += c;
    EXPECT_EQ(collected, "abc");
}

TEST(StringViewStdGlue, ToStdString)
{
    EXPECT_EQ(StringView("hello").ToStdString(), std::string("hello"));
}

TEST(StringViewStdGlue, ToStdStringView)
{
    EXPECT_EQ(StringView("hello").ToStdStringView(), std::string_view("hello"));
}

// =============================================================================
// BasicString — 생성 / 소멸
// =============================================================================

TEST(StringConstruct, Default_IsEmpty)
{
    String s;
    EXPECT_TRUE(s.IsEmpty());
    EXPECT_EQ(s.GetLength(), 0u);
    EXPECT_NE(s.GetPtr(), nullptr);
    EXPECT_EQ(s.GetPtr()[0], '\0');
}

TEST(StringConstruct, FromCStr)
{
    String s("hello");
    EXPECT_EQ(s.GetLength(), 5u);
    EXPECT_EQ(s, StringView("hello"));
}

TEST(StringConstruct, FromCStrAndLen)
{
    String s("hello world", 5);
    EXPECT_EQ(s.GetLength(), 5u);
    EXPECT_EQ(s, StringView("hello"));
}

TEST(StringConstruct, FromCountAndChar)
{
    String s(5, 'x');
    EXPECT_EQ(s, StringView("xxxxx"));
}

TEST(StringConstruct, FromStringView)
{
    StringView sv("world");
    String     s(sv);
    EXPECT_EQ(s, sv);
}

TEST(StringConstruct, CopyConstruct_Independent)
{
    String s1("hello");
    String s2(s1);
    EXPECT_EQ(s1, s2);
    s2.Append("!");
    EXPECT_NE(s1, s2);
}

TEST(StringConstruct, MoveConstruct)
{
    String s1("hello");
    String s2(std::move(s1));
    EXPECT_EQ(s2, StringView("hello"));
    EXPECT_TRUE(s1.IsEmpty());
}

TEST(StringConstruct, CopyAssign)
{
    String s1("hello"), s2;
    s2 = s1;
    EXPECT_EQ(s2, StringView("hello"));
}

TEST(StringConstruct, MoveAssign)
{
    String s1("hello"), s2;
    s2 = std::move(s1);
    EXPECT_EQ(s2, StringView("hello"));
    EXPECT_TRUE(s1.IsEmpty());
}

TEST(StringConstruct, SelfCopyAssign_Safe)
{
    String s("hello");
    s = s;
    EXPECT_EQ(s, StringView("hello"));
}

TEST(StringConstruct, AssignFromStringView)
{
    String s;
    s = StringView("world");
    EXPECT_EQ(s, StringView("world"));
}

TEST(StringConstruct, AssignFromChar)
{
    String s;
    s = 'X';
    EXPECT_EQ(s, StringView("X"));
}

// ─────────────────────────────────────────────
// BasicString — SSO 경계
// ─────────────────────────────────────────────

TEST(StringSSO, ExactlyAtCapacity_StaysSSO)
{
    std::string data(k_SSOCap, 'a');
    String      s(data.c_str());
    EXPECT_EQ(s.GetLength(), k_SSOCap);
    EXPECT_EQ(s.GetCapacity(), k_SSOCap);
    EXPECT_EQ(s.ToStdString(), data);
}

TEST(StringSSO, OneBeyondCapacity_GoesHeap)
{
    std::string data(k_SSOCap + 1, 'b');
    String      s(data.c_str());
    EXPECT_EQ(s.GetLength(), k_SSOCap + 1);
    EXPECT_GT(s.GetCapacity(), k_SSOCap);
    EXPECT_EQ(s.ToStdString(), data);
}

TEST(StringSSO, SSOToHeap_OnAppend)
{
    String s(std::string(k_SSOCap, 'c').c_str());
    s.Append('!');
    EXPECT_EQ(s.GetLength(), k_SSOCap + 1u);
    EXPECT_EQ(s.Back(), '!');
}

TEST(StringSSO, HeapGrowth_100Appends)
{
    String s;
    for (int i = 0; i < 100; ++i)
        s.Append('x');
    EXPECT_EQ(s.GetLength(), 100u);
    for (SizeT i = 0; i < 100; ++i)
        EXPECT_EQ(s[i], 'x');
}

// ─────────────────────────────────────────────
// BasicString — RBSet / WriteElement / Insert / Erase
// ─────────────────────────────────────────────

TEST(StringMutation, Set_Overwrites)
{
    String s("hello");
    s.Set("world");
    EXPECT_EQ(s, StringView("world"));
}

TEST(StringMutation, Set_WithLen)
{
    String s;
    s.Set("hello world", 5);
    EXPECT_EQ(s, StringView("hello"));
}

TEST(StringMutation, Set_SingleChar)
{
    String s("old");
    s.Set('A');
    EXPECT_EQ(s, StringView("A"));
}

TEST(StringMutation, Append_CStr)
{
    String s("hello");
    s.Append(" world");
    EXPECT_EQ(s, StringView("hello world"));
}

TEST(StringMutation, Append_Char)
{
    String s("hello");
    s.Append('!');
    EXPECT_EQ(s, StringView("hello!"));
}

TEST(StringMutation, Append_StringView)
{
    String s("foo");
    s.Append(StringView("bar"));
    EXPECT_EQ(s, StringView("foobar"));
}

TEST(StringMutation, Append_Multiple_CrossSSO)
{
    String      s;
    std::string expected;
    for (int i = 0; i < 10; ++i)
    {
        s.Append("hello");
        expected += "hello";
    }
    EXPECT_EQ(s.ToStdString(), expected);
}

TEST(StringMutation, Insert_AtBeginning)
{
    String s("world");
    s.Insert(0, "hello ");
    EXPECT_EQ(s, StringView("hello world"));
}

TEST(StringMutation, Insert_AtMiddle)
{
    String s("helloworld");
    s.Insert(5, " ");
    EXPECT_EQ(s, StringView("hello world"));
}

TEST(StringMutation, Insert_AtEnd)
{
    String s("hello");
    s.Insert(5, "!");
    EXPECT_EQ(s, StringView("hello!"));
}

TEST(StringMutation, Insert_Char)
{
    String s("helo");
    s.Insert(3, 'l');
    EXPECT_EQ(s, StringView("hello"));
}

TEST(StringMutation, Insert_InHeapMode)
{
    std::string data(k_SSOCap + 5, 'a');
    String      s(data.c_str());
    s.Insert(k_SSOCap / 2, "XXX");
    std::string expected = data;
    expected.insert(k_SSOCap / 2, "XXX");
    EXPECT_EQ(s.ToStdString(), expected);
}

TEST(StringMutation, Erase_Middle)
{
    String s("hello world");
    s.Erase(5, 6);
    EXPECT_EQ(s, StringView("hello"));
}

TEST(StringMutation, Erase_Beginning)
{
    String s("hello");
    s.Erase(0, 2);
    EXPECT_EQ(s, StringView("llo"));
}

TEST(StringMutation, Erase_End)
{
    String s("hello!");
    s.Erase(5, 1);
    EXPECT_EQ(s, StringView("hello"));
}

TEST(StringMutation, Erase_InHeapMode)
{
    std::string data(k_SSOCap + 10, 'z');
    String      s(data.c_str());
    s.Erase(5, 3);
    std::string expected = data;
    expected.erase(5, 3);
    EXPECT_EQ(s.ToStdString(), expected);
}

// ─────────────────────────────────────────────
// BasicString — Clear / Reserve / Resize
// ─────────────────────────────────────────────

TEST(StringResize, Clear_BecomesEmpty)
{
    String s("hello");
    s.Clear();
    EXPECT_TRUE(s.IsEmpty());
    EXPECT_EQ(s.GetPtr()[0], '\0');
}

TEST(StringResize, Reserve_SSO)
{
    String s("hi");
    s.Reserve(10);
    EXPECT_GE(s.GetCapacity(), 10u);
    EXPECT_EQ(s, StringView("hi"));
}

TEST(StringResize, Reserve_ForcesHeap)
{
    String s("hi");
    s.Reserve(k_SSOCap + 10);
    EXPECT_GE(s.GetCapacity(), k_SSOCap + 10);
    EXPECT_EQ(s, StringView("hi"));
}

TEST(StringResize, Reserve_Smaller_NoOp)
{
    String s("hello");
    SizeT  cap = s.GetCapacity();
    s.Reserve(1);
    EXPECT_GE(s.GetCapacity(), cap);
    EXPECT_EQ(s, StringView("hello"));
}

TEST(StringResize, Resize_Grow_DefaultFill)
{
    String s("hi");
    s.Resize(5);
    EXPECT_EQ(s.GetLength(), 5u);
    EXPECT_EQ(s[0], 'h');
    EXPECT_EQ(s[1], 'i');
    EXPECT_EQ(s[2], '\0');
}

TEST(StringResize, Resize_Grow_WithChar)
{
    String s("hi");
    s.Resize(5, 'x');
    EXPECT_EQ(s, StringView("hixxx"));
}

TEST(StringResize, Resize_Shrink)
{
    String s("hello");
    s.Resize(3);
    EXPECT_EQ(s, StringView("hel"));
    EXPECT_EQ(s.GetPtr()[3], '\0');
}

TEST(StringResize, Resize_ToZero)
{
    String s("hello");
    s.Resize(0);
    EXPECT_TRUE(s.IsEmpty());
}

// ─────────────────────────────────────────────
// BasicString — 연산자
// ─────────────────────────────────────────────

TEST(StringOperator, PlusEqual_StringView)
{
    String s("hello");
    s += StringView(" world");
    EXPECT_EQ(s, StringView("hello world"));
}

TEST(StringOperator, PlusEqual_Char)
{
    String s("hello");
    s += '!';
    EXPECT_EQ(s, StringView("hello!"));
}

TEST(StringOperator, Plus_StringView_OriginalUnchanged)
{
    String s("hello");
    String r = s + StringView(" world");
    EXPECT_EQ(r, StringView("hello world"));
    EXPECT_EQ(s, StringView("hello"));
}

TEST(StringOperator, Plus_Char)
{
    String s("hello");
    EXPECT_EQ(s + '!', StringView("hello!"));
}

TEST(StringOperator, Equal_StringView)
{
    String s("hello");
    EXPECT_TRUE(s == StringView("hello"));
    EXPECT_FALSE(s == StringView("world"));
}

TEST(StringOperator, NotEqual)
{
    EXPECT_TRUE(String("hello") != StringView("world"));
}

TEST(StringOperator, Index_Read)
{
    String s("hello");
    EXPECT_EQ(s[0], 'h');
    EXPECT_EQ(s[4], 'o');
}

TEST(StringOperator, Index_Write)
{
    String s("hello");
    s[0] = 'H';
    EXPECT_EQ(s, StringView("Hello"));
}

TEST(StringOperator, FrontBack_ReadWrite)
{
    String s("hello");
    EXPECT_EQ(s.Front(), 'h');
    EXPECT_EQ(s.Back(), 'o');
    s.Back() = '!';
    EXPECT_EQ(s, StringView("hell!"));
}

TEST(StringOperator, ImplicitConvertToStringView)
{
    String     s("hello");
    StringView sv = s;
    EXPECT_EQ(sv, StringView("hello"));
}

// ─────────────────────────────────────────────
// BasicString — Format
// ─────────────────────────────────────────────

TEST(StringFormat, Format_Basic)
{
    String s;
    s.Format("Hello {}!", "world");
    EXPECT_EQ(s, StringView("Hello world!"));
}

TEST(StringFormat, Format_Numbers)
{
    String s;
    s.Format("{} + {} = {}", 1, 2, 3);
    EXPECT_EQ(s, StringView("1 + 2 = 3"));
}

TEST(StringFormat, Format_OverwritesPrevious)
{
    String s("old");
    s.Format("new {}", "content");
    EXPECT_EQ(s, StringView("new content"));
}

TEST(StringFormat, AppendFormat)
{
    String s("start ");
    s.AppendFormat("mid {}", 42);
    EXPECT_EQ(s, StringView("start mid 42"));
}

TEST(StringFormat, Formatted_Static)
{
    EXPECT_EQ(String::Formatted("{}-{}-{}", "a", "b", "c"), StringView("a-b-c"));
}

// ─────────────────────────────────────────────
// BasicString — ConvertedFrom / ParseAs
// ─────────────────────────────────────────────

TEST(StringConvert, ConvertedFrom_Int)
{
    EXPECT_EQ(String::ConvertedFrom(42), StringView("42"));
    EXPECT_EQ(String::ConvertedFrom(-1), StringView("-1"));
    EXPECT_EQ(String::ConvertedFrom(0), StringView("0"));
}

TEST(StringConvert, ConvertedFrom_Float_ContainsDot)
{
    String s = String::ConvertedFrom(3.14f);
    EXPECT_FALSE(s.IsEmpty());
    EXPECT_TRUE(s.Find('.').IsSucceeded());
}

TEST(StringConvert, ParseAs_Int32)
{
    auto r = String("12345").ParseAs<Int32>();
    ASSERT_TRUE(r.HasValue());
    EXPECT_EQ(*r, 12345);
}

TEST(StringConvert, ParseAs_Negative)
{
    auto r = String("-42").ParseAs<Int32>();
    ASSERT_TRUE(r.HasValue());
    EXPECT_EQ(*r, -42);
}

TEST(StringConvert, ParseAs_Float)
{
    auto r = String("1.5").ParseAs<float>();
    ASSERT_TRUE(r.HasValue());
    EXPECT_NEAR(*r, 1.5f, 0.0001f);
}

TEST(StringConvert, ParseAs_HexBase)
{
    auto r = String("FF").ParseAs<UInt32>(16);
    ASSERT_TRUE(r.HasValue());
    EXPECT_EQ(*r, 255u);
}

// ─────────────────────────────────────────────
// BasicString — Find/FindNotOf 위임 확인
// ─────────────────────────────────────────────

TEST(StringFindDelegate, Find_Char)
{
    String s("hello world");
    EXPECT_FOUND(s.Find('o'), 4u);
    EXPECT_NOT_FOUND(s.Find('z'));
}

TEST(StringFindDelegate, RFind_Char)
{
    EXPECT_FOUND(String("hello").RFind('l'), 3u);
}

TEST(StringFindDelegate, FindNotOf_Char)
{
    EXPECT_FOUND(String("aaabcd").FindNotOf('a'), 3u);
}

TEST(StringFindDelegate, FindNotOf_AllSame)
{
    EXPECT_NOT_FOUND(String("aaaa").FindNotOf('a'));
}

TEST(StringFindDelegate, RFindNotOf_Char)
{
    EXPECT_FOUND(String("bcdaaa").RFindNotOf('a'), 2u);
}

TEST(StringFindDelegate, StartsWith_Substring)
{
    String s("hello world");
    EXPECT_TRUE(s.StartsWith(StringView("hello")));
    EXPECT_FALSE(s.StartsWith(StringView("world")));
}

TEST(StringFindDelegate, EndsWith_Substring)
{
    String s("hello world");
    EXPECT_TRUE(s.EndsWith(StringView("world")));
    EXPECT_FALSE(s.EndsWith(StringView("hello")));
}

TEST(StringFindDelegate, CompareEqual_CaseSensitive)
{
    String s("hello");
    EXPECT_TRUE(s.CompareEqual(StringView("hello")));
    EXPECT_FALSE(s.CompareEqual(StringView("HELLO")));
}

TEST(StringFindDelegate, CompareEqual_IgnoreCase)
{
    EXPECT_TRUE(String("hello").CompareEqualIgnoreCase(StringView("HELLO"), 0));
}

// ─────────────────────────────────────────────
// BasicString — 파일시스템
// ─────────────────────────────────────────────

TEST(StringFS, GetDirectory)
{
    EXPECT_EQ(String("C:/foo/bar.txt").GetDirectory(), StringView("C:/foo/"));
}

TEST(StringFS, GetFileName)
{
    EXPECT_EQ(String("C:/foo/bar.txt").GetFileName(), StringView("bar.txt"));
}

TEST(StringFS, GetBaseName)
{
    EXPECT_EQ(String("C:/foo/bar.txt").GetBaseName(), StringView("bar"));
}

TEST(StringFS, GetExtension)
{
    EXPECT_EQ(String("C:/foo/bar.txt").GetExtension(), StringView(".txt"));
}

TEST(StringFS, IsAbsolute_Drive)
{
    EXPECT_TRUE(String("C:/foo").IsAbsolute());
}

TEST(StringFS, IsAbsolute_UnixRoot)
{
    EXPECT_TRUE(String("/foo").IsAbsolute());
}

TEST(StringFS, IsRelative)
{
    EXPECT_TRUE(String("foo/bar").IsRelative());
}

TEST(StringFS, IsDirectory_IsFile)
{
    EXPECT_TRUE(String("C:/foo/").IsDirectory());
    EXPECT_TRUE(String("C:/foo/bar.txt").IsFile());
}

TEST(StringFS, MatchExtension_CaseInsensitive)
{
    EXPECT_TRUE(String("image.PNG").MatchExtension(StringView(".png")));
}

TEST(StringFS, ReplaceExtension_Existing)
{
    String s("C:/foo/bar.txt");
    s.ReplaceExtension(StringView(".md"));
    EXPECT_EQ(s, StringView("C:/foo/bar.md"));
}

TEST(StringFS, ReplaceExtension_NoDotPrefix)
{
    String s("C:/foo/bar.txt");
    s.ReplaceExtension(StringView("log"));
    EXPECT_EQ(s, StringView("C:/foo/bar.log"));
}

TEST(StringFS, ReplaceExtension_NoExtension)
{
    String s("C:/foo/bar");
    s.ReplaceExtension(StringView(".bin"));
    EXPECT_EQ(s, StringView("C:/foo/bar.bin"));
}

TEST(StringFS, ReplaceBaseName)
{
    String s("C:/foo/bar.txt");
    s.ReplaceBaseName(StringView("baz"));
    EXPECT_EQ(s, StringView("C:/foo/baz.txt"));
}

TEST(StringFS, ReplaceFileName)
{
    String s("C:/foo/bar.txt");
    s.ReplaceFileName(StringView("newname.log"));
    EXPECT_EQ(s, StringView("C:/foo/newname.log"));
}

TEST(StringFS, Simplify_DotDot)
{
    String s("a/b/../c");
    s.Simplify();
    EXPECT_EQ(s, StringView("a/c"));
}

TEST(StringFS, Simplify_SingleDot)
{
    String s("a/./b");
    s.Simplify();
    EXPECT_EQ(s, StringView("a/b"));
}

TEST(StringFS, Simplify_MultipleDotDot)
{
    String s("a/b/c/../../d");
    s.Simplify();
    EXPECT_EQ(s, StringView("a/d"));
}

TEST(StringFS, Simplify_DriveLetterUppercase)
{
    String s("c:/foo/bar");
    s.Simplify();
    EXPECT_EQ(s[0], 'C');
}

// ─────────────────────────────────────────────
// BasicString — Join
// v2: Auto는 base의 IsDirectory 여부로 판별
// ─────────────────────────────────────────────

TEST(StringJoin, File_NoTrailingSlash)
{
    String s("C:/aaa/bbb");
    s.Join(StringView("ccc"), eJoinType::File);
    EXPECT_EQ(s, StringView("C:/aaa/bbb/ccc"));
}

TEST(StringJoin, Directory_AddsTrailingSlash)
{
    String s("C:/aaa/bbb");
    s.Join(StringView("ccc"), eJoinType::Directory);
    EXPECT_EQ(s, StringView("C:/aaa/bbb/ccc/"));
}

TEST(StringJoin, Auto_BaseIsFile_TreatedAsFile)
{
    String s("C:/aaa/bbb");   // 파일 → Auto → File
    s.Join(StringView("ccc"), eJoinType::Auto);
    EXPECT_EQ(s, StringView("C:/aaa/bbb/ccc"));
}

TEST(StringJoin, Auto_BaseIsDirectory_TreatedAsDirectory)
{
    String s("C:/aaa/bbb/");   // 디렉터리 → Auto → Directory
    s.Join(StringView("ccc"), eJoinType::Directory);
    EXPECT_EQ(s, StringView("C:/aaa/bbb/ccc/"));
}

TEST(StringJoin, AlreadyHasSeparator_NoDouble)
{
    String s("C:/aaa/");
    s.Join(StringView("bbb"), eJoinType::File);
    EXPECT_EQ(s, StringView("C:/aaa/bbb"));
}

TEST(StringJoin, Part_StartsWithSeparator_NoDouble)
{
    String s("C:/aaa");
    s.Join(StringView("/bbb"), eJoinType::File);
    EXPECT_EQ(s, StringView("C:/aaa/bbb"));
}

TEST(StringJoin, EmptyBase_SetsToStr)
{
    String s;
    s.Join(StringView("ccc"), eJoinType::File);
    EXPECT_EQ(s, StringView("ccc"));
}

TEST(StringJoin, Joined_Static)
{
    String r = String::Joined(StringView("C:/aaa"), StringView("bbb"), eJoinType::File);
    EXPECT_EQ(r, StringView("C:/aaa/bbb"));
}

// ─────────────────────────────────────────────
// BasicString — Hash
// ─────────────────────────────────────────────

TEST(StringHash, Hash32_Deterministic)
{
    EXPECT_EQ(String("hello").Hash32(), String("hello").Hash32());
}

TEST(StringHash, Hash64_Deterministic)
{
    EXPECT_EQ(String("hello").Hash64(), String("hello").Hash64());
}

TEST(StringHash, Hash64_DifferentStrings)
{
    EXPECT_NE(String("hello").Hash64(), String("world").Hash64());
}

TEST(StringHash, Hash64_IgnoreCase)
{
    EXPECT_EQ(String("HELLO").IgnoreCaseHash64(), String("hello").IgnoreCaseHash64());
}

TEST(StringHash, ConsistentWithStringView)
{
    EXPECT_EQ(String("hello").Hash64(), StringView("hello").Hash64());
}

// ─────────────────────────────────────────────
// BasicString — IteratorBase / std glue
// ─────────────────────────────────────────────

TEST(StringStdGlue, RangeFor)
{
    std::string collected;
    for (char c: String("abc"))
        collected += c;
    EXPECT_EQ(collected, "abc");
}

TEST(StringStdGlue, push_back)
{
    String s("hello");
    s.push_back('!');
    EXPECT_EQ(s, StringView("hello!"));
}

TEST(StringStdGlue, ToStdString)
{
    EXPECT_EQ(String("hello").ToStdString(), std::string("hello"));
}

TEST(StringStdGlue, ToStdStringView)
{
    EXPECT_EQ(String("hello").ToStdStringView(), std::string_view("hello"));
}

TEST(StringStdGlue, ToStringView_Offset)
{
    EXPECT_EQ(String("hello world").ToStringView(6), StringView("world"));
}

TEST(StringStdGlue, ToStringView_OffsetAndLen)
{
    EXPECT_EQ(String("hello world").ToStringView(0, 5), StringView("hello"));
}

// ─────────────────────────────────────────────
// std::hash 특수화
// ─────────────────────────────────────────────

TEST(StdHash, String_InUnorderedMap)
{
    std::unordered_map<String, int> m;
    m[String("key1")] = 1;
    m[String("key2")] = 2;
    EXPECT_EQ(m[String("key1")], 1);
    EXPECT_EQ(m[String("key2")], 2);
    EXPECT_EQ(m.count(String("nope")), 0u);
}

TEST(StdHash, StringView_InUnorderedMap)
{
    std::unordered_map<StringView, int> m;
    const char*                         k1 = "key1";
    const char*                         k2 = "key2";
    m[StringView(k1)]                      = 10;
    m[StringView(k2)]                      = 20;
    EXPECT_EQ(m[StringView(k1)], 10);
    EXPECT_EQ(m[StringView(k2)], 20);
}

// ─────────────────────────────────────────────
// fmt::formatter 특수화
// ─────────────────────────────────────────────

TEST(FmtFormatter, FormatString)
{
    EXPECT_EQ(fmt::format("hello {}", String("world")), "hello world");
}

TEST(FmtFormatter, FormatStringView)
{
    EXPECT_EQ(fmt::format("hello {}", StringView("world")), "hello world");
}

// ─────────────────────────────────────────────
// BasicString — Case Conversion
// ─────────────────────────────────────────────

TEST(StringCaseConversion, ToCamelCase_FromSpaceSeparated)
{
    EXPECT_EQ(String("hello world").ToCamelCase(), StringView("helloWorld"));
    EXPECT_EQ(String("foo bar baz").ToCamelCase(), StringView("fooBarBaz"));
}

TEST(StringCaseConversion, ToCamelCase_FromSnakeCase)
{
    EXPECT_EQ(String("hello_world").ToCamelCase(), StringView("hello_world"));
}

TEST(StringCaseConversion, ToCamelCase_MultipleSpaces)
{
    EXPECT_EQ(String("hello   world").ToCamelCase(), StringView("helloWorld"));
}

TEST(StringCaseConversion, ToCamelCase_LeadingTrailingSpaces)
{
    EXPECT_EQ(String("  hello world  ").ToCamelCase(), StringView("helloWorld"));
}

TEST(StringCaseConversion, ToCamelCase_SingleWord)
{
    EXPECT_EQ(String("hello").ToCamelCase(), StringView("hello"));
    EXPECT_EQ(String("HELLO").ToCamelCase(), StringView("hello"));
}

TEST(StringCaseConversion, ToCamelCase_Empty)
{
    EXPECT_TRUE(String("").ToCamelCase().IsEmpty());
}

TEST(StringCaseConversion, ToCamelCase_WithTabs)
{
    EXPECT_EQ(String("hello\tworld").ToCamelCase(), StringView("helloWorld"));
}

TEST(StringCaseConversion, ToPascalCase_FromSpaceSeparated)
{
    EXPECT_EQ(String("hello world").ToPascalCase(), StringView("HelloWorld"));
    EXPECT_EQ(String("foo bar baz").ToPascalCase(), StringView("FooBarBaz"));
}

TEST(StringCaseConversion, ToPascalCase_FromSnakeCase)
{
    EXPECT_EQ(String("hello_world").ToPascalCase(), StringView("Hello_world"));
}

TEST(StringCaseConversion, ToPascalCase_MultipleSpaces)
{
    EXPECT_EQ(String("hello   world").ToPascalCase(), StringView("HelloWorld"));
}

TEST(StringCaseConversion, ToPascalCase_LeadingTrailingSpaces)
{
    EXPECT_EQ(String("  hello world  ").ToPascalCase(), StringView("HelloWorld"));
}

TEST(StringCaseConversion, ToPascalCase_Empty)
{
    EXPECT_TRUE(String("").ToPascalCase().IsEmpty());
}

TEST(StringCaseConversion, ToPascalCase_AllUpperCase)
{
    EXPECT_EQ(String("HELLO WORLD").ToPascalCase(), StringView("HelloWorld"));
}

TEST(StringCaseConversion, ToSnakeCase_FromSpaceSeparated)
{
    EXPECT_EQ(String("hello world").ToSnakeCase(), StringView("hello_world"));
    EXPECT_EQ(String("foo bar baz").ToSnakeCase(), StringView("foo_bar_baz"));
}

TEST(StringCaseConversion, ToSnakeCase_FromCamelCase)
{
    EXPECT_EQ(String("helloWorld").ToSnakeCase(), StringView("helloworld"));
}

TEST(StringCaseConversion, ToSnakeCase_FromPascalCase)
{
    EXPECT_EQ(String("HelloWorld").ToSnakeCase(), StringView("helloworld"));
}

TEST(StringCaseConversion, ToSnakeCase_MultipleSpaces)
{
    EXPECT_EQ(String("hello   world").ToSnakeCase(), StringView("hello_world"));
}

TEST(StringCaseConversion, ToSnakeCase_LeadingTrailingSpaces)
{
    EXPECT_EQ(String("  hello world  ").ToSnakeCase(), StringView("hello_world"));
}

TEST(StringCaseConversion, ToSnakeCase_SingleWord)
{
    EXPECT_EQ(String("hello").ToSnakeCase(), StringView("hello"));
    EXPECT_EQ(String("HELLO").ToSnakeCase(), StringView("hello"));
}

TEST(StringCaseConversion, ToSnakeCase_Empty)
{
    EXPECT_TRUE(String("").ToSnakeCase().IsEmpty());
}

TEST(StringCaseConversion, ToSnakeCase_AlreadySnakeCase)
{
    EXPECT_EQ(String("hello_world").ToSnakeCase(), StringView("hello_world"));
}

TEST(StringCaseConversion, ToSnakeCase_MixedCase)
{
    EXPECT_EQ(String("Hello World").ToSnakeCase(), StringView("hello_world"));
}

TEST(StringCaseConversion, ToSnakeCase_WithTabs)
{
    EXPECT_EQ(String("hello\tworld").ToSnakeCase(), StringView("hello_world"));
}

// ─────────────────────────────────────────────
// 엣지 케이스 / 회귀 테스트
// ─────────────────────────────────────────────

TEST(EdgeCase, NullTermination_AfterAppend)
{
    String s("hi");
    s.Append("!");
    EXPECT_EQ(s.GetPtr()[s.GetLength()], '\0');
}

TEST(EdgeCase, NullTermination_AfterResize)
{
    String s("hello");
    s.Resize(3);
    EXPECT_EQ(s.GetPtr()[3], '\0');
}

TEST(EdgeCase, NullTermination_AfterErase)
{
    String s("hello");
    s.Erase(2, 2);
    EXPECT_EQ(s.GetPtr()[s.GetLength()], '\0');
}

TEST(EdgeCase, LargeString_1000Appends)
{
    String      s;
    std::string expected;
    for (int i = 0; i < 1000; ++i)
    {
        s.Append("ab");
        expected += "ab";
    }
    EXPECT_EQ(s.GetLength(), 2000u);
    EXPECT_EQ(s.ToStdString(), expected);
}

TEST(EdgeCase, EmptyAppend_NoChange)
{
    String s("hello");
    s.Append("");
    EXPECT_EQ(s, StringView("hello"));
}

TEST(EdgeCase, StringView_SingleCharString)
{
    StringView sv("a");
    EXPECT_FOUND(sv.Find('a'), 0u);
    EXPECT_NOT_FOUND(sv.Find('b'));
}

TEST(EdgeCase, CharResult_DefaultIsFailed)
{
    CharResult cr;
    EXPECT_TRUE(cr.IsFailed());
    EXPECT_FALSE(cr.IsSucceeded());
}

TEST(EdgeCase, CharResult_ConstructWithIndex)
{
    CharResult cr(42);
    EXPECT_TRUE(cr.IsSucceeded());
    EXPECT_EQ(cr.pos, 42u);
}

TEST(EdgeCase, CharResult_FailedFactory)
{
    EXPECT_TRUE(CharResult::Failed().IsFailed());
}