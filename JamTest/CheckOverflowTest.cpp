#include "pch.h"

// 1. Integral: Same Signedness (크기 축소 및 확장)
TEST(CheckOverflowTest, Integral_SameSign)
{
    // Downscaling
    EXPECT_FALSE((CheckOverflow<int8_t, int32_t>(127)));
    EXPECT_FALSE((CheckOverflow<int8_t, int32_t>(-128)));
    EXPECT_TRUE((CheckOverflow<int8_t, int32_t>(128)));    
    EXPECT_TRUE((CheckOverflow<int8_t, int32_t>(-129)));   

    EXPECT_FALSE((CheckOverflow<uint16_t, uint32_t>(65535U)));
    EXPECT_TRUE((CheckOverflow<uint16_t, uint32_t>(65536U)));   

    // Upscaling (항상 false)
    EXPECT_FALSE((CheckOverflow<int64_t, int8_t>(-128)));
    EXPECT_FALSE((CheckOverflow<uint64_t, uint32_t>(4294967295U)));
}

// 2. Integral: Signed to Unsigned (음수 및 범위 초과)
TEST(CheckOverflowTest, Integral_SignedToUnsigned)
{
    EXPECT_TRUE((CheckOverflow<uint32_t, int32_t>(-1)));    
    EXPECT_TRUE((CheckOverflow<uint8_t, int16_t>(-150)));   
    EXPECT_FALSE((CheckOverflow<uint16_t, int32_t>(65535)));
    EXPECT_TRUE((CheckOverflow<uint16_t, int32_t>(65536)));   
}

// 3. Integral: Unsigned to Signed (양수 최댓값 초과)
TEST(CheckOverflowTest, Integral_UnsignedToSigned)
{
    EXPECT_FALSE((CheckOverflow<int8_t, uint8_t>(127)));
    EXPECT_TRUE((CheckOverflow<int8_t, uint8_t>(128)));   
    EXPECT_FALSE((CheckOverflow<int32_t, uint64_t>(2147483647ULL)));
    EXPECT_TRUE((CheckOverflow<int32_t, uint64_t>(2147483648ULL)));   
}

// 4. Floating Point: Float to Float (정밀도와 무관한 범위 체크)
TEST(CheckOverflowTest, Floating_To_Floating)
{
    EXPECT_FALSE((CheckOverflow<float, double>(3.4e38)));
    EXPECT_TRUE((CheckOverflow<float, double>(1e40)));    
    EXPECT_TRUE((CheckOverflow<float, double>(-1e40)));   
}

// 5. Floating Point: Float to Integral (실수를 정수로)
TEST(CheckOverflowTest, Floating_To_Integral)
{
    // Positive overflow
    EXPECT_FALSE((CheckOverflow<int32_t, float>(2147483520.0f)));
    EXPECT_TRUE((CheckOverflow<int32_t, float>(3000000000.0f)));   

    // Negative overflow
    EXPECT_FALSE((CheckOverflow<int16_t, double>(-32768.0)));
    EXPECT_TRUE((CheckOverflow<int16_t, double>(-32769.0)));   

    // Unsigned target
    EXPECT_FALSE((CheckOverflow<uint8_t, float>(255.0f)));
    EXPECT_TRUE((CheckOverflow<uint8_t, float>(256.0f)));   
    EXPECT_TRUE((CheckOverflow<uint8_t, float>(-1.0f)));    
}

// 6. Floating Point: Integral to Float (항상 범위 내)
TEST(CheckOverflowTest, Integral_To_Floating)
{
    EXPECT_FALSE((CheckOverflow<float, int64_t>(9223372036854775807LL)));
    EXPECT_FALSE((CheckOverflow<double, uint64_t>(18446744073709551615ULL)));
}

// 7. Extreme: 64-bit Boundary (최대 비트수 경계)
TEST(CheckOverflowTest, Extreme_Boundary)
{
    EXPECT_TRUE((CheckOverflow<int64_t, uint64_t>(18446744073709551615ULL)));   
    EXPECT_TRUE((CheckOverflow<uint64_t, int64_t>(-1)));                        
}

// 8. Identity & Zero
TEST(CheckOverflowTest, Invariants)
{
    EXPECT_FALSE((CheckOverflow<int32_t, int32_t>(12345)));
    EXPECT_FALSE((CheckOverflow<float, float>(1.1f)));
    EXPECT_FALSE((CheckOverflow<uint8_t, int32_t>(0)));
    EXPECT_FALSE((CheckOverflow<int16_t, double>(0.0)));
}