#include "pch.h"
#include <gtest/gtest.h>
#include <fstream>

namespace Jam::FileSystem::Test
{

class FileSystemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        testRoot = Utf8::Current();
        testRoot.Join("TestSandbox/", eJoinType::Directory);

        if (Utf8::Exists(testRoot))
        {
            Utf8::Remove(testRoot);
        }
        Utf8::MakeDirectory(testRoot, eMakeDirectoryCollisionPolicy::Fail);
    }

    void TearDown() override
    {
        if (Utf8::Exists(testRoot))
        {
            Utf8::Remove(testRoot);
        }
    }

    void CreateDummyFile(const StringView _path, const StringView _content = "dummy")
    {
        std::ofstream ofs(_path.GetPtr());
        ofs << _content.GetPtr();
        ofs.close();
    }

    String testRoot;
};

TEST_F(FileSystemTest, MakeDirectory_Success)
{
    String subDir = testRoot;
    subDir.Join("NewFolder/", eJoinType::Directory);

    Error err = Utf8::MakeDirectory(subDir, eMakeDirectoryCollisionPolicy::Fail);

    EXPECT_EQ(err, Error::None);
    bool bExists = Utf8::Exists(subDir);
    EXPECT_TRUE(bExists);
}

TEST_F(FileSystemTest, MakeDirectory_Recursive)
{
    String deepDir = testRoot;
    deepDir.Join("Level1/Level2/Level3/", eJoinType::Directory);

    Error err = Utf8::MakeDirectory(deepDir, eMakeDirectoryCollisionPolicy::Fail);

    EXPECT_EQ(err, Error::None);
    bool bExists = Utf8::Exists(deepDir);
    EXPECT_TRUE(bExists);
}

TEST_F(FileSystemTest, MakeDirectory_CollisionPolicy)
{
    String subDir = testRoot;
    subDir.Join("Conflict/", eJoinType::Directory);
    Utf8::MakeDirectory(subDir, eMakeDirectoryCollisionPolicy::Fail);

    Error failErr = Utf8::MakeDirectory(subDir, eMakeDirectoryCollisionPolicy::Fail);
    EXPECT_EQ(failErr, Error::AlreadyExists);

    Error skipErr = Utf8::MakeDirectory(subDir, eMakeDirectoryCollisionPolicy::Skip);
    EXPECT_EQ(skipErr, Error::None);

    Error renameErr = Utf8::MakeDirectory(subDir, eMakeDirectoryCollisionPolicy::Rename);
    EXPECT_EQ(renameErr, Error::None);

    String renamedDir = testRoot;
    renamedDir.Join("Conflict (1)/", eJoinType::Directory);
    bool bExists = Utf8::Exists(renamedDir);
    EXPECT_TRUE(bExists);
}

TEST_F(FileSystemTest, Remove_FileAndDirectory)
{
    String filePath = testRoot;
    filePath.Join("ToDelete.txt", eJoinType::File);
    CreateDummyFile(filePath);

    Error fileErr = Utf8::Remove(filePath);
    EXPECT_EQ(fileErr, Error::None);
    bool bFileExists = Utf8::Exists(filePath);
    EXPECT_FALSE(bFileExists);

    String dirPath = testRoot;
    dirPath.Join("ToDeleteDir/", eJoinType::Directory);
    Utf8::MakeDirectory(dirPath, eMakeDirectoryCollisionPolicy::Fail);

    String childPath = dirPath;
    childPath.Join("Child.txt", eJoinType::File);
    CreateDummyFile(childPath);

    Error dirErr = Utf8::Remove(dirPath);
    EXPECT_EQ(dirErr, Error::None);
    bool bDirExists = Utf8::Exists(dirPath);
    EXPECT_FALSE(bDirExists);
}

