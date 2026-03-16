#include "pch.h"
#include <gtest/gtest.h>
#include <fstream>

#include "../JamEngine/MemoryIO.h"

namespace Jam::File::Test
{

class FileRWTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        testRoot = FileSystem::Current();
        testRoot.Join("TestFileRW/", eJoinType::Directory);

        if (FileSystem::Exists(testRoot))
        {
            FileSystem::Remove(testRoot);
        }
        FileSystem::MakeDirectory(testRoot, FileSystem::eMakeDirectoryCollisionPolicy::Fail);
    }

    void TearDown() override
    {
        if (FileSystem::Exists(testRoot))
        {
            FileSystem::Remove(testRoot);
        }
    }

    String GetTestFilePath(const StringView _filename)
    {
        String path = testRoot;
        path.Join(_filename, eJoinType::File);
        return path;
    }

    bool VerifyFileContent(const StringView _path, const CMemoryBlock _expected)
    {
        std::ifstream ifs(_path.GetPtr(), std::ios::binary);
        if (!ifs.is_open())
            return false;

        ifs.seekg(0, std::ios::end);
        const SizeT fileSize = static_cast<SizeT>(ifs.tellg());
        ifs.seekg(0, std::ios::beg);

        Vector<Byte> content(fileSize);
        ifs.read(reinterpret_cast<char*>(content.data()), fileSize);

        if (fileSize != _expected.GetByteWidth())
            return false;

        return memcmp(content.data(), _expected.GetPtr(), _expected.GetByteWidth()) == 0;
    }

    bool VerifyFileContentString(const StringView _path, const StringView _expected)
    {
        return VerifyFileContent(_path, CMemoryBlock { _expected.GetPtr(), _expected.GetLength() });
    }

    String testRoot;
};

// ============================================================================
// FileWriter Open Tests
// ============================================================================

TEST_F(FileRWTest, FileWriter_Open_Success)
{
    String path = GetTestFilePath("test_open.txt");

    Expect<FileWriter> writer = FileWriter::Open(path);

    EXPECT_TRUE(writer.HasValue());
    EXPECT_FALSE(writer->IsClosed());
    EXPECT_FALSE(writer->IsError());
}

TEST_F(FileRWTest, FileWriter_Open_CreateDirectories)
{
    String path = testRoot;
    path.Join("SubDir1/SubDir2/test.txt", eJoinType::File);

    Expect<FileWriter> writer = FileWriter::Open(path);

    EXPECT_TRUE(writer.HasValue());
    EXPECT_TRUE(FileSystem::Exists(path));
}

TEST_F(FileRWTest, FileWriter_Open_CollisionPolicy_Overwrite)
{
    String path = GetTestFilePath("collision_overwrite.txt");

    auto writer1 = FileWriter::Open(path, eFileWriterCollisionPolicy::Overwrite);
    ASSERT_TRUE(writer1.HasValue());
    writer1->WriteString("initial");
    writer1->Close();

    auto writer2 = FileWriter::Open(path, eFileWriterCollisionPolicy::Overwrite);
    ASSERT_TRUE(writer2.HasValue());
    writer2->WriteString("new");
    writer2->Close();

    EXPECT_TRUE(VerifyFileContentString(path, "new"));
}

TEST_F(FileRWTest, FileWriter_Open_CollisionPolicy_Rename)
{
    String path = GetTestFilePath("collision_rename.txt");

    auto writer1 = FileWriter::Open(path, eFileWriterCollisionPolicy::Overwrite);
    ASSERT_TRUE(writer1.HasValue());
    writer1->WriteString("first");
    writer1->Close();

    auto writer2 = FileWriter::Open(path, eFileWriterCollisionPolicy::Rename);
    ASSERT_TRUE(writer2.HasValue());
    writer2->WriteString("second");
    writer2->Close();

    EXPECT_TRUE(FileSystem::Exists(path));
    EXPECT_TRUE(VerifyFileContentString(path, "first"));

    String renamedPath = GetTestFilePath("collision_rename (1).txt");
    EXPECT_TRUE(FileSystem::Exists(renamedPath));
}

TEST_F(FileRWTest, FileWriter_Open_CollisionPolicy_Fail)
{
    String path = GetTestFilePath("collision_fail.txt");

    auto writer1 = FileWriter::Open(path, eFileWriterCollisionPolicy::Overwrite);
    ASSERT_TRUE(writer1.HasValue());
    writer1->Close();

    auto writer2 = FileWriter::Open(path, eFileWriterCollisionPolicy::Fail);
    EXPECT_FALSE(writer2.HasValue());
    EXPECT_EQ(writer2.GetError(), Error::AlreadyExists);
}

TEST_F(FileRWTest, FileWriter_Open_CollisionPolicy_Append)
{
    String path = GetTestFilePath("collision_append.txt");

    auto writer1 = FileWriter::Open(path, eFileWriterCollisionPolicy::Overwrite);
    ASSERT_TRUE(writer1.HasValue());
    writer1->WriteString("Hello");
    writer1->Close();

    auto writer2 = FileWriter::Open(path, eFileWriterCollisionPolicy::Append);
    ASSERT_TRUE(writer2.HasValue());
    writer2->WriteString("World");
    writer2->Close();

    EXPECT_TRUE(VerifyFileContentString(path, "HelloWorld"));
}

// ============================================================================
// FileWriter WriteString Tests
// ============================================================================

TEST_F(FileRWTest, FileWriter_Write_Success)
{
    String path = GetTestFilePath("write_test.txt");

    auto                writer = FileWriter::Open(path);
    const char          data[] = "Test Data";
    const Expect<SizeT> ret    = writer->WriteBinary(CMemoryBlock { data, 9 });

    EXPECT_TRUE(ret.HasValue());
    EXPECT_EQ(ret.GetValue(), 9);
    writer->Close();

    EXPECT_TRUE(VerifyFileContentString(path, "Test Data"));
}

TEST_F(FileRWTest, FileWriter_Write_EmptyData)
{
    String path = GetTestFilePath("write_empty.txt");

    auto                writer = FileWriter::Open(path);
    const Expect<SizeT> ret    = writer->WriteBinary(CMemoryBlock { "", 0 });

    EXPECT_TRUE(ret.HasValue());
    EXPECT_EQ(ret.GetValue(), 0);
}

TEST_F(FileRWTest, FileWriter_WriteAsString_ExcludesNullTerminator)
{
    String path = GetTestFilePath("write_as_string_no_null.txt");

    auto writer = FileWriter::Open(path);
    writer->WriteString("Test");
    writer->Close();

    auto reader = FileReader::Open(path);
    EXPECT_EQ(reader->CalcTotalSize(), 4);
}

TEST_F(FileRWTest, FileWriter_Write_IncludesNullTerminator)
{
    String path = GetTestFilePath("write_with_null.txt");

    auto        writer = FileWriter::Open(path);
    const char* str    = "Test";
    writer->WriteBinary(CMemoryBlock { str, 5 });
    writer->Close();

    auto reader = FileReader::Open(path);
    EXPECT_EQ(reader->CalcTotalSize(), 5);

    char buffer[5] = {};
    reader->ReadBinary(MemoryBlock { buffer, 5 });
    EXPECT_EQ(buffer[4], '\0');
}

TEST_F(FileRWTest, FileWriter_Write_LargeData)
{
    String path = GetTestFilePath("write_large.bin");

    const SizeT  dataSize = 1024 * 1024;
    Vector<Byte> data(dataSize);
    for (SizeT i = 0; i < dataSize; ++i)
    {
        data[i] = static_cast<Byte>(i % 256);
    }

    auto                writer = FileWriter::Open(path);
    const Expect<SizeT> ret    = writer->WriteBinary(CMemoryBlock { data.data(), dataSize });

    EXPECT_TRUE(ret.HasValue());
    EXPECT_EQ(ret.GetValue(), dataSize);
    writer->Close();

    EXPECT_TRUE(VerifyFileContent(path, CMemoryBlock { data.data(), dataSize }));
}

TEST_F(FileRWTest, FileWriter_Write_MultipleWrites)
{
    String path = GetTestFilePath("write_multiple.txt");

    auto writer = FileWriter::Open(path);

    writer->WriteString("Hello");
    writer->WriteString(" ");
    writer->WriteString("World");

    writer->Close();

    EXPECT_TRUE(VerifyFileContentString(path, "Hello World"));
}

