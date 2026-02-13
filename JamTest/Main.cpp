#include "pch.h"
#include "../JamEngine/MockApplication.h"

JAM_MOCK_APP();

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}