TEST_F(FileSystemTest, Copy_File)
{
    String srcPath = testRoot;
    srcPath.Join("Source.txt", eJoinType::File);
    CreateDummyFile(srcPath, "Hello World");

    String dstPath = testRoot;
    dstPath.Join("Dest.txt", eJoinType::File);

    Error err = Utf8::Copy(srcPath, dstPath, eCollisionPolicy::Fail);

    EXPECT_EQ(err, Error::None);
    bool bExists = Utf8::Exists(dstPath);
    EXPECT_TRUE(bExists);
    EXPECT_EQ(Utf8::QueryFileSize(srcPath), Utf8::QueryFileSize(dstPath));
}

TEST_F(FileSystemTest, Rename_File)
{
    String srcPath = testRoot;
    srcPath.Join("OldName.txt", eJoinType::File);
    CreateDummyFile(srcPath);

    String dstPath = testRoot;
    dstPath.Join("NewName.txt", eJoinType::File);

    Error err = Utf8::Rename(srcPath, dstPath, eCollisionPolicy::Fail);

    EXPECT_EQ(err, Error::None);
    bool bOldExists = Utf8::Exists(srcPath);
    bool bNewExists = Utf8::Exists(dstPath);
    EXPECT_FALSE(bOldExists);
    EXPECT_TRUE(bNewExists);
}

TEST_F(FileSystemTest, QueryFileAttribute)
{
    String filePath = testRoot;
    filePath.Join("AttrTest.txt", eJoinType::File);
    String content = "Attribute Testing Data";
    CreateDummyFile(filePath, content);

    FileAttribute attr = Utf8::QueryFileAttribute(filePath);

    EXPECT_EQ(attr.type, eFileType::File);
    EXPECT_EQ(attr.fileSize, content.GetLength());
    EXPECT_GT(attr.creationTime, 0);
    EXPECT_GT(attr.lastWriteTime, 0);
}

TEST_F(FileSystemTest, IsEmpty_Directory)
{
    String emptyDir = testRoot;
    emptyDir.Join("Empty/", eJoinType::Directory);
    Utf8::MakeDirectory(emptyDir, eMakeDirectoryCollisionPolicy::Fail);

    bool bEmpty = Utf8::IsEmpty(emptyDir);
    EXPECT_TRUE(bEmpty);

    String nonEmptyDir = testRoot;
    nonEmptyDir.Join("NotEmpty/", eJoinType::Directory);
    Utf8::MakeDirectory(nonEmptyDir, eMakeDirectoryCollisionPolicy::Fail);

    String fileInDir = nonEmptyDir;
    fileInDir.Join("Content.txt", eJoinType::File);
    CreateDummyFile(fileInDir);

    bool bNotEmpty = Utf8::IsEmpty(nonEmptyDir);
    EXPECT_FALSE(bNotEmpty);
}

TEST_F(FileSystemTest, Path_AbsoluteAndRelative)
{
    String currentDir = Utf8::Current();
    String absPath    = Utf8::Absolute(".");

    EXPECT_STREQ(currentDir.GetPtr(), absPath.GetPtr());

    String fromPath = "C:/Jam/Project/";
    String toPath   = "C:/Jam/Project/Bin/Data/";

    String relative = Utf8::Relative(fromPath, toPath);

    bool bIsEmpty = relative.IsEmpty();
    EXPECT_FALSE(bIsEmpty);
}

TEST_F(FileSystemTest, IsDirectory_IsFile)
{
    String dirPath = testRoot;
    dirPath.Join("TestDir/", eJoinType::Directory);
    Utf8::MakeDirectory(dirPath, eMakeDirectoryCollisionPolicy::Fail);

    String filePath = testRoot;
    filePath.Join("TestFile.txt", eJoinType::File);
    CreateDummyFile(filePath);

    EXPECT_TRUE(Utf8::IsDirectory(dirPath));
    EXPECT_FALSE(Utf8::IsFile(dirPath));

    EXPECT_TRUE(Utf8::IsFile(filePath));
    EXPECT_FALSE(Utf8::IsDirectory(filePath));
}

