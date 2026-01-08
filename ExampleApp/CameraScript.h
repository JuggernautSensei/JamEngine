#pragma once

struct CameraScriptDataComponent
{
    JAM_COMPONENT(CameraScriptDataComponent);

    void Panel_EditComponent(const Scene* _pScene, Entity _entity);

    float moveSpeed   = 10.f;
    float rotSpeedRad = ToRad(0.2f);
    bool  bEnable     = true;

    float yaw   = 0.f;
    float pitch = 0.f;
};

class CameraScript : public Script
{
    JAM_SCRIPT(CameraScript);

public:
    explicit CameraScript(Entity _entity);
    void OnUpdate(float _deltaSec) override;

private:
    CameraScriptDataComponent* m_pData  = nullptr;
    SceneComponent*        m_pTrans = nullptr;
};

void EditCameraScriptDataComponentUI(CameraScriptDataComponent& _component);