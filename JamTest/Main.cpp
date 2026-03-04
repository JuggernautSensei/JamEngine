#include "pch.h"
#include <entt/entt.hpp>
#include <chrono>
#include <iostream>
#include <vector>
#include <memory>

int main(int argc, char** argv)
{
    Log::CreateInstance();
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    Log::DestroyInstance();
    return ret;
}