TEST_F(FileSystemTest, QueryFileSize_QueryLastWriteTime)
{
    String filePath = testRoot;
    filePath.Join("SizeTest.txt", eJoinType::File);
    String content = "Test Content 1234567890";
    CreateDummyFile(filePath, content);

    UInt64 fileSize = Utf8::QueryFileSize(filePath);
    EXPECT_EQ(fileSize, content.GetLength());

    UInt64 lastWriteTime = Utf8::QueryLastWriteTime(filePath);
    EXPECT_GT(lastWriteTime, 0);
}

TEST_F(FileSystemTest, Copy_CollisionPolicy)
{
    String srcPath = testRoot;
    srcPath.Join("Original.txt", eJoinType::File);
    CreateDummyFile(srcPath, "Original Content");

    String dstPath = testRoot;
    dstPath.Join("Destination.txt", eJoinType::File);
    CreateDummyFile(dstPath, "Old");

    Error failErr = Utf8::Copy(srcPath, dstPath, eCollisionPolicy::Fail);
    EXPECT_EQ(failErr, Error::AlreadyExists);

    Error skipErr = Utf8::Copy(srcPath, dstPath, eCollisionPolicy::Skip);
    EXPECT_EQ(skipErr, Error::None);

    Error overwriteErr = Utf8::Copy(srcPath, dstPath, eCollisionPolicy::Overwrite);
    EXPECT_EQ(overwriteErr, Error::None);

    Error renameErr = Utf8::Copy(srcPath, dstPath, eCollisionPolicy::Rename);
    EXPECT_EQ(renameErr, Error::None);

    String renamedPath = testRoot;
    renamedPath.Join("Destination (1).txt", eJoinType::File);
    EXPECT_TRUE(Utf8::Exists(renamedPath));
}

TEST_F(FileSystemTest, Rename_CollisionPolicy)
{
    String srcPath1 = testRoot;
    srcPath1.Join("ToRename1.txt", eJoinType::File);
    CreateDummyFile(srcPath1, "Content1");

    String srcPath2 = testRoot;
    srcPath2.Join("ToRename2.txt", eJoinType::File);
    CreateDummyFile(srcPath2, "Content2");

    String dstPath = testRoot;
    dstPath.Join("Target.txt", eJoinType::File);
    CreateDummyFile(dstPath, "Existing");

    Error failErr = Utf8::Rename(srcPath1, dstPath, eCollisionPolicy::Fail);
    EXPECT_EQ(failErr, Error::AlreadyExists);
    EXPECT_TRUE(Utf8::Exists(srcPath1));

    Error overwriteErr = Utf8::Rename(srcPath1, dstPath, eCollisionPolicy::Overwrite);
    EXPECT_EQ(overwriteErr, Error::None);
    EXPECT_FALSE(Utf8::Exists(srcPath1));
    EXPECT_TRUE(Utf8::Exists(dstPath));

    Error renameErr = Utf8::Rename(srcPath2, dstPath, eCollisionPolicy::Rename);
    EXPECT_EQ(renameErr, Error::None);

    String renamedPath = testRoot;
    renamedPath.Join("Target (1).txt", eJoinType::File);
    EXPECT_TRUE(Utf8::Exists(renamedPath));
    EXPECT_FALSE(Utf8::Exists(srcPath2));
}

TEST_F(FileSystemTest, GenerateUniquePath)
{
    String basePath = testRoot;
    basePath.Join("UniqueFile.txt", eJoinType::File);

    String uniquePath1 = Utf8::GenerateUniquePath(basePath);
    EXPECT_STREQ(uniquePath1.GetPtr(), basePath.GetPtr());

    CreateDummyFile(basePath);

    String uniquePath2 = Utf8::GenerateUniquePath(basePath);
    EXPECT_STRNE(uniquePath2.GetPtr(), basePath.GetPtr());

    String expectedPath = testRoot;
    expectedPath.Join("UniqueFile (1).txt", eJoinType::File);
    EXPECT_STREQ(uniquePath2.GetPtr(), expectedPath.GetPtr());

    CreateDummyFile(uniquePath2);

    String uniquePath3 = Utf8::GenerateUniquePath(basePath);
    String expectedPath2 = testRoot;
    expectedPath2.Join("UniqueFile (2).txt", eJoinType::File);
    EXPECT_STREQ(uniquePath3.GetPtr(), expectedPath2.GetPtr());
}

