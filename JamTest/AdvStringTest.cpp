#include "pch.h"

/**
 * @brief 파일 시스템 관련 유틸리티 및 String/StringView의 경로 처리 기능을 테스트합니다.
 * 테스트 항목: Classification, Extraction, Modification, Simplification, Join, Relationship
 */

// 1. 경로 성격 판별 테스트 (Absolutize, Relativize, Directory, RegularFile)
TEST(AdvString, PathClassification)
{
    // Windows Style Absolutize
    EXPECT_TRUE(StringView("C:/Windows").IsAbsolute());
    EXPECT_TRUE(StringView("D:\\Games").IsAbsolute());
    EXPECT_FALSE(StringView("C:/Windows").IsRelative());

    // Unix/Root Style Absolutize
    EXPECT_TRUE(StringView("/usr/bin").IsAbsolute());
    EXPECT_FALSE(StringView("usr/bin").IsAbsolute());
    EXPECT_TRUE(StringView("usr/bin").IsRelative());

    // Directory vs Regular File (마지막 슬래시 유무 기준)
    EXPECT_TRUE(StringView("C:/Temp/").IsDirectory());
    EXPECT_FALSE(StringView("C:/Temp/").IsFile());

    EXPECT_TRUE(StringView("C:/Temp/file.txt").IsFile());
    EXPECT_FALSE(StringView("C:/Temp/file.txt").IsDirectory());

    // Edge cases
    EXPECT_FALSE(StringView("").IsDirectory());
    EXPECT_FALSE(StringView("").IsFile());
    EXPECT_TRUE(StringView("/").IsDirectory());
}

// 2. 경로 요소 추출 테스트 (Parent, FileName, BaseName, Extension)
TEST(AdvString, PathExtraction)
{
    struct TestCase
    {
        const char* input;
        const char* expectedParent;
        const char* expectedFileName;
        const char* expectedBaseName;
        const char* expectedExt;
    };

    TestCase cases[] = {
        // 일반적인 파일
        { "C:/Engine/Shaders/Lit.hlsl", "C:/Engine/Shaders/",       "Lit.hlsl",         "Lit",      ".hlsl" },
        // 확장자 없는 파일
        {               "/usr/bin/gcc",          "/usr/bin/",            "gcc",         "gcc",           "" },
        // 디렉터리 (마지막 슬래시 포함)
        {         "C:/User/Documents/",           "C:/User/",      "Documents",   "Documents",           "" },
        // 숨김 파일
        {                 ".gitignore",                   "",     ".gitignore",            "", ".gitignore" },
        // 경로 없는 파일명만
        {                  "README.md",                   "",      "README.md",      "README",        ".md" },
        // 중복 확장자
        {             "archive.tar.gz",                   "", "archive.tar.gz", "archive.tar",        ".gz" },
        // 루트
        {                          "/",                   "",               "",            "",           "" },
        // 드라이브 루트
        {                        "C:/",                   "",             "C:",          "C:",           "" }
    };

    for (const auto& c: cases)
    {
        StringView sv(c.input);
        EXPECT_EQ(sv.GetDirectory(), c.expectedParent) << "Failed Parent: " << c.input;
        EXPECT_EQ(sv.GetFileName(), c.expectedFileName) << "Failed FileName: " << c.input;
        EXPECT_EQ(sv.GetBaseName(), c.expectedBaseName) << "Failed BaseName: " << c.input;
        EXPECT_EQ(sv.GetExtension(), c.expectedExt) << "Failed Extension: " << c.input;
    }
}

// 3. 경로 단순화 테스트 (Simplify) - 핵심 로직
TEST(AdvString, PathSimplify)
{
    auto simplify = [](const char* input)
    {
        String s(input);
        s.Simplify();
        return std::string(s.GetPtr());
    };

    // 일반적인 정리
    EXPECT_EQ(simplify("C:\\Windows\\System32\\..\\"), "C:/Windows/");
    EXPECT_EQ(simplify("a/b/c/../../d"), "a/d");
    EXPECT_EQ(simplify("a/./b/./c"), "a/b/c");

    // 슬래시 중복 처리
    EXPECT_EQ(simplify("C:////Engine///Binaries//"), "C:/Engine/Binaries/");
    EXPECT_EQ(simplify("///usr////local//bin"), "/usr/local/bin");

    // 상위 경로 이동 경계값
    EXPECT_EQ(simplify("a/b/../../../../c"), "../../c");   // 루트 위로 가려고 할 때 (상대경로)
    EXPECT_EQ(simplify("/a/b/../../../../c"), "/c");       // 루트 위로 가려고 할 때 (절대경로)

    // 드라이브 문자 대문자화 및 정리
    EXPECT_EQ(simplify("d:assets/textures"), "D:/assets/textures");
    EXPECT_EQ(simplify("c:/"), "C:/");

    // 특수 케이스
    EXPECT_EQ(simplify("."), ".");
    EXPECT_EQ(simplify("././."), ".");
    EXPECT_EQ(simplify(".."), "..");
    EXPECT_EQ(simplify(""), ".");
}