TEST_F(FileRWTest, FileWriter_Write_BinaryData)
{
    String path = GetTestFilePath("write_binary.bin");

    Byte data[] = { 0x00, 0x01, 0x7F, 0x80, 0xFF, 0xDE, 0xAD, 0xBE, 0xEF };

    auto writer = FileWriter::Open(path);
    writer->WriteBinary(CMemoryBlock { data, sizeof(data) });
    writer->Close();

    EXPECT_TRUE(VerifyFileContent(path, CMemoryBlock { data, sizeof(data) }));
}

TEST_F(FileRWTest, FileWriter_WriteFileAsMemory_Success)
{
    String path = GetTestFilePath("write_from_memory.txt");

    const char data[] = "Memory Data";
    Error      err    = FileWriter::WriteFileAsBinary(CMemoryBlock { data, 11 }, path);

    EXPECT_EQ(err, Error::None);
    EXPECT_TRUE(VerifyFileContentString(path, "Memory Data"));
}

TEST_F(FileRWTest, FileWriter_WriteFileAsString_Success)
{
    String path = GetTestFilePath("write_from_string.txt");

    Error err = FileWriter::WriteFileAsString("String Data", path);

    EXPECT_EQ(err, Error::None);
    EXPECT_TRUE(VerifyFileContentString(path, "String Data"));
}

TEST_F(FileRWTest, FileWriter_WriteFileAsString_WithPolicy)
{
    String path = GetTestFilePath("write_string_policy.txt");

    FileWriter::WriteFileAsString("First", path, eFileWriterCollisionPolicy::Overwrite);
    Error err = FileWriter::WriteFileAsString("Second", path, eFileWriterCollisionPolicy::Append);

    EXPECT_EQ(err, Error::None);
    EXPECT_TRUE(VerifyFileContentString(path, "FirstSecond"));
}

TEST_F(FileRWTest, FileWriter_Close)
{
    String path = GetTestFilePath("close_test.txt");

    auto writer = FileWriter::Open(path);
    EXPECT_FALSE(writer->IsClosed());

    writer->Close();
    EXPECT_TRUE(writer->IsClosed());

    writer->Close();
    EXPECT_TRUE(writer->IsClosed());
}

TEST_F(FileRWTest, FileWriter_MoveConstructor)
{
    String path = GetTestFilePath("move_constructor.txt");

    auto writer1 = FileWriter::Open(path);
    ASSERT_TRUE(writer1.HasValue());

    FileWriter writer2 = std::move(writer1.GetValue());

    EXPECT_FALSE(writer2.IsClosed());
    writer2.WriteString("Moved");
    writer2.Close();

    EXPECT_TRUE(VerifyFileContentString(path, "Moved"));
}

TEST_F(FileRWTest, FileWriter_MoveAssignment)
{
    String path1 = GetTestFilePath("move_assign1.txt");
    String path2 = GetTestFilePath("move_assign2.txt");

    auto writer1 = FileWriter::Open(path1);
    auto writer2 = FileWriter::Open(path2);

    ASSERT_TRUE(writer1.HasValue());
    ASSERT_TRUE(writer2.HasValue());

    writer2.GetValue() = std::move(writer1.GetValue());

    writer2->WriteString("Test");
    writer2->Close();
}

// ============================================================================
// FileReader Open Tests
// ============================================================================

TEST_F(FileRWTest, FileReader_Open_Success)
{
    String path = GetTestFilePath("reader_test.txt");
    FileWriter::WriteFileAsString("Test", path);

    Expect<FileReader> reader = FileReader::Open(path);

    EXPECT_TRUE(reader.HasValue());
    EXPECT_FALSE(reader->IsClosed());
    EXPECT_FALSE(reader->IsError());
}

TEST_F(FileRWTest, FileReader_Open_NonExistent)
{
    String path = GetTestFilePath("non_existent.txt");

    Expect<FileReader> reader = FileReader::Open(path);

    EXPECT_FALSE(reader.HasValue());
    EXPECT_EQ(reader.GetError(), Error::OpenFailed);
}

// ============================================================================
// FileReader ReadBinary Tests
// ============================================================================

