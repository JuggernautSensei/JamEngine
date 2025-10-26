#pragma once

class HelloBasicRenderer : public Scene
{
    JAM_SCENE(HelloBasicRenderer);

public:
    void OnEnter() override;
    void OnExit() override;
    void OnUpdate(float _deltaSec) override;
    void OnRender(const SceneView& _view) override;
    void OnImGuiRender() override;
    void OnResize(Int32 _width, Int32 _height) override;

private:
    
};
