#pragma once

class SandboxApp : public Application
{
public:
    SandboxApp();

    void Init(Int32 _argc, char** _argv) override;
    void Shutdown() override;
    void ImGuiRender() override;
};