TEST_F(FileRWTest, FileReader_Read_Success)
{
    String path = GetTestFilePath("read_test.txt");
    FileWriter::WriteFileAsString("Hello World", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    char                buffer[11] = {};
    const Expect<SizeT> ret        = reader->ReadBinary(MemoryBlock { buffer, 11 });

    EXPECT_TRUE(ret.HasValue());
    EXPECT_EQ(ret.GetValue(), 11);
    EXPECT_TRUE(std::strncmp(buffer, "Hello World", 11) == 0);
}

TEST_F(FileRWTest, FileReader_Read_PartialReads)
{
    String path = GetTestFilePath("partial_read.txt");
    FileWriter::WriteFileAsString("ABCDEFGHIJ", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    char buffer1[5] = {};
    reader->ReadBinary(MemoryBlock { buffer1, 5 });
    EXPECT_TRUE(std::strncmp(buffer1, "ABCDE", 5) == 0);

    char buffer2[5] = {};
    reader->ReadBinary(MemoryBlock { buffer2, 5 });
    EXPECT_TRUE(std::strncmp(buffer2, "FGHIJ", 5) == 0);
}

TEST_F(FileRWTest, FileReader_Read_EmptyFile)
{
    String path = GetTestFilePath("empty_file.txt");
    FileWriter::WriteFileAsString("", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    char                buffer[10] = {};
    const Expect<SizeT> ret        = reader->ReadBinary(MemoryBlock { buffer, 10 });

    EXPECT_TRUE(ret.GetValue() == 0 || !ret.HasValue());
}

TEST_F(FileRWTest, FileReader_Read_BinaryData)
{
    String path = GetTestFilePath("read_binary.bin");

    Byte writeData[] = { 0x00, 0x01, 0x7F, 0x80, 0xFF, 0xDE, 0xAD, 0xBE, 0xEF };
    FileWriter::WriteFileAsBinary(CMemoryBlock { writeData, sizeof(writeData) }, path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    Byte readData[9] = {};
    reader->ReadBinary(MemoryBlock { readData, sizeof(readData) });

    for (SizeT i = 0; i < sizeof(writeData); ++i)
    {
        EXPECT_EQ(readData[i], writeData[i]);
    }
}

TEST_F(FileRWTest, FileReader_Read_WithCount)
{
    String path = GetTestFilePath("read_with_count.txt");
    FileWriter::WriteFileAsString("0123456789", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    char                buffer[10] = {};
    const Expect<SizeT> ret        = reader->ReadBinary(MemoryBlock { buffer, 10 }, 5);

    ASSERT_TRUE(ret.HasValue());
    EXPECT_EQ(ret.GetValue(), 5);
    EXPECT_STREQ(buffer, "01234");
}

TEST_F(FileRWTest, FileReader_Read_ZeroCount_ReadRemaining)
{
    String path = GetTestFilePath("read_zero_count.txt");
    FileWriter::WriteFileAsString("ABCDEFGHIJ", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    reader->Seek(3, eSeekOp::Set);

    char                buffer[20] = {};
    const Expect<SizeT> ret        = reader->ReadBinary(MemoryBlock { buffer, 20 }, 0);

    ASSERT_TRUE(ret.HasValue());
    EXPECT_EQ(ret.GetValue(), 7);
    EXPECT_STREQ(buffer, "DEFGHIJ");
}

TEST_F(FileRWTest, FileReader_Read_CountLargerThanRemaining)
{
    String path = GetTestFilePath("read_count_larger.txt");
    FileWriter::WriteFileAsString("12345", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    char                buffer[10] = {};
    const Expect<SizeT> ret        = reader->ReadBinary(MemoryBlock { buffer, 10 }, 3);

    ASSERT_TRUE(ret.HasValue());
    EXPECT_EQ(ret.GetValue(), 3);
    EXPECT_STREQ(buffer, "123");
}

// ============================================================================
// FileReader ReadFileAsBinary Tests
// ============================================================================

TEST_F(FileRWTest, FileReader_ReadFileAsMemory_Success)
{
    String path = GetTestFilePath("read_as_memory.txt");
    FileWriter::WriteFileAsString("Test Content", path);

    Expect<Vector<Byte>> data = FileReader::ReadFileAsBinary(path);

    ASSERT_TRUE(data.HasValue());
    EXPECT_EQ(data->size(), 12);
    EXPECT_EQ(memcmp(data->data(), "Test Content", 12), 0);
}

TEST_F(FileRWTest, FileReader_ReadFileAsMemory_NonExistent)
{
    String path = GetTestFilePath("non_existent_memory.txt");

    Expect<Vector<Byte>> data = FileReader::ReadFileAsBinary(path);

    EXPECT_FALSE(data.HasValue());
    EXPECT_EQ(data.GetError(), Error::OpenFailed);
}

TEST_F(FileRWTest, FileReader_ReadFileAsMemory_LargeFile)
{
    String path = GetTestFilePath("read_large.bin");

    const SizeT  dataSize = 1024 * 1024;
    Vector<Byte> writeData(dataSize);
    for (SizeT i = 0; i < dataSize; ++i)
    {
        writeData[i] = static_cast<Byte>(i % 256);
    }
    FileWriter::WriteFileAsBinary(CMemoryBlock { writeData.data(), dataSize }, path);

    Expect<Vector<Byte>> readData = FileReader::ReadFileAsBinary(path);

    ASSERT_TRUE(readData.HasValue());
    EXPECT_EQ(readData->size(), dataSize);
    EXPECT_EQ(memcmp(readData->data(), writeData.data(), dataSize), 0);
}

TEST_F(FileRWTest, FileReader_ReadFileAsMemory_BinaryData)
{
    String path = GetTestFilePath("read_binary_mem.bin");

    Byte writeData[] = { 0x00, 0xFF, 0x7F, 0x80, 0x01, 0xFE };
    FileWriter::WriteFileAsBinary(CMemoryBlock { writeData, sizeof(writeData) }, path);

    Expect<Vector<Byte>> readData = FileReader::ReadFileAsBinary(path);

    ASSERT_TRUE(readData.HasValue());
    ASSERT_EQ(readData->size(), sizeof(writeData));

    for (SizeT i = 0; i < sizeof(writeData); ++i)
    {
        EXPECT_EQ((*readData)[i], writeData[i]);
    }
}

// ============================================================================
// FileReader ReadFileAsString Tests
// ============================================================================

TEST_F(FileRWTest, FileReader_ReadFileAsString_Success)
{
    String path = GetTestFilePath("read_as_string.txt");
    FileWriter::WriteFileAsString("String Content", path);

    Expect<String> str = FileReader::ReadFileAsString(path);

    ASSERT_TRUE(str.HasValue());
    EXPECT_EQ(str.GetValue(), "String Content");
}

TEST_F(FileRWTest, FileReader_ReadFileAsString_NonExistent)
{
    String path = GetTestFilePath("non_existent_string.txt");

    Expect<String> str = FileReader::ReadFileAsString(path);

    EXPECT_FALSE(str.HasValue());
    EXPECT_EQ(str.GetError(), Error::OpenFailed);
}

TEST_F(FileRWTest, FileReader_ReadFileAsString_MultiLine)
{
    String path = GetTestFilePath("multiline.txt");
    FileWriter::WriteFileAsString("Line 1\nLine 2\nLine 3", path);

    Expect<String> str = FileReader::ReadFileAsString(path);

    ASSERT_TRUE(str.HasValue());
    EXPECT_EQ(str.GetValue(), "Line 1\nLine 2\nLine 3");
}

TEST_F(FileRWTest, FileReader_ReadFileAsString_EmptyFile)
{
    String path = GetTestFilePath("empty_string_file.txt");
    FileWriter::WriteFileAsString("", path);

    Expect<String> str = FileReader::ReadFileAsString(path);

    ASSERT_TRUE(str.HasValue());
    EXPECT_EQ(str.GetValue().GetLength(), 0);
}

// ============================================================================
// FileReader ReadString Tests (New API)
// ============================================================================

TEST_F(FileRWTest, FileReader_ReadAsString_WholeFile)
{
    String path = GetTestFilePath("read_as_string_whole.txt");
    FileWriter::WriteFileAsString("Complete Text", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    Expect<String> result = reader->ReadString();

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.GetValue(), "Complete Text");
}

TEST_F(FileRWTest, FileReader_ReadAsString_WithCount)
{
    String path = GetTestFilePath("read_as_string_count.txt");
    FileWriter::WriteFileAsString("ABCDEFGHIJ", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    Expect<String> result1 = reader->ReadString(5);
    ASSERT_TRUE(result1.HasValue());
    EXPECT_EQ(result1.GetValue(), "ABCDE");

    Expect<String> result2 = reader->ReadString(3);
    ASSERT_TRUE(result2.HasValue());
    EXPECT_EQ(result2.GetValue(), "FGH");
}

TEST_F(FileRWTest, FileReader_ReadAsString_ZeroCount_ReadRemaining)
{
    String path = GetTestFilePath("read_as_string_zero.txt");
    FileWriter::WriteFileAsString("0123456789", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    reader->Seek(5, eSeekOp::Set);

    Expect<String> result = reader->ReadString(0);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.GetValue(), "56789");
}

TEST_F(FileRWTest, FileReader_ReadAsString_MultipleReads)
{
    String path = GetTestFilePath("read_as_string_multiple.txt");
    FileWriter::WriteFileAsString("AAA-BBB-CCC", path);

    auto reader = FileReader::Open(path);

    Expect<String> part1 = reader->ReadString(3);
    EXPECT_EQ(part1.GetValue(), "AAA");

    reader->Skip(1);

    Expect<String> part2 = reader->ReadString(3);
    EXPECT_EQ(part2.GetValue(), "BBB");

    reader->Skip(1);

    Expect<String> part3 = reader->ReadString(3);
    EXPECT_EQ(part3.GetValue(), "CCC");
}

// ============================================================================
// FileReader State Tests
// ============================================================================

TEST_F(FileRWTest, FileReader_IsEof)
{
    String path = GetTestFilePath("eof_test.txt");
    FileWriter::WriteFileAsString("12345", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    EXPECT_FALSE(reader->IsEof());

    char buffer[5];
    reader->ReadBinary(MemoryBlock { buffer, 5 });

    EXPECT_FALSE(reader->IsEof());

    char extraBuffer[1];
    reader->ReadBinary(MemoryBlock { extraBuffer, 1 });

    EXPECT_TRUE(reader->IsEof());
}

TEST_F(FileRWTest, FileReader_Close)
{
    String path = GetTestFilePath("reader_close.txt");
    FileWriter::WriteFileAsString("Test", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    EXPECT_FALSE(reader->IsClosed());

    reader->Close();
    EXPECT_TRUE(reader->IsClosed());

    reader->Close();
    EXPECT_TRUE(reader->IsClosed());
}

TEST_F(FileRWTest, FileReader_MoveConstructor)
{
    String path = GetTestFilePath("reader_move_ctor.txt");
    FileWriter::WriteFileAsString("Test", path);

    auto reader1 = FileReader::Open(path);
    ASSERT_TRUE(reader1.HasValue());

    FileReader reader2 = std::move(reader1.GetValue());

    EXPECT_FALSE(reader2.IsClosed());

    char buffer[4];
    reader2.ReadBinary(MemoryBlock { buffer, 4 });
    EXPECT_EQ(memcmp(buffer, "Test", 4), 0);
}

TEST_F(FileRWTest, FileReader_MoveAssignment)
{
    String path1 = GetTestFilePath("reader_move_assign1.txt");
    String path2 = GetTestFilePath("reader_move_assign2.txt");

    FileWriter::WriteFileAsString("Data1", path1);
    FileWriter::WriteFileAsString("Data2", path2);

    auto reader1 = FileReader::Open(path1);
    auto reader2 = FileReader::Open(path2);

    ASSERT_TRUE(reader1.HasValue());
    ASSERT_TRUE(reader2.HasValue());

    reader2.GetValue() = std::move(reader1.GetValue());

    Expect<String> str = reader2->ReadString();
    EXPECT_EQ(str.GetValue(), "Data1");
}

// ============================================================================
// FileIO Seek Tests
// ============================================================================

TEST_F(FileRWTest, Seek_Set)
{
    String path = GetTestFilePath("seek_set.txt");
    FileWriter::WriteFileAsString("0123456789", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    reader->Seek(5, eSeekOp::Set);
    EXPECT_EQ(reader->Tell(), 5);

    char buffer[5];
    reader->ReadBinary(MemoryBlock { buffer, 5 });
    EXPECT_EQ(memcmp(buffer, "56789", 5), 0);
}

TEST_F(FileRWTest, Seek_Current)
{
    String path = GetTestFilePath("seek_current.txt");
    FileWriter::WriteFileAsString("ABCDEFGHIJ", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    char buffer[2];
    reader->ReadBinary(MemoryBlock { buffer, 2 });

    reader->Seek(3, eSeekOp::Current);
    EXPECT_EQ(reader->Tell(), 5);

    reader->ReadBinary(MemoryBlock { buffer, 2 });
    EXPECT_EQ(buffer[0], 'F');
    EXPECT_EQ(buffer[1], 'G');
}

TEST_F(FileRWTest, Seek_End)
{
    String path = GetTestFilePath("seek_end.txt");
    FileWriter::WriteFileAsString("0123456789", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    reader->Seek(0, eSeekOp::End);
    EXPECT_EQ(reader->Tell(), 10);

    reader->Seek(-5, eSeekOp::End);
    EXPECT_EQ(reader->Tell(), 5);

    char buffer[5];
    reader->ReadBinary(MemoryBlock { buffer, 5 });
    EXPECT_EQ(memcmp(buffer, "56789", 5), 0);
}

TEST_F(FileRWTest, Seek_NegativeOffset_Current)
{
    String path = GetTestFilePath("seek_negative.txt");
    FileWriter::WriteFileAsString("0123456789", path);

    auto reader = FileReader::Open(path);
    reader->Seek(7, eSeekOp::Set);

    reader->Seek(-3, eSeekOp::Current);
    EXPECT_EQ(reader->Tell(), 4);

    Expect<String> str = reader->ReadString(3);
    EXPECT_EQ(str.GetValue(), "456");
}

TEST_F(FileRWTest, Seek_MultipleOperations)
{
    String path = GetTestFilePath("seek_multiple.txt");
    FileWriter::WriteFileAsString("ABCDEFGHIJ", path);

    auto reader = FileReader::Open(path);

    reader->Seek(5, eSeekOp::Set);
    EXPECT_EQ(reader->Tell(), 5);

    reader->Seek(2, eSeekOp::Current);
    EXPECT_EQ(reader->Tell(), 7);

    reader->Seek(-2, eSeekOp::End);
    EXPECT_EQ(reader->Tell(), 8);

    reader->Seek(0, eSeekOp::Set);
    EXPECT_EQ(reader->Tell(), 0);
}

// ============================================================================
// FileIO Skip, Tell, Rewind Tests
// ============================================================================

TEST_F(FileRWTest, Skip)
{
    String path = GetTestFilePath("skip_test.txt");
    FileWriter::WriteFileAsString("ABCDEFGHIJ", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    reader->Skip(3);
    EXPECT_EQ(reader->Tell(), 3);

    char buffer[2];
    reader->ReadBinary(MemoryBlock { buffer, 2 });
    EXPECT_EQ(buffer[0], 'D');
    EXPECT_EQ(buffer[1], 'E');
}

TEST_F(FileRWTest, Skip_Multiple)
{
    String path = GetTestFilePath("skip_multiple.txt");
    FileWriter::WriteFileAsString("0123456789", path);

    auto reader = FileReader::Open(path);

    reader->Skip(2);
    reader->Skip(2);
    reader->Skip(2);

    EXPECT_EQ(reader->Tell(), 6);

    Expect<String> str = reader->ReadString(4);
    EXPECT_EQ(str.GetValue(), "6789");
}

TEST_F(FileRWTest, Tell)
{
    String path = GetTestFilePath("tell_test.txt");
    FileWriter::WriteFileAsString("Test", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    EXPECT_EQ(reader->Tell(), 0);

    char buffer[2];
    reader->ReadBinary(MemoryBlock { buffer, 2 });
    EXPECT_EQ(reader->Tell(), 2);

    reader->ReadBinary(MemoryBlock { buffer, 2 });
    EXPECT_EQ(reader->Tell(), 4);
}

TEST_F(FileRWTest, Tell_AfterSeek)
{
    String path = GetTestFilePath("tell_after_seek.txt");
    FileWriter::WriteFileAsString("0123456789", path);

    auto reader = FileReader::Open(path);

    EXPECT_EQ(reader->Tell(), 0);

    char buffer[3];
    reader->ReadBinary(MemoryBlock { buffer, 3 }, 3);
    EXPECT_EQ(reader->Tell(), 3);

    reader->Skip(2);
    EXPECT_EQ(reader->Tell(), 5);

    reader->Seek(-2, eSeekOp::Current);
    EXPECT_EQ(reader->Tell(), 3);

    reader->Rewind();
    EXPECT_EQ(reader->Tell(), 0);
}

TEST_F(FileRWTest, Rewind)
{
    String path = GetTestFilePath("rewind_test.txt");
    FileWriter::WriteFileAsString("ABCDEF", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    char buffer[3];
    reader->ReadBinary(MemoryBlock { buffer, 3 });
    EXPECT_EQ(reader->Tell(), 3);

    reader->Rewind();
    EXPECT_EQ(reader->Tell(), 0);

    reader->ReadBinary(MemoryBlock { buffer, 3 });
    EXPECT_EQ(memcmp(buffer, "ABC", 3), 0);
}

TEST_F(FileRWTest, Rewind_Multiple)
{
    String path = GetTestFilePath("rewind_multiple.txt");
    FileWriter::WriteFileAsString("ABCDEF", path);

    auto reader = FileReader::Open(path);

    for (int i = 0; i < 5; ++i)
    {
        reader->Rewind();
        EXPECT_EQ(reader->Tell(), 0);

        Expect<String> str = reader->ReadString(3);
        EXPECT_EQ(str.GetValue(), "ABC");
    }
}

// ============================================================================
// FileIO Remaining and Size Tests
// ============================================================================

TEST_F(FileRWTest, Remaining)
{
    String path = GetTestFilePath("remaining_test.txt");
    FileWriter::WriteFileAsString("0123456789", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    EXPECT_EQ(reader->Remaining(), 10);

    char buffer[3];
    reader->ReadBinary(MemoryBlock { buffer, 3 });
    EXPECT_EQ(reader->Remaining(), 7);

    reader->ReadBinary(MemoryBlock { buffer, 3 });
    EXPECT_EQ(reader->Remaining(), 4);
}

TEST_F(FileRWTest, Remaining_AfterSeek)
{
    String path = GetTestFilePath("remaining_after_seek.txt");
    FileWriter::WriteFileAsString("ABCDEFGHIJ", path);

    auto reader = FileReader::Open(path);

    EXPECT_EQ(reader->Remaining(), 10);

    reader->Seek(3, eSeekOp::Set);
    EXPECT_EQ(reader->Remaining(), 7);

    reader->Seek(0, eSeekOp::End);
    EXPECT_EQ(reader->Remaining(), 0);

    reader->Rewind();
    EXPECT_EQ(reader->Remaining(), 10);
}

TEST_F(FileRWTest, CalcTotalSize)
{
    String path = GetTestFilePath("total_size.txt");
    FileWriter::WriteFileAsString("TestData", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    EXPECT_EQ(reader->CalcTotalSize(), 8);

    char buffer[3];
    reader->ReadBinary(MemoryBlock { buffer, 3 });
    EXPECT_EQ(reader->CalcTotalSize(), 8);
    EXPECT_EQ(reader->Tell(), 3);
}

TEST_F(FileRWTest, CalcTotalSize_EmptyFile)
{
    String path = GetTestFilePath("empty_size.txt");
    FileWriter::WriteFileAsString("", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    EXPECT_EQ(reader->CalcTotalSize(), 0);
}

TEST_F(FileRWTest, CalcTotalSize_LargeFile)
{
    String path = GetTestFilePath("large_size.bin");

    const SizeT  dataSize = 10 * 1024 * 1024;
    Vector<Byte> data(dataSize, Byte(0xAB));
    FileWriter::WriteFileAsBinary(CMemoryBlock { data.data(), dataSize }, path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    EXPECT_EQ(reader->CalcTotalSize(), dataSize);
}

TEST_F(FileRWTest, CalcTotalSize_DoesNotChangePosition)
{
    String path = GetTestFilePath("size_no_change_pos.txt");
    FileWriter::WriteFileAsString("ABCDEFGHIJ", path);

    auto reader = FileReader::Open(path);

    reader->Seek(5, eSeekOp::Set);
    EXPECT_EQ(reader->Tell(), 5);

    SizeT size = reader->CalcTotalSize();
    EXPECT_EQ(size, 10);
    EXPECT_EQ(reader->Tell(), 5);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(FileRWTest, Integration_WriteAndReadBack)
{
    String path = GetTestFilePath("integration_write_read.txt");

    {
        auto writer = FileWriter::Open(path);
        writer->WriteString("Line 1\n");
        writer->WriteString("Line 2\n");
        writer->WriteString("Line 3");
    }

    {
        auto   reader = FileReader::Open(path);
        String result = FileReader::ReadFileAsString(path).GetValue();
        EXPECT_EQ(result, "Line 1\nLine 2\nLine 3");
    }
}

TEST_F(FileRWTest, Integration_MultipleFilesSimultaneously)
{
    String path1 = GetTestFilePath("multi_file_1.txt");
    String path2 = GetTestFilePath("multi_file_2.txt");
    String path3 = GetTestFilePath("multi_file_3.txt");

    auto writer1 = FileWriter::Open(path1);
    auto writer2 = FileWriter::Open(path2);
    auto writer3 = FileWriter::Open(path3);

    writer1->WriteString("File1");
    writer2->WriteString("File2");
    writer3->WriteString("File3");

    writer1->Close();
    writer2->Close();
    writer3->Close();

    EXPECT_TRUE(VerifyFileContentString(path1, "File1"));
    EXPECT_TRUE(VerifyFileContentString(path2, "File2"));
    EXPECT_TRUE(VerifyFileContentString(path3, "File3"));
}

TEST_F(FileRWTest, Integration_ReadWriteSeek)
{
    String path = GetTestFilePath("read_write_seek.txt");

    FileWriter::WriteFileAsString("0123456789ABCDEF", path);

    auto reader = FileReader::Open(path);

    reader->Seek(5, eSeekOp::Set);
    char buffer1[5];
    reader->ReadBinary(MemoryBlock { buffer1, 5 });
    EXPECT_EQ(memcmp(buffer1, "56789", 5), 0);

    reader->Rewind();
    char buffer2[4];
    reader->ReadBinary(MemoryBlock { buffer2, 4 });
    EXPECT_EQ(memcmp(buffer2, "0123", 4), 0);

    reader->Seek(-4, eSeekOp::End);
    char buffer3[4];
    reader->ReadBinary(MemoryBlock { buffer3, 4 });
    EXPECT_EQ(memcmp(buffer3, "CDEF", 4), 0);
}

TEST_F(FileRWTest, Integration_SequentialOperations)
{
    String path = GetTestFilePath("sequential_ops.txt");

    for (int i = 0; i < 10; ++i)
    {
        String content = "Iteration ";
        content += std::to_string(i).c_str();

        FileWriter::WriteFileAsString(content, path, eFileWriterCollisionPolicy::Overwrite);

        String readBack = FileReader::ReadFileAsString(path).GetValue();
        EXPECT_EQ(readBack, content);
    }
}

TEST_F(FileRWTest, Integration_BinaryRoundTrip)
{
    String path = GetTestFilePath("binary_roundtrip.bin");

    struct TestData
    {
        int32_t  intVal;
        float    floatVal;
        uint8_t  byteArray[10];
        uint64_t longVal;
    };

    TestData writeData = {};
    writeData.intVal   = -12345;
    writeData.floatVal = 3.14159f;
    for (int i = 0; i < 10; ++i)
        writeData.byteArray[i] = static_cast<uint8_t>(i * 25);
    writeData.longVal = 9876543210ULL;

    FileWriter::WriteFileAsBinary(CMemoryBlock { &writeData, sizeof(TestData) }, path);

    auto readData = FileReader::ReadFileAsBinary(path);
    ASSERT_TRUE(readData.HasValue());
    ASSERT_EQ(readData->size(), sizeof(TestData));

    TestData* pReadData = reinterpret_cast<TestData*>(readData->data());
    EXPECT_EQ(pReadData->intVal, writeData.intVal);
    EXPECT_FLOAT_EQ(pReadData->floatVal, writeData.floatVal);
    EXPECT_EQ(pReadData->longVal, writeData.longVal);
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(pReadData->byteArray[i], writeData.byteArray[i]);
    }
}

TEST_F(FileRWTest, Integration_MixedReadOperations)
{
    String path = GetTestFilePath("mixed_reads.txt");
    FileWriter::WriteFileAsString("0123456789ABCDEF", path);

    auto reader = FileReader::Open(path);

    char buffer[4];
    reader->ReadBinary(MemoryBlock { buffer, 4 }, 4);
    EXPECT_EQ(memcmp(buffer, "0123", 4), 0);

    Expect<String> str = reader->ReadString(4);
    EXPECT_EQ(str.GetValue(), "4567");

    reader->ReadBinary(MemoryBlock { buffer, 4 }, 2);
    EXPECT_EQ(memcmp(buffer, "89", 2), 0);
}

TEST_F(FileRWTest, Integration_VeryLongFilename)
{
    String longName = "very_long_filename_";
    for (int i = 0; i < 10; ++i)
    {
        longName += "abcdefghij";
    }
    longName += ".txt";

    String path = GetTestFilePath(longName);

    auto writer = FileWriter::Open(path);
    ASSERT_TRUE(writer.HasValue());
    writer->WriteString("Test");
    writer->Close();

    EXPECT_TRUE(FileSystem::Exists(path));
    EXPECT_TRUE(VerifyFileContentString(path, "Test"));
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(FileRWTest, Stress_ManySmallWrites)
{
    String path = GetTestFilePath("many_small_writes.txt");

    auto writer = FileWriter::Open(path);
    for (int i = 0; i < 1000; ++i)
    {
        writer->WriteString("X");
    }
    writer->Close();

    auto reader = FileReader::Open(path);
    EXPECT_EQ(reader->CalcTotalSize(), 1000);
}

TEST_F(FileRWTest, Stress_ManySmallReads)
{
    String path = GetTestFilePath("many_small_reads.txt");

    Vector<Byte> writeData(1000);
    for (int i = 0; i < 1000; ++i)
    {
        writeData[i] = static_cast<Byte>(i % 256);
    }
    FileWriter::WriteFileAsBinary(CMemoryBlock { writeData.data(), 1000 }, path);

    auto reader = FileReader::Open(path);
    for (int i = 0; i < 1000; ++i)
    {
        Byte b;
        reader->ReadBinary(MemoryBlock { &b, 1 });
        EXPECT_EQ(b, static_cast<Byte>(i % 256));
    }
}

TEST_F(FileRWTest, Stress_LargeSequentialWrite)
{
    String path = GetTestFilePath("stress_seq_write.bin");

    const SizeT  blockSize  = 64 * 1024;
    const int    blockCount = 16;
    Vector<Byte> block(blockSize);

    for (SizeT i = 0; i < blockSize; ++i)
    {
        block[i] = static_cast<Byte>(i % 256);
    }

    auto writer = FileWriter::Open(path);

    for (int i = 0; i < blockCount; ++i)
    {
        writer->WriteBinary(CMemoryBlock { block.data(), blockSize });
    }

    writer->Close();

    auto reader = FileReader::Open(path);
    EXPECT_EQ(reader->CalcTotalSize(), blockSize * blockCount);
}

TEST_F(FileRWTest, Stress_LargeSequentialRead)
{
    String path = GetTestFilePath("stress_seq_read.bin");

    const SizeT  totalSize = 1024 * 1024;
    Vector<Byte> writeData(totalSize);
    for (SizeT i = 0; i < totalSize; ++i)
    {
        writeData[i] = static_cast<Byte>(i % 256);
    }
    FileWriter::WriteFileAsBinary(CMemoryBlock { writeData.data(), totalSize }, path);

    auto         reader    = FileReader::Open(path);
    const SizeT  chunkSize = 4096;
    Vector<Byte> readBuffer(chunkSize);
    SizeT        totalRead = 0;

    while (totalRead < totalSize)
    {
        SizeT toRead = (totalSize - totalRead) < chunkSize ? (totalSize - totalRead) : chunkSize;
        auto  ret    = reader->ReadBinary(MemoryBlock { readBuffer.data(), chunkSize }, toRead);
        ASSERT_TRUE(ret.HasValue());
        totalRead += ret.GetValue();
    }

    EXPECT_EQ(totalRead, totalSize);
}

TEST_F(FileRWTest, Stress_RandomAccessPattern)
{
    String path = GetTestFilePath("stress_random_access.bin");

    const SizeT  size = 1024 * 1024;
    Vector<Byte> data(size);
    for (SizeT i = 0; i < size; ++i)
    {
        data[i] = static_cast<Byte>(i % 256);
    }
    FileWriter::WriteFileAsBinary(CMemoryBlock { data.data(), size }, path);

    auto reader = FileReader::Open(path);

    const SizeT offsets[] = { 0, 1000, 5000, 10000, 50000, 100000, 500000 };
    for (SizeT offset: offsets)
    {
        reader->Seek(offset, eSeekOp::Set);
        Byte b;
        reader->ReadBinary(MemoryBlock { &b, 1 });
        EXPECT_EQ(b, static_cast<Byte>(offset % 256));
    }
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(FileRWTest, EdgeCase_ReadBeyondEof)
{
    String path = GetTestFilePath("read_beyond.txt");
    FileWriter::WriteFileAsString("ABC", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    char                buffer[10] = {};
    const Expect<SizeT> ret        = reader->ReadBinary(MemoryBlock { buffer, 10 });

    EXPECT_TRUE(ret.HasValue());
}

TEST_F(FileRWTest, EdgeCase_MultipleAppends)
{
    String path = GetTestFilePath("multiple_appends.txt");

    FileWriter::WriteFileAsString("1", path, eFileWriterCollisionPolicy::Overwrite);
    FileWriter::WriteFileAsString("2", path, eFileWriterCollisionPolicy::Append);
    FileWriter::WriteFileAsString("3", path, eFileWriterCollisionPolicy::Append);
    FileWriter::WriteFileAsString("4", path, eFileWriterCollisionPolicy::Append);

    EXPECT_TRUE(VerifyFileContentString(path, "1234"));
}

TEST_F(FileRWTest, EdgeCase_ZeroSizeFile)
{
    String path = GetTestFilePath("zero_size.txt");
    FileWriter::WriteFileAsString("", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    EXPECT_EQ(reader->CalcTotalSize(), 0);
    EXPECT_EQ(reader->Remaining(), 0);
    EXPECT_EQ(reader->Tell(), 0);
}

TEST_F(FileRWTest, EdgeCase_SingleByteFile)
{
    String path = GetTestFilePath("single_byte.txt");
    FileWriter::WriteFileAsString("A", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    EXPECT_EQ(reader->CalcTotalSize(), 1);
    EXPECT_EQ(reader->Remaining(), 1);

    char buffer[1];
    reader->ReadBinary(MemoryBlock { buffer, 1 });
    EXPECT_EQ(buffer[0], 'A');
    EXPECT_EQ(reader->Remaining(), 0);
}

TEST_F(FileRWTest, EdgeCase_SeekBeyondFileSize)
{
    String path = GetTestFilePath("seek_beyond.txt");
    FileWriter::WriteFileAsString("Short", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    reader->Seek(100, eSeekOp::Set);
    EXPECT_TRUE(reader->Tell() >= 100);
}

// ============================================================================
// Comprehensive Tests
// ============================================================================

TEST_F(FileRWTest, Comprehensive_WriteAsString_vs_Write)
{
    String pathWrite      = GetTestFilePath("write_null_term.txt");
    String pathWriteAsStr = GetTestFilePath("write_as_string.txt");

    const char* testStr = "Test";

    {
        auto writer = FileWriter::Open(pathWrite);
        writer->WriteBinary(CMemoryBlock { testStr, 5 });
    }

    {
        auto writer = FileWriter::Open(pathWriteAsStr);
        writer->WriteString(testStr);
    }

    auto reader1 = FileReader::Open(pathWrite);
    auto reader2 = FileReader::Open(pathWriteAsStr);

    EXPECT_EQ(reader1->CalcTotalSize(), 5);
    EXPECT_EQ(reader2->CalcTotalSize(), 4);
}

TEST_F(FileRWTest, Comprehensive_ReadAsString_PartialReads)
{
    String path = GetTestFilePath("read_as_string_partial.txt");
    FileWriter::WriteFileAsString("ABCDEFGHIJKLMNOPQRSTUVWXYZ", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    Expect<String> part1 = reader->ReadString(5);
    EXPECT_EQ(part1.GetValue(), "ABCDE");
    EXPECT_EQ(reader->Tell(), 5);

    reader->Skip(10);

    Expect<String> part2 = reader->ReadString(5);
    EXPECT_EQ(part2.GetValue(), "PQRST");
}

TEST_F(FileRWTest, Comprehensive_SeekAndRead_AllModes)
{
    String path = GetTestFilePath("seek_all_modes.txt");
    FileWriter::WriteFileAsString("ABCDEFGHIJKLMNOP", path);

    auto reader = FileReader::Open(path);

    reader->Seek(5, eSeekOp::Set);
    Expect<String> str1 = reader->ReadString(3);
    EXPECT_EQ(str1.GetValue(), "FGH");

    reader->Seek(2, eSeekOp::Current);
    Expect<String> str2 = reader->ReadString(2);
    EXPECT_EQ(str2.GetValue(), "KL");

    reader->Seek(-4, eSeekOp::End);
    Expect<String> str3 = reader->ReadString(4);
    EXPECT_EQ(str3.GetValue(), "MNOP");
}

TEST_F(FileRWTest, Comprehensive_LargeFileOperations)
{
    String path = GetTestFilePath("large_comprehensive.bin");

    const SizeT  size = 5 * 1024 * 1024;
    Vector<Byte> data(size);
    for (SizeT i = 0; i < size; ++i)
    {
        data[i] = static_cast<Byte>((i * 7 + 13) % 256);
    }

    FileWriter::WriteFileAsBinary(CMemoryBlock { data.data(), size }, path);

    auto reader = FileReader::Open(path);
    EXPECT_EQ(reader->CalcTotalSize(), size);

    reader->Seek(1024 * 1024, eSeekOp::Set);
    Vector<Byte> readBuffer(1024);
    reader->ReadBinary(MemoryBlock { readBuffer.data(), 1024 });

    for (SizeT i = 0; i < 1024; ++i)
    {
        const SizeT originalIdx = 1024 * 1024 + i;
        EXPECT_EQ(readBuffer[i], static_cast<Byte>((originalIdx * 7 + 13) % 256));
    }
}

TEST_F(FileRWTest, Comprehensive_AlternatingWriteRead)
{
    for (int iteration = 0; iteration < 5; ++iteration)
    {
        String path = GetTestFilePath("alternating.txt");

        String content = "Data";
        content += std::to_string(iteration).c_str();

        FileWriter::WriteFileAsString(content, path, eFileWriterCollisionPolicy::Overwrite);

        String readBack = FileReader::ReadFileAsString(path).GetValue();
        EXPECT_EQ(readBack, content);
    }
}

TEST_F(FileRWTest, Comprehensive_BinaryIntegrity)
{
    String path = GetTestFilePath("binary_integrity.bin");

    Vector<Byte> original(256);
    for (int i = 0; i < 256; ++i)
    {
        original[i] = static_cast<Byte>(i);
    }

    FileWriter::WriteFileAsBinary(CMemoryBlock { original.data(), 256 }, path);

    for (int j = 0; j < 10; ++j)
    {
        auto readBack = FileReader::ReadFileAsBinary(path);
        ASSERT_TRUE(readBack.HasValue());
        ASSERT_EQ(readBack->size(), 256);

        for (int i = 0; i < 256; ++i)
        {
            EXPECT_EQ((*readBack)[i], static_cast<Byte>(i));
        }
    }
}

TEST_F(FileRWTest, Comprehensive_ReadWrite_SpecialCharacters)
{
    String path = GetTestFilePath("special_chars.txt");

    const char specialChars[] = "!@#$%^&*()_+-=[]{}|;':\",./<>?\n\r\t";
    FileWriter::WriteFileAsString(specialChars, path);

    String readBack = FileReader::ReadFileAsString(path).GetValue();
    EXPECT_EQ(readBack, specialChars);
}

TEST_F(FileRWTest, Comprehensive_NullByteHandling)
{
    String path = GetTestFilePath("null_bytes.bin");

    Byte data[] = { 0x41, 0x00, 0x42, 0x00, 0x43 };
    FileWriter::WriteFileAsBinary(CMemoryBlock { data, 5 }, path);

    auto readBack = FileReader::ReadFileAsBinary(path);
    ASSERT_TRUE(readBack.HasValue());
    ASSERT_EQ(readBack->size(), 5);

    EXPECT_EQ((*readBack)[0], 0x41);
    EXPECT_EQ((*readBack)[1], 0x00);
    EXPECT_EQ((*readBack)[2], 0x42);
    EXPECT_EQ((*readBack)[3], 0x00);
    EXPECT_EQ((*readBack)[4], 0x43);
}

TEST_F(FileRWTest, Comprehensive_GetFilePtr)
{
    String path = GetTestFilePath("get_file_ptr.txt");
    FileWriter::WriteFileAsString("Test", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    FILE* ptr = reader->GetFilePtr();
    EXPECT_NE(ptr, nullptr);

    reader->Close();
    FILE* ptrAfterClose = reader->GetFilePtr();
    EXPECT_EQ(ptrAfterClose, nullptr);
}

TEST_F(FileRWTest, Comprehensive_IsError_NormalOperation)
{
    String path = GetTestFilePath("is_error.txt");
    FileWriter::WriteFileAsString("Test", path);

    auto reader = FileReader::Open(path);
    ASSERT_TRUE(reader.HasValue());

    EXPECT_FALSE(reader->IsError());

    char buffer[4];
    reader->ReadBinary(MemoryBlock { buffer, 4 });

    EXPECT_FALSE(reader->IsError());
}

// ============================================================================
// Robustness Tests
// ============================================================================

TEST_F(FileRWTest, Robustness_MultipleRenameCollisions)
{
    String path = GetTestFilePath("multi_rename.txt");

    for (int i = 0; i < 5; ++i)
    {
        auto writer = FileWriter::Open(path, eFileWriterCollisionPolicy::Rename);
        ASSERT_TRUE(writer.HasValue());
        writer->WriteString("Version ");
        writer->WriteString(std::to_string(i).c_str());
        writer->Close();
    }

    EXPECT_TRUE(FileSystem::Exists(GetTestFilePath("multi_rename.txt")));
    EXPECT_TRUE(FileSystem::Exists(GetTestFilePath("multi_rename (1).txt")));
    EXPECT_TRUE(FileSystem::Exists(GetTestFilePath("multi_rename (2).txt")));
    EXPECT_TRUE(FileSystem::Exists(GetTestFilePath("multi_rename (3).txt")));
    EXPECT_TRUE(FileSystem::Exists(GetTestFilePath("multi_rename (4).txt")));
}

TEST_F(FileRWTest, Robustness_WriteToNestedDirectories)
{
    String path = testRoot;
    path.Join("A/B/C/D/E/test.txt", eJoinType::File);

    auto writer = FileWriter::Open(path);
    ASSERT_TRUE(writer.HasValue());
    writer->WriteString("Deep File");
    writer->Close();

    EXPECT_TRUE(FileSystem::Exists(path));
    EXPECT_TRUE(VerifyFileContentString(path, "Deep File"));
}

TEST_F(FileRWTest, Robustness_EmptyStringWrite)
{
    String path = GetTestFilePath("empty_string.txt");

    FileWriter::WriteFileAsString("", path);

    String readBack = FileReader::ReadFileAsString(path).GetValue();
    EXPECT_EQ(readBack, "");
    EXPECT_EQ(readBack.GetLength(), 0);
}

TEST_F(FileRWTest, Robustness_SingleCharacterOperations)
{
    String path = GetTestFilePath("single_char_ops.txt");

    auto writer = FileWriter::Open(path);
    for (char c = 'A'; c <= 'Z'; ++c)
    {
        String str;
        str += c;
        writer->WriteString(str);
    }
    writer->Close();

    auto reader = FileReader::Open(path);
    for (char c = 'A'; c <= 'Z'; ++c)
    {
        Expect<String> str = reader->ReadString(1);
        ASSERT_TRUE(str.HasValue());
        EXPECT_EQ(str.GetValue()[0], c);
    }
}

TEST_F(FileRWTest, Robustness_FileExtensions)
{
    const char* extensions[] = { ".txt", ".bin", ".dat", ".tmp", ".log", ".json", ".xml" };

    for (const char* ext: extensions)
    {
        String filename = "test";
        filename += ext;
        String path = GetTestFilePath(filename);

        FileWriter::WriteFileAsString("Test", path);
        EXPECT_TRUE(FileSystem::Exists(path));
    }
}

TEST_F(FileRWTest, Robustness_WriteThenReadSameHandle_NotSupported)
{
    String path = GetTestFilePath("same_handle.txt");

    auto writer = FileWriter::Open(path);
    writer->WriteString("Data");
    writer->Close();

    auto   reader = FileReader::Open(path);
    String result = reader->ReadString().GetValue();
    EXPECT_EQ(result, "Data");
}

// ============================================================================
// Additional Detailed Tests
// ============================================================================

TEST_F(FileRWTest, Detailed_ReadAsString_EmptyAfterSeek)
{
    String path = GetTestFilePath("read_empty_after_seek.txt");
    FileWriter::WriteFileAsString("12345", path);

    auto reader = FileReader::Open(path);
    reader->Seek(0, eSeekOp::End);

    Expect<String> str = reader->ReadString();
    ASSERT_TRUE(str.HasValue());
    EXPECT_EQ(str.GetValue().GetLength(), 0);
}

TEST_F(FileRWTest, Detailed_SkipLargeOffset)
{
    String path = GetTestFilePath("skip_large.txt");

    Vector<Byte> data(10000, Byte('X'));
    FileWriter::WriteFileAsBinary(CMemoryBlock { data.data(), 10000 }, path);

    auto reader = FileReader::Open(path);
    reader->Skip(9995);

    EXPECT_EQ(reader->Tell(), 9995);
    EXPECT_EQ(reader->Remaining(), 5);
}

TEST_F(FileRWTest, Detailed_SeekNegativeFromEnd)
{
    String path = GetTestFilePath("seek_negative_end.txt");
    FileWriter::WriteFileAsString("ABCDEFGHIJKLMNOPQRSTUVWXYZ", path);

    auto reader = FileReader::Open(path);

    reader->Seek(-1, eSeekOp::End);
    char buffer[1];
    reader->ReadBinary(MemoryBlock { buffer, 1 });
    EXPECT_EQ(buffer[0], 'Z');

    reader->Seek(-10, eSeekOp::End);
    Expect<String> str = reader->ReadString(5);
    EXPECT_EQ(str.GetValue(), "QRSTU");
}

TEST_F(FileRWTest, Detailed_ReadPartialThenReadAsString)
{
    String path = GetTestFilePath("partial_then_string.txt");
    FileWriter::WriteFileAsString("0123456789ABCDEF", path);

    auto reader = FileReader::Open(path);

    char buffer[5];
    reader->ReadBinary(MemoryBlock { buffer, 5 }, 5);
    EXPECT_EQ(memcmp(buffer, "01234", 5), 0);

    Expect<String> remaining = reader->ReadString();
    EXPECT_EQ(remaining.GetValue(), "56789ABCDEF");
}

TEST_F(FileRWTest, Detailed_WriteFlushWriteFlush)
{
    String path = GetTestFilePath("write_flush_pattern.txt");

    auto writer = FileWriter::Open(path);

    writer->WriteString("A");
    writer->Flush();

    writer->WriteString("B");
    writer->Flush();

    writer->WriteString("C");
    writer->Close();

    EXPECT_TRUE(VerifyFileContentString(path, "ABC"));
}

TEST_F(FileRWTest, Detailed_ReadBeyondBufferSize)
{
    String path = GetTestFilePath("read_beyond_buffer.txt");
    FileWriter::WriteFileAsString("123", path);

    auto reader = FileReader::Open(path);

    char                buffer[10] = {};
    const Expect<SizeT> ret        = reader->ReadBinary(MemoryBlock { buffer, 10 }, 3);

    ASSERT_TRUE(ret.HasValue());
    EXPECT_EQ(ret.GetValue(), 3);
}

TEST_F(FileRWTest, Detailed_MultipleSeeksWithReads)
{
    String path = GetTestFilePath("multi_seek_reads.txt");
    FileWriter::WriteFileAsString("0123456789", path);

    auto reader = FileReader::Open(path);

    reader->Seek(2, eSeekOp::Set);
    Expect<String> str1 = reader->ReadString(2);
    EXPECT_EQ(str1.GetValue(), "23");

    reader->Seek(5, eSeekOp::Set);
    Expect<String> str2 = reader->ReadString(2);
    EXPECT_EQ(str2.GetValue(), "56");

    reader->Seek(0, eSeekOp::Set);
    Expect<String> str3 = reader->ReadString(2);
    EXPECT_EQ(str3.GetValue(), "01");
}

TEST_F(FileRWTest, Detailed_WriteAsStringVsWrite_NullTerminator)
{
    String pathWriteAs = GetTestFilePath("null_term_writeas.txt");
    String pathWrite   = GetTestFilePath("null_term_write.txt");

    const char* text = "Test";

    {
        auto writer = FileWriter::Open(pathWriteAs);
        writer->WriteString(text);
        writer->Close();
    }

    {
        auto writer = FileWriter::Open(pathWrite);
        writer->WriteBinary(CMemoryBlock { text, 5 });
        writer->Close();
    }

    auto reader1 = FileReader::Open(pathWriteAs);
    auto reader2 = FileReader::Open(pathWrite);

    EXPECT_EQ(reader1->CalcTotalSize(), 4);
    EXPECT_EQ(reader2->CalcTotalSize(), 5);

    Vector<Byte> data1(4);
    Vector<Byte> data2(5);

    reader1->ReadBinary(MemoryBlock { data1.data(), 4 });
    reader2->ReadBinary(MemoryBlock { data2.data(), 5 });

    EXPECT_EQ(memcmp(data1.data(), "Test", 4), 0);
    EXPECT_EQ(memcmp(data2.data(), "Test\0", 5), 0);
}

TEST_F(FileRWTest, Detailed_EmptyFileOperations)
{
    String path = GetTestFilePath("empty_ops.txt");
    FileWriter::WriteFileAsString("", path);

    auto reader = FileReader::Open(path);

    EXPECT_EQ(reader->CalcTotalSize(), 0);
    EXPECT_EQ(reader->Remaining(), 0);
    EXPECT_EQ(reader->Tell(), 0);

    reader->Rewind();
    EXPECT_EQ(reader->Tell(), 0);

    reader->Seek(0, eSeekOp::End);
    EXPECT_EQ(reader->Tell(), 0);
}

TEST_F(FileRWTest, Detailed_SeekPerformance)
{
    String path = GetTestFilePath("seek_performance.txt");

    String longContent;
    for (int i = 0; i < 1000; ++i)
    {
        longContent += "0123456789";
    }
    FileWriter::WriteFileAsString(longContent, path);

    auto reader = FileReader::Open(path);

    for (int i = 0; i < 100; ++i)
    {
        reader->Seek(i * 100, eSeekOp::Set);
        EXPECT_EQ(reader->Tell(), i * 100);
    }
}

// ============================================================================
// Advanced Scenario Tests
// ============================================================================

TEST_F(FileRWTest, Advanced_PartialOverwrite)
{
    String path = GetTestFilePath("partial_overwrite.txt");

    FileWriter::WriteFileAsString("AAAAAAAAAA", path, eFileWriterCollisionPolicy::Overwrite);

    auto writer = FileWriter::Open(path, eFileWriterCollisionPolicy::Overwrite);
    writer->WriteString("BBB");
    writer->Close();

    EXPECT_TRUE(VerifyFileContentString(path, "BBB"));
}

TEST_F(FileRWTest, Advanced_ReadWriteReadPattern)
{
    String path = GetTestFilePath("rwr_pattern.txt");

    FileWriter::WriteFileAsString("Initial", path);

    String read1 = FileReader::ReadFileAsString(path).GetValue();
    EXPECT_EQ(read1, "Initial");

    FileWriter::WriteFileAsString("Modified", path, eFileWriterCollisionPolicy::Overwrite);

    String read2 = FileReader::ReadFileAsString(path).GetValue();
    EXPECT_EQ(read2, "Modified");
}

TEST_F(FileRWTest, Advanced_LargeFileRandomAccess)
{
    String path = GetTestFilePath("large_random.bin");

    const SizeT  size = 1024 * 1024;
    Vector<Byte> data(size);
    for (SizeT i = 0; i < size; ++i)
    {
        data[i] = static_cast<Byte>((i ^ (i >> 8)) & 0xFF);
    }

    FileWriter::WriteFileAsBinary(CMemoryBlock { data.data(), size }, path);

    auto reader = FileReader::Open(path);

    const SizeT testPositions[] = { 0, 1, 127, 128, 255, 256, 1023, 1024, 4095, 4096, size / 2, size - 1 };

    for (SizeT pos: testPositions)
    {
        if (pos >= size)
            continue;

        reader->Seek(pos, eSeekOp::Set);
        Byte b;
        reader->ReadBinary(MemoryBlock { &b, 1 });
        EXPECT_EQ(b, static_cast<Byte>((pos ^ (pos >> 8)) & 0xFF));
    }
}

TEST_F(FileRWTest, Advanced_ChunkedWriteAndVerify)
{
    String path = GetTestFilePath("chunked_write.bin");

    const SizeT chunkSize  = 1024;
    const int   chunkCount = 100;

    auto writer = FileWriter::Open(path);

    for (int chunk = 0; chunk < chunkCount; ++chunk)
    {
        Vector<Byte> chunkData(chunkSize);
        for (SizeT i = 0; i < chunkSize; ++i)
        {
            chunkData[i] = static_cast<Byte>((chunk + i) % 256);
        }
        writer->WriteBinary(CMemoryBlock { chunkData.data(), chunkSize });
    }

    writer->Close();

    auto reader = FileReader::Open(path);
    EXPECT_EQ(reader->CalcTotalSize(), chunkSize * chunkCount);

    for (int chunk = 0; chunk < chunkCount; ++chunk)
    {
        reader->Seek(chunk * chunkSize, eSeekOp::Set);
        Byte firstByte;
        reader->ReadBinary(MemoryBlock { &firstByte, 1 });
        EXPECT_EQ(firstByte, static_cast<Byte>(chunk % 256));
    }
}

TEST_F(FileRWTest, Advanced_InterleavedReadAndSeek)
{
    String path = GetTestFilePath("interleaved_read_seek.txt");
    FileWriter::WriteFileAsString("0123456789ABCDEFGHIJ", path);

    auto reader = FileReader::Open(path);

    Expect<String> str1 = reader->ReadString(2);
    EXPECT_EQ(str1.GetValue(), "01");

    reader->Seek(5, eSeekOp::Set);
    Expect<String> str2 = reader->ReadString(2);
    EXPECT_EQ(str2.GetValue(), "56");

    reader->Seek(-10, eSeekOp::End);
    Expect<String> str3 = reader->ReadString(2);
    EXPECT_EQ(str3.GetValue(), "AB");

    reader->Seek(3, eSeekOp::Current);
    Expect<String> str4 = reader->ReadString(2);
    EXPECT_EQ(str4.GetValue(), "FG");
}

}   // namespace Jam::File::Test
