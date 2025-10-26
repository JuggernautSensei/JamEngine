#pragma once

namespace jam
{

class HelloTriangleScene : public Scene
{
    JAM_SCENE(HelloTriangleScene);

public:
    HelloTriangleScene();
    ~HelloTriangleScene() override;

    void OnRender(const SceneView& _view) override;

private:
    ProgramHandle m_program;

    VertexBufferHandle m_vbh;
    IndexBufferHandle  m_ibh;
};

}   // namespace jam