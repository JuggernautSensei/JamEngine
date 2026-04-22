#include "pch.h"

class TestApp : public Application
{
public:
    TestApp(
        const StringView                _appName,
        const Flags<eAppCoreFlags> _subSystems)
        : Application(_appName, _subSystems)
    {
    }

    void Init(Int32 _argc, char** _argv) override
    {
        ::testing::InitGoogleTest(&_argc, _argv);
    }

    void Update(float _dt) override
    {
        int ret = RUN_ALL_TESTS();
        JAM_ASSERT(ret == 0, "Some tests failed. Check the test results for details.");
        Quit();
    }
};

JAM_MAIN(TestApp, "JamAppTest", kRuntimeDefaltFlags);
