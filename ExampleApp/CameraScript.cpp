#include "pch.h"

#include "CameraScript.h"

CameraScript::CameraScript(Entity _entity)
    : Script(_entity)
{
    _entity.CreateOrReplaceComponent<CameraScriptDataComponent>();
    m_pData  = &_entity.GetComponent<CameraScriptDataComponent>();
    m_pTrans = &_entity.GetComponent<TransformComponent>();
}

void CameraScript::OnUpdate(const float _deltaSec)
{
    if (!m_pData->bEnable)
        return;

    // move

    TransformHierarchy& t = m_pTrans->transform;
    if (Input::IsKeyDown(eKey::A))
    {
        Vec3 deltaTrans = t.GetWorldLeft() * m_pData->moveSpeed * _deltaSec;
        t.AddWorldTranslate(deltaTrans);
    }

    if (Input::IsKeyDown(eKey::D))
    {
        Vec3 deltaTrans = t.GetWorldRight() * m_pData->moveSpeed * _deltaSec;
        t.AddWorldTranslate(deltaTrans);
    }

    if (Input::IsKeyDown(eKey::W))
    {
        Vec3 deltaTrans = t.GetWorldForward() * m_pData->moveSpeed * _deltaSec;
        t.AddWorldTranslate(deltaTrans);
    }

    if (Input::IsKeyDown(eKey::S))
    {
        Vec3 deltaTrans = t.GetWorldBackward() * m_pData->moveSpeed * _deltaSec;
        t.AddWorldTranslate(deltaTrans);
    }

    if (Input::IsKeyDown(eKey::Q))
    {
        Vec3 deltaTrans = t.GetWorldDown() * m_pData->moveSpeed * _deltaSec;
        t.AddWorldTranslate(deltaTrans);
    }

    if (Input::IsKeyDown(eKey::E))
    {
        Vec3 deltaTrans = t.GetWorldUp() * m_pData->moveSpeed * _deltaSec;
        t.AddWorldTranslate(deltaTrans);
    }

    // rotate
    if (Input::IsMouseDown(eMouse::RightButton))
    {
        WindowPos deltaPos = Input::GetMouseDeltaPos();
        float     yawRad   = deltaPos.x * m_pData->rotSpeedRad;
        float     pitchRad = deltaPos.y * m_pData->rotSpeedRad;

        // Yaw
        Quat yawQ = Quat::CreateFromAxisAngle(Vec3::Up, yawRad);

        // Pitch
        Quat pitchQ = Quat::CreateFromAxisAngle(t.GetWorldRight(), pitchRad);

        // 회전 적용
        Quat q = yawQ * t.GetWorldRotate() * pitchQ;
        t.SetWorldRotate(q);

        // Pitch 클램핑
        float currentPitch = t.GetWorldRotatePitch();
        if (currentPitch > ToRad(89.f))
            t.SetWorldRotatePitch(ToRad(89.f));
        else if (currentPitch < ToRad(-89.f))
            t.SetWorldRotatePitch(ToRad(-89.f));

        // Roll 제거
        t.SetWorldRotateRoll(0.f);
    }
}

void EditCameraScriptDataComponentUI(CameraScriptDataComponent& _component)
{
    ImGui::Checkbox("Enable Camera Script", &_component.bEnable);
    ImGui::SliderFloat("Move Speed", &_component.moveSpeed, 0.1f, 20.f);
    ImGui::SliderFloat("Rotation Speed", &_component.rotSpeedRad, 0.1f, 1.f, "%.3f rad");
}