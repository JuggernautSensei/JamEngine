#include "pch.h"

#include "../JamEngine/ResourcePool.h"
#include "../JamEngine/FileSystem.h"
#include "../JamEngine/DirIter.h"
#include "../JamEngine/JsonArchive.h"
#include <filesystem>
#include <fstream>

namespace
{
struct RegressionDirectory
{
    RegressionDirectory()
        : path(String::Formatted("Build/RegressionFiles-{}/", ::GetCurrentProcessId()))
    {
        std::error_code error = {};
        std::filesystem::create_directories(path.GetPtr(), error);
    }

    ~RegressionDirectory()
    {
        std::error_code error = {};
        std::filesystem::remove_all(path.GetPtr(), error);
    }

    String path;
};
}   // namespace

TEST(FileSystemRegression, FailIfExistsPreservesOriginalFile)
{
    const RegressionDirectory directory = {};
    const String              path      = String::Joined(directory.path, "sample.hlsl");
    {
        std::ofstream file { path.GetPtr() };
        file << "original";
    }
    auto writer = FileWriter::Open(path, eFileWrite::FailIfExists);
    EXPECT_FALSE(writer);
    if (writer)
    {
        writer->Close();
    }
    std::ifstream file { path.GetPtr() };
    std::string   contents = {};
    file >> contents;
    EXPECT_EQ(contents, "original");
}

TEST(FileSystemRegression, UniquePathPreservesExtensionWithoutTrailingSpaces)
{
    const RegressionDirectory directory = {};
    const String              path      = String::Joined(directory.path, "sample.hlsl");
    {
        std::ofstream file { path.GetPtr() };
        file << "original";
    }
    EXPECT_EQ(FS::MakeUniquePath(path), String::Joined(directory.path, "sample (1).hlsl"));
}

TEST(FileSystemRegression, DirectoryIterationKeepsParentDirectory)
{
    const RegressionDirectory directory = {};
    const String              path      = String::Joined(directory.path, "sample.hlsl");
    {
        std::ofstream file { path.GetPtr() };
        file << "original";
    }
    auto iterator = DirIter::Create(directory.path);
    ASSERT_TRUE(iterator);
    ASSERT_NE(**iterator, StringView {});
    EXPECT_EQ(**iterator, path);
}

TEST(FileSystemRegression, MakeDirectoryCreatesNestedDirectories)
{
    const RegressionDirectory directory = {};
    const String              path      = String::Joined(directory.path, "Nested/Child/");
    EXPECT_FALSE(FS::MakeDirectory(path));
    EXPECT_TRUE(FS::IsDirectory(path));
}

TEST(FileSystemRegression, FileRoundTripSupportsUnicodeAbsolutePaths)
{
    const RegressionDirectory directory = {};
    const String              path      = FS::Absolute(String::Joined(directory.path, "한글 파일.txt"));
    {
        auto writer = FileWriter::Open(path);
        ASSERT_TRUE(writer);
        const StringView contents = "test";
        const auto       written  = writer->Write(MemoryView { contents.GetPtr(), contents.GetLength() });
        ASSERT_TRUE(written);
        EXPECT_EQ(*written, 4);
    }
    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader);
    char       contents[4] = {};
    const auto read        = reader->Read(contents);
    ASSERT_TRUE(read);
    EXPECT_EQ(StringView(contents, *read), "test");
}

TEST(ArchiveRegression, FlagsAndEnumArraysRoundTrip)
{
    const Flags<eSamplerFlags> flags  = eSamplerFlags::AddressU_Clamp | eSamplerFlags::AddressV_Clamp;
    EnumArray<eShader, bool>   values = {};
    values.Fill(false);
    values[eShader::PixelShader] = true;

    JsonWriter writer = {};
    writer.BeginObject();
    writer.WriteField("flags", flags);
    writer.WriteField("values", values);
    writer.EndObject();
    const auto json = writer.Stringify();
    ASSERT_TRUE(json);
    auto reader = JsonReader::Parse(*json);
    ASSERT_TRUE(reader);
    Flags<eSamplerFlags>     loadedFlags  = {};
    EnumArray<eShader, bool> loadedValues = {};
    EXPECT_FALSE(reader->ReadField("flags", loadedFlags));
    EXPECT_FALSE(reader->ReadField("values", loadedValues));
    EXPECT_EQ(loadedFlags, flags);
    for (SizeT i = 0; i < values.Size(); ++i)
    {
        EXPECT_EQ(loadedValues[i], values[i]);
    }
}

