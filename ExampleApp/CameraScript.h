#pragma once

struct CameraScriptDataComponent
{
    JAM_COMPONENT(CameraScriptDataComponent);

    void DrawEditor(const Scene* _pScene, Entity _entity);

    float moveSpeed   = 5.f;
    float rotSpeedRad = ToRad(0.2f);
    bool  bEnable     = true;
};

class CameraScript : public Script
{
    JAM_SCRIPT(CameraScript);

public:
    explicit CameraScript(Entity _entity);
    void OnUpdate(float _deltaSec) override;

private:
    CameraScriptDataComponent* m_pData  = nullptr;
    TransformComponent*        m_pTrans = nullptr;
};

void EditCameraScriptDataComponentUI(CameraScriptDataComponent& _component);