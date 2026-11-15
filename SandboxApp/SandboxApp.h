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
    void Update(float _dt) override;
    void Render() override;

private:
    Int32  m_remainingFrames = 0;
    String m_screenshotPath  = {};
};