TEST(StringRegression, SameEncodingAcceptsExactBufferCapacity)
{
    char    narrow[3] = {};
    wchar_t wide[3]   = {};
    EXPECT_EQ((CharEncoder<char, char>::Encode("abc", 3, narrow, 3)), 3);
    EXPECT_EQ((CharEncoder<wchar_t, wchar_t>::Encode(L"abc", 3, wide, 3)), 3);
    EXPECT_EQ(StringView(narrow, 3), "abc");
    EXPECT_EQ(WStringView(wide, 3), L"abc");
}

TEST(StringRegression, PrefixAndSuffixRespectViewBounds)
{
    const StringView empty = {};
    EXPECT_FALSE(empty.StartsWith('a'));
    EXPECT_FALSE(empty.EndsWith('a'));
    EXPECT_FALSE(StringView("a").StartsWith("abc"));
    EXPECT_FALSE(StringView("a").EndsWith("abc"));
    EXPECT_TRUE(empty.StartsWith(""));
    EXPECT_TRUE(empty.EndsWith(""));
    EXPECT_FALSE(StringView("file").MatchExtension("png"));
}

TEST(StringRegression, NarrowAndWideStringsKeepNullTerminators)
{
    String narrow = "short";
    narrow.Resize(3);
    EXPECT_EQ(narrow.GetPtr()[3], '\0');
    narrow.Clear();
    EXPECT_EQ(narrow.GetPtr()[0], '\0');

    WString wide = L"short";
    wide.Resize(3);
    EXPECT_EQ(wide.GetPtr()[3], L'\0');
    wide.Clear();
    EXPECT_EQ(wide.GetPtr()[0], L'\0');
}

TEST(ResourcePoolRegression, MutableAndConstIterationSkipFreedHandles)
{
    ResourcePool<Int32> pool   = {};
    const auto          first  = pool.Alloc();
    const auto          second = pool.Alloc();
    pool[first]                = 10;
    pool[second]               = 20;
    pool.Free(first);

    Int32 count = 0;
    for (auto [handle, value]: pool)
    {
        EXPECT_EQ(handle, second);
        value += 1;
        ++count;
    }
    EXPECT_EQ(count, 1);
    EXPECT_EQ(pool[second], 21);

    const ResourcePool<Int32>& constPool = pool;
    count                                = 0;
    for (const auto [handle, value]: constPool)
    {
        EXPECT_EQ(handle, second);
        EXPECT_EQ(value, 21);
        ++count;
    }
    EXPECT_EQ(count, 1);
    pool.Free(second);
    EXPECT_EQ(pool.Begin(), pool.End());
}

TEST(ShaderBinaryRegression, CompiledBytecodeLoadsFromCSO)
{
    ShaderCompileInfo info     = {};
    info.stage                 = eShader::PixelShader;
    info.entryPoint            = "PSMain";
    const ShaderMacro macros[] = {
        { "COLOR", "0.5" }
    };
    info.macros         = macros;
    const auto compiled = ShaderBinary::CompileFromHLSLSource("float4 PSMain() : SV_Target { return float4(COLOR, 0, 0, 1); }", info);
    ASSERT_TRUE(compiled);

    const MemoryView bytecode = compiled->GetBytecode();
    const char*      path     = "ShaderBinaryRegression.cso";
    {
        std::ofstream file { path, std::ios::binary };
        ASSERT_TRUE(file.is_open());
        file.write(reinterpret_cast<const char*>(bytecode.GetPtr()), bytecode.GetByteWidth());
        ASSERT_TRUE(file.good());
    }

    const auto loaded = ShaderBinary::FromCSOFile(path);
    ::remove(path);
    ASSERT_TRUE(loaded);
    EXPECT_EQ(loaded->GetBytecode().GetByteWidth(), bytecode.GetByteWidth());
    EXPECT_EQ(::memcmp(loaded->GetBytecode().GetPtr(), bytecode.GetPtr(), bytecode.GetByteWidth()), 0);
}
