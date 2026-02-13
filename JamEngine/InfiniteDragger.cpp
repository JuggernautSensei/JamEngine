#include "pch.h"
#include "InfiniteDragger.h"

#include "Input.h"

namespace Jam
{

InfiniteDragger::InfiniteDragger(
    const Flags<eInfiniteDraggerFlags> _buttonFlags)
    : m_buttonFlags(_buttonFlags)
{
}

void InfiniteDragger::Update()
{
    // check drag start
    if (!m_bIsDragging)
    {
        const bool bLeft   = (m_buttonFlags & eInfiniteDraggerFlags::Left) && JAM_INPUT.IsMousePressed(eMouse::LeftButton);
        const bool bRight  = (m_buttonFlags & eInfiniteDraggerFlags::Right) && JAM_INPUT.IsMousePressed(eMouse::RightButton);
        const bool bMiddle = (m_buttonFlags & eInfiniteDraggerFlags::Middle) && JAM_INPUT.IsMousePressed(eMouse::MiddleButton);

        if (bLeft || bRight || bMiddle)
        {
            m_bIsDragging = true;
            m_capturedPos = JAM_INPUT.GetMousePos();
            m_deltaPos    = Vec2i::k_zero;
            JAM_INPUT.SetMouseVisibility(false);
        }
    }
    else
    {
        // during dragging
        m_deltaPos = JAM_INPUT.GetMousePos() - m_capturedPos;

        // fix mouse pos to captured pos
        if (m_deltaPos.x != 0 || m_deltaPos.y != 0)
        {
            JAM_INPUT.SetMousePos(m_capturedPos.x, m_capturedPos.y);
        }

        // check drag end
        const bool bLeft   = !(m_buttonFlags & eInfiniteDraggerFlags::Left) || JAM_INPUT.IsMouseUp(eMouse::LeftButton);
        const bool bRight  = !(m_buttonFlags & eInfiniteDraggerFlags::Right) || JAM_INPUT.IsMouseUp(eMouse::RightButton);
        const bool bMiddle = !(m_buttonFlags & eInfiniteDraggerFlags::Middle) || JAM_INPUT.IsMouseUp(eMouse::MiddleButton);

        // all flagged buttons are up -> end drag
        if (bLeft && bRight && bMiddle)
        {
            m_bIsDragging = false;
            m_deltaPos    = Vec2i::k_zero;
            m_capturedPos = Vec2i::k_zero;
            JAM_INPUT.SetMouseVisibility(true);
        }
    }
}

Vec2i InfiniteDragger::GetDragDelta() const
{
    return m_deltaPos;
}

}   // namespace Jam