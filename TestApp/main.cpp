#include "pch.h"

#include "Octree.h"
#include "OctreeTest.h"


//////////////////////////////////////////////////////////////////////////
// APPLICATION
//////////////////////////////////////////////////////////////////////////

class TestApp : public Application
{
public:
    explicit TestApp(const ApplicationCreateInfo& _info)
        : Application(_info)
    {
    }

    // this is main function (like main())
    void OnCreate() override
    {
        // Run all octree tests
        OctreeTest::RunAllTests();
        
        Quit();
    }
};

Application* CreateApplication(const CommandLineArguments& _args)
{
    return new TestApp({});
}

JAM_MAIN();