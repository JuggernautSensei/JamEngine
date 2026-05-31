#pragma once

class SandboxApp : public Application
{
public:
    explicit SandboxApp(
        const StringView           _appName,
        const Flags<eAppInitFlags> _subSystems)
        : Application(_appName, _subSystems)
    {
    }

    void Init(Int32 _argc, char** _argv) override;
    void Shutdown() override;
};