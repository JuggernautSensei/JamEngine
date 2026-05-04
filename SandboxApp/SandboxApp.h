#pragma once

class SandboxApp : public Application
{
public:
    explicit SandboxApp(
        const StringView           _appName,
        const Flags<eAppCoreFlags> _subSystems)
        : Application(_appName, _subSystems)
    {
    }

    void Init(Int32 _argc, char** _argv) override;
    void Shutdown() override;
};

#ifdef JAM_EDITOR_ENABLE
JAM_MAIN(SandboxApp, "SandboxApp", kEditorDefaultFlags);
#else
JAM_MAIN(SandboxApp, "SandboxApp", kRuntimeDefaltFlags);
// JAM_MAIN(SandboxApp, "SandboxApp", kRuntimeDefaltFlags | eAppCoreFlags::AllowTearing);
#endif
