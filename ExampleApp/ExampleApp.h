#pragma once

class ExampleApp : public Application
{
public:
    explicit ExampleApp(const ApplicationCreateInfo& _info);

    void OnCreate() override;
    void OnDestroy() override;
};
