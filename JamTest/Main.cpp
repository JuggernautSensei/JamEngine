#include "pch.h"

int main(int _argc, char** _argv)
{
    ::testing::InitGoogleTest(&_argc, _argv);
    LogSystem::CreateInstance();
    const Int32 result = RUN_ALL_TESTS();
    LogSystem::DestroyInstance();
    return result;
}
