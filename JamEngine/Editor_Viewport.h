#pragma once
#include "Camera.h"
#include "GraphicsBase.h"
#include "Transform.h"

namespace jam
{

class Editor_Viewport
{
    enum class eMode : UInt8
    {
        Edit = 0,
        Run,
        Simulate,
    };

    enum class eSimulateMode : UInt8
    {
        Pause = 0,
        Play,
        Step,
    };

public:
    void Init();
    void Shutdown();
    void Update(float _deltaSec);
    void Render() const;
    void OnImGuiRender();

private:
    void Resize_(Int32 _width, Int32 _height);
    void UpdateEdit_(float _deltaSec) const;
    void UpdateRun_(float _deltaSec) const;
    void UpdateSimulate_(float _deltaSec);

    void DrawSimulateButton();
    void DrawSimulatePauseButton();
    void DrawSimulatePlayButton();

    void DrawPlayButton();
    void DrawStopButton();
    void DrawStepButton();

    Viewport          m_viewport   = {};
    TextureHandle     m_viewportTh = k_invalidHandle;
    FrameBufferHandle m_fbh        = k_invalidHandle;

    Camera    m_camera;
    Transform m_cameraTransform;

    eMode         m_mode         = eMode::Edit;
    eSimulateMode m_simulateMode = eSimulateMode::Pause;
    float         m_stepDt       = 1.f / 60.f;
};

}   // namespace jam
