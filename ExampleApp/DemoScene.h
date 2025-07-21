#pragma once

class DemoScene : public Scene
{
public:
    explicit DemoScene(const std::string_view& _name);
    ~DemoScene() override = default;

    DemoScene(const DemoScene&)                = delete;
    DemoScene& operator=(const DemoScene&)     = delete;
    DemoScene(DemoScene&&) noexcept            = default;
    DemoScene& operator=(DemoScene&&) noexcept = default;

    void OnEnter() override;
    void OnUpdate(float _deltaTime) override;
    void OnRender() override;
    void OnRenderUI() override;

private:

};
