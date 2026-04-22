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
        const bool bLeft   = (m_buttonFlags & eInfiniteDraggerFlags::Left) && GetInput().IsMousePressed(eMouse::LeftButton);
        const bool bRight  = (m_buttonFlags & eInfiniteDraggerFlags::Right) && GetInput().IsMousePressed(eMouse::RightButton);
        const bool bMiddle = (m_buttonFlags & eInfiniteDraggerFlags::Middle) && GetInput().IsMousePressed(eMouse::MiddleButton);

        if (bLeft || bRight || bMiddle)
        {
            m_bIsDragging = true;
            m_capturedPos = GetInput().GetMousePos();
            m_deltaPos    = Vec2i::kZero;
            GetInput().SetMouseVisibility(false);
        }
    }
    else
    {
        // during dragging
        m_deltaPos = GetInput().GetMousePos() - m_capturedPos;

        // fix mouse pos to captured pos
        if (m_deltaPos.x != 0 || m_deltaPos.y != 0)
        {
            GetInput().SetMousePos(m_capturedPos.x, m_capturedPos.y);
        }

        // check drag end
        const bool bLeft   = !(m_buttonFlags & eInfiniteDraggerFlags::Left) || GetInput().IsMouseUp(eMouse::LeftButton);
        const bool bRight  = !(m_buttonFlags & eInfiniteDraggerFlags::Right) || GetInput().IsMouseUp(eMouse::RightButton);
        const bool bMiddle = !(m_buttonFlags & eInfiniteDraggerFlags::Middle) || GetInput().IsMouseUp(eMouse::MiddleButton);

        // all flagged buttons are up -> end drag
        if (bLeft && bRight && bMiddle)
        {
            m_bIsDragging = false;
            m_deltaPos    = Vec2i::kZero;
            m_capturedPos = Vec2i::kZero;
            GetInput().SetMouseVisibility(true);
        }
    }
}

Vec2i InfiniteDragger::GetDragDelta() const
{
    return m_deltaPos;
}

}   // namespace Jam