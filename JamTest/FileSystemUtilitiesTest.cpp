#include "pch.h"

// 1. Path Relationship (Base Path)
TEST(PathTest, IsBasePathOf)
{
    Path base = PathFrom(R"(C:\abv\gd\555)");

    EXPECT_TRUE(IsBasePathOf(base, PathFrom(R"(C:\abv\gd\555\333\pod.mac)")));
    EXPECT_TRUE(IsBasePathOf(base, PathFrom(R"(C:\abv\gd\555)")));   // 동일 경로 (true)
    EXPECT_FALSE(IsBasePathOf(base, PathFrom(R"(C:\abv\gd\55)")));   // 부분 일치하지만 폴더가 다름
    EXPECT_FALSE(IsBasePathOf(base, PathFrom(R"(D:\other\path)")));
}

// 2. View Functions (Non-allocating)
TEST(PathTest, PathViewFunctions)
{
    Path p = PathFrom(R"(C:\abv\gd\555\333\pod.mac)");

    EXPECT_EQ(ParentPathViewOf(p), R"(C:\abv\gd\555\333)");
    EXPECT_EQ(FileNameViewOf(p), "pod.mac");
    EXPECT_EQ(BaseNameViewOf(p), "pod");
    EXPECT_EQ(ExtensionViewOf(p), ".mac");

    // 특이 케이스: 확장자 없는 경우
    Path p2 = PathFrom(R"(C:\Windows\System32\cmd)");
    EXPECT_EQ(BaseNameViewOf(p2), "cmd");
    EXPECT_EQ(ExtensionViewOf(p2), "");

    // 특이 케이스: 숨김 파일 형태
    Path p3 = PathFrom(R"(C:\Users\User\.config)");
    EXPECT_EQ(FileNameViewOf(p3), ".config");
}

// 3. String/Path Returning Functions (Allocating)
TEST(PathTest, PathReturningFunctions)
{
    Path p = PathFrom(R"(C:\abv\gd\555\333\pod.mac)");

    EXPECT_EQ(ParentPathOf(p), R"(C:\abv\gd\555\333)");
    EXPECT_EQ(FileNameOf(p), "pod.mac");
    EXPECT_EQ(BaseNameOf(p), "pod");
    EXPECT_EQ(ExtensionOf(p), ".mac");
}

// 4. Extension Manipulation
TEST(PathTest, ExtensionHandling)
{
    Path p = PathFrom(R"(project\data.json)");

    // MatchesExtension (Case Insensitive & Dot flexibility)
    EXPECT_TRUE(MatchesExtension(p, ".json"));
    EXPECT_TRUE(MatchesExtension(p, "json"));
    EXPECT_TRUE(MatchesExtension(p, ".JSON"));
    EXPECT_FALSE(MatchesExtension(p, ".png"));

    // ReplacedExtension
    Path p2 = ReplacedExtension(p, ".txt");
    EXPECT_EQ(p2, R"(project\data.txt)");
    EXPECT_EQ(ExtensionViewOf(p2), ".txt");

    // ReplaceExtension (In-place)
    Path p3 = PathFrom(R"(temp\script.sh)");
    ReplaceExtension(p3, "bat");
    EXPECT_EQ(p3, R"(temp\script.bat)");
}

// 5. Path Joining
TEST(PathTest, PathJoining)
{
    // JoinedPath
    Path base   = PathFrom(R"(C:\usr\local)");
    Path joined = JoinedPath(base, R"(bin\test.exe)");
    // 결과: C:\usr\local\bin\test.exe
    EXPECT_TRUE(IsBasePathOf(base, joined));
    EXPECT_EQ(joined, R"(C:\usr\local\bin\test.exe)");

    // JoinPath (In-place)
    Path p = PathFrom(R"(C:\Game)");
    JoinPath(p, R"(Assets\Textures)");
    EXPECT_TRUE(IsBasePathOf(PathFrom(R"(C:\Game)"), p));
    EXPECT_EQ(p, R"(C:\Game\Assets\Textures)");

    // Edge case: 빈 경로 조인
    Path emptyBase = PathFrom("");
    JoinPath(emptyBase, "root");
    EXPECT_EQ(emptyBase, "root");
}

// 6. Edge Cases & Boundary
TEST(PathTest, PathEdgeCases)
{
    // 루트 디렉토리 (Windows 스타일)
    Path root = PathFrom("C:\\");
    EXPECT_EQ(ParentPathViewOf(root), "C:");
    EXPECT_EQ(FileNameViewOf(root), "");

    // 파일명만 있는 경우
    Path fileOnly = PathFrom("main.cpp");
    EXPECT_EQ(ParentPathViewOf(fileOnly), "");
    EXPECT_EQ(BaseNameViewOf(fileOnly), "main");
    EXPECT_EQ(ExtensionViewOf(fileOnly), ".cpp");
}