// 4. 경로 결합 테스트 (Join)
TEST(AdvString, PathJoin)
{
    // 1. 디렉터리 형식을 지킨 정상 결합
    {
        String path("C:/Engine/");
        path.Join("Source");
        EXPECT_EQ(path, "C:/Engine/Source");
    }

    // 2. 비어있는 문자열에 결합
    {
        String path("");
        path.Join("Relativize/Path");
        EXPECT_EQ(path, "Relativize/Path");
    }

    // 3. 절대 경로를 Join 할 때 (구현 방식에 따라 다르나 보통 교체)
    // 현재 코드의 Join은 Append 방식이므로 IsAbsolute 체크 로직이 추가되었는지 확인 필요
    // 아래는 단순 Append 동작 기준 테스트
    {
        String path("C:/Old/");
        // 만약 Join 내부에 IsAbsolute 교체 로직이 없다면 Assert가 날 수 있으므로 주의
        // 여기서는 일반적인 상대 경로 Join 위주로 테스트
        path.Join("SubFolder/");
        EXPECT_EQ(path, "C:/Old/SubFolder/");
    }
}

// 5. 경로 구성 요소 변경 테스트 (Replace)
TEST(AdvString, PathReplace)
{
    // ReplaceExtension
    {
        String s("Texture.png");
        s.ReplaceExtension(".tga");
        EXPECT_EQ(s, "Texture.tga");

        s.Set("Texture");            // 확장자 없는 경우
        s.ReplaceExtension("jpg");   // 점 없이 입력
        EXPECT_EQ(s, "Texture.jpg");
    }

    // ReplaceFileName
    {
        String s("C:/Project/Main.cpp");
        s.ReplaceFileName("Buffer.h");
        EXPECT_EQ(s, "C:/Project/Buffer.h");

        s.Set("OnlyFile.txt");
        s.ReplaceFileName("NewFile.exe");
        EXPECT_EQ(s, "NewFile.exe");
    }

    // ReplaceBaseName
    {
        String s("C:/Project/Main.cpp");
        s.ReplaceBaseName("App");
        EXPECT_EQ(s, "C:/Project/App.cpp");

        s.Set(".gitignore");   // 확장자만 있는 파일로 인식되는 경우
        s.ReplaceBaseName("config");
        EXPECT_EQ(s, "config.gitignore");
    }
}

// 6. 경로 관계 테스트 (IsBasePathOf)
TEST(AdvString, PathRelationship)
{
    StringView base("C:/Engine/");

    EXPECT_TRUE(base.IsBasePathOf("C:/Engine/Source/Main.cpp"));
    EXPECT_TRUE(base.IsBasePathOf("C:/Engine/"));

    // 부적합한 케이스 (접두어는 같으나 경로상 남남)
    EXPECT_FALSE(base.IsBasePathOf("C:/EngineBinaries"));
    EXPECT_FALSE(base.IsBasePathOf("D:/Engine/Source"));

    // 디렉터리 구분자 명시 버전
    StringView baseDir("C:/Engine/");
    EXPECT_TRUE(baseDir.IsBasePathOf("C:/Engine/Source"));
    EXPECT_TRUE(baseDir.IsBasePathOf("C:/Engine/"));   // base가 더 길면 안됨 (구현 상세에 따라 다름)
}

// 7. 복합 시나리오 및 대량 데이터 (Stress/Boundary)
TEST(AdvString, PathComplexityStress)
{
    // 아주 긴 경로 처리
    std::string longPath = "C:/";
    for (int i = 0; i < 100; ++i)
        longPath += "Folder/";
    longPath += "File.extension";

    String s(longPath.c_str());
    EXPECT_EQ(s.GetLength(), longPath.length());
    EXPECT_EQ(s.GetExtension(), ".extension");

    // 점(.)이 많은 파일명
    String dots("my.system.config.xml.backup");
    EXPECT_EQ(dots.GetExtension(), ".backup");
    EXPECT_EQ(dots.GetBaseName(), "my.system.config.xml");

    dots.ReplaceExtension(".old");
    EXPECT_EQ(dots, "my.system.config.xml.old");

    // 윈도우/유닉스 구분자 혼용 정리
    String mixed("C:\\Windows/System32\\Drivers/etc/../");
    mixed.Simplify();
    EXPECT_EQ(mixed, "C:/Windows/System32/Drivers/");
}

// 8. WinAPI 의존 기능 테스트 (Absolutize / Relativize)
// 주의: 이 테스트는 실제 파일 시스템 경로에 의존하므로 환경에 따라 결과가 다를 수 있음
TEST(AdvString, WinApiDependent)
{
    // Absolutize 테스트
    String rel = Absolute("test.txt");
    EXPECT_TRUE(rel.IsAbsolute());

    // 현재 경로 획득
    String cur = Current();
    EXPECT_FALSE(cur.IsEmpty());
    EXPECT_TRUE(cur.IsAbsolute());

    // 실행 파일 경로
    String exe = Executable();
    EXPECT_TRUE(exe.IsAbsolute());
    EXPECT_TRUE(exe.EndsWith(".exe") || exe.IsFile());
}

// 9. 정수 파싱 연동 테스트
TEST(AdvString, PathParsingIntegration)
{
    String     path("C:/Assets/Models/Model_1024.mesh");
    StringView fileName = path.GetFileName();   // "Model_1024.mesh"

    auto lastUnderscore = fileName.RFind('_');
    auto dot            = fileName.RFind('.');

    ASSERT_TRUE(lastUnderscore);
    ASSERT_TRUE(dot);

    StringView idStr = fileName.SubView(lastUnderscore.GetValue() + 1, dot.GetValue() - lastUnderscore.GetValue() - 1);
    EXPECT_EQ(idStr, "1024");

    auto id = idStr.ParseAs<Int32>();
    ASSERT_TRUE(id.HasValue());
    EXPECT_EQ(*id, 1024);
}