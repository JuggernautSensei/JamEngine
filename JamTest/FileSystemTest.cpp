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

}   // namespace Jam::FileSystem::Test