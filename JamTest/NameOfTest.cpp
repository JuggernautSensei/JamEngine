#include "pch.h"

using namespace Jam;

// Test enum for NameOf tests
enum class eTestEnum
{
    Value1,
    Value2,
    Value3
};

// Test class for NameOf tests
class TestClass
{
};

namespace TestNamespace
{
class NestedClass
{
};
}   // namespace TestNamespace

// --- 1. NameOf: Basic Type Names ---
TEST(NameOfTest, FundamentalTypes)
{
    EXPECT_EQ(NameOf<int>(), "int");
    EXPECT_EQ(NameOf<float>(), "float");
    EXPECT_EQ(NameOf<double>(), "double");
    EXPECT_EQ(NameOf<bool>(), "bool");
    EXPECT_EQ(NameOf<char>(), "char");
}

// --- 2. NameOf: Custom Types ---
TEST(NameOfTest, CustomTypes)
{
    EXPECT_EQ(NameOf<TestClass>(), "TestClass");
    EXPECT_EQ(NameOf<eTestEnum>(), "eTestEnum");
    EXPECT_EQ(NameOf<TestNamespace::NestedClass>(), "TestNamespace::NestedClass");
}

// --- 3. NameOf: Pointer and Reference Types ---
TEST(NameOfTest, PointerAndReferenceTypes)
{
    EXPECT_TRUE(NameOf<int*>().Find("int").HasValue());
    EXPECT_TRUE(NameOf<const int*>().Find("int").HasValue());
    EXPECT_TRUE(NameOf<int&>().Find("int").HasValue());
    EXPECT_TRUE(NameOf<const int&>().Find("int").HasValue());
}

// --- 4. NameOf: Const and Volatile ---
TEST(NameOfTest, CvQualifiers)
{
    EXPECT_TRUE(NameOf<const int>().Find("int").HasValue());
    EXPECT_TRUE(NameOf<volatile int>().Find("int").HasValue());
    EXPECT_TRUE(NameOf<const volatile int>().Find("int").HasValue());
}

// --- 5. NameOf: Array Types ---
TEST(NameOfTest, ArrayTypes)
{
    EXPECT_TRUE(NameOf<int[10]>().Find("int").HasValue());
    EXPECT_TRUE(NameOf<char[5]>().Find("char").HasValue());
}

// --- 6. NameOf: Template Types ---
TEST(NameOfTest, TemplateTypes)
{
    EXPECT_TRUE(NameOf<Jam::Vector<int>>().Find("vector").HasValue());
    EXPECT_TRUE(NameOf<Jam::Vector<int>>().Find("int").HasValue());
}

// --- 7. NameOf: Constexpr ---
TEST(NameOfTest, ConstexprEvaluation)
{
    constexpr auto intName = NameOf<int>();
    static_assert(intName == "int");
    EXPECT_EQ(intName, "int");

    constexpr auto floatName = NameOf<float>();
    static_assert(floatName == "float");
    EXPECT_EQ(floatName, "float");
}

// --- 8. HashOf: Type Hashing ---
TEST(NameOfTest, HashOfBasics)
{
    constexpr UInt64 intHash   = HashOf<int>();
    constexpr UInt64 floatHash = HashOf<float>();

    EXPECT_NE(intHash, 0u);
    EXPECT_NE(floatHash, 0u);
    EXPECT_NE(intHash, floatHash);

    // Same type should have same hash
    EXPECT_EQ(HashOf<int>(), HashOf<int>());
    EXPECT_EQ(HashOf<TestClass>(), HashOf<TestClass>());
}

// --- 9. HashOf: Constexpr ---
TEST(NameOfTest, HashOfConstexpr)
{
    static_assert(HashOf<int>() == HashOf<int>());
    static_assert(HashOf<float>() != HashOf<double>());
    static_assert(HashOf<TestClass>() == HashOf<TestClass>());
}

// --- 10. HashOf: Collision Check ---
TEST(NameOfTest, HashOfCollision)
{
    // Different types should (very likely) have different hashes
    UInt64 hash1 = HashOf<int>();
    UInt64 hash2 = HashOf<float>();
    UInt64 hash3 = HashOf<double>();
    UInt64 hash4 = HashOf<TestClass>();
    UInt64 hash5 = HashOf<eTestEnum>();

    EXPECT_NE(hash1, hash2);
    EXPECT_NE(hash1, hash3);
    EXPECT_NE(hash1, hash4);
    EXPECT_NE(hash1, hash5);
    EXPECT_NE(hash2, hash3);
    EXPECT_NE(hash2, hash4);
    EXPECT_NE(hash2, hash5);
}
