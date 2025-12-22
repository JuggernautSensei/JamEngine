#include "pch.h"

#include "CameraScript.h"

CameraScript::CameraScript(Entity _entity)
    : Script(_entity)
{
    _entity.CreateComponent<CameraScriptDataComponent>();
    m_pData  = &_entity.GetComponent<CameraScriptDataComponent>();
    m_pTrans = &_entity.GetComponent<SceneComponent>();
}

void CameraScript::OnUpdate(const float _deltaSec)
{
    if (!m_pData->bEnable)
        return;

    // move
    SceneNode& t = m_pTrans->node;
    if (Input::IsKeyDown(eKey::A))
    {
        Vec3 deltaTrans = t.GetLocalLeft() * m_pData->moveSpeed * _deltaSec;
        t.AddLocalTranslation(deltaTrans);
    }

    if (Input::IsKeyDown(eKey::D))
    {
        Vec3 deltaTrans = t.GetLocalRight() * m_pData->moveSpeed * _deltaSec;
        t.AddLocalTranslation(deltaTrans);
    }

    if (Input::IsKeyDown(eKey::W))
    {
        Vec3 deltaTrans = t.GetLocalForward() * m_pData->moveSpeed * _deltaSec;
        t.AddLocalTranslation(deltaTrans);
    }

    if (Input::IsKeyDown(eKey::S))
    {
        Vec3 deltaTrans = t.GetLocalBackward() * m_pData->moveSpeed * _deltaSec;
        t.AddLocalTranslation(deltaTrans);
    }

    if (Input::IsKeyDown(eKey::Q))
    {
        Vec3 deltaTrans = t.GetLocalDown() * m_pData->moveSpeed * _deltaSec;
        t.AddLocalTranslation(deltaTrans);
    }

    if (Input::IsKeyDown(eKey::E))
    {
        Vec3 deltaTrans = t.GetLocalUp() * m_pData->moveSpeed * _deltaSec;
        t.AddLocalTranslation(deltaTrans);
    }

    if (Input::IsMouseDown(eMouse::RightButton))
    {
        const Vec2i dPos   = Input::GetMouseDeltaPos();
        const float     dYaw   = static_cast<float>(dPos.x) * m_pData->rotSpeedRad;
        const float     dPitch = static_cast<float>(dPos.y) * m_pData->rotSpeedRad;

        m_pData->yaw += dYaw;
        m_pData->pitch += dPitch;
        m_pData->pitch = Clamp(m_pData->pitch, ToRad(-89.f), ToRad(89.f));
        m_pTrans->node.SetLocalRotation(m_pData->pitch, m_pData->yaw, 0.f);
    }

    if (Input::IsKeyDown(eKey::LeftArrow))
    {
        m_pData->yaw -= ToDeg(m_pData->rotSpeedRad) * _deltaSec;
        m_pTrans->node.SetLocalRotation(m_pData->pitch, m_pData->yaw, 0.f);
    }

    if (Input::IsKeyDown(eKey::RightArrow))
    {
        m_pData->yaw += ToDeg(m_pData->rotSpeedRad) * _deltaSec;
        m_pTrans->node.SetLocalRotation(m_pData->pitch, m_pData->yaw, 0.f);
    }

    if (Input::IsKeyDown(eKey::UpArrow))
    {
        m_pData->pitch -= ToDeg(m_pData->rotSpeedRad) * _deltaSec;
        m_pData->pitch = Clamp(m_pData->pitch, ToRad(-89.f), ToRad(89.f));
        m_pTrans->node.SetLocalRotation(m_pData->pitch, m_pData->yaw, 0.f);
    }

    if (Input::IsKeyDown(eKey::DownArrow))
    {
        m_pData->pitch += ToDeg(m_pData->rotSpeedRad) * _deltaSec;
        m_pData->pitch = Clamp(m_pData->pitch, ToRad(-89.f), ToRad(89.f));
        m_pTrans->node.SetLocalRotation(m_pData->pitch, m_pData->yaw, 0.f);
    }
}

void EditCameraScriptDataComponentUI(CameraScriptDataComponent& _component)
{
    ImGui::Checkbox("Enable Camera Script", &_component.bEnable);
    ImGui::SliderFloat("Move Speed", &_component.moveSpeed, 0.1f, 20.f);
    ImGui::SliderFloat("Rotation Speed", &_component.rotSpeedRad, 0.1f, 1.f, "%.3f rad");
}