TEST_F(FileSystemTest, ComparePathEqual)
{
    String path1 = testRoot;
    path1.Join("TestFile.txt", eJoinType::File);

    String path2 = testRoot;
    path2.Join("testfile.txt", eJoinType::File);

    String path3 = testRoot;
    path3.Join("TestFile.txt", eJoinType::File);

    String path4 = testRoot;
    path4.Join("Different.txt", eJoinType::File);

    EXPECT_TRUE(Utf8::ComparePathEqual(path1, path2));
    EXPECT_TRUE(Utf8::ComparePathEqual(path1, path3));
    EXPECT_FALSE(Utf8::ComparePathEqual(path1, path4));
}

TEST_F(FileSystemTest, Executable_Path)
{
    String execPath = Utf8::Executable();
    EXPECT_FALSE(execPath.IsEmpty());
    EXPECT_TRUE(Utf8::Exists(execPath));
    EXPECT_TRUE(Utf8::IsFile(execPath));
}

TEST_F(FileSystemTest, Copy_Directory)
{
    String srcDir = testRoot;
    srcDir.Join("SourceDir/", eJoinType::Directory);
    Utf8::MakeDirectory(srcDir, eMakeDirectoryCollisionPolicy::Fail);

    String fileInSrc = srcDir;
    fileInSrc.Join("FileInSource.txt", eJoinType::File);
    CreateDummyFile(fileInSrc, "Source Data");

    String dstDir = testRoot;
    dstDir.Join("DestDir/", eJoinType::Directory);

    Error err = Utf8::Copy(srcDir, dstDir, eCollisionPolicy::Fail);
    EXPECT_EQ(err, Error::None);
    EXPECT_TRUE(Utf8::Exists(dstDir));
    EXPECT_TRUE(Utf8::IsDirectory(dstDir));

    String fileInDst = dstDir;
    fileInDst.Join("FileInSource.txt", eJoinType::File);
    EXPECT_TRUE(Utf8::Exists(fileInDst));
}

TEST_F(FileSystemTest, Remove_NonExistent)
{
    String nonExistent = testRoot;
    nonExistent.Join("DoesNotExist.txt", eJoinType::File);

    Error err = Utf8::Remove(nonExistent);
    EXPECT_NE(err, Error::None);
}

TEST_F(FileSystemTest, IsEmpty_File)
{
    String emptyFile = testRoot;
    emptyFile.Join("Empty.txt", eJoinType::File);
    CreateDummyFile(emptyFile, "");

    String nonEmptyFile = testRoot;
    nonEmptyFile.Join("NonEmpty.txt", eJoinType::File);
    CreateDummyFile(nonEmptyFile, "Content");

    bool bEmptyFileIsEmpty = Utf8::IsEmpty(emptyFile);
    EXPECT_TRUE(bEmptyFileIsEmpty);

    bool bNonEmptyFileIsEmpty = Utf8::IsEmpty(nonEmptyFile);
    EXPECT_FALSE(bNonEmptyFileIsEmpty);
}

TEST_F(FileSystemTest, QueryFileAttribute_Directory)
{
    String dirPath = testRoot;
    dirPath.Join("AttributeDir/", eJoinType::Directory);
    Utf8::MakeDirectory(dirPath, eMakeDirectoryCollisionPolicy::Fail);

    FileAttribute attr = Utf8::QueryFileAttribute(dirPath);

    EXPECT_EQ(attr.type, eFileType::Directory);
    EXPECT_GT(attr.creationTime, 0);
    EXPECT_GT(attr.lastWriteTime, 0);
}

}   // namespace Jam::FileSystem::Test