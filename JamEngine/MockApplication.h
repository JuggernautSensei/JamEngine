#pragma once
#include "Application.h"

namespace Jam
{

class MockApplication : public Application
{
public:
    explicit MockApplication(
        const ApplicationCreateInfo& _info)
        : Application(_info)
    {
    }
};

}   // namespace Jam

#define JAM_MOCK_APP()                                                  \
    Jam::Application* CreateApplication(Jam::Int32 _argc, char** _argv) \
    {                                                                   \
        Jam::ApplicationCreateInfo info {};                             \
        return new Jam::MockApplication(info);                          \
    }
