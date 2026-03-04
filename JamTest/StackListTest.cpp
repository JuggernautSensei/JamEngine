#include "pch.h"

#include "../JamEngine/StackList.h"
#include <gtest/gtest.h>

using namespace Jam;

// ─────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────
class StackListTest : public ::testing::Test
{
protected:
    StackList<int, 16> list;
};

// ─────────────────────────────────────────
// 초기 상태
// ─────────────────────────────────────────
TEST_F(StackListTest, InitiallyEmpty)
{
    EXPECT_TRUE(list.IsEmpty());
    EXPECT_EQ(list.Begin(), list.End());
}

// ─────────────────────────────────────────
// PushBack
// ─────────────────────────────────────────
TEST_F(StackListTest, PushBackSingleElement)
{
    list.PushBack(42);

    EXPECT_FALSE(list.IsEmpty());
    EXPECT_EQ(*list.Begin(), 42);
}

TEST_F(StackListTest, PushBackMultipleElements_OrderPreserved)
{
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);

    auto it = list.Begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(it, list.End());
}

// ─────────────────────────────────────────
// PopBack
// ─────────────────────────────────────────
TEST_F(StackListTest, PopBackReducesSize)
{
    list.PushBack(10);
    list.PushBack(20);
    list.PopBack();

    auto it = list.Begin();
    EXPECT_EQ(*it, 10);
    ++it;
    EXPECT_EQ(it, list.End());
}

TEST_F(StackListTest, PopBackUntilEmpty)
{
    list.PushBack(1);
    list.PushBack(2);
    list.PopBack();
    list.PopBack();

    EXPECT_TRUE(list.IsEmpty());
}

TEST_F(StackListTest, PopBackOnEmptyAsserts)
{
    // JAM_ASSERT가 assert/abort 계열이면 Death Test로 검증
    EXPECT_DEATH(list.PopBack(), ".*");
}

// ─────────────────────────────────────────
// Insert (중간 삽입)
// ─────────────────────────────────────────
TEST_F(StackListTest, InsertAtBegin)
{
    list.PushBack(2);
    list.PushBack(3);
    list.Insert(list.Begin(), 1);   // 앞에 삽입

    auto it = list.Begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(it, list.End());
}

TEST_F(StackListTest, InsertInMiddle)
{
    list.PushBack(1);
    list.PushBack(3);

    auto it = list.Begin();
    ++it;   // 2번째 위치(3 앞)
    list.Insert(it, 2);

    it = list.Begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(it, list.End());
}

TEST_F(StackListTest, InsertAtEnd_BehavesLikePushBack)
{
    list.Insert(list.End(), 99);

    EXPECT_FALSE(list.IsEmpty());
    EXPECT_EQ(*list.Begin(), 99);
}

// ─────────────────────────────────────────
// Remove
// ─────────────────────────────────────────
TEST_F(StackListTest, RemoveOnlyElement)
{
    list.PushBack(5);
    list.Remove(list.Begin());

    EXPECT_TRUE(list.IsEmpty());
}

TEST_F(StackListTest, RemoveHeadElement)
{
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    list.Remove(list.Begin());   // 1 제거

    auto it = list.Begin();
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(it, list.End());
}

TEST_F(StackListTest, RemoveMiddleElement)
{
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);

    auto it = list.Begin();
    ++it;   // 2를 가리킴
    list.Remove(it);

    it = list.Begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(it, list.End());
}

TEST_F(StackListTest, RemoveEndIteratorAsserts)
{
    EXPECT_DEATH(list.Remove(list.End()), ".*");
}

// ─────────────────────────────────────────
// 노드 재사용 (free list)
// ─────────────────────────────────────────
TEST_F(StackListTest, NodeReuse_AfterRemove)
{
    // 삽입 → 삭제 → 재삽입 시 free list에서 노드를 재사용해야 함
    list.PushBack(1);
    list.PushBack(2);
    list.Remove(list.Begin());   // 1 제거 → free list에 반환
    list.PushBack(3);            // 재사용된 노드에 삽입

    auto it = list.Begin();
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(it, list.End());
}

// ─────────────────────────────────────────
// 반복자 양방향 순회
// ─────────────────────────────────────────
TEST_F(StackListTest, BidirectionalIteration)
{
    list.PushBack(10);
    list.PushBack(20);
    list.PushBack(30);

    // 정방향
    auto it = list.Begin();
    EXPECT_EQ(*it, 10);
    ++it;
    EXPECT_EQ(*it, 20);
    ++it;
    EXPECT_EQ(*it, 30);
    ++it;
    EXPECT_EQ(it, list.End());

    // 역방향: End()는 nullptr이므로 tail부터 직접 역순 검증
    --it;   // End에서 한 칸 뒤로 → 하지만 End()가 nullptr이라 UB 주의
    // 역방향은 tail 노드를 가진 유효 iterator로 시작
    auto rit = list.Begin();
    ++rit;
    ++rit;   // 30을 가리킴
    EXPECT_EQ(*rit, 30);
    --rit;
    EXPECT_EQ(*rit, 20);
    --rit;
    EXPECT_EQ(*rit, 10);
}

// ─────────────────────────────────────────
// 용량 경계
// ─────────────────────────────────────────
TEST(StackListCapacityTest, FillToCapacity)
{
    StackList<int, 8> small_;
    for (int i = 0; i < 8; ++i)
    {
        small_.PushBack(i);
    }

    int expected = 0;
    for (auto it = small_.Begin(); it != small_.End(); ++it)
    {
        EXPECT_EQ(*it, expected++);
    }
    EXPECT_EQ(expected, 8);
}

// ─────────────────────────────────────────
// 값 타입 (비 int)
// ─────────────────────────────────────────
TEST(StackListTypeTest, WorksWithString)
{
    StackList<std::string, 8> strList;
    strList.PushBack("hello");
    strList.PushBack("world");

    auto it = strList.Begin();
    EXPECT_EQ(*it++, "hello");
    EXPECT_EQ(*it++, "world");
    EXPECT_EQ(it, strList.End());
}