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
};

#ifdef JAM_EDITOR_ENABLE
JAM_MAIN(SandboxApp, "SandboxApp", k_editorDefaultFlags);
#else
JAM_MAIN(SandboxApp, "SandboxApp", k_runtimeDefaltFlags);
// JAM_MAIN(SandboxApp, "SandboxApp", k_runtimeDefaltFlags | eAppCoreFlags::AllowTearing);
#endif
